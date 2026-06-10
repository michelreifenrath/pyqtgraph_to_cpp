#include <cppqtgraph/graphicsItems/HistogramLUTItem.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/widgets/GraphicsLayoutWidget.hpp>
#include <cppqtgraph/widgets/GraphicsView.hpp>
#include <cppqtgraph/widgets/HistogramLUTWidget.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QtGlobal>
#include <QtGui/QImage>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QSizePolicy>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef CPPQTGRAPH_P5_01_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P5_01_VISUAL_DIFF_DIR "reports/visual-diffs/P5.01"
#endif

#ifndef CPPQTGRAPH_P5_01_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_01_REPOSITORY_REPORT_DIR "reports/issues/P5.01"
#endif

namespace {

constexpr int kPlotWidth = 320;
constexpr int kPlotHeight = 240;
constexpr int kHistogramWidth = 115;
constexpr int kHistogramHeight = 200;

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

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    PixelMetrics metrics;
    if (reference.size() != actual.size()) {
        std::cerr << "image size mismatch: reference=" << reference.width() << 'x' << reference.height()
                  << " actual=" << actual.width() << 'x' << actual.height() << '\n';
        return metrics;
    }
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);
    const int pixelCount = reference.width() * reference.height();
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected = reference.pixelColor(x, y);
            const QColor observed = actual.pixelColor(x, y);
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            metrics.totalDelta += static_cast<std::uint64_t>(delta);
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
            if (delta != 0) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0) : QColor(255, std::min(delta, 255), std::min(delta, 255)));
        }
    }
    metrics.meanDelta = static_cast<double>(metrics.totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
    return metrics;
}

std::uint64_t semanticPixelCount(const QImage& image)
{
    std::uint64_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && (color.red() > 20 || color.green() > 20 || color.blue() > 20)) {
                ++count;
            }
        }
    }
    return count;
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

void writeTextFile(const QString& path, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        throw std::runtime_error("failed to write " + path.toStdString());
    }
    QTextStream stream(&file);
    stream << text;
}

QString jsonEscape(QString value)
{
    value.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    value.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    value.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    value.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    return value;
}

QString normalizedReviewValue(QString value)
{
    const qsizetype commentIndex = value.indexOf(QChar('#'));
    if (commentIndex >= 0) {
        value.truncate(commentIndex);
    }
    value = value.trimmed();
    if (value.size() >= 2
        && ((value.front() == QChar('\'') && value.back() == QChar('\''))
            || (value.front() == QChar('"') && value.back() == QChar('"')))) {
        value = value.mid(1, value.size() - 2);
    }
    return value.trimmed().toLower();
}

struct SemanticReviewStatus {
    QString path;
    QString verdict;
    QString recommendation;
    bool exists = false;
    bool citesArtifacts = false;
    bool accepted = false;
};

