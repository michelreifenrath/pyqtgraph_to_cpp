#include <pyqtgraph/graphicsItems/GraphicsItem.hpp>
#include <pyqtgraph/graphicsItems/GraphicsObject.hpp>
#include <pyqtgraph/graphicsItems/GraphicsWidget.hpp>

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
    CHECK(widget.getViewWidget() == nullptr);

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

    return 0;
}
