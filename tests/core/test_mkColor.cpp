#include "../../include/pyqtgraph/functions.hpp"

#include <QColor>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string_view>
#include <tuple>

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

bool checkRgba(const QColor& color, int red, int green, int blue, int alpha, std::string_view label)
{
    if (color.red() != red || color.green() != green || color.blue() != blue || color.alpha() != alpha) {
        std::cerr << label << ": expected rgba(" << red << ", " << green << ", " << blue << ", " << alpha
                  << ") got rgba(" << color.red() << ", " << color.green() << ", " << color.blue() << ", "
                  << color.alpha() << ")\n";
        return false;
    }
    return true;
}

#define CHECK_RGBA(color, red, green, blue, alpha) \
    do { \
        if (!checkRgba((color), (red), (green), (blue), (alpha), #color)) { \
            return false; \
        } \
    } while (false)

bool checkHsva(const QColor& color, int hue, int saturation, int value, int alpha, std::string_view label)
{
    int actualHue = 0;
    int actualSaturation = 0;
    int actualValue = 0;
    int actualAlpha = 0;
    color.getHsv(&actualHue, &actualSaturation, &actualValue, &actualAlpha);

    if (actualHue != hue || actualSaturation != saturation || actualValue != value || actualAlpha != alpha) {
        std::cerr << label << ": expected hsva(" << hue << ", " << saturation << ", " << value << ", " << alpha
                  << ") got hsva(" << actualHue << ", " << actualSaturation << ", " << actualValue << ", "
                  << actualAlpha << ")\n";
        return false;
    }
    return true;
}

#define CHECK_HSVA(color, hue, saturation, value, alpha) \
    do { \
        if (!checkHsva((color), (hue), (saturation), (value), (alpha), #color)) { \
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

bool testShortNames()
{
    CHECK_RGBA(pyqtgraph::mkColor("r"), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor('r'), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("g"), 0, 255, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("b"), 0, 0, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor("d"), 150, 150, 150, 255);
    CHECK_RGBA(pyqtgraph::mkColor("l"), 200, 200, 200, 255);
    CHECK_RGBA(pyqtgraph::mkColor("s"), 100, 100, 150, 255);
    return true;
}

bool testStringsAndHex()
{
    CHECK_RGBA(pyqtgraph::mkColor("steelblue"), 70, 130, 180, 255);
    CHECK_RGBA(pyqtgraph::mkColor("#0f8"), 0, 255, 136, 255);
    CHECK_RGBA(pyqtgraph::mkColor("#0f8c"), 0, 255, 136, 204);
    CHECK_RGBA(pyqtgraph::mkColor("#336699"), 51, 102, 153, 255);
    CHECK_RGBA(pyqtgraph::mkColor("#33669980"), 51, 102, 153, 128);
    return true;
}

bool testNumericInputs()
{
    CHECK_RGBA(pyqtgraph::mkColor(0.5), 127, 127, 127, 255);
    CHECK_RGBA(pyqtgraph::mkColor(1.0), 255, 255, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor(1.0, 2.0, 3.0), 1, 2, 3, 255);
    CHECK_RGBA(pyqtgraph::mkColor(4.0, 5.0, 6.0, 7.0), 4, 5, 6, 7);
    CHECK_RGBA(pyqtgraph::mkColor({8.0, 9.0, 10.0}), 8, 9, 10, 255);
    CHECK_RGBA(pyqtgraph::mkColor({11.0, 12.0, 13.0, 14.0}), 11, 12, 13, 14);
    CHECK_RGBA(pyqtgraph::mkColor(1.0, std::numeric_limits<double>::infinity(), std::nan(""), 4.0), 1, 0, 0, 4);
    return true;
}

bool testIntColorInputs()
{
    CHECK_RGBA(pyqtgraph::intColor(0), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor(0), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::intColor(1), 255, 170, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor(1U), 255, 170, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor(std::size_t{2}), 170, 255, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor(std::uint32_t{3}), 0, 255, 0, 255);
    CHECK(pyqtgraph::mkColor(static_cast<signed char>('r')) ==
          pyqtgraph::intColor(static_cast<int>(static_cast<signed char>('r'))));
    CHECK(pyqtgraph::mkColor(static_cast<unsigned char>('r')) ==
          pyqtgraph::intColor(static_cast<int>(static_cast<unsigned char>('r'))));
    CHECK(pyqtgraph::mkColor(static_cast<signed char>(-1)) == pyqtgraph::intColor(-1));
    CHECK(pyqtgraph::mkColor(static_cast<unsigned char>(1)) == pyqtgraph::intColor(1));
    CHECK(pyqtgraph::mkColor(std::int8_t{-1}) == pyqtgraph::intColor(-1));
    CHECK(pyqtgraph::mkColor(std::uint8_t{3}) == pyqtgraph::intColor(3));

    const auto maxIndex = std::numeric_limits<std::uint64_t>::max();
    CHECK(pyqtgraph::mkColor(maxIndex) == pyqtgraph::intColor(static_cast<int>(maxIndex % 9U)));

    const auto minIndex = std::numeric_limits<std::int64_t>::min();
    CHECK(pyqtgraph::mkColor(minIndex) == pyqtgraph::intColor(static_cast<int>(minIndex % 9)));

    CHECK_RGBA(pyqtgraph::mkColor(std::array<int, 2>{2, 9}), 170, 255, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor(std::tuple<int, int>{2, 9}), 170, 255, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor({2.0, 9.0}), 170, 255, 0, 255);

    CHECK_HSVA(pyqtgraph::intColor(1, 4, 1, 255, 150, 0, 10), 7, 255, 255, 255);
    CHECK_HSVA(pyqtgraph::intColor(1, 1, 4, 150, 254, 360, 0), 0, 255, 219, 255);
    return true;
}

bool testCopyAndErrors()
{
    const QColor source(12, 34, 56, 78);
    const QColor copied = pyqtgraph::mkColor(source);
    CHECK_RGBA(copied, 12, 34, 56, 78);
    CHECK(copied == source);

    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkColor("q"); }, "unknown short name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkColor('q'); }, "unknown char short name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkColor("not-a-real-color-name"); }, "invalid color name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkColor("#12"); }, "invalid hex length"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkColor({1.0}); }, "unsupported sequence length 1"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkColor(std::array<int, 5>{1, 2, 3, 4, 5}); },
                                     "unsupported array length 5"));
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testShortNames() && success;
    success = testStringsAndHex() && success;
    success = testNumericInputs() && success;
    success = testIntColorInputs() && success;
    success = testCopyAndErrors() && success;

    return success ? 0 : 1;
}
