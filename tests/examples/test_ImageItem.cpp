#define CPPQTGRAPH_IMAGEITEM_NO_MAIN
#include "../../examples/ImageItem.cpp"

#include <QtCore/QDir>
#include <QtCore/QSize>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

#ifndef CPPQTGRAPH_P2_14_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P2_14_VISUAL_DIFF_DIR "reports/visual-diffs/ImageItem-example"
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

std::string quote(std::string_view value)
{
    std::string result = "\"";
    for (const char character : value) {
        if (character == '\\' || character == '"') {
            result.push_back('\\');
        }
        result.push_back(character);
    }
    result.push_back('"');
    return result;
}

QImage expectedReferenceImage(const cppqtgraph::examples::ImageItemExample& example)
{
    QImage image(static_cast<int>(cppqtgraph::examples::imageItemExampleWidth()),
                 static_cast<int>(cppqtgraph::examples::imageItemExampleHeight()),
                 QImage::Format_ARGB32);
    const auto& frame = example.state->frames.front();
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const auto value = frame[static_cast<std::size_t>(x) * cppqtgraph::examples::imageItemExampleHeight()
                                     + static_cast<std::size_t>(y)];
            line[x] = qRgb(value, value, value);
        }
    }

    for (int x = 0; x < image.width(); ++x) {
        image.setPixel(x, 0, qRgb(255, 255, 255));
        image.setPixel(x, image.height() - 1, qRgb(255, 255, 255));
    }
    for (int y = 0; y < image.height(); ++y) {
        image.setPixel(0, y, qRgb(255, 255, 255));
        image.setPixel(image.width() - 1, y, qRgb(255, 255, 255));
    }
    return image;
}

int borderWhitePixels(const QImage& image)
{
    int pixels = 0;
    auto isWhite = [](QRgb pixel) {
        const QColor color(pixel);
        return color.red() >= 245 && color.green() >= 245 && color.blue() >= 245;
    };
    for (int x = 0; x < image.width(); ++x) {
        pixels += isWhite(image.pixel(x, 0)) ? 1 : 0;
        pixels += isWhite(image.pixel(x, image.height() - 1)) ? 1 : 0;
    }
    for (int y = 1; y < image.height() - 1; ++y) {
        pixels += isWhite(image.pixel(0, y)) ? 1 : 0;
        pixels += isWhite(image.pixel(image.width() - 1, y)) ? 1 : 0;
    }
    return pixels;
}

int uniqueColorCount(const QImage& image)
{
    std::set<QRgb> colors;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            colors.insert(image.pixel(x, y) & 0x00ffffffU);
        }
    }
    return static_cast<int>(colors.size());
}

struct DiffMetrics {
    int changedPixels = 0;
    double changedPixelPercentage = 0.0;
    double meanAbsoluteDelta = 0.0;
    int maxDelta = 0;
};

DiffMetrics writeDiffImage(const QImage& reference, const QImage& actual, const QString& path)
{
    QImage diff(reference.size(), QImage::Format_ARGB32);
    DiffMetrics metrics;
    long long absoluteDelta = 0;

    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor ref(reference.pixel(x, y));
            const QColor got(actual.pixel(x, y));
            const int red = std::abs(ref.red() - got.red());
            const int green = std::abs(ref.green() - got.green());
            const int blue = std::abs(ref.blue() - got.blue());
            const int maxChannel = std::max({red, green, blue});
            if (maxChannel != 0) {
                ++metrics.changedPixels;
            }
            metrics.maxDelta = std::max(metrics.maxDelta, maxChannel);
            absoluteDelta += red + green + blue;
            diff.setPixel(x, y, qRgb(red, green, blue));
        }
    }

    const auto pixelCount = static_cast<double>(reference.width() * reference.height());
    metrics.changedPixelPercentage = 100.0 * static_cast<double>(metrics.changedPixels) / pixelCount;
    metrics.meanAbsoluteDelta = static_cast<double>(absoluteDelta) / (pixelCount * 3.0);
    diff.save(path);
    return metrics;
}

