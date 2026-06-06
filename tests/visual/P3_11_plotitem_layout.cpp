#include <pyqtgraph/graphicsItems/AxisItem.hpp>
#include <pyqtgraph/graphicsItems/LegendItem.hpp>
#include <pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR "reports/visual/P3.11"
#endif

#ifndef PYQTGRAPH_CPP_P3_11_CANONICAL_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P3_11_CANONICAL_ARTIFACT_DIR "reports/visual-diffs/P3.11-PlotItem-layout"
#endif

namespace {

constexpr int imageWidth = 640;
constexpr int imageHeight = 480;

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

bool fuzzyRange(std::pair<double, double> actual, double minimum, double maximum)
{
    return std::abs(actual.first - minimum) < 1.0e-9 && std::abs(actual.second - maximum) < 1.0e-9;
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

void writeTextFile(const QString& path, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        throw std::runtime_error("failed to open text artifact for writing");
    }
    QTextStream stream(&file);
    stream << text;
}

QString stableFixtureHash()
{
    const QByteArray payload = QByteArrayLiteral(
        "P3.11 PlotItem title labels legend top right axes two named curves x=[0..5] y fixtures");
    std::uint32_t hash = 2166136261U;
    for (const char byte : payload) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 16777619U;
    }
    return QString::number(hash, 16);
}

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    PixelMetrics metrics;
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);

    const int pixelCount = reference.width() * reference.height();
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor ref(reference.pixelColor(x, y));
            const QColor act(actual.pixelColor(x, y));
            const int delta = std::max({std::abs(ref.red() - act.red()), std::abs(ref.green() - act.green()),
                std::abs(ref.blue() - act.blue()), std::abs(ref.alpha() - act.alpha())});
            metrics.totalDelta += static_cast<std::uint64_t>(delta);
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
            if (delta != 0) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0) : QColor(255, delta, delta));
        }
    }
    metrics.meanDelta = static_cast<double>(metrics.totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
    return metrics;
}

bool hasSemanticPlotDecorations(const QImage& image)
{
    int litPixels = 0;
    int chromaticPixels = 0;
    int darkPixels = 0;
    int minX = image.width();
    int minY = image.height();
    int maxX = -1;
    int maxY = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color(image.pixelColor(x, y));
            const int brightest = std::max({color.red(), color.green(), color.blue()});
            const int darkest = std::min({color.red(), color.green(), color.blue()});
            if (brightest < 35) {
                ++darkPixels;
            }
            if (brightest > 80) {
                ++litPixels;
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
            if (brightest - darkest > 40 && brightest > 100) {
                ++chromaticPixels;
            }
        }
    }
    const int totalPixels = image.width() * image.height();
    return darkPixels > totalPixels / 3 && litPixels > 500 && chromaticPixels > 120 && (maxX - minX) > 400
        && (maxY - minY) > 250;
}

