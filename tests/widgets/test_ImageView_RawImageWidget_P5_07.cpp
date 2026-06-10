#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>
#include <cppqtgraph/widgets/RawImageWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtWidgets/QApplication>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P5_07_ARTIFACT_DIR
#define CPPQTGRAPH_P5_07_ARTIFACT_DIR "artifacts/P5.07"
#endif

#ifndef CPPQTGRAPH_P5_07_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_07_REPOSITORY_REPORT_DIR "reports/issues/P5.07"
#endif

namespace {

constexpr double kLevelTolerance = 0.0;
constexpr int kExactPixelTolerance = 0;

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

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

bool writeTextFile(const QString& path, const QString& content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        std::cerr << "failed to write " << path.toStdString() << '\n';
        return false;
    }
    QTextStream out(&file);
    out << content;
    return true;
}

bool checkIndexed8(const QImage& image, int x, int y, int index)
{
    if (image.format() != QImage::Format_Indexed8) {
        std::cerr << "expected Format_Indexed8 got " << static_cast<int>(image.format()) << '\n';
        return false;
    }
    const uchar* row = image.constScanLine(y);
    if (static_cast<int>(row[x]) != index) {
        std::cerr << "indexed8(" << x << ',' << y << ") expected " << index << " got " << static_cast<int>(row[x]) << '\n';
        return false;
    }
    return true;
}

bool checkIndexedColor(const QImage& image, int index, int red, int green, int blue)
{
    if (image.format() != QImage::Format_Indexed8) {
        std::cerr << "expected Format_Indexed8 got " << static_cast<int>(image.format()) << '\n';
        return false;
    }
    const QRgb color = image.color(index);
    if (qRed(color) != red || qGreen(color) != green || qBlue(color) != blue) {
        std::cerr << "indexed color " << index << " expected (" << red << ',' << green << ',' << blue << ") got ("
                  << qRed(color) << ',' << qGreen(color) << ',' << qBlue(color) << ")\n";
        return false;
    }
    return true;
}

bool checkGray8(const QImage& image, int x, int y, int gray)
{
    if (image.format() != QImage::Format_Grayscale8) {
        std::cerr << "expected Format_Grayscale8 got " << static_cast<int>(image.format()) << '\n';
        return false;
    }
    const uchar* row = image.constScanLine(y);
    if (static_cast<int>(row[x]) != gray) {
        std::cerr << "gray8(" << x << ',' << y << ") expected " << gray << " got " << static_cast<int>(row[x]) << '\n';
        return false;
    }
    return true;
}

bool checkGray16(const QImage& image, int x, int y, std::uint16_t gray)
{
    if (QImage::Format_Grayscale16 == QImage::Format_Invalid) {
        return true;
    }
    if (image.format() != QImage::Format_Grayscale16) {
        std::cerr << "expected Format_Grayscale16 got " << static_cast<int>(image.format()) << '\n';
        return false;
    }
    const auto* row = reinterpret_cast<const std::uint16_t*>(image.constScanLine(y));
    if (row[x] != gray) {
        std::cerr << "gray16(" << x << ',' << y << ") expected " << gray << " got " << row[x] << '\n';
        return false;
    }
    return true;
}

bool checkRgb888(const QImage& image, int x, int y, int red, int green, int blue)
{
    if (image.format() != QImage::Format_RGB888) {
        std::cerr << "expected Format_RGB888 got " << static_cast<int>(image.format()) << '\n';
        return false;
    }
    const uchar* row = image.constScanLine(y);
    const std::size_t base = static_cast<std::size_t>(x) * 3;
    if (static_cast<int>(row[base + 0]) != red || static_cast<int>(row[base + 1]) != green
        || static_cast<int>(row[base + 2]) != blue) {
        std::cerr << "rgb888(" << x << ',' << y << ") mismatch\n";
        return false;
    }
    return true;
}

cppqtgraph::ImageLookupTable lutView(const std::vector<std::uint8_t>& lut, std::size_t channels)
{
    return cppqtgraph::ImageLookupTable{
        lut.data(),
        lut.size() / channels,
        channels,
        static_cast<std::ptrdiff_t>(channels),
        1,
    };
}

