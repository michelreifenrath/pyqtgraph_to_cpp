#include <pyqtgraph/functions.hpp>
#include <pyqtgraph/graphicsItems/ScatterPlotItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtGlobal>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P4_01_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_01_ARTIFACT_DIR "reports/visual/P4.01"
#endif

#ifndef PYQTGRAPH_CPP_P4_01_CANONICAL_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_01_CANONICAL_ARTIFACT_DIR "reports/visual-diffs/P4.01-ScatterPlotItem"
#endif

namespace {

constexpr int imageWidth = 420;
constexpr int imageHeight = 260;
const QRectF dataViewport(-0.9, -0.9, 8.0, 6.0);

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

struct ScatterCase {
    QString name;
    bool pxMode = true;
    bool useCache = true;
    bool antialias = true;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<QString> symbols;
    std::vector<qreal> sizes;
    std::vector<QPen> pens;
    std::vector<QBrush> brushes;
};

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

QTransform dataToImageTransform()
{
    QTransform transform;
    transform.translate(0.0, imageHeight - 1.0);
    transform.scale(imageWidth / dataViewport.width(), -imageHeight / dataViewport.height());
    transform.translate(-dataViewport.left(), -dataViewport.top());
    return transform;
}

void drawReferenceSymbol(QPainter& painter, const QString& symbol, qreal size, const QPen& pen, const QBrush& brush)
{
    painter.scale(size, size);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawPath(pyqtgraph::symbolPath(symbol));
}

QImage renderReferenceSymbol(const QString& symbol, qreal size, const QPen& pen, const QBrush& brush, qreal dpr = 1.0)
{
    const int penPxWidth = std::max(static_cast<int>(std::ceil(pen.widthF())), 1);
    const int side = static_cast<int>(std::ceil(dpr * (size + penPxWidth)));
    QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(image.width() / dpr * 0.5, image.height() / dpr * 0.5);
    drawReferenceSymbol(painter, symbol, size, pen, brush);
    painter.end();
    return image;
}

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    return image;
}

QImage renderReference(const ScatterCase& scatterCase)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setTransform(dataToImageTransform());

    if (scatterCase.pxMode) {
        const QTransform world = painter.worldTransform();
        painter.resetTransform();
        painter.setRenderHint(QPainter::Antialiasing, scatterCase.antialias);
        for (std::size_t index = 0; index < scatterCase.x.size(); ++index) {
            const QPointF point = world.map(QPointF(scatterCase.x[index], scatterCase.y[index]));
            if (scatterCase.useCache) {
                const QImage symbol = renderReferenceSymbol(
                    scatterCase.symbols[index], scatterCase.sizes[index], scatterCase.pens[index], scatterCase.brushes[index]);
                const QRectF target(point.x() - symbol.width() * 0.5, point.y() - symbol.height() * 0.5,
                    symbol.width(), symbol.height());
                painter.drawImage(target, symbol, QRectF(0.0, 0.0, symbol.width(), symbol.height()));
            } else {
                painter.save();
                painter.translate(point);
                drawReferenceSymbol(
                    painter, scatterCase.symbols[index], scatterCase.sizes[index], scatterCase.pens[index], scatterCase.brushes[index]);
                painter.restore();
            }
        }
    } else {
        painter.setRenderHint(QPainter::Antialiasing, scatterCase.antialias);
        for (std::size_t index = 0; index < scatterCase.x.size(); ++index) {
            painter.save();
            painter.translate(scatterCase.x[index], scatterCase.y[index]);
            drawReferenceSymbol(
                painter, scatterCase.symbols[index], scatterCase.sizes[index], scatterCase.pens[index], scatterCase.brushes[index]);
            painter.restore();
        }
    }
    painter.end();
    return image;
}

