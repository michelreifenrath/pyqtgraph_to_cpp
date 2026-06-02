#include "../../include/pyqtgraph/functions_qimage.hpp"

#include <QImage>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>

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

bool checkGrayscale8Pixel(const QImage& image, int x, int y, int gray, std::string_view label)
{
    const uchar* row = image.constScanLine(y);
    if (row[x] != static_cast<uchar>(gray)) {
        std::cerr << label << " at (" << x << ", " << y << "): expected " << gray << " got "
                  << static_cast<int>(row[x]) << '\n';
        return false;
    }
    return true;
}

#define CHECK_GRAY8(image, x, y, gray) \
    do { \
        if (!checkGrayscale8Pixel((image), (x), (y), (gray), #image)) { \
            return false; \
        } \
    } while (false)

bool checkRgb888Pixel(const QImage& image, int x, int y, int red, int green, int blue, std::string_view label)
{
    const uchar* row = image.constScanLine(y);
    const std::size_t base = static_cast<std::size_t>(x) * 3;
    if (row[base + 0] != static_cast<uchar>(red) || row[base + 1] != static_cast<uchar>(green)
        || row[base + 2] != static_cast<uchar>(blue)) {
        std::cerr << label << " at (" << x << ", " << y << "): expected rgb(" << red << ", " << green << ", "
                  << blue << ") got rgb(" << static_cast<int>(row[base + 0]) << ", "
                  << static_cast<int>(row[base + 1]) << ", " << static_cast<int>(row[base + 2]) << ")\n";
        return false;
    }
    return true;
}

#define CHECK_RGB888(image, x, y, red, green, blue) \
    do { \
        if (!checkRgb888Pixel((image), (x), (y), (red), (green), (blue), #image)) { \
            return false; \
        } \
    } while (false)

bool checkRgba8888Pixel(const QImage& image,
                        int x,
                        int y,
                        int red,
                        int green,
                        int blue,
                        int alpha,
                        std::string_view label)
{
    const uchar* row = image.constScanLine(y);
    const std::size_t base = static_cast<std::size_t>(x) * 4;
    if (row[base + 0] != static_cast<uchar>(red) || row[base + 1] != static_cast<uchar>(green)
        || row[base + 2] != static_cast<uchar>(blue) || row[base + 3] != static_cast<uchar>(alpha)) {
        std::cerr << label << " at (" << x << ", " << y << "): expected rgba(" << red << ", " << green << ", "
                  << blue << ", " << alpha << ") got rgba(" << static_cast<int>(row[base + 0]) << ", "
                  << static_cast<int>(row[base + 1]) << ", " << static_cast<int>(row[base + 2]) << ", "
                  << static_cast<int>(row[base + 3]) << ")\n";
        return false;
    }
    return true;
}

#define CHECK_RGBA8888(image, x, y, red, green, blue, alpha) \
    do { \
        if (!checkRgba8888Pixel((image), (x), (y), (red), (green), (blue), (alpha), #image)) { \
            return false; \
        } \
    } while (false)

bool checkGrayscale16Pixel(const QImage& image, int x, int y, std::uint16_t value, std::string_view label)
{
    const auto* row = reinterpret_cast<const std::uint16_t*>(image.constScanLine(y));
    if (row[x] != value) {
        std::cerr << label << " at (" << x << ", " << y << "): expected " << value << " got " << row[x] << '\n';
        return false;
    }
    return true;
}

#define CHECK_GRAY16(image, x, y, value) \
    do { \
        if (!checkGrayscale16Pixel((image), (x), (y), (value), #image)) { \
            return false; \
        } \
    } while (false)

bool checkRgba64Pixel(const QImage& image,
                      int x,
                      int y,
                      std::uint16_t red,
                      std::uint16_t green,
                      std::uint16_t blue,
                      std::uint16_t alpha,
                      std::string_view label)
{
    const auto* row = reinterpret_cast<const std::uint16_t*>(image.constScanLine(y));
    const std::size_t base = static_cast<std::size_t>(x) * 4;
    if (row[base + 0] != red || row[base + 1] != green || row[base + 2] != blue || row[base + 3] != alpha) {
        std::cerr << label << " at (" << x << ", " << y << "): channel mismatch\n";
        return false;
    }
    return true;
}

#define CHECK_RGBA64(image, x, y, red, green, blue, alpha) \
    do { \
        if (!checkRgba64Pixel((image), (x), (y), (red), (green), (blue), (alpha), #image)) { \
            return false; \
        } \
    } while (false)

bool testUint8GrayscalePassthrough()
{
    const std::array<std::uint8_t, 6> data{10, 20, 30, 40, 50, 60};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK(image->width() == 3);
    CHECK(image->height() == 2);
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_GRAY8(*image, 0, 0, 10);
    CHECK_GRAY8(*image, 2, 0, 30);
    CHECK_GRAY8(*image, 0, 1, 40);
    CHECK_GRAY8(*image, 2, 1, 60);
    return true;
}

bool testUint8RgbOrder()
{
    const std::array<std::uint8_t, 12> data{1, 2, 3, 10, 20, 30, 100, 110, 120, 200, 210, 220};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {2, 2, 3});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_RGB888);
    CHECK_RGB888(*image, 0, 0, 1, 2, 3);
    CHECK_RGB888(*image, 1, 0, 10, 20, 30);
    CHECK_RGB888(*image, 0, 1, 100, 110, 120);
    CHECK_RGB888(*image, 1, 1, 200, 210, 220);
    return true;
}

bool testUint8RgbaPreservesAlpha()
{
    const std::array<std::uint8_t, 16> data{1, 2, 3, 4, 10, 20, 30, 128, 5, 6, 7, 0, 8, 9, 10, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> view(data.data(), {2, 2, 4});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_RGBA8888);
    CHECK_RGBA8888(*image, 0, 0, 1, 2, 3, 4);
    CHECK_RGBA8888(*image, 1, 0, 10, 20, 30, 128);
    CHECK_RGBA8888(*image, 0, 1, 5, 6, 7, 0);
    CHECK_RGBA8888(*image, 1, 1, 8, 9, 10, 255);
    return true;
}

bool testStridedInputCopiesToContiguous()
{
    const std::array<std::uint8_t, 12> data{1, 99, 2, 99, 3, 99, 4, 99, 5, 99, 6, 99};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 3}, {6, 2});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK(image->bytesPerLine() >= image->width());
    CHECK(image->constBits() != nullptr);
    CHECK_GRAY8(*image, 0, 0, 1);
    CHECK_GRAY8(*image, 2, 0, 3);
    CHECK_GRAY8(*image, 0, 1, 4);
    CHECK_GRAY8(*image, 2, 1, 6);
    return true;
}

bool testCopyDetachesSource()
{
    std::array<std::uint8_t, 4> data{10, 20, 30, 40};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK_GRAY8(*image, 1, 0, 20);

    data[1] = 99;
    CHECK_GRAY8(*image, 1, 0, 20);
    return true;
}

bool testUint16GrayscalePassthrough()
{
    if (QImage::Format_Grayscale16 == QImage::Format_Invalid) {
        return true;
    }

    const std::array<std::uint16_t, 4> data{1000, 2000, 3000, 4000};
    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {2, 2});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale16);
    CHECK_GRAY16(*image, 0, 0, 1000);
    CHECK_GRAY16(*image, 1, 0, 2000);
    CHECK_GRAY16(*image, 0, 1, 3000);
    CHECK_GRAY16(*image, 1, 1, 4000);
    return true;
}