bool testRawImageWidgetGrayscaleCopyAndFormat()
{
    std::vector<std::uint8_t> data{10, 20, 30, 40, 50, 60};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(cppqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    widget.setImage(view);

    CHECK(widget.hasImage());
    CHECK(widget.width() == 3);
    CHECK(widget.height() == 2);

    const QImage& image = widget.cachedImage();
    CHECK(!image.isNull());
    CHECK(image.format() == QImage::Format_Grayscale8);
    CHECK(image.bytesPerLine() >= 3);
    CHECK(checkGray8(image, 0, 0, 10));
    CHECK(checkGray8(image, 2, 0, 30));
    CHECK(checkGray8(image, 0, 1, 40));

    data[0] = 99;
    const QImage& afterMutation = widget.cachedImage();
    CHECK(checkGray8(afterMutation, 0, 0, 10));

    return true;
}

bool testRawImageWidgetStrideView()
{
    std::array<std::uint8_t, 9> padded{{0, 10, 99, 20, 30, 99, 40, 50, 99}};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(padded.data(), {3, 2}, {3, 1});

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(cppqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    widget.setImage(view);

    const QImage& image = widget.cachedImage();
    CHECK(image.width() == 2);
    CHECK(image.height() == 3);
    CHECK(checkGray8(image, 0, 0, 0));
    CHECK(checkGray8(image, 1, 0, 10));
    CHECK(checkGray8(image, 0, 1, 20));
    CHECK(checkGray8(image, 1, 1, 30));
    CHECK(checkGray8(image, 0, 2, 40));
    CHECK(checkGray8(image, 1, 2, 50));

    padded[0] = 200;
    const QImage& afterMutation = widget.cachedImage();
    CHECK(checkGray8(afterMutation, 0, 0, 0));

    return true;
}

bool testRawImageWidgetUint16Grayscale()
{
    if (QImage::Format_Grayscale16 == QImage::Format_Invalid) {
        return true;
    }

    const std::array<std::uint16_t, 4> data{1000, 2000, 3000, 4000};
    cppqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {2, 2});

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(cppqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    widget.setImage(view);

    CHECK(widget.hasImage());
    CHECK(widget.width() == 2);
    CHECK(widget.height() == 2);

    const QImage& image = widget.cachedImage();
    CHECK(!image.isNull());
    CHECK(image.format() == QImage::Format_Grayscale16);
    CHECK(checkGray16(image, 0, 0, 1000));
    CHECK(checkGray16(image, 1, 0, 2000));
    CHECK(checkGray16(image, 0, 1, 3000));
    CHECK(checkGray16(image, 1, 1, 4000));

    return true;
}

bool testRawImageWidgetRgbColorOrder()
{
    const std::array<std::uint8_t, 12> data{
        255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0,
    };
    cppqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {2, 2, 3});

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(cppqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    widget.setImage(view);

    const QImage& image = widget.cachedImage();
    CHECK(image.format() == QImage::Format_RGB888);
    CHECK(checkRgb888(image, 0, 0, 255, 0, 0));
    CHECK(checkRgb888(image, 1, 0, 0, 255, 0));
    CHECK(checkRgb888(image, 0, 1, 0, 0, 255));

    return true;
}

bool testRawImageWidgetStridedLookupTable()
{
    constexpr std::size_t rows = 4;
    constexpr std::size_t channels = 4;
    constexpr std::ptrdiff_t rowStride = 10;
    constexpr std::ptrdiff_t channelStride = 2;
    std::vector<std::uint8_t> padded(rows * static_cast<std::size_t>(rowStride), 99);
    for (std::size_t row = 0; row < rows; ++row) {
        const auto base = static_cast<std::size_t>(row) * static_cast<std::size_t>(rowStride);
        padded[base + 0 * channelStride] = static_cast<std::uint8_t>(row * 10 + 1);
        padded[base + 1 * channelStride] = static_cast<std::uint8_t>(row * 10 + 2);
        padded[base + 2 * channelStride] = static_cast<std::uint8_t>(row * 10 + 3);
        padded[base + 3 * channelStride] = 255;
    }

    cppqtgraph::ImageLookupTable stridedLut{
        padded.data(),
        rows,
        channels,
        rowStride,
        channelStride,
    };

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setLevels(cppqtgraph::ImageLevelRange{0.0, 255.0});
    widget.setLookupTable(stridedLut);

    const auto stored = widget.lookupTable();
    CHECK(stored.has_value());
    CHECK(stored->rowStride == static_cast<std::ptrdiff_t>(channels));
    CHECK(stored->channelStride == 1);
    CHECK((cppqtgraph::applyLookupTable(0, *stored) == std::array<std::uint8_t, 4>{1, 2, 3, 255}));
    CHECK((cppqtgraph::applyLookupTable(1, *stored) == std::array<std::uint8_t, 4>{11, 12, 13, 255}));
    CHECK((cppqtgraph::applyLookupTable(2, *stored) == std::array<std::uint8_t, 4>{21, 22, 23, 255}));
    CHECK((cppqtgraph::applyLookupTable(3, *stored) == std::array<std::uint8_t, 4>{31, 32, 33, 255}));

    const std::array<std::uint8_t, 1> data{64};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, 1});
    widget.setImage(view);

    const QImage& image = widget.cachedImage();
    CHECK(image.format() == QImage::Format_Indexed8);
    CHECK(checkIndexedColor(image, 64, 11, 12, 13));

    padded[0] = 200;
    const auto afterMutation = widget.lookupTable();
    CHECK(afterMutation.has_value());
    CHECK((cppqtgraph::applyLookupTable(0, *afterMutation) == std::array<std::uint8_t, 4>{1, 2, 3, 255}));

    return true;
}

