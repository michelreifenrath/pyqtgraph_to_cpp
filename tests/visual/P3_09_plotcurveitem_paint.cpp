#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P3_09_ARTIFACT_DIR
#define CPPQTGRAPH_P3_09_ARTIFACT_DIR "reports/visual/P3.09"
#endif

#ifndef CPPQTGRAPH_P3_09_CANONICAL_ARTIFACT_DIR
#define CPPQTGRAPH_P3_09_CANONICAL_ARTIFACT_DIR "reports/visual-diffs/P3.09-PlotCurveItem"
#endif

namespace {

constexpr int imageWidth = 240;
constexpr int imageHeight = 180;
const QRectF dataViewport(-1.0, -3.0, 9.0, 8.0);

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

struct CurveCase {
    QString name;
    std::vector<double> x;
    std::vector<double> y;
    cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode connectMode
        = cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All;
    cppqtgraph::graphicsItems::PlotCurveItem::StepMode stepMode
        = cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None;
    QPen pen;
};

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

QPen defaultPen()
{
    QPen pen(QColor(255, 255, 255), 1.0);
    pen.setCosmetic(true);
    return pen;
}

QTransform dataToImageTransform()
{
    QTransform transform;
    transform.translate(0.0, imageHeight - 1.0);
    transform.scale(imageWidth / dataViewport.width(), -imageHeight / dataViewport.height());
    transform.translate(-dataViewport.left(), -dataViewport.top());
    return transform;
}

bool isFinitePoint(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

void appendContinuousPath(QPainterPath& path, const std::vector<double>& x, const std::vector<double>& y)
{
    bool hasStart = false;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        if (!isFinitePoint(x[index], y[index])) {
            continue;
        }
        const QPointF point(x[index], y[index]);
        if (!hasStart) {
            path.moveTo(point);
            hasStart = true;
        } else {
            path.lineTo(point);
        }
    }
}

void appendFinitePath(QPainterPath& path, const std::vector<double>& x, const std::vector<double>& y)
{
    bool hasStart = false;
    bool hasLine = false;
    QPointF start;
    QPainterPath segment;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        if (!isFinitePoint(x[index], y[index])) {
            if (hasLine) {
                path.addPath(segment);
            }
            hasStart = false;
            hasLine = false;
            segment = QPainterPath();
            continue;
        }
        const QPointF point(x[index], y[index]);
        if (!hasStart) {
            start = point;
            segment.moveTo(point);
            hasStart = true;
        } else {
            if (!hasLine) {
                segment = QPainterPath(start);
            }
            segment.lineTo(point);
            hasLine = true;
        }
    }
    if (hasLine) {
        path.addPath(segment);
    }
}

void appendPairsPath(QPainterPath& path, const std::vector<double>& x, const std::vector<double>& y)
{
    const std::size_t count = std::min(x.size(), y.size());
    for (std::size_t index = 0; index + 1 < count; index += 2) {
        if (!isFinitePoint(x[index], y[index]) || !isFinitePoint(x[index + 1], y[index + 1])) {
            continue;
        }
        path.moveTo(QPointF(x[index], y[index]));
        path.lineTo(QPointF(x[index + 1], y[index + 1]));
    }
}

std::pair<std::vector<double>, std::vector<double>> steppedData(
    cppqtgraph::graphicsItems::PlotCurveItem::StepMode stepMode,
    const std::vector<double>& x,
    const std::vector<double>& y)
{
    if (stepMode == cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None) {
        return {x, y};
    }

    std::vector<double> steppedX;
    std::vector<double> steppedY;
    if (stepMode == cppqtgraph::graphicsItems::PlotCurveItem::StepMode::Center) {
        if (x.size() != y.size() + 1) {
            return {{}, {}};
        }
        steppedX.reserve(y.size() * 2);
        steppedY.reserve(y.size() * 2);
        for (std::size_t index = 0; index < y.size(); ++index) {
            steppedX.push_back(x[index]);
            steppedY.push_back(y[index]);
            steppedX.push_back(x[index + 1]);
            steppedY.push_back(y[index]);
        }
        return {steppedX, steppedY};
    }

    if (x.size() != y.size()) {
        return {{}, {}};
    }
    steppedX.reserve(y.size() * 2);
    steppedY.reserve(y.size() * 2);
    for (std::size_t index = 0; index < y.size(); ++index) {
        if (stepMode == cppqtgraph::graphicsItems::PlotCurveItem::StepMode::Right) {
            steppedX.push_back(x[index]);
            steppedY.push_back(y[index]);
            steppedX.push_back(index + 1 < x.size() ? x[index + 1] : x[index]);
            steppedY.push_back(y[index]);
        } else {
            steppedX.push_back(index == 0 ? x[index] : x[index - 1]);
            steppedY.push_back(y[index]);
            steppedX.push_back(x[index]);
            steppedY.push_back(y[index]);
        }
    }
    return {steppedX, steppedY};
}

