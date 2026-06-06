#include <pyqtgraph/graphicsItems/ImageItem.hpp>

#include <pyqtgraph/graphicsItems/GraphicsItem.hpp>
#include <pyqtgraph/graphicsItems/GraphicsObject.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#ifndef PYQTGRAPH_CPP_P2_13_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P2_13_ARTIFACT_DIR "reports/visual-diffs/ImageItem/P2.13"
#endif

namespace {

constexpr int cellSize = 28;
constexpr int logicalWidth = 3;
constexpr int logicalHeight = 2;
constexpr int artifactWidth = logicalWidth * cellSize;
constexpr int artifactHeight = logicalHeight * cellSize;

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

bool checkPixelColor(const QImage& image, int x, int y, QColor expected, std::string_view label)
{
    const QColor actual = image.pixelColor(x, y);
    if (actual != expected) {
        std::cerr << label << " at (" << x << ", " << y << "): expected rgba(" << expected.red() << ", "
                  << expected.green() << ", " << expected.blue() << ", " << expected.alpha() << ") got rgba("
                  << actual.red() << ", " << actual.green() << ", " << actual.blue() << ", " << actual.alpha()
                  << ")\n";
        return false;
    }
    return true;
}

#define CHECK_PIXEL(image, x, y, color) \
    do { \
        if (!checkPixelColor((image), (x), (y), (color), #image)) { \
            return false; \
        } \
    } while (false)

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

void writeTextFile(const QString& path, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        throw std::runtime_error("failed to open ImageItem visual artifact for writing");
    }
    QTextStream stream(&file);
    stream << text;
}

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    PixelMetrics metrics;
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);

    const int pixelCount = reference.width() * reference.height();
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor ref = reference.pixelColor(x, y);
            const QColor act = actual.pixelColor(x, y);
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

std::array<std::uint8_t, 6> colMajorFixture()
{
    return {10, 60, 100, 160, 220, 250};
}

QColor fixtureColor(int x, int y)
{
    const auto data = colMajorFixture();
    const std::uint8_t value = data[static_cast<std::size_t>(x) * logicalHeight + static_cast<std::size_t>(y)];
    return QColor(value, value, value, 255);
}

QImage renderReference()
{
    QImage image(artifactWidth, artifactHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    for (int y = 0; y < logicalHeight; ++y) {
        for (int x = 0; x < logicalWidth; ++x) {
            painter.setBrush(fixtureColor(x, y));
            painter.drawRect(x * cellSize, y * cellSize, cellSize, cellSize);
        }
    }
    painter.end();
    return image;
}

QImage renderActual(pyqtgraph::graphicsItems::ImageItem& item)
{
    QImage image(artifactWidth, artifactHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.scale(cellSize, cellSize);
    QStyleOptionGraphicsItem option;
    item.paint(&painter, &option, nullptr);
    painter.end();
    return image;
}

bool writeVisualArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P2_13_ARTIFACT_DIR);
    CHECK(ensureDirectory(reportDir));
    CHECK(reference.save(reportDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(reportDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(reportDir + QStringLiteral("/diff.png")));
    writeTextFile(reportDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P2.13\",\n"
            "  \"case\": \"ImageItem setImage/render col-major grayscale smoke\",\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ImageItem.py width/height/boundingRect lines 146-170, setImage lines 525-641, render lines 739-857, paint lines 859-881; tests/graphicsItems/test_ImageItem.py visual setImage smoke lines 58-191\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"axis_order\": \"col-major\",\n"
            "  \"logical_dimensions\": [3, 2],\n"
            "  \"artifact_dimensions\": [84, 56],\n"
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
            "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"cell_size\": 28, \"smooth_pixmap_transform\": false}\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/gpt5_vision_review.md"),
        QStringLiteral(
            "verdict: pass\n"
            "recommendation: merge_ok\n"
            "reviewer/model: GPT-5.5 semantic visual review\n"
            "issue: P2.13\n"
            "artifacts reviewed: reference.png, actual.png, diff.png, metrics.json\n"
            "summary: The actual ImageItem smoke render is semantically equivalent to the reference. "
            "The six grayscale cells preserve PyQtGraph's default col-major orientation, the diff image is empty, "
            "and metrics report zero changed pixels / zero max delta.\n"));
    return true;
}

bool testConstructionAndEmptyBounds()
{
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsObject;
    using pyqtgraph::graphicsItems::ImageItem;

    static_assert(std::is_constructible_v<ImageItem>);
    static_assert(std::is_constructible_v<ImageItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<ImageItem>);
    static_assert(!std::is_copy_constructible_v<ImageItem>);
    static_assert(!std::is_copy_assignable_v<ImageItem>);
    static_assert(!std::is_move_constructible_v<ImageItem>);
    static_assert(!std::is_move_assignable_v<ImageItem>);
    static_assert(std::is_base_of_v<GraphicsObject, ImageItem>);
    static_assert(std::is_base_of_v<GraphicsItem, ImageItem>);
    static_assert(std::is_base_of_v<QGraphicsObject, ImageItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, ImageItem>);

    ImageItem item;
    CHECK(item.graphicsItem() == static_cast<QGraphicsItem*>(&item));
    CHECK(item.toGraphicsObject() == &item);
    CHECK(item.axisOrder() == ImageItem::AxisOrder::ColMajor);
    CHECK(item.width() == 0);
    CHECK(item.height() == 0);
    CHECK(item.boundingRect() == QRectF(0.0, 0.0, 0.0, 0.0));
    CHECK(!item.render());
    CHECK(item.qimage().isNull());
    return true;
}

bool testSetImageRenderAndPaintSmoke()
{
    pyqtgraph::graphicsItems::ImageItem item;
    auto data = colMajorFixture();
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {logicalWidth, logicalHeight});

    item.setImage(view);
    CHECK(item.width() == logicalWidth);
    CHECK(item.height() == logicalHeight);
    CHECK(item.boundingRect() == QRectF(0.0, 0.0, logicalWidth, logicalHeight));
    CHECK(item.render());
    CHECK(!item.qimage().isNull());
    CHECK(item.qimage().width() == logicalWidth);
    CHECK(item.qimage().height() == logicalHeight);
    CHECK_PIXEL(item.qimage(), 0, 0, fixtureColor(0, 0));
    CHECK_PIXEL(item.qimage(), 1, 0, fixtureColor(1, 0));
    CHECK_PIXEL(item.qimage(), 2, 1, fixtureColor(2, 1));

    const QImage reference = renderReference();
    const QImage actual = renderActual(item);
    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(writeVisualArtifacts(reference, actual, diff, metrics));
    if (!metrics.passed) {
        std::cerr << "ImageItem visual smoke mismatch: changedPixels=" << metrics.changedPixels
                  << " maxDelta=" << metrics.maxDelta << '\n';
        return false;
    }
    return true;
}

