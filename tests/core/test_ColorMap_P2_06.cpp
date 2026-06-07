#include "../../include/pyqtgraph/colormap.hpp"
#include "../../include/pyqtgraph/colors/palette.hpp"

#include <QColor>
#include <QGradient>
#include <QLinearGradient>
#include <QPalette>
#include <QString>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using MappingMode = pyqtgraph::ColorMap::MappingMode;
using OutputMode = pyqtgraph::ColorMap::OutputMode;
using LookupTable = pyqtgraph::ColorMap::LookupTable;
using Stops = pyqtgraph::ColorMap::Stops;

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

int stopByteAt(const Stops& stops, std::size_t row, std::size_t channel)
{
    return static_cast<int>(stops.bytes[(row * stops.channels) + channel]);
}

bool fixtureMentionsPinnedReference()
{
#ifdef PYQTGRAPH_CPP_P2_06_FIXTURE
    std::ifstream input(PYQTGRAPH_CPP_P2_06_FIXTURE);
    CHECK(input.good());
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    CHECK(text.find("pyqtgraph-0.14.0") != std::string::npos);
    CHECK(text.find("a20028b98294b9cc8770f2015a92eb342224b788") != std::string::npos);
#endif
    return true;
}

bool testEqualSpacingStopsAndLutMatchOracle()
{
    const pyqtgraph::ColorMap map = pyqtgraph::ColorMap::fromEqualSpacing(
        {QColor(0, 0, 0), QColor(128, 64, 32), QColor(255, 255, 255)},
        QStringLiteral("equal-spacing"));

    CHECK_SIZE(map.positions().size(), std::size_t{3});
    CHECK_NEAR(map.positions()[0], 0.0, 0.0);
    CHECK_NEAR(map.positions()[1], 0.5, 0.0);
    CHECK_NEAR(map.positions()[2], 1.0, 0.0);

    const Stops stops = map.getStops(OutputMode::Byte);
    CHECK_SIZE(stops.rows(), std::size_t{3});
    CHECK_SIZE(stops.channels, std::size_t{4});
    CHECK_INT(stopByteAt(stops, 1, 0), 128);
    CHECK_INT(stopByteAt(stops, 1, 1), 64);
    CHECK_INT(stopByteAt(stops, 1, 2), 32);
    CHECK_INT(stopByteAt(stops, 1, 3), 255);

    const LookupTable byteLut = map.getLookupTable(0.0, 1.0, 5);
    const std::array<std::array<int, 3>, 5> expectedByte{{
        {{0, 0, 0}},
        {{64, 32, 16}},
        {{128, 64, 32}},
        {{191, 159, 143}},
        {{255, 255, 255}},
    }};
    CHECK_SIZE(byteLut.channels, std::size_t{3});
    for (std::size_t row = 0; row < expectedByte.size(); ++row) {
        for (std::size_t channel = 0; channel < expectedByte[row].size(); ++channel) {
            CHECK_INT(byteAt(byteLut, row, channel), expectedByte[row][channel]);
        }
    }

    const LookupTable floatLut = map.getLookupTable(0.0, 1.0, 5, true, OutputMode::Float);
    CHECK_NEAR(floatAt(floatLut, 1, 0), 64.0 / 255.0, 1.0e-6);
    CHECK_NEAR(floatAt(floatLut, 3, 0), 191.5 / 255.0, 1.0e-6);
    CHECK_NEAR(floatAt(floatLut, 4, 3), 1.0, 1.0e-12);

    return true;
}

bool testMapConveniencesAndMappingModes()
{
    pyqtgraph::ColorMap ramp = pyqtgraph::ColorMap::fromEqualSpacing({QColor(0, 0, 0), QColor(255, 255, 255)});

    const auto byte = ramp.mapToByte(0.5);
    CHECK_INT(byte[0], 127);
    CHECK_INT(byte[3], 255);
    const auto rgba = ramp.mapToFloat(0.25);
    CHECK_NEAR(rgba[0], 0.25, 1.0e-12);
    CHECK(ramp.mapToQColor(1.0) == QColor(255, 255, 255));
    CHECK(ramp.getByIndex(0) == QColor(0, 0, 0));

    ramp.setMappingMode(MappingMode::Repeat);
    CHECK_INT(ramp.mapToByte(1.25)[0], 63);
    CHECK_INT(ramp.mapToByte(-0.25)[0], 191);

    ramp.setMappingMode(MappingMode::Mirror);
    CHECK_INT(ramp.mapToByte(-0.25)[0], 63);
    CHECK_INT(ramp.mapToByte(1.25)[0], 255);

    ramp.setMappingMode(MappingMode::Diverging);
    CHECK_INT(ramp.mapToByte(-1.0)[0], 0);
    CHECK_INT(ramp.mapToByte(0.0)[0], 127);
    CHECK_INT(ramp.mapToByte(1.0)[0], 255);

    return true;
}

bool testGradientStopsAndSpreadMatchOracle()
{
    pyqtgraph::ColorMap map = pyqtgraph::ColorMap::fromEqualSpacing({QColor(0, 0, 0), QColor(255, 255, 255)});
    const QLinearGradient normal = map.getGradient();
    CHECK_SIZE(static_cast<std::size_t>(normal.stops().size()), std::size_t{2});
    CHECK_NEAR(normal.stops()[0].first, 0.0, 0.0);
    CHECK_NEAR(normal.stops()[1].first, 1.0, 0.0);
    CHECK(normal.spread() == QGradient::PadSpread);

    map.setMappingMode(MappingMode::Mirror);
    const QLinearGradient mirror = map.getGradient();
    CHECK_SIZE(static_cast<std::size_t>(mirror.stops().size()), std::size_t{3});
    CHECK_NEAR(mirror.stops()[0].first, 0.0, 0.0);
    CHECK(mirror.stops()[0].second == QColor(255, 255, 255));
    CHECK_NEAR(mirror.stops()[1].first, 0.5, 0.0);
    CHECK(mirror.stops()[1].second == QColor(0, 0, 0));
    CHECK_NEAR(mirror.stops()[2].first, 1.0, 0.0);
    CHECK(mirror.stops()[2].second == QColor(255, 255, 255));

    map.setMappingMode(MappingMode::Repeat);
    const QLinearGradient repeat = map.getGradient();
    CHECK(repeat.spread() == QGradient::RepeatSpread);

    return true;
}

