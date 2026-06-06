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

bool checkColor(const QImage& image, int x, int y, int red, int green, int blue, int alpha, std::string_view label)
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

#define CHECK_COLOR(image, x, y, red, green, blue, alpha) \
    do { \
        if (!checkColor((image), (x), (y), (red), (green), (blue), (alpha), #image)) { \
            return false; \
        } \
    } while (false)

bool testRescaleDataMatchesPyQtGraphClippingOracle()
{
    const std::array<int, 5> values{-10, 0, 1, 25, 1000};
    const std::vector<std::uint8_t> scaled = pyqtgraph::rescaleData<std::uint8_t>(values, 10.0, 0.0);
    CHECK((scaled == std::vector<std::uint8_t>{0, 0, 10, 250, 255}));

    const std::array<double, 4> floats{-1.0, 0.25, 0.5, 2.0};
    const std::vector<std::uint8_t> clipped = pyqtgraph::rescaleData<std::uint8_t>(
        floats,
        255.0,
        0.0,
        pyqtgraph::RescaleClip{0.0, 127.0});
    CHECK((clipped == std::vector<std::uint8_t>{0, 63, 127, 127}));
    return true;
}

bool testApplyLookupTableClipsIntegerIndices()
{
    const std::array<int, 5> indices{-5, 0, 1, 2, 99};
    const std::array<std::uint8_t, 3> lut{10, 20, 30};
    const std::vector<std::uint8_t> lookedUp = pyqtgraph::applyLookupTable(indices, lut);
    CHECK((lookedUp == std::vector<std::uint8_t>{10, 10, 20, 30, 30}));
    return true;
}

bool testUint8LevelsUseIndexed8ColorTable()
{
    const std::array<std::uint8_t, 5> data{0, 64, 128, 192, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, 5});
    const pyqtgraph::TryMakeQImageOptions options{.levels = pyqtgraph::ImageLevels{64.0, 192.0}};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK(image->width() == 5);
    CHECK(image->height() == 1);
    CHECK_COLOR(*image, 0, 0, 0, 0, 0, 255);
    CHECK_COLOR(*image, 1, 0, 0, 0, 0, 255);
    CHECK_COLOR(*image, 2, 0, 127, 127, 127, 255);
    CHECK_COLOR(*image, 3, 0, 255, 255, 255, 255);
    CHECK_COLOR(*image, 4, 0, 255, 255, 255, 255);
    return true;
}

bool testUint8LevelsAndLutCombineToEffectiveTable()
{
    std::array<std::uint8_t, 256> lut{};
    for (std::size_t i = 0; i < lut.size(); ++i) {
        lut[i] = static_cast<std::uint8_t>(255 - i);
    }
    const std::array<std::uint8_t, 5> data{0, 64, 128, 192, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, 5});
    const pyqtgraph::ImageLookupTable table{pyqtgraph::core::ArrayView<const std::uint8_t, 2>(lut.data(), {256, 1})};
    const pyqtgraph::TryMakeQImageOptions options{.levels = pyqtgraph::ImageLevels{64.0, 192.0}, .lut = table};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK_COLOR(*image, 0, 0, 255, 255, 255, 255);
    CHECK_COLOR(*image, 1, 0, 255, 255, 255, 255);
    CHECK_COLOR(*image, 2, 0, 127, 127, 127, 255);
    CHECK_COLOR(*image, 3, 0, 0, 0, 0, 255);
    CHECK_COLOR(*image, 4, 0, 0, 0, 0, 255);
    return true;
}

bool testUint16LevelsAndRgbLutRescaleToIndexed8()
{
    const std::array<std::uint16_t, 5> data{0, 512, 1024, 1536, 2048};
    const std::array<std::uint8_t, 12> lut{255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255};
    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {1, 5});
    const pyqtgraph::ImageLookupTable table{pyqtgraph::core::ArrayView<const std::uint8_t, 2>(lut.data(), {4, 3})};
    const pyqtgraph::TryMakeQImageOptions options{.levels = pyqtgraph::ImageLevels{512.0, 1536.0}, .lut = table};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK_COLOR(*image, 0, 0, 255, 0, 0, 255);
    CHECK_COLOR(*image, 1, 0, 255, 0, 0, 255);
    CHECK_COLOR(*image, 2, 0, 0, 0, 255, 255);
    CHECK_COLOR(*image, 3, 0, 255, 255, 255, 255);
    CHECK_COLOR(*image, 4, 0, 255, 255, 255, 255);
    return true;
}

bool testUint16LargeGrayscaleLutAppliesLookup()
{
    const std::array<std::uint16_t, 3> data{1000, 1255, 2000};
    std::vector<std::uint8_t> lut(65536, 0);
    for (std::size_t row = 1000; row < 1256; ++row) {
        lut[row] = static_cast<std::uint8_t>(row - 1000);
    }
    for (std::size_t row = 1256; row < lut.size(); ++row) {
        lut[row] = 255;
    }

    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {1, 3});
    const pyqtgraph::ImageLookupTable table{pyqtgraph::core::ArrayView<const std::uint8_t, 2>(lut.data(), {lut.size(), 1})};
    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, pyqtgraph::TryMakeQImageOptions{.lut = table});
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_COLOR(*image, 0, 0, 0, 0, 0, 255);
    CHECK_COLOR(*image, 1, 0, 255, 255, 255, 255);
    CHECK_COLOR(*image, 2, 0, 255, 255, 255, 255);
    return true;
}

bool testFloatMonoRequiresLevelsAndRescales()
{
    const std::array<float, 3> data{1.0F, 5.0F, 9.0F};
    const pyqtgraph::core::ArrayView<const float, 2> view(data.data(), {1, 3});
    CHECK(!pyqtgraph::tryMakeQImage(view).has_value());

    const pyqtgraph::TryMakeQImageOptions options{.levels = pyqtgraph::ImageLevels{1.0, 9.0}};
    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_COLOR(*image, 0, 0, 0, 0, 0, 255);
    CHECK_COLOR(*image, 1, 0, 127, 127, 127, 255);
    CHECK_COLOR(*image, 2, 0, 255, 255, 255, 255);
    return true;
}

bool testUnsupportedEdgeCasesReturnNullopt()
{
    const std::array<std::uint8_t, 8> data{};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {2, 2});
    const pyqtgraph::ImageLookupTable badColumns{pyqtgraph::core::ArrayView<const std::uint8_t, 2>(data.data(), {2, 2})};
    CHECK(!pyqtgraph::tryMakeQImage(view, pyqtgraph::TryMakeQImageOptions{.lut = badColumns}).has_value());

    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> rgba(data.data(), {1, 1, 4});
    CHECK(!pyqtgraph::tryMakeQImage(rgba, pyqtgraph::TryMakeQImageOptions{.levels = pyqtgraph::ImageLevels{0.0, 1.0}})
               .has_value());
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testRescaleDataMatchesPyQtGraphClippingOracle() && success;
    success = testApplyLookupTableClipsIntegerIndices() && success;
    success = testUint8LevelsUseIndexed8ColorTable() && success;
    success = testUint8LevelsAndLutCombineToEffectiveTable() && success;
    success = testUint16LevelsAndRgbLutRescaleToIndexed8() && success;
    success = testUint16LargeGrayscaleLutAppliesLookup() && success;
    success = testFloatMonoRequiresLevelsAndRescales() && success;
    success = testUnsupportedEdgeCasesReturnNullopt() && success;

    return success ? 0 : 1;
}
