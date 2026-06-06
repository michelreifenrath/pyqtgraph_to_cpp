#include "../../include/pyqtgraph/functions.hpp"
#include "../../include/pyqtgraph/functions_qimage.hpp"

#include <QColor>
#include <QImage>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

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

bool checkPixel(const QImage& image, int x, int y, int red, int green, int blue, int alpha, std::string_view label)
{
    const QColor color = image.pixelColor(x, y);
    if (color.red() != red || color.green() != green || color.blue() != blue || color.alpha() != alpha) {
        std::cerr << label << " at (" << x << ", " << y << "): expected rgba(" << red << ", " << green << ", "
                  << blue << ", " << alpha << ") got rgba(" << color.red() << ", " << color.green() << ", "
                  << color.blue() << ", " << color.alpha() << ")\n";
        return false;
    }
    return true;
}

#define CHECK_PIXEL(image, x, y, red, green, blue, alpha) \
    do { \
        if (!checkPixel((image), (x), (y), (red), (green), (blue), (alpha), #image)) { \
            return false; \
        } \
    } while (false)

pyqtgraph::LookupTableView monoLut(const std::uint8_t* data, std::size_t rows)
{
    pyqtgraph::LookupTableView lut;
    lut.data = data;
    lut.rows = rows;
    lut.channels = 1;
    return lut;
}

pyqtgraph::LookupTableView rgbLut(const std::uint8_t* data, std::size_t rows)
{
    pyqtgraph::LookupTableView lut;
    lut.data = data;
    lut.rows = rows;
    lut.channels = 3;
    return lut;
}

pyqtgraph::LookupTableView rgbaLut(const std::uint8_t* data, std::size_t rows)
{
    pyqtgraph::LookupTableView lut;
    lut.data = data;
    lut.rows = rows;
    lut.channels = 4;
    return lut;
}

bool testRescaleAndLookupHelperClipsIndexes()
{
    CHECK(pyqtgraph::detail::clippedLookupIndex(-10.0, 1.0, 0.0, 4) == 0);
    CHECK(pyqtgraph::detail::clippedLookupIndex(2.9, 1.0, 0.0, 4) == 2);
    CHECK(pyqtgraph::detail::clippedLookupIndex(99.0, 1.0, 0.0, 4) == 3);
    CHECK(pyqtgraph::detail::rescaleDataToUint8(128.0, 255.0 / 128.0, 64.0) == 127);
    CHECK(pyqtgraph::detail::rescaleDataToUint8(255.0, -1.0, 255.0) == 0);
    return true;
}

bool testUint8LevelsUseIndexedColorTableWithoutChangingBytes()
{
    const std::array<std::uint8_t, 4> data{0, 64, 128, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, 4});
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevels{64.0, 192.0};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK(image->colorCount() == 256);
    CHECK(image->constScanLine(0)[2] == 128);
    CHECK_PIXEL(*image, 0, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 1, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 2, 0, 127, 127, 127, 255);
    CHECK_PIXEL(*image, 3, 0, 255, 255, 255, 255);
    return true;
}

bool testUint8LevelsCombineWithRgbLookupTable()
{
    const std::array<std::uint8_t, 4> data{0, 64, 128, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, 4});
    const std::array<std::uint8_t, 12> lut{
        10, 20, 30,
        40, 50, 60,
        70, 80, 90,
        100, 110, 120,
    };
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevels{64.0, 192.0};
    options.lut = rgbLut(lut.data(), 4);

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK_PIXEL(*image, 0, 0, 10, 20, 30, 255);
    CHECK_PIXEL(*image, 1, 0, 10, 20, 30, 255);
    CHECK_PIXEL(*image, 2, 0, 70, 80, 90, 255);
    CHECK_PIXEL(*image, 3, 0, 100, 110, 120, 255);
    return true;
}

bool testUint16LevelsOnlyRescalesToGrayscale8()
{
    const std::array<std::uint16_t, 4> data{0, 512, 768, 1023};
    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {1, 4});
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevels{512.0, 1023.0};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_PIXEL(*image, 0, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 1, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 2, 0, 127, 127, 127, 255);
    CHECK_PIXEL(*image, 3, 0, 255, 255, 255, 255);
    return true;
}

