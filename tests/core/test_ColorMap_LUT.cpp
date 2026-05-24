#include "../../include/pyqtgraph/colormap.hpp"

#include <QColor>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using MappingMode = pyqtgraph::ColorMap::MappingMode;
using OutputMode = pyqtgraph::ColorMap::OutputMode;
using LookupTable = pyqtgraph::ColorMap::LookupTable;

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

bool checkSize(std::size_t actual, std::size_t expected, std::string_view label)
{
    if (actual != expected) {
        std::cerr << label << ": expected " << expected << ", got " << actual << '\n';
        return false;
    }
    return true;
}

#define CHECK_SIZE(actual, expected) \
    do { \
        if (!checkSize((actual), (expected), #actual)) { \
            return false; \
        } \
    } while (false)

bool checkInt(int actual, int expected, std::string_view label)
{
    if (actual != expected) {
        std::cerr << label << ": expected " << expected << ", got " << actual << '\n';
        return false;
    }
    return true;
}

#define CHECK_INT(actual, expected) \
    do { \
        if (!checkInt((actual), (expected), #actual)) { \
            return false; \
        } \
    } while (false)

bool checkNear(double actual, double expected, double tolerance, std::string_view label)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << label << ": expected " << expected << ", got " << actual << '\n';
        return false;
    }
    return true;
}

#define CHECK_NEAR(actual, expected, tolerance) \
    do { \
        if (!checkNear((actual), (expected), (tolerance), #actual)) { \
            return false; \
        } \
    } while (false)

int byteAt(const LookupTable& table, std::size_t row, std::size_t channel)
{
    return static_cast<int>(table.bytes[(row * table.channels) + channel]);
}

double floatAt(const LookupTable& table, std::size_t row, std::size_t channel)
{
    return table.floats[(row * table.channels) + channel];
}

bool testDefaultByteLookupTableDropsOpaqueAlpha()
{
    const pyqtgraph::ColorMap map({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 128, 64)});

    const LookupTable table = map.getLookupTable(0.0, 1.0, 3);

    CHECK(table.mode == OutputMode::Byte);
    CHECK_SIZE(table.channels, std::size_t{3});
    CHECK_SIZE(table.rows(), std::size_t{3});
    CHECK_SIZE(table.bytes.size(), std::size_t{9});
    CHECK_INT(byteAt(table, 0, 0), 0);
    CHECK_INT(byteAt(table, 0, 1), 0);
    CHECK_INT(byteAt(table, 0, 2), 0);
    CHECK_INT(byteAt(table, 1, 0), 127);
    CHECK_INT(byteAt(table, 1, 1), 64);
    CHECK_INT(byteAt(table, 1, 2), 32);
    CHECK_INT(byteAt(table, 2, 0), 255);
    CHECK_INT(byteAt(table, 2, 1), 128);
    CHECK_INT(byteAt(table, 2, 2), 64);

    return true;
}

bool testAlphaPolicyMatchesPyQtGraphLookupTables()
{
    const pyqtgraph::ColorMap transparentMap(
        {0.0, 1.0},
        {QColor(0, 0, 0, 255), QColor(255, 0, 0, 128)});

    CHECK(transparentMap.usesAlpha());

    const LookupTable automaticAlpha = transparentMap.getLookupTable(0.0, 1.0, 3);
    CHECK_SIZE(automaticAlpha.channels, std::size_t{4});
    CHECK_INT(byteAt(automaticAlpha, 0, 3), 255);
    CHECK_INT(byteAt(automaticAlpha, 1, 3), 191);
    CHECK_INT(byteAt(automaticAlpha, 2, 3), 128);

    const LookupTable forcedRgb = transparentMap.getLookupTable(0.0, 1.0, 3, false);
    CHECK_SIZE(forcedRgb.channels, std::size_t{3});
    CHECK_SIZE(forcedRgb.bytes.size(), std::size_t{9});

    const pyqtgraph::ColorMap opaqueMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
    CHECK(!opaqueMap.usesAlpha());

    const LookupTable forcedRgba = opaqueMap.getLookupTable(0.0, 1.0, 2, true);
    CHECK_SIZE(forcedRgba.channels, std::size_t{4});
    CHECK_INT(byteAt(forcedRgba, 0, 3), 255);
    CHECK_INT(byteAt(forcedRgba, 1, 3), 255);

    return true;
}