bool testPaletteLookupAndQPalettes()
{
    const std::vector<QString> names = pyqtgraph::listMaps();
    CHECK(std::find(names.begin(), names.end(), QStringLiteral("PAL-relaxed")) != names.end());
    CHECK(std::find(names.begin(), names.end(), QStringLiteral("PAL-relaxed_bright")) != names.end());

    const auto relaxed = pyqtgraph::get(QStringLiteral("PAL-relaxed"));
    CHECK(relaxed.has_value());
    CHECK(relaxed->name() == QStringLiteral("PAL-relaxed"));
    CHECK_SIZE(relaxed->size(), std::size_t{10});
    CHECK_NEAR(relaxed->positions()[1], 1.0 / 9.0, 1.0e-12);
    CHECK(relaxed->getByIndex(0) == QColor(QStringLiteral("#f97f10")));
    CHECK(relaxed->getByIndex(5) == QColor(QStringLiteral("#0e56c2")));
    CHECK(relaxed->getByIndex(6) == QColor(QStringLiteral("#813be3")));
    CHECK(relaxed->getByIndex(7) == QColor(QStringLiteral("#c01188")));
    CHECK(relaxed->getByIndex(9) == QColor(QStringLiteral("#f97f10")));

    const auto relaxedBright = pyqtgraph::get(QStringLiteral("PAL-relaxed_bright"));
    CHECK(relaxedBright.has_value());
    CHECK(relaxedBright->getByIndex(5) == QColor(QStringLiteral("#1f78ff")));
    CHECK(relaxedBright->getByIndex(6) == QColor(QStringLiteral("#a54dff")));

    CHECK(!pyqtgraph::get(QStringLiteral("missing-map")).has_value());
    CHECK(pyqtgraph::listMaps(QStringLiteral("matplotlib")).empty());

    const QPalette dark = pyqtgraph::colors::getQDarkStyleDarkQPalette();
    CHECK(dark.color(QPalette::Active, QPalette::Base) == QColor(QStringLiteral("#19232D")));
    CHECK(dark.color(QPalette::Active, QPalette::ButtonText) == QColor(QStringLiteral("#F0F0F0")));
    CHECK(dark.color(QPalette::Disabled, QPalette::Text) == QColor(QStringLiteral("#9DA9B5")));

    const QPalette light = pyqtgraph::colors::getQDarkStyleLightQPalette();
    CHECK(light.color(QPalette::Active, QPalette::Base) == QColor(QStringLiteral("#FAFAFA")));
    CHECK(light.color(QPalette::Active, QPalette::Highlight) == QColor(QStringLiteral("#9FCBFF")));
    CHECK(light.color(QPalette::Disabled, QPalette::HighlightedText) == QColor(QStringLiteral("#293544")));

    return true;
}

bool writeSwatchArtifact()
{
#ifdef PYQTGRAPH_CPP_P2_06_ARTIFACT_DIR
    const auto relaxed = pyqtgraph::get(QStringLiteral("PAL-relaxed"));
    CHECK(relaxed.has_value());
    const LookupTable lut = relaxed->getLookupTable(0.0, 1.0, 90);

    const std::filesystem::path dir = PYQTGRAPH_CPP_P2_06_ARTIFACT_DIR;
    std::filesystem::create_directories(dir);
    const std::filesystem::path ppm = dir / "PAL-relaxed-swatch.ppm";
    std::ofstream out(ppm);
    CHECK(out.good());
    out << "P3\n90 12\n255\n";
    for (int y = 0; y < 12; ++y) {
        for (std::size_t x = 0; x < lut.rows(); ++x) {
            out << byteAt(lut, x, 0) << ' ' << byteAt(lut, x, 1) << ' ' << byteAt(lut, x, 2) << ' ';
        }
        out << '\n';
    }
    std::cout << "P2.06 swatch artifact: " << ppm << '\n';
#endif
    return true;
}

} // namespace

int main()
{
    const std::array tests{
        std::pair<std::string_view, bool (*)()>{"fixtureMentionsPinnedReference", fixtureMentionsPinnedReference},
        std::pair<std::string_view, bool (*)()>{"testEqualSpacingStopsAndLutMatchOracle", testEqualSpacingStopsAndLutMatchOracle},
        std::pair<std::string_view, bool (*)()>{"testMapConveniencesAndMappingModes", testMapConveniencesAndMappingModes},
        std::pair<std::string_view, bool (*)()>{"testGradientStopsAndSpreadMatchOracle", testGradientStopsAndSpreadMatchOracle},
        std::pair<std::string_view, bool (*)()>{"testPaletteLookupAndQPalettes", testPaletteLookupAndQPalettes},
        std::pair<std::string_view, bool (*)()>{"writeSwatchArtifact", writeSwatchArtifact},
    };

    bool success = true;
    for (const auto& [name, test] : tests) {
        try {
            success = test() && success;
        } catch (const std::exception& error) {
            std::cerr << name << ": unexpected exception: " << error.what() << '\n';
            success = false;
        }
    }
    return success ? 0 : 1;
}
