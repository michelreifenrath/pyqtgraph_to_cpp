#include <pyqtgraph/graphicsItems/GraphicsWidget.hpp>

#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>

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
    using pyqtgraph::graphicsItems::GraphicsWidget;

    static_assert(std::is_constructible_v<GraphicsWidget>);
    static_assert(std::is_constructible_v<GraphicsWidget, QGraphicsItem*>);
    static_assert(std::is_destructible_v<GraphicsWidget>);
    static_assert(std::is_base_of_v<QGraphicsWidget, GraphicsWidget>);
    static_assert(std::is_base_of_v<QGraphicsItem, GraphicsWidget>);
    static_assert(std::is_base_of_v<GraphicsItem, GraphicsWidget>);

    GraphicsWidget widget;
    CHECK(widget.graphicsItem() == static_cast<QGraphicsItem*>(&widget));
    CHECK(widget.getViewWidget() == nullptr);

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    pyqtgraph::graphicsItems::GraphicsWidget widget;
    QGraphicsScene scene;
    scene.addItem(&widget);

    CHECK(widget.getViewWidget() == nullptr);

    QGraphicsView view(&scene);
    CHECK(widget.getViewWidget() == &view);
    CHECK(widget.getViewWidget() == &view);

    scene.removeItem(&widget);
    widget.forgetViewWidget();
    CHECK(widget.getViewWidget() == nullptr);

    return true;
}

bool testUpstreamGeometryConvenienceMethods()
{
    pyqtgraph::graphicsItems::GraphicsWidget widget;

    widget.setGeometry(QRectF(0.0, 0.0, 12.0, 34.0));
    CHECK(widget.width() == 12.0);
    CHECK(widget.height() == 34.0);

    widget.setFixedWidth(56.0);
    CHECK(widget.minimumWidth() == 56.0);
    CHECK(widget.maximumWidth() == 56.0);

    widget.setFixedHeight(78.0);
    CHECK(widget.minimumHeight() == 78.0);
    CHECK(widget.maximumHeight() == 78.0);

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
    if (!testUpstreamGeometryConvenienceMethods()) {
        return 1;
    }

    return 0;
}
