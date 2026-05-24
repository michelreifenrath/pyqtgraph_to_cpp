#include <pyqtgraph/graphicsItems/AxisItem.hpp>

#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsRectItem>
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
    CHECK(axis.getViewWidget() == nullptr);

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    pyqtgraph::graphicsItems::AxisItem axis;
    QGraphicsScene firstScene;
    firstScene.addItem(&axis);

    CHECK(axis.getViewWidget() == nullptr);

    QGraphicsView firstView(&firstScene);
    CHECK(axis.getViewWidget() == &firstView);
    CHECK(axis.getViewWidget() == &firstView);

    firstScene.removeItem(&axis);
    CHECK(axis.getViewWidget() == nullptr);

    QGraphicsScene secondScene;
    QGraphicsView secondView(&secondScene);
    secondScene.addItem(&axis);
    CHECK(axis.getViewWidget() == &secondView);

    secondScene.removeItem(&axis);
    CHECK(axis.getViewWidget() == nullptr);

    return true;
}

bool testParentConstruction()
{
    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    pyqtgraph::graphicsItems::AxisItem axis(&parent);

    CHECK(axis.parentItem() == &parent);
    CHECK(axis.graphicsItem() == static_cast<QGraphicsItem*>(&axis));
    CHECK(axis.getViewWidget() == nullptr);

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

    return 0;
}