QImage renderActual(const ScatterCase& scatterCase)
{
    pyqtgraph::graphicsItems::ScatterPlotItem item;
    item.setPxMode(scatterCase.pxMode);
    item.setUseCache(scatterCase.useCache);
    item.setAntialias(scatterCase.antialias);
    item.setData(std::span<const double>(scatterCase.x), std::span<const double>(scatterCase.y));
    item.setSymbols(std::span<const QString>(scatterCase.symbols));
    item.setSizes(std::span<const qreal>(scatterCase.sizes));
    item.setPens(std::span<const QPen>(scatterCase.pens));
    item.setBrushes(std::span<const QBrush>(scatterCase.brushes));

    QImage image = blankImage();
    QPainter painter(&image);
    painter.setTransform(dataToImageTransform());
    QStyleOptionGraphicsItem option;
    item.paint(&painter, &option, nullptr);
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
    metrics.passed = metrics.changedPixels <= 8 && metrics.maxDelta <= 2;
    return metrics;
}

bool hasSemanticScatterPixels(const QImage& image)
{
    int litPixels = 0;
    int minX = image.width();
    int minY = image.height();
    int maxX = -1;
    int maxY = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color(image.pixelColor(x, y));
            if (color.red() > 25 || color.green() > 25 || color.blue() > 25) {
                ++litPixels;
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }
    return litPixels >= 120 && (maxX - minX) >= 230 && (maxY - minY) >= 80;
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

QString stableFixtureHash(const ScatterCase& scatterCase)
{
    std::uint32_t hash = 2166136261U;
    const auto mixByte = [&hash](unsigned char byte) {
        hash ^= byte;
        hash *= 16777619U;
    };
    const QByteArray nameBytes = scatterCase.name.toUtf8();
    for (const char byte : nameBytes) {
        mixByte(static_cast<unsigned char>(byte));
    }
    for (std::size_t index = 0; index < scatterCase.x.size(); ++index) {
        const auto mixScaled = [&](double value) {
            const auto scaled = static_cast<std::int64_t>(std::llround(value * 1000.0));
            for (int shift = 0; shift < 64; shift += 8) {
                mixByte(static_cast<unsigned char>((scaled >> shift) & 0xff));
            }
        };
        mixScaled(scatterCase.x[index]);
        mixScaled(scatterCase.y[index]);
        mixScaled(scatterCase.sizes[index]);
        for (const char byte : scatterCase.symbols[index].toUtf8()) {
            mixByte(static_cast<unsigned char>(byte));
        }
    }
    return QString::number(hash, 16);
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

bool writeCaseArtifacts(const ScatterCase& scatterCase, const QImage& reference, const QImage& actual, const QImage& diff,
    const PixelMetrics& metrics)
{
    const QString reportCaseDir = QStringLiteral(PYQTGRAPH_CPP_P4_01_ARTIFACT_DIR) + QStringLiteral("/") + scatterCase.name;
    const QString canonicalCaseDir
        = QStringLiteral(PYQTGRAPH_CPP_P4_01_CANONICAL_ARTIFACT_DIR) + QStringLiteral("-") + scatterCase.name;
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
                + scatterCase.name
                + QStringLiteral(
                    "\",\n"
                    "  \"issue\": \"P4.01\",\n"
                    "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                    "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ScatterPlotItem.py name_list, drawSymbol, renderSymbol, SymbolAtlas, paint\",\n"
                    "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                    "  \"dimensions\": [420, 260],\n"
                    "  \"fixture_hash\": \"")
                + stableFixtureHash(scatterCase)
                + QStringLiteral(
                    "\",\n"
                    "  \"thresholds\": {\"max_changed_pixels\": 8, \"max_pixel_delta\": 2},\n"
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
                    "  \"blank_placeholder_guard\": \"passed\",\n"
                    "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"black\", \"dpr\": 1.0}\n"
                    "}\n"));
    }
    return true;
}

QPen cosmeticPen(const QColor& color, qreal width)
{
    QPen pen(color, width);
    pen.setCosmetic(true);
    return pen;
}