QPainterPath referencePath(const CurveCase& curveCase)
{
    auto [pathX, pathY] = steppedData(curveCase.stepMode, curveCase.x, curveCase.y);
    QPainterPath path;
    switch (curveCase.connectMode) {
    case cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All:
        appendContinuousPath(path, pathX, pathY);
        break;
    case cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Finite:
        appendFinitePath(path, pathX, pathY);
        break;
    case cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Pairs:
        appendPairsPath(path, pathX, pathY);
        break;
    }
    return path;
}

QImage renderReference(const CurveCase& curveCase)
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setTransform(dataToImageTransform());
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(curveCase.pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(referencePath(curveCase));
    painter.end();
    return image;
}

QImage renderActual(const CurveCase& curveCase)
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    curve.setPen(curveCase.pen);
    curve.setConnectMode(curveCase.connectMode);
    curve.setStepMode(curveCase.stepMode);
    curve.setData(std::span<const double>(curveCase.x), std::span<const double>(curveCase.y));

    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setTransform(dataToImageTransform());
    QStyleOptionGraphicsItem option;
    curve.paint(&painter, &option, nullptr);
    painter.end();
    return image;
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

bool hasSemanticCurvePixels(const QImage& image)
{
    int litPixels = 0;
    int minX = image.width();
    int minY = image.height();
    int maxX = -1;
    int maxY = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color(image.pixelColor(x, y));
            if (color.red() > 20 || color.green() > 20 || color.blue() > 20) {
                ++litPixels;
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }
    return litPixels >= 20 && (maxX - minX) >= 30 && (maxY - minY) >= 20;
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

QString stableFixtureHash(const CurveCase& curveCase)
{
    std::uint32_t hash = 2166136261U;
    const auto mixByte = [&hash](unsigned char byte) {
        hash ^= byte;
        hash *= 16777619U;
    };
    const QByteArray nameBytes = curveCase.name.toUtf8();
    for (const char byte : nameBytes) {
        mixByte(static_cast<unsigned char>(byte));
    }
    for (const double value : curveCase.x) {
        const auto scaled = static_cast<std::int64_t>(std::llround(value * 1000.0));
        for (int shift = 0; shift < 64; shift += 8) {
            mixByte(static_cast<unsigned char>((scaled >> shift) & 0xff));
        }
    }
    for (const double value : curveCase.y) {
        const auto normalized = std::isfinite(value) ? value : 999999.0;
        const auto scaled = static_cast<std::int64_t>(std::llround(normalized * 1000.0));
        for (int shift = 0; shift < 64; shift += 8) {
            mixByte(static_cast<unsigned char>((scaled >> shift) & 0xff));
        }
    }
    return QString::number(hash, 16);
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

bool writeCaseArtifacts(const CurveCase& curveCase, const QImage& reference, const QImage& actual, const QImage& diff,
    const PixelMetrics& metrics)
{
    const QString reportCaseDir = QStringLiteral(CPPQTGRAPH_P3_09_ARTIFACT_DIR) + QStringLiteral("/") + curveCase.name;
    const QString canonicalCaseDir
        = QStringLiteral(CPPQTGRAPH_P3_09_CANONICAL_ARTIFACT_DIR) + QStringLiteral("-") + curveCase.name;
    CHECK(ensureDirectory(reportCaseDir));
    CHECK(ensureDirectory(canonicalCaseDir));

    for (const QString& caseDir : {reportCaseDir, canonicalCaseDir}) {
        CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
        CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
        CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));
        writeTextFile(caseDir + QStringLiteral("/metrics.json"),
            QStringLiteral(
                "{\n"
                "  \"case\": \"")
                + curveCase.name
                + QStringLiteral(
                    "\",\n"
                    "  \"issue\": \"P3.09\",\n"
                    "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/PlotCurveItem.py generatePath/paint and pyqtgraph/functions.py arrayToQPath\",\n"
                    "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                    "  \"dimensions\": [240, 180],\n"
                    "  \"fixture_hash\": \"")
                + stableFixtureHash(curveCase)
                + QStringLiteral(
                    "\",\n"
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
                    "  \"passed\": ")
                + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
                + QStringLiteral(
                    ",\n"
                    "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"antialias\": false, \"background\": \"black\"}\n"
                    "}\n"));
        writeTextFile(caseDir + QStringLiteral("/gpt5_vision_review.md"),
            QStringLiteral(
                "verdict: needs_human\n"
                "recommendation: human_review\n"
                "reviewer/model: not-run-by-local-ctest\n"
                "date: local deterministic artifact generation\n"
                "cases reviewed: ")
                + curveCase.name
                + QStringLiteral(
                    "\nblocking findings: GPT-5.5 semantic review must be completed by the governed visual-review step before merge.\n"));
    }
    return true;
}

