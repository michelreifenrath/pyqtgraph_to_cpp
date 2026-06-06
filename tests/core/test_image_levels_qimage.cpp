#include "../../include/pyqtgraph/functions.hpp"
#include "../../include/pyqtgraph/functions_qimage.hpp"

#include <QColor>
#include <QImage>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
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

template <typename Callable>
bool checkThrowsInvalidArgument(Callable callable, std::string_view label)
{
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << label << ": expected std::invalid_argument, got " << error.what() << '\n';
        return false;
    }

    std::cerr << label << ": expected std::invalid_argument\n";
    return false;
}

bool testApplyLookupTableClipsIndices()
{
    const std::array<std::uint8_t, 9> lutData{10, 20, 30, 40, 50, 60, 70, 80, 90};
    const pyqtgraph::ImageLookupTable lut{lutData.data(), 3, 3, 3, 1};

    CHECK((pyqtgraph::applyLookupTable(-7, lut) == std::array<std::uint8_t, 4>{10, 20, 30, 255}));
    CHECK((pyqtgraph::applyLookupTable(1, lut) == std::array<std::uint8_t, 4>{40, 50, 60, 255}));
    CHECK((pyqtgraph::applyLookupTable(99, lut) == std::array<std::uint8_t, 4>{70, 80, 90, 255}));
    return true;
}

bool testUint8LevelsUseIndexedColorTable()
{
    const std::array<std::uint8_t, 5> data{0, 1, 2, 3, 4};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, data.size()});
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevelRange{1.0, 3.0};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK(static_cast<int>(image->constScanLine(0)[4]) == 4);
    CHECK_PIXEL(*image, 0, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 1, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 2, 0, 127, 127, 127, 255);
    CHECK_PIXEL(*image, 3, 0, 255, 255, 255, 255);
    CHECK_PIXEL(*image, 4, 0, 255, 255, 255, 255);
    return true;
}

bool testUint8LutWithoutLevelsUsesClippedEffectiveTable()
{
    std::array<std::uint8_t, 256 * 3> lutData{};
    for (std::size_t index = 0; index < 256; ++index) {
        lutData[index * 3 + 0] = static_cast<std::uint8_t>(index);
        lutData[index * 3 + 1] = static_cast<std::uint8_t>(255 - index);
        lutData[index * 3 + 2] = 7;
    }

    const std::array<std::uint8_t, 3> data{0, 1, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, data.size()});
    pyqtgraph::TryMakeQImageOptions options;
    options.lut = pyqtgraph::ImageLookupTable{lutData.data(), 256, 3, 3, 1};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK_PIXEL(*image, 0, 0, 0, 255, 7, 255);
    CHECK_PIXEL(*image, 1, 0, 1, 254, 7, 255);
    CHECK_PIXEL(*image, 2, 0, 255, 0, 7, 255);
    return true;
}

bool testLevelsLutMoreThan256RowsDoesNotTruncateEffectiveIndices()
{
    std::vector<std::uint8_t> lutData(512);
    for (std::size_t index = 0; index < lutData.size(); ++index) {
        lutData[index] = index < 256 ? 10 : static_cast<std::uint8_t>(200 + ((index - 256) % 50));
    }

    const std::array<std::uint8_t, 3> data{0, 128, 255};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, data.size()});
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevelRange{0.0, 255.0};
    options.lut = pyqtgraph::ImageLookupTable{lutData.data(), lutData.size(), 1, 1, 1};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Indexed8);
    CHECK_PIXEL(*image, 0, 0, 10, 10, 10, 255);
    CHECK_PIXEL(*image, 1, 0, 201, 201, 201, 255);
    CHECK_PIXEL(*image, 2, 0, 205, 205, 205, 255);
    return true;
}

bool testUint16LevelsRescaleToGray8()
{
    const std::array<std::uint16_t, 4> data{0, 512, 32768, 65535};
    const pyqtgraph::core::ArrayView<const std::uint16_t, 2> view(data.data(), {1, data.size()});
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevelRange{512.0, 65536.0};

    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_PIXEL(*image, 0, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 1, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 2, 0, 126, 126, 126, 255);
    CHECK_PIXEL(*image, 3, 0, 254, 254, 254, 255);
    return true;
}

bool testFloatRequiresLevelsAndRescales()
{
    const std::array<float, 4> data{1.0F, 5.0F, 13.0F, 17.0F};
    const pyqtgraph::core::ArrayView<const float, 2> view(data.data(), {1, data.size()});

    CHECK(!pyqtgraph::tryMakeQImage(view).has_value());

    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevelRange{5.0, 13.0};
    const std::optional<QImage> image = pyqtgraph::tryMakeQImage(view, options);
    CHECK(image.has_value());
    CHECK(image->format() == QImage::Format_Grayscale8);
    CHECK_PIXEL(*image, 0, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 1, 0, 0, 0, 0, 255);
    CHECK_PIXEL(*image, 2, 0, 255, 255, 255, 255);
    CHECK_PIXEL(*image, 3, 0, 255, 255, 255, 255);
    return true;
}

bool testTryMakeQImageGuardsUnsupportedCombinations()
{
    const std::array<std::uint8_t, 4> rgba{1, 2, 3, 4};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 3> rgbaView(rgba.data(), {1, 1, 4});
    pyqtgraph::TryMakeQImageOptions options;
    options.levels = pyqtgraph::ImageLevelRange{0.0, 255.0};
    CHECK(!pyqtgraph::tryMakeQImage(rgbaView, options).has_value());

    const std::array<std::uint8_t, 4> data{0, 1, 2, 3};
    const pyqtgraph::core::ArrayView<const std::uint8_t, 2> view(data.data(), {1, data.size()});
    options.lut = pyqtgraph::ImageLookupTable{data.data(), 2, 2, 2, 1};
    CHECK(checkThrowsInvalidArgument([&] { (void)pyqtgraph::tryMakeQImage(view, options); }, "two-channel lut"));
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testApplyLookupTableClipsIndices() && success;
    success = testUint8LevelsUseIndexedColorTable() && success;
    success = testUint8LutWithoutLevelsUsesClippedEffectiveTable() && success;
    success = testLevelsLutMoreThan256RowsDoesNotTruncateEffectiveIndices() && success;
    success = testUint16LevelsRescaleToGray8() && success;
    success = testFloatRequiresLevelsAndRescales() && success;
    success = testTryMakeQImageGuardsUnsupportedCombinations() && success;

    return success ? 0 : 1;
}
