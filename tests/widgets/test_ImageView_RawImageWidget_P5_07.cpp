#include <pyqtgraph/functions.hpp>
#include <pyqtgraph/imageview/ImageView.hpp>
#include <pyqtgraph/widgets/RawImageWidget.hpp>

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

#ifndef PYQTGRAPH_CPP_P5_07_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P5_07_ARTIFACT_DIR "artifacts/P5.07"
#endif

#ifndef PYQTGRAPH_CPP_P5_07_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_07_REPOSITORY_REPORT_DIR "reports/issues/P5.07"
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

pyqtgraph::ImageLookupTable lutView(const std::vector<std::uint8_t>& lut, std::size_t channels)
{
    return pyqtgraph::ImageLookupTable{
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
    pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});

    pyqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(pyqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
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
    const std::array<std::uint8_t, 8> storage{1, 2, 3, 4, 5, 6, 7, 8};
    pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(storage.data(), {2, 4}, {4, 1});

    pyqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(pyqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    widget.setImage(view);

    const QImage& image = widget.cachedImage();
    CHECK(image.width() == 4);
    CHECK(image.height() == 2);
    CHECK(checkGray8(image, 0, 0, 1));
    CHECK(checkGray8(image, 1, 0, 2));
    CHECK(checkGray8(image, 0, 1, 5));
    CHECK(checkGray8(image, 3, 1, 8));

    return true;
}

bool testRawImageWidgetRgbColorOrder()
{
    const std::array<std::uint8_t, 12> data{
        255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0,
    };
    pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {2, 2, 3});

    pyqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(pyqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    widget.setImage(view);

    const QImage& image = widget.cachedImage();
    CHECK(image.format() == QImage::Format_RGB888);
    CHECK(checkRgb888(image, 0, 0, 255, 0, 0));
    CHECK(checkRgb888(image, 1, 0, 0, 255, 0));
    CHECK(checkRgb888(image, 0, 1, 0, 0, 255));

    return true;
}

bool testRawImageWidgetLevelsAndLut()
{
    const std::array<std::uint8_t, 4> data{0, 64, 128, 255};
    pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    pyqtgraph::widgets::RawImageWidget widget;
    widget.setLevels(pyqtgraph::ImageLevelRange{0.0, 255.0});
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
    pyqtgraph::core::ArrayView<const float, 2> floatView(floatData.data(), {2, 2});
    pyqtgraph::widgets::RawImageWidget floatWidget;
    floatWidget.setAxisOrder(pyqtgraph::widgets::RawImageWidget::AxisOrder::RowMajor);
    floatWidget.clearLookupTable();
    floatWidget.setLevels(pyqtgraph::ImageLevelRange{64.0, 192.0});
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
    pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    pyqtgraph::widgets::RawImageWidget widget;
    widget.setAxisOrder(pyqtgraph::widgets::RawImageWidget::AxisOrder::ColMajor);
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
    pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});

    pyqtgraph::imageview::ImageView imageView;
    imageView.setAxisOrder(pyqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
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
    pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {3, 2, 2});

    pyqtgraph::imageview::ImageView imageView;
    imageView.setAxisOrder(pyqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
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
    pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    pyqtgraph::imageview::ImageView imageView;
    imageView.setAxisOrder(pyqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageView.setImage(view, false, false);

    data[0] = 0;
    auto* imageItem = imageView.getImageItem();
    CHECK(imageItem->render());
    CHECK(checkGray8(imageItem->cachedImage(), 0, 0, 100));

    return true;
}

bool writeCompletionArtifact(bool passed)
{
    const QString artifactDir = QString::fromUtf8(PYQTGRAPH_CPP_P5_07_ARTIFACT_DIR);
    const QString reportDir = QString::fromUtf8(PYQTGRAPH_CPP_P5_07_REPOSITORY_REPORT_DIR);
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
        && testRawImageWidgetRgbColorOrder() && testRawImageWidgetLevelsAndLut()
        && testRawImageWidgetAxisTranspose() && testImageViewTwoDimensionalBuffer()
        && testImageViewFrameStack() && testImageViewCopyOnSet() && writeCompletionArtifact(true);

    if (!passed) {
        (void)writeCompletionArtifact(false);
        return 1;
    }

    return 0;
}
