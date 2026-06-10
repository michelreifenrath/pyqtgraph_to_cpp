#include <cppqtgraph/graphicsItems/GraphicsLayout.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/widgets/GraphicsLayoutWidget.hpp>
#include <cppqtgraph/widgets/GraphicsView.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QWidget>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
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

QString artifactDir()
{
#ifdef CPPQTGRAPH_P3_12_ARTIFACT_DIR
    return QString::fromUtf8(CPPQTGRAPH_P3_12_ARTIFACT_DIR);
#else
    return QDir::tempPath() + QStringLiteral("/cppqtgraph_P3_12");
#endif
}

bool writeInteractionReport(const QString& text)
{
    QDir dir(artifactDir());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        std::cerr << "failed to create artifact dir: " << dir.absolutePath().toStdString() << '\n';
        return false;
    }

    QFile file(dir.filePath(QStringLiteral("interaction_report.txt")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "failed to write interaction report: " << file.fileName().toStdString() << '\n';
        return false;
    }

    QTextStream stream(&file);
    stream << text;
    std::cout << text.toStdString();
    std::cout << "artifact: " << file.fileName().toStdString() << '\n';
    return true;
}

bool testConstructionAndApiShape()
{
    using cppqtgraph::graphicsItems::GraphicsLayout;
    using cppqtgraph::graphicsItems::GraphicsWidget;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::graphicsItems::ViewBox;
    using cppqtgraph::widgets::GraphicsLayoutWidget;
    using cppqtgraph::widgets::GraphicsView;

    static_assert(std::is_base_of_v<QGraphicsView, GraphicsView>);
    static_assert(std::is_base_of_v<GraphicsView, GraphicsLayoutWidget>);
    static_assert(std::is_base_of_v<GraphicsWidget, GraphicsLayout>);
    static_assert(std::is_constructible_v<GraphicsView>);
    static_assert(std::is_constructible_v<GraphicsLayoutWidget>);
    static_assert(std::is_constructible_v<GraphicsLayoutWidget, QWidget*>);
    static_assert(!std::is_copy_constructible_v<GraphicsView>);
    static_assert(!std::is_copy_assignable_v<GraphicsView>);
    static_assert(!std::is_copy_constructible_v<GraphicsLayoutWidget>);
    static_assert(!std::is_copy_assignable_v<GraphicsLayoutWidget>);
    static_assert(std::is_same_v<decltype(std::declval<GraphicsLayout&>().addPlot()), PlotItem*>);
    static_assert(std::is_same_v<decltype(std::declval<GraphicsLayout&>().addViewBox()), ViewBox*>);
    static_assert(std::is_same_v<decltype(std::declval<GraphicsLayout&>().addLayout()), GraphicsLayout*>);
    static_assert(std::is_same_v<decltype(std::declval<GraphicsLayoutWidget&>().addPlot()), PlotItem*>);
    static_assert(std::is_same_v<decltype(std::declval<GraphicsLayoutWidget&>().addViewBox()), ViewBox*>);
    static_assert(std::is_same_v<decltype(std::declval<GraphicsLayoutWidget&>().addLayout()), GraphicsLayout*>);

    GraphicsLayoutWidget widget;
    CHECK(widget.scene() != nullptr);
    CHECK(widget.graphicsLayout() != nullptr);
    CHECK(widget.centralItem() == widget.graphicsLayout());
    CHECK(widget.ci == widget.graphicsLayout());

    return true;
}