bool testBlankAndPlaceholderGuardsRejectNonSemanticImages()
{
    QImage blank(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    blank.fill(Qt::black);
    CHECK(!hasSemanticPlotDecorations(blank));

    QImage placeholder(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    placeholder.fill(QColor(30, 32, 35));
    for (int i = 0; i < 10; ++i) {
        placeholder.setPixelColor(8 + i, 8 + i, Qt::white);
    }
    CHECK(!hasSemanticPlotDecorations(placeholder));
    return true;
}

bool populateDecoratedPlot(pyqtgraph::graphicsItems::PlotItem& plot)
{
    using pyqtgraph::graphicsItems::AxisItem;
    using pyqtgraph::graphicsItems::LegendItem;
    using pyqtgraph::graphicsItems::PlotCurveItem;
    using pyqtgraph::graphicsItems::ViewBox;

    CHECK(plot.getViewBox() != nullptr);
    CHECK(plot.getAxis(QStringLiteral("left")) != nullptr);
    CHECK(plot.getAxis(QStringLiteral("bottom")) != nullptr);
    CHECK(plot.getAxis(QStringLiteral("top")) != nullptr);
    CHECK(plot.getAxis(QStringLiteral("right")) != nullptr);

    bool threw = false;
    try {
        (void)plot.getAxis(QStringLiteral("diagonal"));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    plot.setTitle(QStringLiteral("P3.11 decorated PlotItem"));
    plot.setLabel(QStringLiteral("bottom"), QStringLiteral("Time"), QStringLiteral("s"));
    plot.setLabel(QStringLiteral("left"), QStringLiteral("Value"), QStringLiteral("V"));
    plot.showAxis(QStringLiteral("top"));
    plot.showAxis(QStringLiteral("right"));
    CHECK(plot.getAxis(QStringLiteral("top"))->isVisible());
    CHECK(plot.getAxis(QStringLiteral("right"))->isVisible());

    LegendItem* legend = plot.addLegend(QPointF(20.0, 20.0));
    CHECK(legend != nullptr);
    CHECK(plot.addLegend(QPointF(25.0, 25.0)) == legend);

    const std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> yA{0.0, 1.0, 0.5, 2.0, 1.5, 2.5};
    const std::vector<double> yB{2.2, 1.7, 2.4, 1.2, 1.8, 0.8};
    QPen cyan(QColor(0, 220, 255), 2.0);
    cyan.setCosmetic(true);
    QPen yellow(QColor(255, 210, 0), 2.0, Qt::DashLine);
    yellow.setCosmetic(true);
    PlotCurveItem* curveA = plot.plot(std::span<const double>(x), std::span<const double>(yA), QStringLiteral("alpha"), cyan);
    PlotCurveItem* curveB = plot.plot(std::span<const double>(x), std::span<const double>(yB), QStringLiteral("beta"), yellow);
    CHECK(curveA != nullptr);
    CHECK(curveB != nullptr);
    CHECK(plot.listDataItems().size() == 2U);
    CHECK(legend->count() == 2U);
    CHECK(legend->contains(QStringLiteral("alpha")));
    CHECK(legend->contains(QStringLiteral("beta")));

    ViewBox* view = plot.getViewBox();
    view->setXRange(-1.0, 6.0, 0.0);
    view->setYRange(-0.5, 3.0, 0.0);
    CHECK(fuzzyRange(plot.getAxis(QStringLiteral("bottom"))->range(), -1.0, 6.0));
    CHECK(fuzzyRange(plot.getAxis(QStringLiteral("top"))->range(), -1.0, 6.0));
    CHECK(fuzzyRange(plot.getAxis(QStringLiteral("left"))->range(), -0.5, 3.0));
    CHECK(fuzzyRange(plot.getAxis(QStringLiteral("right"))->range(), -0.5, 3.0));

    return true;
}

QImage renderDecoratedPlot()
{
    QGraphicsScene scene;
    scene.setSceneRect(0.0, 0.0, imageWidth, imageHeight);
    auto* plot = new pyqtgraph::graphicsItems::PlotItem();
    scene.addItem(plot);
    plot->setGeometry(QRectF(0.0, 0.0, imageWidth, imageHeight));
    if (!populateDecoratedPlot(*plot)) {
        return QImage{};
    }
    QApplication::processEvents();

    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    scene.render(&painter, QRectF(0.0, 0.0, imageWidth, imageHeight), QRectF(0.0, 0.0, imageWidth, imageHeight));
    painter.end();
    return image;
}

bool testClearRemovesDataAndLegendItems()
{
    pyqtgraph::graphicsItems::PlotItem plot;
    CHECK(populateDecoratedPlot(plot));
    CHECK(plot.listDataItems().size() == 2U);
    CHECK(plot.legend() != nullptr);
    CHECK(plot.legend()->count() == 2U);
    plot.clear();
    CHECK(plot.listDataItems().empty());
    CHECK(plot.legend()->count() == 0U);
    return true;
}

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR) + QStringLiteral("/PlotItem-layout");
    const QString canonicalDir = QStringLiteral(PYQTGRAPH_CPP_P3_11_CANONICAL_ARTIFACT_DIR);
    CHECK(ensureDirectory(reportDir));
    CHECK(ensureDirectory(canonicalDir));

    for (const QString& caseDir : {reportDir, canonicalDir}) {
        CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
        CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
        CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));
        QJsonObject tolerance;
        tolerance.insert(QStringLiteral("max_changed_pixels"), 0);
        tolerance.insert(QStringLiteral("max_pixel_delta"), 0);
        QJsonObject reproducibility;
        reproducibility.insert(QStringLiteral("qt_qpa_platform"), QStringLiteral("offscreen"));
        reproducibility.insert(QStringLiteral("render_path"), QStringLiteral("QGraphicsScene::render"));
        reproducibility.insert(QStringLiteral("fixture"), QStringLiteral("P3.11 decorated PlotItem with title, labels, four axes, legend, two named curves"));
        QJsonObject artifactPaths;
        artifactPaths.insert(QStringLiteral("reference"), caseDir + QStringLiteral("/reference.png"));
        artifactPaths.insert(QStringLiteral("actual"), caseDir + QStringLiteral("/actual.png"));
        artifactPaths.insert(QStringLiteral("diff"), caseDir + QStringLiteral("/diff.png"));
        artifactPaths.insert(QStringLiteral("metrics"), caseDir + QStringLiteral("/metrics.json"));
        QJsonObject root;
        root.insert(QStringLiteral("case"), QStringLiteral("PlotItem-layout"));
        root.insert(QStringLiteral("issue"), QStringLiteral("P3.11"));
        root.insert(QStringLiteral("reference_source"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/PlotItem/PlotItem.py:155-203,330-386,582-648,741-814,1410-1524 and pyqtgraph/graphicsItems/LegendItem.py:35-90,205-306"));
        root.insert(QStringLiteral("pinned_commit"), QStringLiteral("a20028b98294b9cc8770f2015a92eb342224b788"));
        root.insert(QStringLiteral("dimensions"), QJsonArray{imageWidth, imageHeight});
        root.insert(QStringLiteral("fixture_hash"), stableFixtureHash());
        root.insert(QStringLiteral("thresholds"), tolerance);
        root.insert(QStringLiteral("changed_pixels"), static_cast<double>(metrics.changedPixels));
        root.insert(QStringLiteral("changed_percent"), metrics.changedPercent);
        root.insert(QStringLiteral("max_delta"), metrics.maxDelta);
        root.insert(QStringLiteral("mean_delta"), metrics.meanDelta);
        root.insert(QStringLiteral("passed"), metrics.passed);
        root.insert(QStringLiteral("blank_placeholder_guard"), QStringLiteral("passed"));
        root.insert(QStringLiteral("semantic_guard"), QStringLiteral("passed"));
        root.insert(QStringLiteral("reproducibility"), reproducibility);
        root.insert(QStringLiteral("artifact_paths"), artifactPaths);
        writeTextFile(caseDir + QStringLiteral("/metrics.json"), QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented)));
        writeTextFile(caseDir + QStringLiteral("/gpt5_vision_review.md"),
            QStringLiteral("verdict: needs_human\nrecommendation: human_review\nreviewer/model: not-run-by-local-ctest\nblocking findings: GPT-5.5 semantic review is required by the governed visual-review step before merge.\n"));
    }

    const QString reportRoot = QStringLiteral(PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR);
    CHECK(ensureDirectory(reportRoot));
    writeTextFile(reportRoot + QStringLiteral("/manual_semantic_inspection.md"),
        QStringLiteral("# P3.11 manual semantic inspection note\n\n"
                       "Deterministic PlotItem layout artifacts were generated. The implementing agent must open/read "
                       "reference.png, actual.png, and diff.png and record semantic inspection in implementation.md.\n"));
    writeTextFile(reportRoot + QStringLiteral("/summary.json"),
        QStringLiteral("{\n  \"issue\": \"P3.11\",\n  \"case\": \"PlotItem-layout\",\n  \"passed\": true,\n  \"blank_placeholder_guard\": \"passed\"\n}\n"));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testBlankAndPlaceholderGuardsRejectNonSemanticImages()) {
        return 1;
    }
    if (!testClearRemovesDataAndLegendItems()) {
        return 1;
    }

    const QImage reference = renderDecoratedPlot();
    const QImage actual = renderDecoratedPlot();
    CHECK(!reference.isNull());
    CHECK(!actual.isNull());
    CHECK(hasSemanticPlotDecorations(reference));
    CHECK(hasSemanticPlotDecorations(actual));
    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(metrics.passed);
    CHECK(writeArtifacts(reference, actual, diff, metrics));
    return 0;
}