bool testUint16Rgba64Passthrough()
{
    if (QImage::Format_RGBA64 == QImage::Format_Invalid) {
        return true;
    }

    const std::array<std::uint16_t, 16> data{
        100, 200, 300, 400,
        500, 600, 700, 800,
        900, 1000, 1100, 1200,
        1300, 1400, 1500, 1600,
    };
    const pyqtgraph::core::ArrayView<const std::uint16_t, 3> view(data.data(), {2, 2, 4});

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_RGBA64);
    CHECK_RGBA64(*image, 0, 0, 100, 200, 300, 400);
    CHECK_RGBA64(*image, 1, 1, 1300, 1400, 1500, 1600);
    return true;
}

bool testUnsupportedReturnsNullopt()
{
    const std::array<std::uint8_t, 12> data{};
    const std::array<std::uint16_t, 12> data16{};

    CHECK(!pyqtgraph::tryMakeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2>(nullptr, {1, 1})).has_value());
    CHECK(!pyqtgraph::tryMakeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2>(data.data(), {0, 2})).has_value());
    CHECK(!pyqtgraph::tryMakeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3>(data.data(), {1, 1, 1})).has_value());
    CHECK(!pyqtgraph::tryMakeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3>(data.data(), {1, 1, 2})).has_value());
    CHECK(!pyqtgraph::tryMakeQImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3>(data.data(), {1, 1, 5})).has_value());
    CHECK(!pyqtgraph::tryMakeQImage(pyqtgraph::core::ArrayView<const std::uint16_t, 3>(data16.data(), {1, 1, 3})).has_value());
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testUint8GrayscalePassthrough() && success;
    success = testUint8RgbOrder() && success;
    success = testUint8RgbaPreservesAlpha() && success;
    success = testStridedInputCopiesToContiguous() && success;
    success = testCopyDetachesSource() && success;
    success = testUint16GrayscalePassthrough() && success;
    success = testUint16Rgba64Passthrough() && success;
    success = testUnsupportedReturnsNullopt() && success;

    return success ? 0 : 1;
}
