#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>

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
    using cppqtgraph::graphicsItems::GraphicsItem;
    using cppqtgraph::graphicsItems::GraphicsWidget;
    using cppqtgraph::graphicsItems::PlotItem;

    static_assert(std::is_constructible_v<PlotItem>);
    static_assert(std::is_constructible_v<PlotItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<PlotItem>);
    static_assert(!std::is_copy_constructible_v<PlotItem>);
    static_assert(!std::is_copy_assignable_v<PlotItem>);
    static_assert(!std::is_move_constructible_v<PlotItem>);
    static_assert(!std::is_move_assignable_v<PlotItem>);
    static_assert(std::is_base_of_v<GraphicsWidget, PlotItem>);
    static_assert(std::is_base_of_v<GraphicsItem, PlotItem>);
    static_assert(std::is_base_of_v<QGraphicsWidget, PlotItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, PlotItem>);
    static_assert(!std::is_final_v<PlotItem>);

    PlotItem plot;
    CHECK(plot.graphicsItem() == static_cast<QGraphicsItem*>(&plot));
    CHECK(plot.getViewWidget() == nullptr);

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    cppqtgraph::graphicsItems::PlotItem plot;
    QGraphicsScene firstScene;
    firstScene.addItem(&plot);

    CHECK(plot.getViewWidget() == nullptr);

    QGraphicsView firstView(&firstScene);
    CHECK(plot.getViewWidget() == &firstView);
    CHECK(plot.getViewWidget() == &firstView);

    firstScene.removeItem(&plot);
    CHECK(plot.getViewWidget() == nullptr);

    QGraphicsScene secondScene;
    QGraphicsView secondView(&secondScene);
    secondScene.addItem(&plot);
    CHECK(plot.getViewWidget() == &secondView);

    secondScene.removeItem(&plot);
    CHECK(plot.getViewWidget() == nullptr);

    return true;
}

bool testParentConstruction()
{
    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    cppqtgraph::graphicsItems::PlotItem plot(&parent);

    CHECK(plot.parentItem() == &parent);
    CHECK(plot.graphicsItem() == static_cast<QGraphicsItem*>(&plot));
    CHECK(plot.getViewWidget() == nullptr);

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