bool testFloatAndQColorModesUseNormalizedChannels()
{
    const pyqtgraph::ColorMap map({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});

    const LookupTable floatTable = map.getLookupTable(0.25, 0.75, 2, true, OutputMode::Float);
    CHECK(floatTable.mode == OutputMode::Float);
    CHECK_SIZE(floatTable.channels, std::size_t{4});
    CHECK_SIZE(floatTable.rows(), std::size_t{2});
    CHECK_NEAR(floatAt(floatTable, 0, 0), 0.25, 1.0e-12);
    CHECK_NEAR(floatAt(floatTable, 0, 3), 1.0, 1.0e-12);
    CHECK_NEAR(floatAt(floatTable, 1, 0), 0.75, 1.0e-12);
    CHECK_NEAR(floatAt(floatTable, 1, 3), 1.0, 1.0e-12);

    const LookupTable qcolorTable = map.getLookupTable(0.0, 1.0, 3, false, OutputMode::QColor);
    CHECK(qcolorTable.mode == OutputMode::QColor);
    CHECK_SIZE(qcolorTable.channels, std::size_t{4});
    CHECK_SIZE(qcolorTable.rows(), std::size_t{3});
    CHECK_NEAR(qcolorTable.colors[1].redF(), 0.5, 1.0 / 255.0);
    CHECK_NEAR(qcolorTable.colors[1].greenF(), 0.5, 1.0 / 255.0);
    CHECK_NEAR(qcolorTable.colors[1].blueF(), 0.5, 1.0 / 255.0);
    CHECK_NEAR(qcolorTable.colors[1].alphaF(), 1.0, 1.0e-12);

    return true;
}

bool testClippingAndNonDefaultSampling()
{
    const pyqtgraph::ColorMap map({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});

    const LookupTable clipped = map.getLookupTable(-1.0, 2.0, 4);
    CHECK_INT(byteAt(clipped, 0, 0), 0);
    CHECK_INT(byteAt(clipped, 1, 0), 0);
    CHECK_INT(byteAt(clipped, 2, 0), 255);
    CHECK_INT(byteAt(clipped, 3, 0), 255);

    const LookupTable partial = map.getLookupTable(0.25, 0.75, 2);
    CHECK_INT(byteAt(partial, 0, 0), 63);
    CHECK_INT(byteAt(partial, 1, 0), 191);

    return true;
}

bool testStopsAreSortedBeforeInterpolation()
{
    const pyqtgraph::ColorMap map({1.0, 0.0}, {QColor(255, 255, 255), QColor(0, 0, 0)});

    CHECK_SIZE(map.positions().size(), std::size_t{2});
    CHECK_NEAR(map.positions()[0], 0.0, 0.0);
    CHECK_NEAR(map.positions()[1], 1.0, 0.0);

    const LookupTable table = map.getLookupTable(0.0, 1.0, 2);
    CHECK_INT(byteAt(table, 0, 0), 0);
    CHECK_INT(byteAt(table, 1, 0), 255);

    return true;
}

bool testRepeatMappingWrapsSamplesBeforeInterpolation()
{
    pyqtgraph::ColorMap map({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
    map.setMappingMode(MappingMode::Repeat);

    const LookupTable table = map.getLookupTable(0.75, 1.25, 3);

    CHECK(map.mappingMode() == MappingMode::Repeat);
    CHECK_INT(byteAt(table, 0, 0), 191);
    CHECK_INT(byteAt(table, 1, 0), 0);
    CHECK_INT(byteAt(table, 2, 0), 63);

    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testDefaultByteLookupTableDropsOpaqueAlpha() && success;
    success = testAlphaPolicyMatchesPyQtGraphLookupTables() && success;
    success = testFloatAndQColorModesUseNormalizedChannels() && success;
    success = testClippingAndNonDefaultSampling() && success;
    success = testStopsAreSortedBeforeInterpolation() && success;
    success = testRepeatMappingWrapsSamplesBeforeInterpolation() && success;

    return success ? 0 : 1;
}
