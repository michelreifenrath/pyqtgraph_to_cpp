#include <pyqtgraph/core/ArrayView.hpp>
#include <pyqtgraph/functions.hpp>
#include <pyqtgraph/graphicsItems/ImageItem.hpp>
#include <pyqtgraph/graphicsItems/NonUniformImage.hpp>

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

#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P4_05_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_05_ARTIFACT_DIR "artifacts/P4.05"
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

QColor pixelColor(const QImage& image, int x, int y)
{
    return image.pixelColor(x, y);
}

bool sameColor(const QColor& observed, const QColor& expected)
{
    return observed.red() == expected.red() && observed.green() == expected.green() && observed.blue() == expected.blue()
        && observed.alpha() == expected.alpha();
}

bool expectColor(const QImage& image, int x, int y, const QColor& expected)
{
    const QColor observed = pixelColor(image, x, y);
    if (!sameColor(observed, expected)) {
        std::cerr << "pixel(" << x << ',' << y << ") expected rgba(" << expected.red() << ',' << expected.green() << ','
                  << expected.blue() << ',' << expected.alpha() << ") observed rgba(" << observed.red() << ','
                  << observed.green() << ',' << observed.blue() << ',' << observed.alpha() << ")\n";
        return false;
    }
    return true;
}

pyqtgraph::ImageLookupTable lutView(const std::vector<std::uint8_t>& lut, std::size_t channels)
{
    return pyqtgraph::ImageLookupTable{lut.data(), lut.size() / channels, channels, static_cast<std::ptrdiff_t>(channels), 1};
}

bool throwsInvalidArgument(auto&& callable)
{
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << "unexpected exception type: " << error.what() << '\n';
        return false;
    }
    return false;
}