bool testP312InteractionReplay()
{
    using cppqtgraph::graphicsItems::GraphicsLayout;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::graphicsItems::ViewBox;
    using cppqtgraph::widgets::GraphicsLayoutWidget;

    GraphicsLayoutWidget widget;
    widget.resize(280, 180);
    widget.show();
    QApplication::processEvents();

    GraphicsLayout* layout = widget.graphicsLayout();
    CHECK(layout != nullptr);
    CHECK(layout->scene() == widget.scene());
    CHECK(widget.scene()->items().contains(layout));

    const QRectF preSceneRect = widget.scene()->sceneRect();
    const QRectF preCentralGeometry = layout->geometry();
    const int preCurrentRow = layout->currentRow();
    const int preCurrentColumn = layout->currentColumn();

    int rangeSignalCount = 0;
    int transformSignalCount = 0;
    int scaleSignalCount = 0;
    int layoutGeometryCount = 0;
    QObject::connect(&widget, &cppqtgraph::widgets::GraphicsView::sigDeviceRangeChanged,
                     [&rangeSignalCount](cppqtgraph::widgets::GraphicsView*, const QRectF&) { ++rangeSignalCount; });
    QObject::connect(&widget, &cppqtgraph::widgets::GraphicsView::sigDeviceTransformChanged,
                     [&transformSignalCount](cppqtgraph::widgets::GraphicsView*) { ++transformSignalCount; });
    QObject::connect(&widget, &cppqtgraph::widgets::GraphicsView::sigScaleChanged,
                     [&scaleSignalCount](cppqtgraph::widgets::GraphicsView*) { ++scaleSignalCount; });
    QObject::connect(layout, &QGraphicsWidget::geometryChanged, [&layoutGeometryCount]() { ++layoutGeometryCount; });

    PlotItem* plot = widget.addPlot(0, 0);
    ViewBox* viewBox = widget.addViewBox(0, 1);
    widget.nextRow();
    GraphicsLayout* nested = widget.addLayout(-1, -1, 1, 2);
    QApplication::processEvents();

    CHECK(plot != nullptr);
    CHECK(viewBox != nullptr);
    CHECK(nested != nullptr);
    CHECK(widget.getItem(0, 0) == plot);
    CHECK(widget.getItem(0, 1) == viewBox);
    CHECK(widget.getItem(1, 0) == nested);
    CHECK(widget.getItem(1, 1) == nested);
    CHECK(widget.getItem(4, 4) == nullptr);
    CHECK(widget.itemIndex(plot) >= 0);
    CHECK(widget.itemIndex(viewBox) >= 0);
    CHECK(widget.itemIndex(nested) >= 0);

    const QSize targetSize(360, 240);
    widget.resize(targetSize);
    QApplication::processEvents();

    const QRectF expected = widget.viewRect();
    const QRectF postSceneRect = widget.scene()->sceneRect();
    const QRectF postCentralGeometry = layout->geometry();
    const QRectF plotGeometry = plot->geometry();
    const QRectF viewBoxGeometry = viewBox->geometry();
    const QRectF nestedGeometry = nested->geometry();

    const int rangeSignalsAfterResize = rangeSignalCount;
    const int transformSignalsAfterResize = transformSignalCount;
    const int scaleSignalsAfterResize = scaleSignalCount;
    const int geometrySignalsAfterResize = layoutGeometryCount;

    const QRectF noOpBaselineScene = widget.scene()->sceneRect();
    const QRectF noOpBaselineGeometry = layout->geometry();
    widget.setRange(widget.range(), 0.0);
    QApplication::processEvents();
    const bool noOpKeptScene = sameRect(widget.scene()->sceneRect(), noOpBaselineScene);
    const bool noOpKeptGeometry = sameRect(layout->geometry(), noOpBaselineGeometry);

    widget.removeItem(viewBox);
    QApplication::processEvents();
    const bool removedCellIsEmpty = widget.getItem(0, 1) == nullptr;
    const bool removedIndexMissing = widget.itemIndex(viewBox) < 0;

    std::ostringstream report;
    report << "P3.12 interaction report\n"
           << "pre_state: sceneRect=" << preSceneRect.width() << 'x' << preSceneRect.height()
           << " centralGeometry=" << preCentralGeometry.width() << 'x' << preCentralGeometry.height()
           << " currentRow=" << preCurrentRow << " currentColumn=" << preCurrentColumn << '\n'
           << "event_sequence: show processEvents addPlot(0,0) addViewBox(0,1) nextRow addLayout(row=auto,col=auto,rowspan=1,colspan=2) resize_to="
           << targetSize.width() << 'x' << targetSize.height()
           << " processEvents setRange(existing_range,padding=0) removeItem(viewBox)\n"
           << "post_state: sceneRect=" << postSceneRect.width() << 'x' << postSceneRect.height()
           << " centralGeometry=" << postCentralGeometry.width() << 'x' << postCentralGeometry.height()
           << " expectedViewRect=" << expected.width() << 'x' << expected.height()
           << " plotGeometry=" << plotGeometry.width() << 'x' << plotGeometry.height()
           << " viewBoxGeometry=" << viewBoxGeometry.width() << 'x' << viewBoxGeometry.height()
           << " nestedGeometry=" << nestedGeometry.width() << 'x' << nestedGeometry.height() << '\n'
           << "signals: range=" << rangeSignalsAfterResize
           << " transform=" << transformSignalsAfterResize
           << " scale=" << scaleSignalsAfterResize
           << " layoutGeometry=" << geometrySignalsAfterResize << '\n'
           << "negative_noop: emptyCell=" << (widget.getItem(4, 4) == nullptr)
           << " noOpKeptScene=" << noOpKeptScene
           << " noOpKeptGeometry=" << noOpKeptGeometry
           << " removedCellIsEmpty=" << removedCellIsEmpty
           << " removedIndexMissing=" << removedIndexMissing << '\n';
    CHECK(writeInteractionReport(QString::fromStdString(report.str())));

    CHECK(sameRect(postSceneRect, expected));
    CHECK(sameRect(postCentralGeometry, expected));
    CHECK(plotGeometry.width() > 0.0);
    CHECK(plotGeometry.height() > 0.0);
    CHECK(viewBoxGeometry.width() > 0.0);
    CHECK(viewBoxGeometry.height() > 0.0);
    CHECK(nestedGeometry.width() > 0.0);
    CHECK(nestedGeometry.height() > 0.0);
    CHECK(rangeSignalsAfterResize > 0);
    CHECK(transformSignalsAfterResize > 0);
    CHECK(scaleSignalsAfterResize >= 0);
    CHECK(geometrySignalsAfterResize > 0);
    CHECK(noOpKeptScene);
    CHECK(noOpKeptGeometry);
    CHECK(removedCellIsEmpty);
    CHECK(removedIndexMissing);

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
    if (!testP312InteractionReplay()) {
        return 1;
    }

    return 0;
}