bool writeVisualArtifacts(cppqtgraph::examples::ImageItemExample& example)
{
    const QString artifactDir = QString::fromUtf8(CPPQTGRAPH_P2_14_VISUAL_DIFF_DIR);
    CHECK(QDir().mkpath(artifactDir));

    example.timer->stop();
    example.widget->show();
    QApplication::processEvents();
    const QImage actual = example.widget->viewport()->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    const QImage reference = expectedReferenceImage(example).convertToFormat(QImage::Format_ARGB32);
    CHECK(actual.size() == reference.size());

    const QString referencePath = artifactDir + QStringLiteral("/reference.png");
    const QString actualPath = artifactDir + QStringLiteral("/actual.png");
    const QString diffPath = artifactDir + QStringLiteral("/diff.png");
    const QString metricsPath = artifactDir + QStringLiteral("/metrics.json");
    CHECK(reference.save(referencePath));
    CHECK(actual.save(actualPath));
    const DiffMetrics diff = writeDiffImage(reference, actual, diffPath);

    const int borderPixels = borderWhitePixels(actual);
    const int uniqueColors = uniqueColorCount(actual);
    const bool passed = actual.size() == QSize(600, 600) && borderPixels >= 1'000 && uniqueColors >= 128;

    std::ofstream metrics(metricsPath.toStdString());
    CHECK(metrics.good());
    metrics << "{\n"
            << "  \"case\": \"ImageItem-example\",\n"
            << "  \"pyqtgraph_reference\": \"pyqtgraph-0.14.0 pyqtgraph/examples/ImageItem.py\",\n"
            << "  \"deterministic_verdict\": " << quote(passed ? "pass" : "fail") << ",\n"
            << "  \"passed\": " << (passed ? "true" : "false") << ",\n"
            << "  \"dimensions\": {\"width\": " << actual.width() << ", \"height\": " << actual.height() << "},\n"
            << "  \"semantic_checks\": {\n"
            << "    \"white_border_pixels\": " << borderPixels << ",\n"
            << "    \"unique_colors\": " << uniqueColors << ",\n"
            << "    \"animated_frame_count\": " << example.state->frames.size() << "\n"
            << "  },\n"
            << "  \"diff_metrics\": {\n"
            << "    \"changed_pixels\": " << diff.changedPixels << ",\n"
            << "    \"changed_pixel_percentage\": " << diff.changedPixelPercentage << ",\n"
            << "    \"mean_absolute_delta\": " << diff.meanAbsoluteDelta << ",\n"
            << "    \"max_delta\": " << diff.maxDelta << "\n"
            << "  },\n"
            << "  \"reference_path\": " << quote(referencePath.toStdString()) << ",\n"
            << "  \"actual_path\": " << quote(actualPath.toStdString()) << ",\n"
            << "  \"diff_image_path\": " << quote(diffPath.toStdString()) << "\n"
            << "}\n";
    CHECK(metrics.good());
    CHECK(passed);
    return true;
}

bool testImageItemFactory()
{
    auto example = cppqtgraph::examples::createImageItemExample();

    CHECK(example.scene != nullptr);
    CHECK(example.widget != nullptr);
    CHECK(example.viewBox != nullptr);
    CHECK(example.image != nullptr);
    CHECK(example.border != nullptr);
    CHECK(example.timer != nullptr);
    CHECK(example.state != nullptr);
    CHECK(example.widget->windowTitle() == QStringLiteral("pyqtgraph example: ImageItem"));
    CHECK(example.widget->size() == QSize(600, 600));
    CHECK(example.viewBox->targetRect() == QRectF(0.0, 0.0, 600.0, 600.0));
    CHECK(example.viewBox->viewRect() == QRectF(0.0, 0.0, 600.0, 600.0));
    CHECK(example.image->hasImage());
    CHECK(example.image->width() == cppqtgraph::examples::imageItemExampleWidth());
    CHECK(example.image->height() == cppqtgraph::examples::imageItemExampleHeight());
    CHECK(example.image->channels() == 1);
    CHECK(example.state->frames.size() == cppqtgraph::examples::imageItemExampleFrameCount());
    CHECK(example.state->frames.front().size()
          == cppqtgraph::examples::imageItemExampleWidth() * cppqtgraph::examples::imageItemExampleHeight());
    CHECK(example.state->nextFrame == 1);
    CHECK(example.timer->isSingleShot());
    CHECK(example.timer->interval() == 1);
    CHECK(example.timer->isActive());

    return writeVisualArtifacts(example);
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testImageItemFactory()) {
        return 1;
    }

    return 0;
}
