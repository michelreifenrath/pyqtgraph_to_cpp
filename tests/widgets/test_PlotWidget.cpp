#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>
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

bool sameRect(const QRectF& left, const QRectF& right)
{
    return qFuzzyCompare(left.x(), right.x()) && qFuzzyCompare(left.y(), right.y())
        && qFuzzyCompare(left.width(), right.width()) && qFuzzyCompare(left.height(), right.height());
}

QRectF sceneRectForViewport(const pyqtgraph::widgets::PlotWidget& widget)
{
    const QSize viewportSize = widget.viewport()->size();
    return QRectF(0.0, 0.0, static_cast<qreal>(viewportSize.width()), static_cast<qreal>(viewportSize.height()));
}

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

bool testUsesPyqtgraphGraphicsScene()
{
    pyqtgraph::widgets::PlotWidget widget;

    auto* scene = dynamic_cast<pyqtgraph::GraphicsScene::GraphicsScene*>(widget.scene());
    CHECK(scene != nullptr);
    CHECK(scene->clickRadius() == 2);
    CHECK(scene->moveDistance() == 5.0);

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

bool testP301OwnershipInteractionReplay()
{
    using pyqtgraph::GraphicsScene::GraphicsScene;
    using pyqtgraph::graphicsItems::PlotItem;
    using pyqtgraph::widgets::PlotWidget;

    PlotWidget widget;
    widget.resize(240, 180);
    widget.show();
    QApplication::processEvents();

    auto* scene = dynamic_cast<GraphicsScene*>(widget.scene());
    PlotItem* plotItem = widget.getPlotItem();
    CHECK(scene != nullptr);
    CHECK(plotItem != nullptr);

    const QRectF preSceneRect = scene->sceneRect();
    const QRectF prePlotGeometry = plotItem->geometry();
    const bool preParentedScene = scene->parent() == &widget;
    const bool prePlotInScene = plotItem->scene() == scene && scene->items().contains(plotItem);
    const bool preStableGetPlotItem = widget.getPlotItem() == plotItem && widget.getPlotItem() == plotItem;

    int sceneRectChangedCount = 0;
    int plotGeometryChangedCount = 0;
    QObject::connect(scene, &QGraphicsScene::sceneRectChanged, [&sceneRectChangedCount](const QRectF&) {
        ++sceneRectChangedCount;
    });
    QObject::connect(plotItem, &QGraphicsWidget::geometryChanged, [&plotGeometryChangedCount]() {
        ++plotGeometryChangedCount;
    });

    const QSize targetSize = widget.size() == QSize(321, 239) ? QSize(347, 251) : QSize(321, 239);
    widget.resize(targetSize);
    QApplication::processEvents();

    const QRectF expectedPostRect = sceneRectForViewport(widget);
    const QRectF postSceneRect = scene->sceneRect();
    const QRectF postPlotGeometry = plotItem->geometry();

    const int sceneRectCountAfterResize = sceneRectChangedCount;
    const int plotGeometryCountAfterResize = plotGeometryChangedCount;
    const QRectF noOpBaselineSceneRect = scene->sceneRect();
    const QRectF noOpBaselinePlotGeometry = plotItem->geometry();
    widget.resize(widget.size());
    QApplication::processEvents();
    const bool noOpKeptSceneRect = sameRect(scene->sceneRect(), noOpBaselineSceneRect);
    const bool noOpKeptPlotGeometry = sameRect(plotItem->geometry(), noOpBaselinePlotGeometry);
    const bool noOpStableGetPlotItem = widget.getPlotItem() == plotItem;

    std::cout << "P3.01 interaction report\n"
              << "pre_state: scene_parent_is_widget=" << preParentedScene
              << " plot_in_scene=" << prePlotInScene
              << " stable_getPlotItem=" << preStableGetPlotItem
              << " sceneRect=" << preSceneRect.width() << 'x' << preSceneRect.height()
              << " plotGeometry=" << prePlotGeometry.width() << 'x' << prePlotGeometry.height() << '\n'
              << "event_sequence: show processEvents resize_to=" << targetSize.width() << 'x' << targetSize.height()
              << " processEvents resize_same_size processEvents destruction_probe\n"
              << "post_state: sceneRect=" << postSceneRect.width() << 'x' << postSceneRect.height()
              << " plotGeometry=" << postPlotGeometry.width() << 'x' << postPlotGeometry.height()
              << " expectedViewport=" << expectedPostRect.width() << 'x' << expectedPostRect.height()
              << " sceneRectChanged=" << sceneRectCountAfterResize
              << " plotGeometryChanged=" << plotGeometryCountAfterResize
              << " noOpKeptSceneRect=" << noOpKeptSceneRect
              << " noOpKeptPlotGeometry=" << noOpKeptPlotGeometry
              << " noOpStableGetPlotItem=" << noOpStableGetPlotItem << '\n';

    CHECK(preParentedScene);
    CHECK(prePlotInScene);
    CHECK(preStableGetPlotItem);
    CHECK(sameRect(postSceneRect, expectedPostRect));
    CHECK(sameRect(postPlotGeometry, expectedPostRect));
    CHECK(plotItem->scene() == scene);
    CHECK(scene->items().contains(plotItem));
    CHECK(sceneRectCountAfterResize > 0);
    CHECK(plotGeometryCountAfterResize > 0);
    CHECK(noOpKeptSceneRect);
    CHECK(noOpKeptPlotGeometry);
    CHECK(noOpStableGetPlotItem);

    QPointer<GraphicsScene> sceneAfterDestruction;
    QPointer<PlotItem> plotAfterDestruction;
    {
        auto ownedWidget = std::make_unique<PlotWidget>();
        sceneAfterDestruction = dynamic_cast<GraphicsScene*>(ownedWidget->scene());
        plotAfterDestruction = ownedWidget->getPlotItem();
        CHECK(sceneAfterDestruction != nullptr);
        CHECK(plotAfterDestruction != nullptr);
        CHECK(sceneAfterDestruction->parent() == ownedWidget.get());
        CHECK(plotAfterDestruction->scene() == sceneAfterDestruction);
    }
    QApplication::processEvents();
    std::cout << "destruction_post_state: scene_destroyed=" << sceneAfterDestruction.isNull()
              << " plotItem_destroyed=" << plotAfterDestruction.isNull() << '\n';
    CHECK(sceneAfterDestruction.isNull());
    CHECK(plotAfterDestruction.isNull());

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
    if (!testUsesPyqtgraphGraphicsScene()) {
        return 1;
    }
    if (!testOwnedPlotItemInScene()) {
        return 1;
    }
    if (!testP301OwnershipInteractionReplay()) {
        return 1;
    }

    return 0;
}
