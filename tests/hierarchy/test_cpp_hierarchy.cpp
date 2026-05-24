#include <pyqtgraph/graphicsItems/GraphicsItem.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsRectItem>

#include <iostream>
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

} // namespace

int main()
{
    if (!testGraphicsItemApiShape()) {
        return 1;
    }

    return 0;
}