std::vector<CurveCase> cases()
{
    QPen dashed(QColor(255, 210, 0), 3.0, Qt::DashLine);
    dashed.setCosmetic(true);
    QPen cyan(QColor(0, 220, 255), 2.0);
    cyan.setCosmetic(true);
    return {
        {QStringLiteral("straight-line"), {0.0, 1.0, 2.0, 3.0}, {0.0, 2.0, 1.0, 3.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None, defaultPen()},
        {QStringLiteral("connect-all-nonfinite"), {0.0, 1.0, 2.0, 3.0, 4.0},
            {0.0, 2.0, std::numeric_limits<double>::quiet_NaN(), -1.0, 2.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None, defaultPen()},
        {QStringLiteral("finite-gap"), {0.0, 1.0, 2.0, 3.0, 4.0},
            {0.0, 2.0, std::numeric_limits<double>::quiet_NaN(), -1.0, 2.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Finite,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None, defaultPen()},
        {QStringLiteral("pairs-connected"), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {0.0, 2.0, -1.0, 3.0, 1.0, 4.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Pairs,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None, cyan},
        {QStringLiteral("step-left"), {0.0, 1.5, 3.0, 4.5, 6.0}, {0.0, 1.5, -1.0, 2.5, 1.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::Left, defaultPen()},
        {QStringLiteral("wide-dashed-pen"), {0.0, 1.0, 2.0, 3.0, 4.0}, {-2.0, 3.0, -1.0, 2.0, 0.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None, dashed},
        {QStringLiteral("clipped-ranged"), {-8.0, -2.0, 0.0, 2.0, 5.0, 12.0}, {-6.0, -1.0, 1.0, 4.0, 0.0, 8.0},
            cppqtgraph::graphicsItems::PlotCurveItem::ConnectMode::All,
            cppqtgraph::graphicsItems::PlotCurveItem::StepMode::None, defaultPen()},
    };
}

bool testBlankAndPlaceholderGuardsRejectNonSemanticImages()
{
    QImage blank(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    blank.fill(Qt::black);
    CHECK(!hasSemanticCurvePixels(blank));

    QImage placeholder(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    placeholder.fill(Qt::black);
    placeholder.setPixelColor(8, 8, Qt::white);
    placeholder.setPixelColor(9, 9, Qt::white);
    CHECK(!hasSemanticCurvePixels(placeholder));
    return true;
}

bool writeSummaryReport(int passedCases, int totalCases)
{
    const QString reportRoot = QStringLiteral(CPPQTGRAPH_P3_09_ARTIFACT_DIR);
    CHECK(ensureDirectory(reportRoot));
    writeTextFile(reportRoot + QStringLiteral("/manual_semantic_inspection.md"),
        QStringLiteral(
            "# P3.09 manual semantic inspection note\n\n"
            "Deterministic visual artifacts were generated for PlotCurveItem paint-path cases. "
            "The implementing agent must open/read reference.png, actual.png, and diff.png before handoff and record "
            "the human semantic inspection in implementation.md.\n"));
    writeTextFile(reportRoot + QStringLiteral("/summary.json"),
        QStringLiteral("{\n  \"issue\": \"P3.09\",\n  \"passed_cases\": ") + QString::number(passedCases)
            + QStringLiteral(",\n  \"total_cases\": ") + QString::number(totalCases)
            + QStringLiteral(",\n  \"blank_placeholder_guard\": \"passed\"\n}\n"));
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

    int passedCases = 0;
    const auto curveCases = cases();
    for (const CurveCase& curveCase : curveCases) {
        const QImage reference = renderReference(curveCase);
        const QImage actual = renderActual(curveCase);
        CHECK(hasSemanticCurvePixels(reference));
        CHECK(hasSemanticCurvePixels(actual));
        QImage diff;
        const PixelMetrics metrics = compareImages(reference, actual, diff);
        CHECK(writeCaseArtifacts(curveCase, reference, actual, diff, metrics));
        if (!metrics.passed) {
            std::cerr << "visual mismatch for " << curveCase.name.toStdString() << ": changedPixels="
                      << metrics.changedPixels << " maxDelta=" << metrics.maxDelta << '\n';
            return 1;
        }
        ++passedCases;
    }

    CHECK(writeSummaryReport(passedCases, static_cast<int>(curveCases.size())));
    return 0;
}