QPen nonCosmeticPen(const QColor& color, qreal width)
{
    QPen pen(color, width);
    pen.setCosmetic(false);
    return pen;
}

std::vector<QString> upstreamSymbols()
{
    std::vector<QString> names;
    for (const auto& entry : pyqtgraph::symbolPaths()) {
        names.push_back(entry.first);
    }
    return names;
}

ScatterCase makePxModeAtlasCase()
{
    ScatterCase scatterCase;
    scatterCase.name = QStringLiteral("pxmode-cache-symbol-atlas");
    scatterCase.pxMode = true;
    scatterCase.useCache = true;
    scatterCase.antialias = true;
    scatterCase.symbols = upstreamSymbols();
    for (std::size_t index = 0; index < scatterCase.symbols.size(); ++index) {
        scatterCase.x.push_back(static_cast<double>(index % 5) * 1.55);
        scatterCase.y.push_back(static_cast<double>(index / 5) * 1.35);
        scatterCase.sizes.push_back(11.0 + static_cast<qreal>(index % 4) * 2.0);
        scatterCase.pens.push_back(cosmeticPen(QColor::fromHsv(static_cast<int>((index * 31) % 360), 220, 255), 1.0 + (index % 3)));
        scatterCase.brushes.push_back(QBrush(QColor::fromHsv(static_cast<int>((index * 47 + 90) % 360), 170, 210, 210)));
    }
    return scatterCase;
}

ScatterCase makeScaledDirectCase()
{
    ScatterCase scatterCase;
    scatterCase.name = QStringLiteral("scaled-direct-symbols");
    scatterCase.pxMode = false;
    scatterCase.useCache = false;
    scatterCase.antialias = false;
    scatterCase.symbols = {QStringLiteral("o"), QStringLiteral("s"), QStringLiteral("t"), QStringLiteral("d"),
        QStringLiteral("+"), QStringLiteral("x"), QStringLiteral("star"), QStringLiteral("arrow_right")};
    for (std::size_t index = 0; index < scatterCase.symbols.size(); ++index) {
        scatterCase.x.push_back(0.5 + static_cast<double>(index % 4) * 1.75);
        scatterCase.y.push_back(0.6 + static_cast<double>(index / 4) * 2.2);
        scatterCase.sizes.push_back(0.22 + static_cast<qreal>(index % 3) * 0.05);
        scatterCase.pens.push_back(nonCosmeticPen(QColor(255, 240 - static_cast<int>(index * 12), 80), 0.025));
        scatterCase.brushes.push_back(QBrush(QColor(70, 130 + static_cast<int>(index * 12), 235, 230)));
    }
    return scatterCase;
}

std::vector<ScatterCase> cases()
{
    return {makePxModeAtlasCase(), makeScaledDirectCase()};
}

bool testBlankAndPlaceholderGuardsRejectNonSemanticImages()
{
    QImage blank = blankImage();
    CHECK(!hasSemanticScatterPixels(blank));

    QImage placeholder = blankImage();
    placeholder.setPixelColor(8, 8, Qt::white);
    placeholder.setPixelColor(9, 9, Qt::white);
    placeholder.setPixelColor(10, 10, Qt::white);
    CHECK(!hasSemanticScatterPixels(placeholder));
    return true;
}