bool exerciseImageItemLevelsLuts(const QString& artifactDir)
{
    using pyqtgraph::ImageLevelRange;
    using pyqtgraph::core::ArrayView;
    using pyqtgraph::graphicsItems::ImageItem;

    // P4.05 oracle: deterministic uint8, col-major display, non-contiguous input stride, copied data, scalar levels.
    std::array<std::uint8_t, 9> padded{{0, 10, 99, 20, 30, 99, 40, 50, 99}};
    ImageItem grayItem;
    grayItem.setImage(ArrayView<const std::uint8_t, 2>(padded.data(), {3, 2}, {3, 1}));
    padded[0] = 200; // setImage must copy strided input immediately.
    grayItem.setLevels(ImageLevelRange{10.0, 30.0});
    CHECK(grayItem.render());
    const QImage gray = grayItem.cachedImage();
    CHECK(gray.format() == QImage::Format_Indexed8);
    CHECK(gray.bytesPerLine() >= gray.width());
    CHECK(expectColor(gray, 0, 0, QColor(0, 0, 0, 255)));       // below black level clips to black
    CHECK(expectColor(gray, 1, 0, QColor(127, 127, 127, 255))); // midpoint uses scalar levels
    CHECK(expectColor(gray, 2, 0, QColor(255, 255, 255, 255))); // above white level clips to white
    CHECK(gray.save(artifactDir + "/imageitem_uint8_levels.png"));

    // P4.05 oracle: RGB(A) LUT color order/alpha and LUT copy behavior.
    std::array<std::uint8_t, 4> indices{{0, 1, 2, 3}};
    std::vector<std::uint8_t> rgbaLut{
        10, 0, 0, 255,
        0, 20, 0, 128,
        0, 0, 30, 255,
        40, 40, 0, 64,
    };
    ImageItem lutItem;
    lutItem.setImage(ArrayView<const std::uint8_t, 2>(indices.data(), {4, 1}));
    lutItem.setLookupTable(lutView(rgbaLut, 4));
    lutItem.setLevels(ImageLevelRange{0.0, 4.0});
    rgbaLut[0] = 255; // setLookupTable must own a copy.
    CHECK(lutItem.render());
    const QImage lutImage = lutItem.cachedImage();
    CHECK(lutImage.format() == QImage::Format_Indexed8);
    CHECK(expectColor(lutImage, 0, 0, QColor(10, 0, 0, 255)));
    CHECK(expectColor(lutImage, 1, 0, QColor(0, 20, 0, 128)));
    CHECK(expectColor(lutImage, 2, 0, QColor(0, 0, 30, 255)));
    CHECK(expectColor(lutImage, 3, 0, QColor(40, 40, 0, 64)));
    CHECK(lutImage.save(artifactDir + "/imageitem_uint8_lut.png"));

    // P4.05 oracle: uint16 scalar data with >256-row LUT must not truncate effective LUT row indices to uint8.
    std::array<std::uint16_t, 3> wideValues{{0, 512, 1023}};
    std::vector<std::uint8_t> wideLut(512 * 4, 0);
    for (std::size_t row = 0; row < 512; ++row) {
        wideLut[row * 4 + 0] = static_cast<std::uint8_t>(row & 0xffU);
        wideLut[row * 4 + 1] = static_cast<std::uint8_t>((row >> 1U) & 0xffU);
        wideLut[row * 4 + 2] = static_cast<std::uint8_t>((row >> 2U) & 0xffU);
        wideLut[row * 4 + 3] = 255;
    }
    ImageItem wideItem;
    wideItem.setImage(ArrayView<const std::uint16_t, 2>(wideValues.data(), {3, 1}));
    wideItem.setLookupTable(lutView(wideLut, 4));
    wideItem.setLevels(ImageLevelRange{0.0, 1024.0});
    CHECK(wideItem.render());
    const QImage wideImage = wideItem.cachedImage();
    CHECK(wideImage.format() == QImage::Format_RGBA8888);
    CHECK(expectColor(wideImage, 0, 0, QColor(0, 0, 0, 255)));
    CHECK(expectColor(wideImage, 1, 0, QColor(0, 128, 64, 255)));     // row 256, not row 0
    CHECK(expectColor(wideImage, 2, 0, QColor(255, 255, 127, 255))); // row 511, not row 255
    CHECK(wideImage.save(artifactDir + "/imageitem_uint16_wide_lut.png"));

    // P4.05 oracle: float dtype uses explicit levels for reproducible grayscale conversion.
    std::array<float, 3> floats{{0.0F, 0.5F, 1.0F}};
    ImageItem floatItem;
    floatItem.setImage(ArrayView<const float, 2>(floats.data(), {3, 1}));
    floatItem.setLevels(ImageLevelRange{0.0, 1.0});
    CHECK(floatItem.render());
    const QImage floatImage = floatItem.cachedImage();
    CHECK(floatImage.format() == QImage::Format_Grayscale8);
    CHECK(expectColor(floatImage, 0, 0, QColor(0, 0, 0, 255)));
    CHECK(expectColor(floatImage, 1, 0, QColor(127, 127, 127, 255)));
    CHECK(expectColor(floatImage, 2, 0, QColor(255, 255, 255, 255)));
    CHECK(floatImage.save(artifactDir + "/imageitem_float_levels.png"));

    const auto levels = floatItem.getLevels();
    CHECK(levels.has_value());
    CHECK(levels->minimum == 0.0 && levels->maximum == 1.0);
    floatItem.setLevels(std::nullopt);
    CHECK(!floatItem.getLevels().has_value());
    lutItem.clearLookupTable();
    CHECK(!lutItem.lookupTable().has_value());

    return true;
}