bool testRawImageWidgetLevelsAndLut()
{
    const std::array<std::uint8_t, 4> data{0, 64, 128, 255};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setLevels(cppqtgraph::ImageLevelRange{0.0, 255.0});
    widget.setImage(view);

    const std::vector<std::uint8_t> lut{
        0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
    };
    widget.setLookupTable(lutView(lut, 4));

    const QImage& image = widget.cachedImage();
    CHECK(image.format() == QImage::Format_Indexed8);
    CHECK(checkIndexed8(image, 0, 0, 0));
    CHECK(checkIndexed8(image, 1, 1, 255));

    const std::array<float, 4> floatData{0.f, 64.f, 128.f, 255.f};
    cppqtgraph::core::ArrayView<const float, 2> floatView(floatData.data(), {2, 2});
    cppqtgraph::widgets::RawImageWidget floatWidget;
    floatWidget.setAxisOrder(cppqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    floatWidget.clearLookupTable();
    floatWidget.setLevels(cppqtgraph::ImageLevelRange{64.0, 192.0});
    floatWidget.setImage(floatView);

    const QImage& leveled = floatWidget.cachedImage();
    CHECK(leveled.format() == QImage::Format_Grayscale8);
    CHECK(checkGray8(leveled, 0, 0, 0));
    CHECK(checkGray8(leveled, 1, 0, 0));
    CHECK(checkGray8(leveled, 0, 1, 127));

    (void)kLevelTolerance;
    (void)kExactPixelTolerance;

    return true;
}

bool testRawImageWidgetAxisTranspose()
{
    const std::array<std::uint8_t, 4> data{1, 2, 3, 4};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    cppqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(cppqtgraph::widgets::RawImageWidget::AxisOrder::ColMajor);
    widget.setImage(view);

    CHECK(widget.width() == 2);
    CHECK(widget.height() == 2);
    CHECK(checkGray8(widget.cachedImage(), 0, 0, 1));
    CHECK(checkGray8(widget.cachedImage(), 1, 0, 3));

    return true;
}

bool testImageViewTwoDimensionalBuffer()
{
    const std::array<std::uint8_t, 6> data{5, 10, 15, 20, 25, 30};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});

    cppqtgraph::imageview::ImageView imageView;
    imageView.setAxisOrder(cppqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageView.setImage(view, true, false);

    CHECK(imageView.hasImage());
    auto* imageItem = imageView.getImageItem();
    CHECK(imageItem != nullptr);
    CHECK(imageItem->hasImage());
    CHECK(imageItem->width() == 3);
    CHECK(imageItem->height() == 2);

    const auto levels = imageItem->getLevels();
    CHECK(levels.has_value());
    CHECK(levels->minimum == 5.0);
    CHECK(levels->maximum == 30.0);

    imageItem->setLevels(std::nullopt);
    CHECK(imageItem->render());
    const QImage& rendered = imageItem->cachedImage();
    CHECK(!rendered.isNull());
    CHECK(rendered.format() == QImage::Format_Grayscale8);
    CHECK(checkGray8(rendered, 0, 0, 5));
    CHECK(checkGray8(rendered, 2, 1, 30));

    return true;
}

