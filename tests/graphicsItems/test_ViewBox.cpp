#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

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
    CHECK(viewBox.getViewWidget() == nullptr);

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    pyqtgraph::graphicsItems::ViewBox viewBox;
    QGraphicsScene firstScene;
    firstScene.addItem(&viewBox);

    CHECK(viewBox.getViewWidget() == nullptr);

    QGraphicsView firstView(&firstScene);
    CHECK(viewBox.getViewWidget() == &firstView);
    CHECK(viewBox.getViewWidget() == &firstView);

    firstScene.removeItem(&viewBox);
    CHECK(viewBox.getViewWidget() == nullptr);

    QGraphicsScene secondScene;
    QGraphicsView secondView(&secondScene);
    secondScene.addItem(&viewBox);
    CHECK(viewBox.getViewWidget() == &secondView);

    secondScene.removeItem(&viewBox);
    CHECK(viewBox.getViewWidget() == nullptr);

    return true;
}

bool testParentConstruction()
{
    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    pyqtgraph::graphicsItems::ViewBox viewBox(&parent);

    CHECK(viewBox.parentItem() == &parent);
    CHECK(viewBox.graphicsItem() == static_cast<QGraphicsItem*>(&viewBox));
    CHECK(viewBox.getViewWidget() == nullptr);

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
