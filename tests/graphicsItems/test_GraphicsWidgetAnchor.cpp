#include <cppqtgraph/graphicsItems/GraphicsWidgetAnchor.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsWidget>

#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }

    return true;
}

#define CHECK(expression) \
    do { \
        if (!check((expression), #expression, __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (false)

class ApplicationGuard {
public:
    ApplicationGuard(int& argc, char** argv)
    {
        if (QApplication::instance() == nullptr) {
            application_ = std::make_unique<QApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QApplication> application_;
};

bool pointsAlmostEqual(const QPointF& lhs, const QPointF& rhs)
{
    constexpr qreal epsilon = 1.0e-9;
    return std::abs(lhs.x() - rhs.x()) < epsilon && std::abs(lhs.y() - rhs.y()) < epsilon;
}

bool testGraphicsWidgetAnchorInteractionReplay()
{
    using cppqtgraph::graphicsItems::GraphicsWidgetAnchor;

    static_assert(std::is_constructible_v<GraphicsWidgetAnchor, QGraphicsWidget*>);
    static_assert(!std::is_copy_constructible_v<GraphicsWidgetAnchor>);
    static_assert(!std::is_copy_assignable_v<GraphicsWidgetAnchor>);

    QGraphicsWidget parent;
    QGraphicsWidget child(&parent);
    parent.setGeometry(QRectF(0.0, 0.0, 100.0, 80.0));
    child.setGeometry(QRectF(0.0, 0.0, 20.0, 10.0));
    child.setPos(QPointF(0.0, 0.0));

    int parentGeometryChangedCount = 0;
    QObject::connect(&parent, &QGraphicsWidget::geometryChanged, &parent, [&]() {
        ++parentGeometryChangedCount;
    });

    GraphicsWidgetAnchor anchor(&child);
    std::ostringstream report;
    report << "P3.02 interaction replay\n";
    report << "pre-state: parent.geometry=(0,0,100,80); child.geometry=(0,0,20,10); child.pos=(0,0)\n";
    report << "event[1]: anchor(itemPos=(1,0), parentPos=(1,0), offset=(-10,10))\n";

    anchor.anchor(QPointF(1.0, 0.0), QPointF(1.0, 0.0), QPointF(-10.0, 10.0));
    CHECK(pointsAlmostEqual(child.pos(), QPointF(70.0, 10.0)));
    report << "post[1]: child.pos=(" << child.pos().x() << ',' << child.pos().y() << ")\n";

    report << "event[2]: parent.setGeometry(0,0,200,80); processEvents()\n";
    parent.setGeometry(QRectF(0.0, 0.0, 200.0, 80.0));
    QApplication::processEvents();
    CHECK(parentGeometryChangedCount >= 1);
    CHECK(pointsAlmostEqual(child.pos(), QPointF(170.0, 10.0)));
    report << "signals/callbacks: parent.geometryChanged=" << parentGeometryChangedCount << "\n";
    report << "post[2]: child.pos=(" << child.pos().x() << ',' << child.pos().y() << ")\n";

    report << "event[3]: child.setGeometry(0,0,40,10); processEvents()\n";
    child.setGeometry(QRectF(0.0, 0.0, 40.0, 10.0));
    QApplication::processEvents();
    CHECK(pointsAlmostEqual(child.pos(), QPointF(150.0, 10.0)));
    report << "post[3]: child.pos=(" << child.pos().x() << ',' << child.pos().y() << ")\n";

    std::cout << report.str();
    return true;
}

bool testAutoAnchorChoosesNearestParentBoundary()
{
    QGraphicsWidget parent;
    QGraphicsWidget relativeChild(&parent);
    QGraphicsWidget absoluteChild(&parent);
    parent.setGeometry(QRectF(0.0, 0.0, 100.0, 80.0));
    relativeChild.setGeometry(QRectF(0.0, 0.0, 20.0, 10.0));
    absoluteChild.setGeometry(QRectF(0.0, 0.0, 20.0, 10.0));
    relativeChild.setPos(QPointF(70.0, 10.0));
    absoluteChild.setPos(QPointF(70.0, 10.0));

    cppqtgraph::graphicsItems::GraphicsWidgetAnchor relativeAnchor(&relativeChild);
    relativeAnchor.autoAnchor(relativeChild.pos());
    CHECK(pointsAlmostEqual(relativeChild.pos(), QPointF(70.0, 10.0)));

    cppqtgraph::graphicsItems::GraphicsWidgetAnchor absoluteAnchor(&absoluteChild);
    absoluteAnchor.autoAnchor(absoluteChild.pos(), false);
    CHECK(pointsAlmostEqual(absoluteChild.pos(), QPointF(70.0, 10.0)));

    parent.setGeometry(QRectF(0.0, 0.0, 200.0, 80.0));
    QApplication::processEvents();
    CHECK(pointsAlmostEqual(relativeChild.pos(), QPointF(160.0, 10.0)));
    CHECK(pointsAlmostEqual(absoluteChild.pos(), QPointF(170.0, 10.0)));

    std::cout << "autoAnchor: relative child moved to (160,10); absolute-offset child moved to (170,10) after parent resize\n";
    return true;
}

bool testAnchorWithoutParentThrowsAndLeavesPositionUnchanged()
{
    cppqtgraph::graphicsItems::GraphicsWidgetAnchor anchor;
    QGraphicsWidget orphan;
    orphan.setPos(QPointF(3.0, 4.0));
    anchor.setAnchorItem(&orphan);

    bool threw = false;
    try {
        anchor.anchor(QPointF(1.0, 0.0), QPointF(1.0, 0.0), QPointF(-10.0, 10.0));
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(pointsAlmostEqual(orphan.pos(), QPointF(3.0, 4.0)));
    std::cout << "negative/no-op: orphan anchor threw std::runtime_error; child.pos remained (3,4)\n";
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testGraphicsWidgetAnchorInteractionReplay()) {
        return 1;
    }
    if (!testAutoAnchorChoosesNearestParentBoundary()) {
        return 1;
    }
    if (!testAnchorWithoutParentThrowsAndLeavesPositionUnchanged()) {
        return 1;
    }

    return 0;
}