bool testImageViewFrameStack()
{
    const std::array<std::uint8_t, 12> data{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    };
    cppqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {3, 2, 2});

    cppqtgraph::imageview::ImageView imageView;
    imageView.setAxisOrder(cppqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageView.setImage(view, false, false);

    CHECK(imageView.currentIndex() == 0);
    auto* imageItem = imageView.getImageItem();
    CHECK(imageItem->width() == 2);
    CHECK(imageItem->height() == 2);
    CHECK(imageItem->render());
    CHECK(checkGray8(imageItem->cachedImage(), 0, 0, 1));
    CHECK(checkGray8(imageItem->cachedImage(), 1, 1, 4));

    imageView.setCurrentIndex(2);
    CHECK(imageView.currentIndex() == 2);
    CHECK(imageItem->render());
    CHECK(checkGray8(imageItem->cachedImage(), 0, 0, 9));
    CHECK(checkGray8(imageItem->cachedImage(), 1, 1, 12));

    return true;
}

bool testImageViewCopyOnSet()
{
    std::vector<std::uint8_t> data{100, 110, 120, 130};
    cppqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    cppqtgraph::imageview::ImageView imageView;
    imageView.setAxisOrder(cppqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageView.setImage(view, false, false);

    data[0] = 0;
    auto* imageItem = imageView.getImageItem();
    CHECK(imageItem->render());
    CHECK(checkGray8(imageItem->cachedImage(), 0, 0, 100));

    return true;
}

bool writeCompletionArtifact(bool passed)
{
    const QString artifactDir = QString::fromUtf8(CPPQTGRAPH_P5_07_ARTIFACT_DIR);
    const QString reportDir = QString::fromUtf8(CPPQTGRAPH_P5_07_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(artifactDir));
    CHECK(ensureDirectory(reportDir));

    const QString summary = QStringLiteral(
        "P5.07 pixel-image proof\n"
        "format: Grayscale8/RGB888/Indexed8\n"
        "dtype: uint8/uint16/float via ArrayView\n"
        "stride/copy: non-contiguous ArrayView copied on set; source mutation ignored\n"
        "color order: RGB channel order preserved\n"
        "levels/LUT: ImageLevelRange and ImageLookupTable applied via tryMakeQImage\n"
        "tolerance: exact integer pixels (0)\n"
        "result: %1\n")
                              .arg(passed ? QStringLiteral("passed") : QStringLiteral("failed"));

    CHECK(writeTextFile(reportDir + QStringLiteral("/pixel_image_buffer.json"), summary));
    CHECK(writeTextFile(artifactDir + QStringLiteral("/pixel_image_buffer.txt"), summary));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ApplicationGuard guard(argc, argv);

    const bool passed = testRawImageWidgetGrayscaleCopyAndFormat() && testRawImageWidgetStrideView()
        && testRawImageWidgetUint16Grayscale() && testRawImageWidgetRgbColorOrder()
        && testRawImageWidgetStridedLookupTable() && testRawImageWidgetLevelsAndLut()
        && testRawImageWidgetAxisTranspose() && testImageViewTwoDimensionalBuffer()
        && testImageViewFrameStack() && testImageViewCopyOnSet() && writeCompletionArtifact(true);

    if (!passed) {
        (void)writeCompletionArtifact(false);
        return 1;
    }

    return 0;
}
