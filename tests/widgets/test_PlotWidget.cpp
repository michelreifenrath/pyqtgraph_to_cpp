#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
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

bool testConstructionAndApiShape()
{
    using pyqtgraph::graphicsItems::PlotItem;
    using pyqtgraph::widgets::PlotWidget;

    static_assert(std::is_constructible_v<PlotWidget>);
    static_assert(std::is_constructible_v<PlotWidget, QWidget*>);
    static_assert(std::is_destructible_v<PlotWidget>);
    static_assert(!std::is_copy_constructible_v<PlotWidget>);
    static_assert(!std::is_copy_assignable_v<PlotWidget>);
    static_assert(!std::is_move_constructible_v<PlotWidget>);
    static_assert(!std::is_move_assignable_v<PlotWidget>);
    static_assert(std::is_base_of_v<QGraphicsView, PlotWidget>);
    static_assert(std::is_base_of_v<QWidget, PlotWidget>);
    static_assert(!std::is_final_v<PlotWidget>);
    static_assert(std::is_same_v<decltype(std::declval<PlotWidget&>().getPlotItem()), PlotItem*>);
    static_assert(std::is_same_v<decltype(std::declval<const PlotWidget&>().getPlotItem()), const PlotItem*>);

    PlotWidget widget;
    CHECK(widget.parentWidget() == nullptr);
    CHECK(widget.scene() != nullptr);
    CHECK(widget.getPlotItem() != nullptr);

    return true;
}

bool testParentConstruction()
{
    QWidget parent;
    pyqtgraph::widgets::PlotWidget widget(&parent);

    CHECK(widget.parentWidget() == &parent);
    CHECK(widget.getPlotItem() != nullptr);

    return true;
}

bool testOwnedPlotItemInScene()
{
    pyqtgraph::widgets::PlotWidget widget;

    pyqtgraph::graphicsItems::PlotItem* plotItem = widget.getPlotItem();
    CHECK(plotItem != nullptr);
    CHECK(widget.getPlotItem() == plotItem);

    const pyqtgraph::widgets::PlotWidget& constWidget = widget;
    CHECK(constWidget.getPlotItem() == plotItem);

    CHECK(widget.scene() != nullptr);
    CHECK(plotItem->scene() == widget.scene());
    CHECK(widget.scene()->items().contains(plotItem));
    CHECK(plotItem->getViewWidget() == &widget);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndApiShape()) {
        return 1;
    }
    if (!testParentConstruction()) {
        return 1;
    }
    if (!testOwnedPlotItemInScene()) {
        return 1;
    }

    return 0;
}