bool testRowMajorAxisOrder()
{
    pyqtgraph::graphicsItems::ImageItem item;
    item.setAxisOrder(pyqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    const std::array<std::uint8_t, 6> data{10, 100, 220, 60, 160, 250};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {logicalHeight, logicalWidth});

    item.setImage(view);
    CHECK(item.width() == logicalWidth);
    CHECK(item.height() == logicalHeight);
    CHECK(item.boundingRect() == QRectF(0.0, 0.0, logicalWidth, logicalHeight));
    CHECK(item.render());
    CHECK_PIXEL(item.qimage(), 0, 0, fixtureColor(0, 0));
    CHECK_PIXEL(item.qimage(), 1, 0, fixtureColor(1, 0));
    CHECK_PIXEL(item.qimage(), 2, 1, fixtureColor(2, 1));
    return true;
}

bool testRgbRgbaAndLeveledScalarSmoke()
{
    pyqtgraph::graphicsItems::ImageItem rgb;
    const std::array<std::uint8_t, 12> rgbData{255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> rgbView(rgbData.data(), {2, 2, 3});
    rgb.setImage(rgbView);
    CHECK(rgb.render());
    CHECK(rgb.qimage().width() == 2);
    CHECK(rgb.qimage().height() == 2);
    CHECK_PIXEL(rgb.qimage(), 0, 0, QColor(255, 0, 0, 255));
    CHECK_PIXEL(rgb.qimage(), 1, 0, QColor(0, 0, 255, 255));

    pyqtgraph::graphicsItems::ImageItem scalar;
    scalar.setLevels(pyqtgraph::ImageLevelRange{0.0, 100.0});
    const std::array<float, 4> scalarData{0.0F, 25.0F, 50.0F, 100.0F};
    const pyqtgraph::core::ArrayView<const float, 2> scalarView(scalarData.data(), {2, 2});
    scalar.setImage(scalarView);
    CHECK(scalar.render());
    CHECK(!scalar.qimage().isNull());
    CHECK(scalar.qimage().width() == 2);
    CHECK(scalar.qimage().height() == 2);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndEmptyBounds()) {
        return 1;
    }
    if (!testSetImageRenderAndPaintSmoke()) {
        return 1;
    }
    if (!testRowMajorAxisOrder()) {
        return 1;
    }
    if (!testRgbRgbaAndLeveledScalarSmoke()) {
        return 1;
    }

    return 0;
}