bool expectInvalidArgument(auto&& callable)
{
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool testScatterPlotItemMutatingSettersValidateFirst()
{
    pyqtgraph::graphicsItems::ScatterPlotItem item;
    std::vector<double> x{1.0, 2.0};
    std::vector<double> y{3.0, 4.0};
    std::vector<QString> symbols{QStringLiteral("s"), QStringLiteral("t")};
    std::vector<qreal> sizes{6.0, 8.0};
    item.setData(x, y);
    item.setSymbols(symbols);
    item.setSizes(sizes);

    CHECK(expectInvalidArgument([&]() {
        std::vector<double> invalidX{9.0};
        std::vector<double> invalidY;
        item.setData(invalidX, invalidY);
    }));
    CHECK(item.xData().size() == 2);
    CHECK(item.xData()[0] == 1.0);
    CHECK(item.yData()[1] == 4.0);

    CHECK(expectInvalidArgument([&]() { item.setSymbol(QStringLiteral("not-a-symbol")); }));
    CHECK(item.symbol() == QStringLiteral("o"));

    CHECK(expectInvalidArgument([&]() {
        std::vector<QString> invalidSymbols{QStringLiteral("x"), QStringLiteral("not-a-symbol")};
        item.setSymbols(invalidSymbols);
    }));
    const auto pointsAfterInvalidSymbols = item.points();
    CHECK(pointsAfterInvalidSymbols[0].symbol() == QStringLiteral("s"));
    CHECK(pointsAfterInvalidSymbols[1].symbol() == QStringLiteral("t"));

    CHECK(expectInvalidArgument([&]() { item.setSize(std::numeric_limits<qreal>::quiet_NaN()); }));
    CHECK(item.size() == 7.0);

    CHECK(expectInvalidArgument([&]() {
        std::vector<qreal> invalidSizes{5.0, std::numeric_limits<qreal>::infinity()};
        item.setSizes(invalidSizes);
    }));
    const auto pointsAfterInvalidSizes = item.points();
    CHECK(pointsAfterInvalidSizes[0].size() == 6.0);
    CHECK(pointsAfterInvalidSizes[1].size() == 8.0);

    CHECK(expectInvalidArgument([&]() {
        (void)pyqtgraph::graphicsItems::renderSymbol(
            QStringLiteral("o"), std::numeric_limits<qreal>::quiet_NaN(), QPen(Qt::white), QBrush(Qt::white));
    }));

    item.setData(item.xData(), item.yData());
    CHECK(item.xData().size() == 2);
    CHECK(item.xData()[0] == 1.0);
    CHECK(item.yData()[1] == 4.0);
    item.addPoints(item.xData(), item.yData());
    CHECK(item.xData().size() == 4);
    CHECK(item.xData()[2] == 1.0);
    CHECK(item.yData()[3] == 4.0);
    return true;
}

bool testScatterPlotItemNonFiniteDataKeepsEmptyBounds()
{
    pyqtgraph::graphicsItems::ScatterPlotItem item;
    std::vector<double> x{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()};
    std::vector<double> y{-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    item.setData(x, y);
    CHECK(item.boundingRect().isNull());
    const auto [xMin, xMax] = item.dataBounds(0);
    const auto [yMin, yMax] = item.dataBounds(1);
    CHECK(std::isnan(static_cast<double>(xMin)));
    CHECK(std::isnan(static_cast<double>(xMax)));
    CHECK(std::isnan(static_cast<double>(yMin)));
    CHECK(std::isnan(static_cast<double>(yMax)));

    x = {1.0, 2.0, 3.0};
    y = {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), 4.0};
    item.setData(x, y);
    const auto [finiteXMin, finiteXMax] = item.dataBounds(0);
    const auto [finiteYMin, finiteYMax] = item.dataBounds(1);
    CHECK(finiteXMin == 3.0);
    CHECK(finiteXMax == 3.0);
    CHECK(finiteYMin == 4.0);
    CHECK(finiteYMax == 4.0);
    CHECK(!item.boundingRect().isNull());
    return true;
}

bool testSymbolAtlasKeysIncludeGradientBrushState()
{
    QLinearGradient horizontal(0.0, 0.0, 1.0, 0.0);
    horizontal.setColorAt(0.0, Qt::red);
    horizontal.setColorAt(1.0, Qt::blue);
    QLinearGradient vertical(0.0, 0.0, 0.0, 1.0);
    vertical.setColorAt(0.0, Qt::red);
    vertical.setColorAt(1.0, Qt::blue);

    pyqtgraph::graphicsItems::SymbolAtlas atlas;
    const QPen pen(Qt::white, 1.0);
    const QRect first = atlas.sourceRect(QStringLiteral("o"), 13.0, pen, QBrush(horizontal));
    const QRect second = atlas.sourceRect(QStringLiteral("o"), 13.0, pen, QBrush(vertical));
    CHECK(!first.isEmpty());
    CHECK(!second.isEmpty());
    CHECK(first != second);
    CHECK(atlas.size() == 2);
    return true;
}

bool testScatterPlotItemCurrentCachePaddingAndPixelHitScale()
{
    pyqtgraph::graphicsItems::ScatterPlotItem item;
    item.setPxMode(true);
    item.setUseCache(true);
    std::vector<double> x{0.0};
    std::vector<double> y{0.0};
    item.setData(x, y);
    item.setSize(40.0);
    const qreal largePadding = item.pixelPadding();
    item.setSize(4.0);
    CHECK(item.pixelPadding() < largePadding * 0.5);

    QGraphicsScene scene;
    scene.addItem(&item);
    QGraphicsView view(&scene);
    view.resize(240, 240);
    view.setTransform(QTransform::fromScale(10.0, 10.0));

    item.setSize(20.0);
    CHECK(item.pointsAt(QPointF(1.4, 0.0)).empty());
    CHECK(!item.pointsAt(QPointF(0.7, 0.0)).empty());
    scene.removeItem(&item);
    return true;
}

bool writeSummaryReport(int passedCases, int totalCases)
{
    const QString reportRoot = QStringLiteral(PYQTGRAPH_CPP_P4_01_ARTIFACT_DIR);
    CHECK(ensureDirectory(reportRoot));
    writeTextFile(reportRoot + QStringLiteral("/manual_semantic_inspection.md"),
        QStringLiteral(
            "# P4.01 manual semantic inspection note\n\n"
            "Deterministic visual artifacts were generated for ScatterPlotItem symbol atlas cases. "
            "The implementing agent must open/read reference.png, actual.png, and diff.png before handoff and record "
            "the human semantic inspection in implementation.md.\n"));
    writeTextFile(reportRoot + QStringLiteral("/summary.json"),
        QStringLiteral("{\n  \"issue\": \"P4.01\",\n  \"passed_cases\": ") + QString::number(passedCases)
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
    if (!testScatterPlotItemMutatingSettersValidateFirst()) {
        return 1;
    }
    if (!testScatterPlotItemNonFiniteDataKeepsEmptyBounds()) {
        return 1;
    }
    if (!testSymbolAtlasKeysIncludeGradientBrushState()) {
        return 1;
    }
    if (!testScatterPlotItemCurrentCachePaddingAndPixelHitScale()) {
        return 1;
    }

    int passedCases = 0;
    const auto scatterCases = cases();
    for (const ScatterCase& scatterCase : scatterCases) {
        const QImage reference = renderReference(scatterCase);
        const QImage actual = renderActual(scatterCase);
        if (!hasSemanticScatterPixels(reference)) {
            std::cerr << "reference image failed semantic scatter blank guard for " << scatterCase.name.toStdString()
                      << '\n';
            return 1;
        }
        if (!hasSemanticScatterPixels(actual)) {
            std::cerr << "actual image failed semantic scatter blank guard for " << scatterCase.name.toStdString() << '\n';
            return 1;
        }
        QImage diff;
        const PixelMetrics metrics = compareImages(reference, actual, diff);
        if (!writeCaseArtifacts(scatterCase, reference, actual, diff, metrics)) {
            return 1;
        }
        if (!metrics.passed) {
            std::cerr << "visual mismatch for " << scatterCase.name.toStdString() << ": changedPixels="
                      << metrics.changedPixels << " maxDelta=" << metrics.maxDelta << '\n';
            return 1;
        }
        ++passedCases;
    }

    if (!writeSummaryReport(passedCases, static_cast<int>(scatterCases.size()))) {
        return 1;
    }
    return 0;
}