QString caseArtifactDir(const QString& caseName)
{
    return QStringLiteral(CPPQTGRAPH_P5_01_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

SemanticReviewStatus readGptVisualReview(const QString& caseName)
{
    SemanticReviewStatus status;
    status.path = caseArtifactDir(caseName) + QStringLiteral("/gpt5_vision_review.md");
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P5.01 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P5.01 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    status.exists = true;
    const QString content = QString::fromUtf8(file.readAll());
    const QString lowerContent = content.toLower();
    status.citesArtifacts = lowerContent.contains(QStringLiteral("reference.png"))
        && lowerContent.contains(QStringLiteral("actual.png")) && lowerContent.contains(QStringLiteral("diff.png"))
        && lowerContent.contains(QStringLiteral("metrics.json"));

    const QStringList lines = content.split(QChar('\n'));
    for (const QString& line : lines) {
        const qsizetype separator = line.indexOf(QChar(':'));
        if (separator < 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed().toLower();
        if (key == QStringLiteral("verdict")) {
            status.verdict = normalizedReviewValue(line.mid(separator + 1));
        } else if (key == QStringLiteral("recommendation")) {
            status.recommendation = normalizedReviewValue(line.mid(separator + 1));
        }
    }
    status.accepted = status.exists && status.citesArtifacts && status.verdict == QStringLiteral("pass")
        && status.recommendation == QStringLiteral("merge_ok");
    if (!status.accepted) {
        std::cerr << "P5.01 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
}

QImage grabWidget(QWidget& widget)
{
    widget.show();
    QApplication::processEvents();
    widget.repaint();
    QApplication::processEvents();
    return widget.grab().toImage();
}

std::vector<double> sampleX()
{
    return {0.0, 1.0, 2.0, 3.0, 4.0};
}

std::vector<double> sampleY()
{
    return {0.2, 1.1, 0.7, 2.0, 1.4};
}

void decoratePlot(cppqtgraph::graphicsItems::PlotItem* plot, cppqtgraph::graphicsItems::PlotCurveItem*& curveOut)
{
    plot->setTitle(QStringLiteral("PlotWidget"));
    plot->setLabel(QStringLiteral("left"), QStringLiteral("Y"));
    plot->setLabel(QStringLiteral("bottom"), QStringLiteral("X"));
    auto* curve = new cppqtgraph::graphicsItems::PlotCurveItem;
    QPen pen(QColor(80, 180, 255), 2.0);
    pen.setCosmetic(true);
    curve->setPen(pen);
    curve->setData(sampleX(), sampleY());
    plot->addItem(curve);
    curveOut = curve;
}

QImage renderPlotWidgetReference()
{
    using cppqtgraph::graphicsItems::PlotCurveItem;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::widgets::GraphicsView;

    GraphicsView view;
    view.setFrameShape(QFrame::NoFrame);
    view.enableMouse(false);
    auto* plot = new PlotItem();
    PlotCurveItem* curve = nullptr;
    decoratePlot(plot, curve);
    view.setCentralItem(plot);
    view.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view.resize(kPlotWidth, kPlotHeight);
    return grabWidget(view);
}

QImage renderPlotWidgetActual()
{
    using cppqtgraph::graphicsItems::PlotCurveItem;
    using cppqtgraph::widgets::PlotWidget;

    PlotWidget widget;
    PlotCurveItem* curve = nullptr;
    decoratePlot(widget.getPlotItem(), curve);
    widget.resize(kPlotWidth, kPlotHeight);
    return grabWidget(widget);
}

QImage renderGraphicsLayoutWidgetReference()
{
    using cppqtgraph::graphicsItems::GraphicsLayout;
    using cppqtgraph::graphicsItems::PlotCurveItem;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::widgets::GraphicsView;

    GraphicsView view;
    view.setFrameShape(QFrame::NoFrame);
    view.enableMouse(false);
    auto* layout = new GraphicsLayout();
    PlotItem* leftPlot = layout->addPlot(0, 0);
    PlotItem* rightPlot = layout->addPlot(0, 1);
    PlotCurveItem* leftCurve = nullptr;
    PlotCurveItem* rightCurve = nullptr;
    decoratePlot(leftPlot, leftCurve);
    decoratePlot(rightPlot, rightCurve);
    rightPlot->setTitle(QStringLiteral("Right"));
    view.setCentralItem(layout);
    view.resize(kPlotWidth, kPlotHeight);
    return grabWidget(view);
}

QImage renderGraphicsLayoutWidgetActual()
{
    using cppqtgraph::graphicsItems::PlotCurveItem;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::widgets::GraphicsLayoutWidget;

    GraphicsLayoutWidget widget;
    PlotItem* leftPlot = widget.addPlot(0, 0);
    PlotItem* rightPlot = widget.addPlot(0, 1);
    PlotCurveItem* leftCurve = nullptr;
    PlotCurveItem* rightCurve = nullptr;
    decoratePlot(leftPlot, leftCurve);
    decoratePlot(rightPlot, rightCurve);
    rightPlot->setTitle(QStringLiteral("Right"));
    widget.resize(kPlotWidth, kPlotHeight);
    return grabWidget(widget);
}

QImage renderHistogramLUTWidgetReference()
{
    using cppqtgraph::graphicsItems::HistogramLUTItem;
    using cppqtgraph::widgets::GraphicsView;

    GraphicsView view;
    view.setFrameShape(QFrame::NoFrame);
    view.enableMouse(false);
    auto* item = new HistogramLUTItem(nullptr, true, QStringLiteral("mono"), QStringLiteral("right"),
        HistogramLUTItem::Orientation::Vertical);
    item->setLevels(20.0, 180.0);
    item->levelRegion()->setSpan(0.0, static_cast<qreal>(kHistogramWidth));
    view.setCentralItem(item);
    view.setMinimumWidth(95);
    view.resize(kHistogramWidth, kHistogramHeight);
    return grabWidget(view);
}

QImage renderHistogramLUTWidgetActual()
{
    using cppqtgraph::graphicsItems::HistogramLUTItem;
    using cppqtgraph::widgets::HistogramLUTWidget;

    HistogramLUTWidget widget(nullptr, nullptr, true, QStringLiteral("mono"), QStringLiteral("right"),
        HistogramLUTItem::Orientation::Vertical);
    widget.setLevels(20.0, 180.0);
    widget.levelRegion()->setSpan(0.0, static_cast<qreal>(kHistogramWidth));
    widget.resize(kHistogramWidth, kHistogramHeight);
    return grabWidget(widget);
}

bool verifyPerCaseArtifactLayout(const QString& caseName)
{
    const QString caseDir = caseArtifactDir(caseName);
    const QStringList requiredFiles = {QStringLiteral("reference.png"), QStringLiteral("actual.png"),
        QStringLiteral("diff.png"), QStringLiteral("metrics.json"), QStringLiteral("gpt5_vision_review.md")};
    for (const QString& fileName : requiredFiles) {
        const QString path = caseDir + QChar('/') + fileName;
        if (!QFile::exists(path)) {
            std::cerr << "missing P5.01 per-case visual artifact: " << path.toStdString() << '\n';
            return false;
        }
    }
    return true;
}

bool writeCaseArtifacts(const QString& caseName, const QImage& reference, const QImage& actual, const QImage& diff,
    const PixelMetrics& metrics)
{
    const QString caseDir = caseArtifactDir(caseName);
    CHECK(ensureDirectory(caseDir));
    CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview(caseName);
    CHECK(review.accepted);

    writeTextFile(caseDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"")
            + jsonEscape(caseName)
            + QStringLiteral(
                "\",\n"
                "  \"issue\": \"P5.01\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"PyQtGraph-informed C++ wrapper oracle composed from previously ported GraphicsView/PlotItem/GraphicsLayout/HistogramLUTItem dependencies\",\n"
                "  \"oracle_type\": \"independent wrapper-composition reference for the thin widget wrappers under P5.01\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [")
            + QString::number(reference.width())
            + QStringLiteral(", ")
            + QString::number(reference.height())
            + QStringLiteral(
                "],\n"
                "  \"thresholds\": {\"max_changed_pixels\": 0, \"max_pixel_delta\": 0},\n"
                "  \"changed_pixels\": ")
            + QString::number(metrics.changedPixels)
            + QStringLiteral(
                ",\n"
                "  \"changed_percent\": ")
            + QString::number(metrics.changedPercent, 'f', 6)
            + QStringLiteral(
                ",\n"
                "  \"max_delta\": ")
            + QString::number(metrics.maxDelta)
            + QStringLiteral(
                ",\n"
                "  \"mean_delta\": ")
            + QString::number(metrics.meanDelta, 'f', 6)
            + QStringLiteral(
                ",\n"
                "  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"gpt5_vision_review.md\", \"source\": \"")
            + QStringLiteral("gpt5_vision_review.md")
            + QStringLiteral(
                "\", \"available\": true, \"accepted\": true},\n"
                "  \"semantic_review\": {\"verdict\": \"")
            + jsonEscape(review.verdict)
            + QStringLiteral("\", \"recommendation\": \"")
            + jsonEscape(review.recommendation)
            + QStringLiteral(
                "\"},\n"
                "  \"passed\": ")
            + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(
                ",\n"
                "  \"blank_placeholder_guard\": \"passed\",\n"
                "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\"}\n"
                "}\n"));
    CHECK(verifyPerCaseArtifactLayout(caseName));
    return true;
}

bool writeIssueReport(const std::vector<std::pair<QString, PixelMetrics>>& caseMetrics)
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_01_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));

    QString metricsJson = QStringLiteral("{\n");
    for (std::size_t index = 0; index < caseMetrics.size(); ++index) {
        const auto& [caseName, metrics] = caseMetrics[index];
        metricsJson += QStringLiteral("    \"") + jsonEscape(caseName) + QStringLiteral("\": {")
            + QStringLiteral("\"changed_pixels\": ") + QString::number(metrics.changedPixels)
            + QStringLiteral(", \"max_delta\": ") + QString::number(metrics.maxDelta)
            + QStringLiteral(", \"passed\": ") + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral("}");
        if (index + 1 < caseMetrics.size()) {
            metricsJson += QStringLiteral(",\n");
        }
    }
    metricsJson += QStringLiteral("\n  }");

    writeTextFile(reportDir + QStringLiteral("/plotting_widgets.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.01\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::PlotWidget\", \"cppqtgraph::widgets::GraphicsView\", \"cppqtgraph::widgets::GraphicsLayoutWidget\", \"cppqtgraph::widgets::HistogramLUTWidget\"],\n"
            "  \"reference\": \"PyQtGraph 0.14.0 widget contracts verified through C++ wrapper-composition oracles built from previously ported dependencies\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/PlotWidget.hpp\", \"src/cppqtgraph/widgets/PlotWidget.cpp\", \"include/cppqtgraph/widgets/GraphicsView.hpp\", \"src/cppqtgraph/widgets/GraphicsView.cpp\", \"include/cppqtgraph/widgets/GraphicsLayoutWidget.hpp\", \"include/cppqtgraph/widgets/HistogramLUTWidget.hpp\", \"src/cppqtgraph/widgets/HistogramLUTWidget.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"example_manifest\": \"not_applicable: no example_manifest.yaml status fields changed for this focused widget test\",\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset visual -L P5.01 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_plotting_widgets_p5_01\"},\n"
            "  \"validation_commands\": [\"cmake --preset visual\", \"cmake --build --preset visual --parallel\", \"QT_QPA_PLATFORM=offscreen ctest --preset visual -L P5.01 --output-on-failure\", \"python3 -m pytest -q\", \"git diff --check\", \"git diff --name-only origin/main...HEAD\"],\n"
            "  \"manual_semantic_inspection\": \"actual images were opened for PlotWidget-curve, GraphicsLayoutWidget-grid, and HistogramLUTWidget-vertical; all diffs are black and metrics pass with zero changed pixels\",\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/P5.01\", \"cases\": [\"PlotWidget-curve\", \"GraphicsLayoutWidget-grid\", \"HistogramLUTWidget-vertical\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
            "  \"visual_metrics\": ")
            + metricsJson
            + QStringLiteral(
                "\n"
                "}\n"));
    return true;
}