bool exerciseNonUniformImage(const QString& artifactDir)
{
    using pyqtgraph::ImageLevelRange;
    using pyqtgraph::core::ArrayView;
    using pyqtgraph::graphicsItems::NonUniformImage;

    const std::array<double, 3> x{{0.0, 2.0, 5.0}};
    const std::array<double, 3> y{{0.0, 1.0, 3.0}};
    const std::array<double, 9> z{{
        0.0, 1.0, 2.0,
        3.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), 4.0, 2.0,
    }};
    std::vector<std::uint8_t> lut{
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 0, 255,
    };

    NonUniformImage image(ArrayView<const double, 1>(x.data(), {x.size()}),
                          ArrayView<const double, 1>(y.data(), {y.size()}),
                          ArrayView<const double, 2>(z.data(), {x.size(), y.size()}));
    CHECK(image.boundingRect() == QRectF(0.0, 0.0, 5.0, 3.0));
    image.setLookupTable(lutView(lut, 4));
    image.setLevels(ImageLevelRange{0.0, 4.0});
    lut[0] = 0; // setLookupTable must copy input bytes.

    QImage canvas(50, 30, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.scale(10.0, 10.0);
    image.paint(&painter, nullptr, nullptr);
    painter.end();

    CHECK(canvas.save(artifactDir + "/nonuniform_actual.png"));
    CHECK(expectColor(canvas, 5, 2, QColor(255, 0, 0, 255)));       // z=0 low clip/color
    CHECK(expectColor(canvas, 5, 7, QColor(0, 255, 0, 255)));       // z=1
    CHECK(expectColor(canvas, 5, 20, QColor(0, 0, 255, 255)));      // z=2
    CHECK(expectColor(canvas, 20, 2, QColor(255, 255, 0, 255)));    // z=3
    CHECK(expectColor(canvas, 20, 10, QColor(0, 0, 0, 0)));         // NaN skipped, transparent background remains
    CHECK(expectColor(canvas, 20, 25, QColor(255, 255, 0, 255)));   // +inf clips high
    CHECK(expectColor(canvas, 42, 2, QColor(255, 0, 0, 255)));      // -inf clips low

    const ImageLevelRange lazyLevels = NonUniformImage(ArrayView<const double, 1>(x.data(), {x.size()}),
                                                       ArrayView<const double, 1>(y.data(), {y.size()}),
                                                       ArrayView<const double, 2>(z.data(), {x.size(), y.size()}))
                                            .getLevels();
    CHECK(lazyLevels.minimum == 0.0);
    CHECK(lazyLevels.maximum == 4.0);

    const std::array<double, 3> badX{{0.0, 2.0, 1.0}};
    CHECK(throwsInvalidArgument([&] {
        NonUniformImage bad(ArrayView<const double, 1>(badX.data(), {badX.size()}),
                            ArrayView<const double, 1>(y.data(), {y.size()}),
                            ArrayView<const double, 2>(z.data(), {x.size(), y.size()}));
    }));
    CHECK(throwsInvalidArgument([&] {
        NonUniformImage bad(ArrayView<const double, 1>(x.data(), {x.size()}),
                            ArrayView<const double, 1>(y.data(), {y.size()}),
                            ArrayView<const double, 2>(z.data(), {2, y.size()}));
    }));

    return true;
}

bool runP405()
{
    const QString artifactDir = QString::fromUtf8(PYQTGRAPH_CPP_P4_05_ARTIFACT_DIR);
    CHECK(ensureDirectory(artifactDir));

    const bool imageItemPassed = exerciseImageItemLevelsLuts(artifactDir);
    const bool nonUniformPassed = exerciseNonUniformImage(artifactDir);
    const bool passed = imageItemPassed && nonUniformPassed;

    const QString metrics = QStringLiteral(
        "{\n"
        "  \"issue\": \"P4.05\",\n"
        "  \"format_dtype_stride_copy_color_order_levels_lut\": \"exact zero-tolerance pixel checks; "
        "uint8/uint16/float scalar buffers; non-contiguous stride copied; RGBA LUT order and alpha checked; "
        "levels clipping checked; >256-row LUT indices checked without uint8 truncation; NonUniformImage NaN skip and infinities clip checked\",\n"
        "  \"tolerance\": 0,\n"
        "  \"imageitem_passed\": %1,\n"
        "  \"nonuniform_passed\": %2,\n"
        "  \"passed\": %3\n"
        "}\n")
                                .arg(imageItemPassed ? QStringLiteral("true") : QStringLiteral("false"))
                                .arg(nonUniformPassed ? QStringLiteral("true") : QStringLiteral("false"))
                                .arg(passed ? QStringLiteral("true") : QStringLiteral("false"));
    CHECK(writeTextFile(artifactDir + "/metrics.json", metrics));
    return passed;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    ApplicationGuard application(argc, argv);
    return runP405() ? 0 : 1;
}