bool testUint16LargeLookupTableDoesNotTruncateIndexesToUint8()
{
    const std::array<std::uint16_t, 4> data{0, 255, 256, 65535};
    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {1, 4});
    std::vector<std::uint8_t> lut(65536, 0);
    lut[0] = 10;
    lut[255] = 20;
    lut[256] = 99;
    lut[65535] = 77;
    pyqtgraph::TryMakeQImageOptions options;
    options.lut = monoLut(lut.data(), lut.size());

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_PIXEL(*image, 0, 0, 10, 10, 10, 255);
    CHECK_PIXEL(*image, 1, 0, 20, 20, 20, 255);
    CHECK_PIXEL(*image, 2, 0, 99, 99, 99, 255);
    CHECK_PIXEL(*image, 3, 0, 77, 77, 77, 255);
    return true;
}

bool testLevelsWithLargeRgbaLookupTableUsesRowsAbove255()
{
    const std::array<std::uint16_t, 3> data{0, 300, 1023};
    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {1, 3});
    std::vector<std::uint8_t> lut(1024 * 4, 0);
    lut[0] = 1;
    lut[1] = 2;
    lut[2] = 3;
    lut[3] = 4;
    lut[300 * 4 + 0] = 30;
    lut[300 * 4 + 1] = 40;
    lut[300 * 4 + 2] = 50;
    lut[300 * 4 + 3] = 60;
    lut[1023 * 4 + 0] = 200;
    lut[1023 * 4 + 1] = 210;
    lut[1023 * 4 + 2] = 220;
    lut[1023 * 4 + 3] = 230;
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevels{0.0, 1023.0};
    options.lut = rgbaLut(lut.data(), 1024);

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_RGBA8888);
    CHECK_PIXEL(*image, 0, 0, 1, 2, 3, 4);
    CHECK_PIXEL(*image, 1, 0, 30, 40, 50, 60);
    CHECK_PIXEL(*image, 2, 0, 200, 210, 220, 230);
    return true;
}

bool testFloatLevelsAndLookupTable()
{
    const std::array<float, 4> data{-1.0F, 0.0F, 0.5F, 1.0F};
    const pyqtgraph::core::ArrayView<const float, 2> view(data.data(), {1, 4});
    const std::array<std::uint8_t, 12> lut{
        0, 0, 0,
        10, 20, 30,
        40, 50, 60,
        70, 80, 90,
    };
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevels{-1.0, 1.0};
    options.lut = rgbLut(lut.data(), 4);

    CHECK(!pyqtgraph::tryMakeQImage(view).has_value());
    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK_PIXEL(*image, 0, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 1, 0, 40, 50, 60, 255);
    CHECK_PIXEL(*image, 2, 0, 70, 80, 90, 255);
    CHECK_PIXEL(*image, 3, 0, 70, 80, 90, 255);
    return true;
}

bool testUnsupportedLookupTableShapeReturnsNullopt()
{
    const std::array<std::uint8_t, 1> data{0};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, 1});
    const std::array<std::uint8_t, 2> lut{1, 2};
    pyqtgraph::TryMakeQImageOptions options;
    pyqtgraph::LookupTableView invalid;
    invalid.data = lut.data();
    invalid.rows = 1;
    invalid.channels = 2;
    options.lut = invalid;
    CHECK(!pyqtgraph::tryMakeQImage(view, options).has_value());
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testRescaleAndLookupHelperClipsIndexes() && success;
    success = testUint8LevelsUseIndexedColorTableWithoutChangingBytes() && success;
    success = testUint8LevelsCombineWithRgbLookupTable() && success;
    success = testUint16LevelsOnlyRescalesToGrayscale8() && success;
    success = testUint16LargeLookupTableDoesNotTruncateIndexesToUint8() && success;
    success = testLevelsWithLargeRgbaLookupTableUsesRowsAbove255() && success;
    success = testFloatLevelsAndLookupTable() && success;
    success = testUnsupportedLookupTableShapeReturnsNullopt() && success;

    return success ? 0 : 1;
}