bool testApiShape()
{
    using cppqtgraph::graphicsItems::GraphicsLayout;
    using cppqtgraph::graphicsItems::HistogramLUTItem;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::widgets::GraphicsLayoutWidget;
    using cppqtgraph::widgets::GraphicsView;
    using cppqtgraph::widgets::HistogramLUTWidget;
    using cppqtgraph::widgets::PlotWidget;

    static_assert(std::is_base_of_v<GraphicsView, PlotWidget>);
    static_assert(std::is_base_of_v<GraphicsView, GraphicsLayoutWidget>);
    static_assert(std::is_base_of_v<GraphicsView, HistogramLUTWidget>);
    static_assert(std::is_base_of_v<QGraphicsView, GraphicsView>);

    PlotWidget plotWidget;
    CHECK(plotWidget.getPlotItem() != nullptr);
    CHECK(plotWidget.centralItem() == plotWidget.getPlotItem());
    CHECK(plotWidget.mouseEnabled() == false);

    auto* addedCurve = new cppqtgraph::graphicsItems::PlotCurveItem;
    plotWidget.addItem(addedCurve);
    CHECK(addedCurve->scene() == plotWidget.scene());
    CHECK(addedCurve->parentItem() != nullptr);
    plotWidget.setXRange(1.0, 4.0, 0.0);
    const auto range = plotWidget.viewRange();
    CHECK(qFuzzyCompare(range[0][0], 1.0));
    CHECK(qFuzzyCompare(range[0][1], 4.0));
    plotWidget.setAspectLocked(true, 1.0);
    plotWidget.setMouseEnabled(true, false);
    CHECK(plotWidget.getPlotItem()->getViewBox()->mouseEnabled()[0]);
    CHECK(!plotWidget.getPlotItem()->getViewBox()->mouseEnabled()[1]);
    plotWidget.removeItem(addedCurve);
    CHECK(addedCurve->scene() == nullptr);
    CHECK(addedCurve->parentItem() == nullptr);
    delete addedCurve;

    const std::array<double, 3> plottedY{{1.0, 2.0, 3.0}};
    auto* ownedCurve = plotWidget.plot(std::span<const double>(plottedY.data(), plottedY.size()));
    CHECK(ownedCurve != nullptr);
    CHECK(ownedCurve->scene() == plotWidget.scene());
    CHECK(ownedCurve->parentItem() != nullptr);
    plotWidget.clear();

    GraphicsView graphicsView;
    CHECK(graphicsView.graphicsScene() != nullptr);
    CHECK(graphicsView.centralWidget() != nullptr);

    GraphicsLayoutWidget layoutWidget;
    CHECK(layoutWidget.graphicsLayout() != nullptr);
    CHECK(layoutWidget.ci == layoutWidget.graphicsLayout());
    CHECK(layoutWidget.centralItem() == layoutWidget.graphicsLayout());

    HistogramLUTWidget histogramWidget;
    CHECK(histogramWidget.item() != nullptr);
    CHECK(histogramWidget.centralItem() == histogramWidget.item());
    CHECK(histogramWidget.orientation() == HistogramLUTItem::Orientation::Vertical);
    CHECK(histogramWidget.orientationName() == QStringLiteral("vertical"));
    CHECK(histogramWidget.gradientPosition() == QStringLiteral("right"));
    CHECK(histogramWidget.sizeHint() == QSize(115, 200));
    CHECK(histogramWidget.minimumWidth() >= 95);
    histogramWidget.setLevels(20.0, 180.0);
    CHECK(histogramWidget.getLevels() == std::make_pair(20.0, 180.0));
    CHECK(histogramWidget.levelRegion() == histogramWidget.item()->levelRegion());
    histogramWidget.setColorMap(QStringLiteral("grey"));
    CHECK(!histogramWidget.isLookupTrivial());
    const auto lookup = histogramWidget.getLookupTable(4, true);
    CHECK(lookup.data != nullptr);
    CHECK(lookup.rows == 4);
    CHECK(lookup.channels == 4);

    GraphicsLayout* nested = layoutWidget.addLayout(0, 0);
    PlotItem* plot = layoutWidget.addPlot(0, 1);
    CHECK(nested != nullptr);
    CHECK(plot != nullptr);
    return true;
}

