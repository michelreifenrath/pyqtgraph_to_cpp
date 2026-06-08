#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>
#include <pyqtgraph/graphicsItems/ButtonItem.hpp>
#include <pyqtgraph/graphicsItems/GraphicsWidgetAnchor.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItemGroup>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsWidget>

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

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

bool rectsAlmostEqual(const QRectF& lhs, const QRectF& rhs)
{
    return pointsAlmostEqual(lhs.topLeft(), rhs.topLeft()) && pointsAlmostEqual(lhs.bottomRight(), rhs.bottomRight());
}

QPixmap makePixmap(int width, int height, qreal devicePixelRatio = 1.0)
{
    QPixmap pixmap(width, height);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::red);
    return pixmap;
}

pyqtgraph::GraphicsScene::MouseClickEvent makeClickEvent(Qt::MouseButton button)
{
    QGraphicsSceneMouseEvent qtEvent(QEvent::GraphicsSceneMouseRelease);
    qtEvent.setButton(button);
    qtEvent.setButtons(button == Qt::NoButton ? Qt::MouseButtons(Qt::NoButton) : Qt::MouseButtons(button));
    qtEvent.ignore();
    return pyqtgraph::GraphicsScene::MouseClickEvent(&qtEvent);
}

bool testButtonGeometryStateEventsAndLifetime()
{
    using pyqtgraph::GraphicsScene::HoverEvent;
    using pyqtgraph::GraphicsScene::MouseClickEvent;
    using pyqtgraph::graphicsItems::ButtonItem;

    ButtonItem defaultButton(makePixmap(32, 32));
    CHECK(rectsAlmostEqual(defaultButton.boundingRect(), QRectF(0.0, 0.0, 32.0, 32.0)));

    ButtonItem highDpiDefaultButton(makePixmap(32, 32, 2.0));
    CHECK(rectsAlmostEqual(highDpiDefaultButton.boundingRect(), QRectF(0.0, 0.0, 16.0, 16.0)));

    CHECK(defaultButton.enabled());
    CHECK(qFuzzyCompare(defaultButton.opacity() + 1.0, 1.7));

    QGraphicsItemGroup group;
    ButtonItem button(makePixmap(8, 8), 14.0, &group);
    CHECK(button.parentItem() == &group);
    CHECK(rectsAlmostEqual(button.boundingRect(), QRectF(0.0, 0.0, 14.0, 14.0)));
    CHECK(group.childrenBoundingRect().contains(button.boundingRect()));

    int clickedCount = 0;
    ButtonItem* clickedButton = nullptr;
    QObject::connect(&button, &ButtonItem::clicked, &button, [&](ButtonItem* emittedButton) {
        ++clickedCount;
        clickedButton = emittedButton;
    });

    HoverEvent enterEvent(nullptr, true);
    enterEvent.setEnter(true);
    button.hoverEvent(&enterEvent);
    CHECK(qFuzzyCompare(button.opacity() + 1.0, 2.0));

    HoverEvent exitEvent(nullptr, true);
    exitEvent.setExit(true);
    button.hoverEvent(&exitEvent);
    CHECK(qFuzzyCompare(button.opacity() + 1.0, 1.7));

    MouseClickEvent leftClickEvent = makeClickEvent(Qt::LeftButton);
    button.mouseClickEvent(&leftClickEvent);
    CHECK(leftClickEvent.isAccepted());
    CHECK(leftClickEvent.acceptedItem() == &button);
    CHECK(clickedCount == 1);
    CHECK(clickedButton == &button);

    MouseClickEvent rightClickEvent = makeClickEvent(Qt::RightButton);
    button.mouseClickEvent(&rightClickEvent);
    CHECK(!rightClickEvent.isAccepted());
    CHECK(clickedCount == 1);

    button.disable();
    CHECK(!button.enabled());
    CHECK(qFuzzyCompare(button.opacity() + 1.0, 1.4));
    button.hoverEvent(&enterEvent);
    CHECK(qFuzzyCompare(button.opacity() + 1.0, 1.4));
    MouseClickEvent disabledClickEvent = makeClickEvent(Qt::LeftButton);
    button.mouseClickEvent(&disabledClickEvent);
    CHECK(!disabledClickEvent.isAccepted());
    CHECK(clickedCount == 1);

    button.enable();
    CHECK(button.enabled());
    CHECK(qFuzzyCompare(button.opacity() + 1.0, 1.7));
    MouseClickEvent enabledClickEvent = makeClickEvent(Qt::LeftButton);
    button.mouseClickEvent(&enabledClickEvent);
    CHECK(enabledClickEvent.isAccepted());
    CHECK(clickedCount == 2);

    auto* lifetimeParent = new QGraphicsItemGroup;
    auto* lifetimeButton = new ButtonItem(makePixmap(6, 6), 6.0, lifetimeParent);
    QPointer<ButtonItem> lifetimePointer(lifetimeButton);
    delete lifetimeParent;
    CHECK(lifetimePointer.isNull());

    std::cout << "P4.13 ButtonItem: default geometry, parent/group bounds, hover/click state, and parent-owned lifetime passed\n";
    return true;
}

