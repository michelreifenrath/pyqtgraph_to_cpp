#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QEvent>
#include <QtCore/QPointF>
#include <QtCore/QtGlobal>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#ifndef PYQTGRAPH_CPP_P3_01_REPORT_PATH
#define PYQTGRAPH_CPP_P3_01_REPORT_PATH "reports/issues/P3.01/plotwidget-interaction.md"
#endif

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

struct InteractionObservation {
    int itemAdded = 0;
    int itemRemoved = 0;
    int prepareForPaint = 0;
};

bool writeReport(const std::string& contents)
{
    const std::filesystem::path reportPath(PYQTGRAPH_CPP_P3_01_REPORT_PATH);
    std::error_code error;
    std::filesystem::create_directories(reportPath.parent_path(), error);
    if (error) {
        std::cerr << "failed to create report directory " << reportPath.parent_path() << ": " << error.message() << '\n';
        return false;
    }

    std::ofstream report(reportPath);
    if (!report) {
        std::cerr << "failed to open report path " << reportPath << '\n';
        return false;
    }

    report << contents;
    return true;
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
    static_assert(!std::is_base_of_v<QGraphicsView, PlotWidget>);
    static_assert(std::is_base_of_v<QWidget, PlotWidget>);
    static_assert(!std::is_final_v<PlotWidget>);
    static_assert(std::is_same_v<decltype(std::declval<PlotWidget&>().getPlotItem()), PlotItem*>);
    static_assert(std::is_same_v<decltype(std::declval<const PlotWidget&>().getPlotItem()), const PlotItem*>);

    PlotWidget widget;
    CHECK(widget.parentWidget() == nullptr);
    CHECK(qobject_cast<QGraphicsView*>(static_cast<QObject*>(&widget)) == nullptr);
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

bool testWrapperOwnsInternalViewSceneAndPlotItem()
{
    pyqtgraph::widgets::PlotWidget widget;

    const QList<QGraphicsView*> directViews = widget.findChildren<QGraphicsView*>(QString(), Qt::FindDirectChildrenOnly);
    CHECK(directViews.size() == 1);

    QGraphicsView* view = directViews.front();
    CHECK(view != nullptr);
    CHECK(view->parentWidget() == &widget);
    CHECK(view->scene() != nullptr);

    auto* scene = dynamic_cast<pyqtgraph::GraphicsScene::GraphicsScene*>(view->scene());
    CHECK(scene != nullptr);
    CHECK(scene->clickRadius() == 2);
    CHECK(scene->moveDistance() == 5.0);
    CHECK(scene->getViewWidget() == view);

    pyqtgraph::graphicsItems::PlotItem* plotItem = widget.getPlotItem();
    CHECK(plotItem != nullptr);
    CHECK(widget.getPlotItem() == plotItem);

    const pyqtgraph::widgets::PlotWidget& constWidget = widget;
    CHECK(constWidget.getPlotItem() == plotItem);

    CHECK(plotItem->scene() == scene);
    CHECK(scene->items().contains(plotItem));
    CHECK(plotItem->getViewWidget() == view);

    widget.resize(320, 240);
    widget.show();
    QApplication::processEvents();
    const QPixmap snapshot = widget.grab();
    CHECK(!snapshot.isNull());

    return true;
}

bool testScriptedInteractionNoOpReplayWritesReport()
{
    pyqtgraph::widgets::PlotWidget widget;
    widget.resize(320, 240);
    widget.show();
    QApplication::processEvents();

    const QList<QGraphicsView*> preViews = widget.findChildren<QGraphicsView*>(QString(), Qt::FindDirectChildrenOnly);
    CHECK(preViews.size() == 1);
    QGraphicsView* preView = preViews.front();
    auto* preScene = dynamic_cast<pyqtgraph::GraphicsScene::GraphicsScene*>(preView->scene());
    pyqtgraph::graphicsItems::PlotItem* prePlotItem = widget.getPlotItem();
    CHECK(preScene != nullptr);
    CHECK(prePlotItem != nullptr);

    InteractionObservation observation;
    QObject::connect(preScene, &pyqtgraph::GraphicsScene::GraphicsScene::sigItemAdded,
        [&observation](QGraphicsItem*) { ++observation.itemAdded; });
    QObject::connect(preScene, &pyqtgraph::GraphicsScene::GraphicsScene::sigItemRemoved,
        [&observation](QGraphicsItem*) { ++observation.itemRemoved; });
    QObject::connect(preScene, &pyqtgraph::GraphicsScene::GraphicsScene::sigPrepareForPaint,
        [&observation]() { ++observation.prepareForPaint; });

    const int preItemCount = preScene->items().size();
    const int preChildViewCount = preViews.size();

    QMouseEvent mouseMove(QEvent::MouseMove, QPointF(0.0, 0.0), QPointF(0.0, 0.0), QPointF(0.0, 0.0),
        Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(preView->viewport(), &mouseMove);
    widget.updateGeometry();
    QApplication::processEvents();
    const QPixmap snapshot = widget.grab();
    CHECK(!snapshot.isNull());

    const QList<QGraphicsView*> postViews = widget.findChildren<QGraphicsView*>(QString(), Qt::FindDirectChildrenOnly);
    CHECK(postViews.size() == 1);
    CHECK(postViews.size() == preChildViewCount);
    CHECK(postViews.front() == preView);
    CHECK(widget.getPlotItem() == prePlotItem);
    CHECK(preView->scene() == preScene);
    CHECK(prePlotItem->scene() == preScene);
    CHECK(preScene->items().contains(prePlotItem));
    CHECK(preScene->items().size() == preItemCount);
    CHECK(prePlotItem->getViewWidget() == preView);
    CHECK(observation.itemAdded == 0);
    CHECK(observation.itemRemoved == 0);

    std::ostringstream report;
    report << "# P3.01 PlotWidget interaction proof\n\n";
    report << "Screenshots are not required for this non-pixel ownership architecture proof.\n\n";
    report << "## Pre-state\n";
    report << "- wrapper is QWidget and not QGraphicsView: true\n";
    report << "- direct child QGraphicsView count: " << preChildViewCount << "\n";
    report << "- scene type: pyqtgraph::GraphicsScene::GraphicsScene\n";
    report << "- scene item count: " << preItemCount << "\n";
    report << "- plot item address: " << prePlotItem << "\n";
    report << "- view address: " << preView << "\n\n";
    report << "## Event sequence\n";
    report << "1. show PlotWidget offscreen\n";
    report << "2. send no-button mouse move to the owned QGraphicsView viewport\n";
    report << "3. call updateGeometry and process Qt events\n";
    report << "4. grab the wrapper widget\n\n";
    report << "## Post-state\n";
    report << "- direct child QGraphicsView count: " << postViews.size() << "\n";
    report << "- view preserved: " << (postViews.front() == preView ? "true" : "false") << "\n";
    report << "- scene preserved: " << (preView->scene() == preScene ? "true" : "false") << "\n";
    report << "- plot item preserved: " << (widget.getPlotItem() == prePlotItem ? "true" : "false") << "\n";
    report << "- scene item count: " << preScene->items().size() << "\n";
    report << "- wrapper grab non-null: " << (!snapshot.isNull() ? "true" : "false") << "\n\n";
    report << "## Signals/callbacks\n";
    report << "- sigItemAdded observed after pre-state: " << observation.itemAdded << "\n";
    report << "- sigItemRemoved observed after pre-state: " << observation.itemRemoved << "\n";
    report << "- sigPrepareForPaint observed: " << observation.prepareForPaint << "\n";
    report << "- no item add/remove callbacks are expected for the no-op replay.\n\n";
    report << "## Negative/no-op case\n";
    report << "The no-button mouse move and geometry/event processing did not replace the owned view, scene, or plot item, "
              "did not add extra views/items, and emitted no item add/remove signals.\n";

    CHECK(writeReport(report.str()));

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
    if (!testWrapperOwnsInternalViewSceneAndPlotItem()) {
        return 1;
    }
    if (!testScriptedInteractionNoOpReplayWritesReport()) {
        return 1;
    }

    return 0;
}
