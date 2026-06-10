#include <cppqtgraph/core/ArrayView.hpp>
#include <cppqtgraph/graphicsItems/ImageItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P2_13_ARTIFACT_DIR
#define CPPQTGRAPH_P2_13_ARTIFACT_DIR "reports/visual-diffs/ImageItem/P2.13"
#endif

namespace {

constexpr int scale = 32;
constexpr int logicalWidth = 3;
constexpr int logicalHeight = 2;
constexpr int canvasWidth = logicalWidth * scale;
constexpr int canvasHeight = logicalHeight * scale;

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

QImage expectedQImageFromColMajorData(const std::array<std::uint8_t, logicalWidth * logicalHeight>& data)
{
    QImage image(logicalWidth, logicalHeight, QImage::Format_Grayscale8);
    for (int y = 0; y < logicalHeight; ++y) {
        auto* row = image.scanLine(y);
        for (int x = 0; x < logicalWidth; ++x) {
            row[x] = data[static_cast<std::size_t>(x * logicalHeight + y)];
        }
    }
    return image;
}

QImage renderReference(const std::array<std::uint8_t, logicalWidth * logicalHeight>& data)
{
    QImage canvas(canvasWidth, canvasHeight, QImage::Format_ARGB32);
    canvas.fill(Qt::black);
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.scale(scale, scale);
    painter.drawImage(QRectF(0.0, 0.0, logicalWidth, logicalHeight), expectedQImageFromColMajorData(data));
    painter.end();
    return canvas;
}

QImage renderActual(const std::array<std::uint8_t, logicalWidth * logicalHeight>& data)
{
    const auto fail = [] { return QImage(); };

    cppqtgraph::graphicsItems::ImageItem item;
    const cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {logicalWidth, logicalHeight});
    item.setImage(view);
    if (!check(item.hasImage(), "item.hasImage()", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.width() == logicalWidth, "item.width() == logicalWidth", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.height() == logicalHeight, "item.height() == logicalHeight", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.channels() == 1, "item.channels() == 1", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.boundingRect() == QRectF(0.0, 0.0, logicalWidth, logicalHeight),
            "item.boundingRect() == QRectF(0.0, 0.0, logicalWidth, logicalHeight)", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.render(), "item.render()", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(!item.cachedImage().isNull(), "!item.cachedImage().isNull()", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.cachedImage().width() == logicalWidth, "item.cachedImage().width() == logicalWidth", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(item.cachedImage().height() == logicalHeight, "item.cachedImage().height() == logicalHeight", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(QColor(item.cachedImage().pixel(0, 0)).red() == data[0],
            "QColor(item.cachedImage().pixel(0, 0)).red() == data[0]", __FILE__, __LINE__)) {
        return fail();
    }
    if (!check(QColor(item.cachedImage().pixel(1, 0)).red() == data[2],
            "QColor(item.cachedImage().pixel(1, 0)).red() == data[2]", __FILE__, __LINE__)) {
        return fail();
    }

    QImage canvas(canvasWidth, canvasHeight, QImage::Format_ARGB32);
    canvas.fill(Qt::black);
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.scale(scale, scale);
    item.paint(&painter, nullptr, nullptr);
    painter.end();
    return canvas;
}

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    diff = QImage(reference.size(), QImage::Format_ARGB32);
    diff.fill(Qt::black);

    PixelMetrics metrics;
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected(reference.pixel(x, y));
            const QColor observed(actual.pixel(x, y));
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            if (delta > 0) {
                ++metrics.changedPixels;
                diff.setPixelColor(x, y, QColor(255, 0, 0, 255));
            }
            metrics.totalDelta += static_cast<std::uint64_t>(delta);
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
        }
    }

    const auto totalPixels = static_cast<std::uint64_t>(reference.width()) * static_cast<std::uint64_t>(reference.height());
    metrics.meanDelta = static_cast<double>(metrics.totalDelta) / static_cast<double>(totalPixels);
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(totalPixels);
    metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
    return metrics;
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

bool writeMetrics(const QString& path, const PixelMetrics& metrics)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        std::cerr << "failed to write metrics to " << path.toStdString() << '\n';
        return false;
    }
    QTextStream out(&file);
    out << "{\n";
    out << "  \"changed_pixels\": " << metrics.changedPixels << ",\n";
    out << "  \"total_delta\": " << metrics.totalDelta << ",\n";
    out << "  \"max_delta\": " << metrics.maxDelta << ",\n";
    out << "  \"mean_delta\": " << metrics.meanDelta << ",\n";
    out << "  \"changed_percent\": " << metrics.changedPercent << ",\n";
    out << "  \"passed\": " << (metrics.passed ? "true" : "false") << '\n';
    out << "}\n";
    return true;
}

bool runSmoke()
{
    const std::array<std::uint8_t, logicalWidth * logicalHeight> colMajorData{{0, 50, 100, 150, 200, 250}};
    const QImage reference = renderReference(colMajorData);
    const QImage actual = renderActual(colMajorData);
    CHECK(!reference.isNull());
    CHECK(!actual.isNull());
    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);

    const QString artifactDir = QString::fromUtf8(CPPQTGRAPH_P2_13_ARTIFACT_DIR);
    CHECK(ensureDirectory(artifactDir));
    CHECK(reference.save(artifactDir + "/reference.png"));
    CHECK(actual.save(artifactDir + "/actual.png"));
    CHECK(diff.save(artifactDir + "/diff.png"));
    CHECK(writeMetrics(artifactDir + "/metrics.json", metrics));

    CHECK(metrics.passed);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    ApplicationGuard application(argc, argv);
    return runSmoke() ? 0 : 1;
}