bool testAnchorGroupGeometryAndErrorPath()
{
    using pyqtgraph::graphicsItems::GraphicsWidgetAnchor;

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
    anchor.anchor(QPointF(1.0, 0.0), QPointF(1.0, 0.0), QPointF(-10.0, 10.0));
    CHECK(pointsAlmostEqual(child.pos(), QPointF(70.0, 10.0)));

    parent.setGeometry(QRectF(0.0, 0.0, 200.0, 80.0));
    QApplication::processEvents();
    CHECK(parentGeometryChangedCount >= 1);
    CHECK(pointsAlmostEqual(child.pos(), QPointF(170.0, 10.0)));

    QGraphicsWidget autoParent;
    QGraphicsWidget relativeChild(&autoParent);
    QGraphicsWidget absoluteChild(&autoParent);
    autoParent.setGeometry(QRectF(0.0, 0.0, 100.0, 80.0));
    relativeChild.setGeometry(QRectF(0.0, 0.0, 20.0, 10.0));
    absoluteChild.setGeometry(QRectF(0.0, 0.0, 20.0, 10.0));
    relativeChild.setPos(QPointF(70.0, 10.0));
    absoluteChild.setPos(QPointF(70.0, 10.0));

    GraphicsWidgetAnchor relativeAnchor(&relativeChild);
    relativeAnchor.autoAnchor(relativeChild.pos());
    CHECK(pointsAlmostEqual(relativeChild.pos(), QPointF(70.0, 10.0)));

    GraphicsWidgetAnchor absoluteAnchor(&absoluteChild);
    absoluteAnchor.autoAnchor(absoluteChild.pos(), false);
    CHECK(pointsAlmostEqual(absoluteChild.pos(), QPointF(70.0, 10.0)));

    autoParent.setGeometry(QRectF(0.0, 0.0, 200.0, 80.0));
    QApplication::processEvents();
    CHECK(pointsAlmostEqual(relativeChild.pos(), QPointF(160.0, 10.0)));
    CHECK(pointsAlmostEqual(absoluteChild.pos(), QPointF(170.0, 10.0)));

    QGraphicsWidget orphan;
    orphan.setPos(QPointF(3.0, 4.0));
    GraphicsWidgetAnchor orphanAnchor(&orphan);
    bool threw = false;
    try {
        orphanAnchor.anchor(QPointF(1.0, 0.0), QPointF(1.0, 0.0), QPointF(-10.0, 10.0));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(pointsAlmostEqual(orphan.pos(), QPointF(3.0, 4.0)));

    std::cout << "P4.13 GraphicsWidgetAnchor: resize callbacks, relative/absolute autoAnchor, and no-parent error path passed\n";
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testButtonGeometryStateEventsAndLifetime()) {
        return 1;
    }
    if (!testAnchorGroupGeometryAndErrorPath()) {
        return 1;
    }

    return 0;
}