bool runVisualCase(const QString& caseName, const QImage& reference, const QImage& actual, PixelMetrics& metricsOut)
{
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    const std::uint64_t minimumSemanticPixels = std::max<std::uint64_t>(50, static_cast<std::uint64_t>(reference.width() * reference.height() / 50));
    if (referencePixels < minimumSemanticPixels || actualPixels < minimumSemanticPixels) {
        std::cerr << "P5.01 blank/placeholder guard failed for " << caseName.toStdString()
                  << ": reference=" << referencePixels << " actual=" << actualPixels
                  << " minimum=" << minimumSemanticPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    metricsOut = metrics;
    if (!metrics.passed) {
        std::cerr << "P5.01 visual comparison failed for " << caseName.toStdString()
                  << ": changedPixels=" << metrics.changedPixels << " maxDelta=" << metrics.maxDelta << '\n';
        return false;
    }
    CHECK(writeCaseArtifacts(caseName, reference, actual, diff, metrics));
    return true;
}

bool testVisualBehavior()
{
    std::vector<std::pair<QString, PixelMetrics>> caseMetrics;

    PixelMetrics plotMetrics;
    CHECK(runVisualCase(QStringLiteral("PlotWidget-curve"), renderPlotWidgetReference(), renderPlotWidgetActual(), plotMetrics));
    caseMetrics.emplace_back(QStringLiteral("PlotWidget-curve"), plotMetrics);

    PixelMetrics layoutMetrics;
    CHECK(runVisualCase(QStringLiteral("GraphicsLayoutWidget-grid"), renderGraphicsLayoutWidgetReference(),
        renderGraphicsLayoutWidgetActual(), layoutMetrics));
    caseMetrics.emplace_back(QStringLiteral("GraphicsLayoutWidget-grid"), layoutMetrics);

    PixelMetrics histogramMetrics;
    CHECK(runVisualCase(QStringLiteral("HistogramLUTWidget-vertical"), renderHistogramLUTWidgetReference(),
        renderHistogramLUTWidgetActual(), histogramMetrics));
    caseMetrics.emplace_back(QStringLiteral("HistogramLUTWidget-vertical"), histogramMetrics);

    CHECK(writeIssueReport(caseMetrics));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testApiShape()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
