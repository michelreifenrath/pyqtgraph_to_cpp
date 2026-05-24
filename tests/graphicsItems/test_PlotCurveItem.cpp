#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <pyqtgraph/graphicsItems/GraphicsItem.hpp>
#include <pyqtgraph/graphicsItems/GraphicsObject.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <iostream>
#include <memory>
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

bool testConstructionAndHierarchy()
{
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsObject;
    using pyqtgraph::graphicsItems::PlotCurveItem;

    static_assert(std::is_constructible_v<PlotCurveItem>);
    static_assert(std::is_constructible_v<PlotCurveItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<PlotCurveItem>);
    static_assert(!std::is_copy_constructible_v<PlotCurveItem>);
    static_assert(!std::is_copy_assignable_v<PlotCurveItem>);
    static_assert(!std::is_move_constructible_v<PlotCurveItem>);
    static_assert(!std::is_move_assignable_v<PlotCurveItem>);
    static_assert(std::is_base_of_v<GraphicsObject, PlotCurveItem>);
    static_assert(std::is_base_of_v<GraphicsItem, PlotCurveItem>);
    static_assert(std::is_base_of_v<QGraphicsObject, PlotCurveItem>);
    static_assert(std::is_base_of_v<QObject, PlotCurveItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, PlotCurveItem>);

    PlotCurveItem curve;
    CHECK(curve.graphicsItem() == static_cast<QGraphicsItem*>(&curve));
    CHECK(curve.toGraphicsObject() == &curve);
    CHECK(curve.flags().testFlag(QGraphicsItem::ItemSendsGeometryChanges));
    CHECK(curve.getViewWidget() == nullptr);
    CHECK(curve.boundingRect().isNull());

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    QGraphicsScene firstScene;
    firstScene.addItem(&curve);

    CHECK(curve.getViewWidget() == nullptr);

    QGraphicsView firstView(&firstScene);
    CHECK(curve.getViewWidget() == &firstView);
    CHECK(curve.getViewWidget() == &firstView);

    firstScene.removeItem(&curve);
    CHECK(curve.getViewWidget() == nullptr);

    QGraphicsScene secondScene;
    QGraphicsView secondView(&secondScene);
    secondScene.addItem(&curve);
    CHECK(curve.getViewWidget() == &secondView);

    secondScene.removeItem(&curve);
    CHECK(curve.getViewWidget() == nullptr);

    return true;
}

bool testParentConstruction()
{
    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    pyqtgraph::graphicsItems::PlotCurveItem curve(&parent);

    CHECK(curve.parentItem() == &parent);
    CHECK(curve.graphicsItem() == static_cast<QGraphicsItem*>(&curve));
    CHECK(curve.toGraphicsObject() == &curve);
    CHECK(curve.getViewWidget() == nullptr);

    return true;
}

bool testPaintNoOpSmoke()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QStyleOptionGraphicsItem option;

    curve.paint(&painter, &option, nullptr);
    painter.end();

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndHierarchy()) {
        return 1;
    }
    if (!testInheritedViewWidgetDiscovery()) {
        return 1;
    }
    if (!testParentConstruction()) {
        return 1;
    }
    if (!testPaintNoOpSmoke()) {
        return 1;
    }

    return 0;
}
