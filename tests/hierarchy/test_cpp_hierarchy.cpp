#include <pyqtgraph/graphicsItems/AxisItem.hpp>
#include <pyqtgraph/graphicsItems/GraphicsItem.hpp>
#include <pyqtgraph/graphicsItems/GraphicsObject.hpp>
#include <pyqtgraph/graphicsItems/GraphicsWidget.hpp>
#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QStyleOptionGraphicsItem>

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

class ConcreteGraphicsObject final : public pyqtgraph::graphicsItems::GraphicsObject {
public:
    using pyqtgraph::graphicsItems::GraphicsObject::GraphicsObject;

    QRectF boundingRect() const override { return QRectF(0.0, 0.0, 1.0, 1.0); }

    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}
};

bool testGraphicsItemApiShape()
{
    using pyqtgraph::graphicsItems::GraphicsItem;

    static_assert(!std::is_base_of_v<QObject, GraphicsItem>);
    static_assert(!std::is_base_of_v<QGraphicsItem, GraphicsItem>);
    static_assert(std::is_constructible_v<GraphicsItem>);
    static_assert(std::is_constructible_v<GraphicsItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<GraphicsItem>);

    GraphicsItem unbound;
    CHECK(unbound.graphicsItem() == nullptr);
    CHECK(unbound.getViewWidget() == nullptr);
    unbound.forgetViewWidget();

    QGraphicsRectItem host(QRectF(0.0, 0.0, 1.0, 1.0));
    GraphicsItem bound(&host);
    CHECK(bound.graphicsItem() == &host);
    CHECK(bound.getViewWidget() == nullptr);

    unbound.setGraphicsItem(&host);
    CHECK(unbound.graphicsItem() == &host);
    CHECK(unbound.getViewWidget() == nullptr);

    unbound.setGraphicsItem(nullptr);
    CHECK(unbound.graphicsItem() == nullptr);

    return true;
}

bool testGraphicsObjectApiShape()
{
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsObject;

    static_assert(std::is_base_of_v<QObject, GraphicsObject>);
    static_assert(std::is_base_of_v<QGraphicsItem, GraphicsObject>);
    static_assert(std::is_base_of_v<QGraphicsObject, GraphicsObject>);
    static_assert(std::is_base_of_v<GraphicsItem, GraphicsObject>);
    static_assert(std::is_abstract_v<GraphicsObject>);
    static_assert(std::is_constructible_v<ConcreteGraphicsObject>);
    static_assert(std::is_destructible_v<ConcreteGraphicsObject>);

    ConcreteGraphicsObject object;
    CHECK(object.graphicsItem() == static_cast<QGraphicsItem*>(&object));
    CHECK(object.getViewWidget() == nullptr);

    return true;
}

bool testGraphicsWidgetApiShape()
{
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsWidget;

    static_assert(std::is_constructible_v<GraphicsWidget>);
    static_assert(std::is_constructible_v<GraphicsWidget, QGraphicsItem*>);
    static_assert(std::is_destructible_v<GraphicsWidget>);
    static_assert(std::is_base_of_v<GraphicsItem, GraphicsWidget>);
    static_assert(std::is_base_of_v<QGraphicsWidget, GraphicsWidget>);
    static_assert(std::is_base_of_v<QGraphicsItem, GraphicsWidget>);

    GraphicsWidget widget;
    CHECK(widget.graphicsItem() == static_cast<QGraphicsItem*>(&widget));

    QGraphicsRectItem host(QRectF(0.0, 0.0, 1.0, 1.0));
    widget.setGraphicsItem(&host);
    CHECK(widget.graphicsItem() == &host);

    widget.setGraphicsItem(static_cast<QGraphicsItem*>(&widget));
    CHECK(widget.graphicsItem() == static_cast<QGraphicsItem*>(&widget));
    CHECK(widget.getViewWidget() == nullptr);

    return true;
}

bool testAxisItemApiShape()
{
    using pyqtgraph::graphicsItems::AxisItem;
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsWidget;

    static_assert(std::is_constructible_v<AxisItem>);
    static_assert(std::is_constructible_v<AxisItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<AxisItem>);
    static_assert(std::is_base_of_v<GraphicsWidget, AxisItem>);
    static_assert(std::is_base_of_v<GraphicsItem, AxisItem>);
    static_assert(std::is_base_of_v<QGraphicsWidget, AxisItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, AxisItem>);

    AxisItem axis;
    CHECK(axis.graphicsItem() == static_cast<QGraphicsItem*>(&axis));

    return true;
}

bool testViewBoxApiShape()
{
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsWidget;
    using pyqtgraph::graphicsItems::ViewBox;

    static_assert(std::is_constructible_v<ViewBox>);
    static_assert(std::is_constructible_v<ViewBox, QGraphicsItem*>);
    static_assert(std::is_destructible_v<ViewBox>);
    static_assert(std::is_base_of_v<GraphicsWidget, ViewBox>);
    static_assert(std::is_base_of_v<GraphicsItem, ViewBox>);
    static_assert(std::is_base_of_v<QGraphicsWidget, ViewBox>);
    static_assert(std::is_base_of_v<QGraphicsItem, ViewBox>);

    CHECK(ViewBox::PanMode == 3);
    CHECK(ViewBox::RectMode == 1);
    CHECK(ViewBox::XAxis == 0);
    CHECK(ViewBox::YAxis == 1);
    CHECK(ViewBox::XYAxes == 2);

    ViewBox viewBox;
    CHECK(viewBox.graphicsItem() == static_cast<QGraphicsItem*>(&viewBox));

    return true;
}

bool testPlotCurveItemApiShape()
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
    CHECK(curve.boundingRect().isNull());
    CHECK(curve.flags().testFlag(QGraphicsItem::ItemSendsGeometryChanges));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testGraphicsItemApiShape()) {
        return 1;
    }
    if (!testGraphicsObjectApiShape()) {
        return 1;
    }
    if (!testGraphicsWidgetApiShape()) {
        return 1;
    }
    if (!testAxisItemApiShape()) {
        return 1;
    }
    if (!testViewBoxApiShape()) {
        return 1;
    }
    if (!testPlotCurveItemApiShape()) {
        return 1;
    }

    return 0;
}
