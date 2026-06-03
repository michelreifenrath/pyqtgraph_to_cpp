#include "../../include/pyqtgraph/functions.hpp"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QString>

#include <array>
#include <cmath>
#include <exception>
#include <iostream>
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

bool checkPen(const QPen& pen,
              int red,
              int green,
              int blue,
              int alpha,
              double width,
              Qt::PenStyle style,
              bool cosmetic,
              std::string_view label)
{
    if (!checkRgba(pen.color(), red, green, blue, alpha, label)) {
        return false;
    }
    if (pen.widthF() != width || pen.style() != style || pen.isCosmetic() != cosmetic) {
        std::cerr << label << ": expected width/style/cosmetic " << width << '/' << static_cast<int>(style) << '/'
                  << cosmetic << " got " << pen.widthF() << '/' << static_cast<int>(pen.style()) << '/'
                  << pen.isCosmetic() << '\n';
        return false;
    }
    return true;
}

#define CHECK_PEN(pen, red, green, blue, alpha, width, style, cosmetic) \
    do { \
        if (!checkPen((pen), (red), (green), (blue), (alpha), (width), (style), (cosmetic), #pen)) { \
            return false; \
        } \
    } while (false)

bool testUpstreamMkColorOracleCases()
{
    // Representative cases from pinned pyqtgraph-0.14.0 tests/test_functions.py::test_mkColor.
    CHECK_RGBA(pyqtgraph::mkColor("r"), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("g"), 0, 255, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("b"), 0, 0, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor("c"), 0, 255, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor("m"), 255, 0, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor("y"), 255, 255, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("k"), 0, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("w"), 255, 255, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor("d"), 150, 150, 150, 255);
    CHECK_RGBA(pyqtgraph::mkColor("l"), 200, 200, 200, 255);
    CHECK_RGBA(pyqtgraph::mkColor("s"), 100, 100, 150, 255);
    CHECK_RGBA(pyqtgraph::mkColor(0.75), 191, 191, 191, 255);
    CHECK_RGBA(pyqtgraph::mkColor(11.0, 22.0, 33.0), 11, 22, 33, 255);
    CHECK_RGBA(pyqtgraph::mkColor(11.0, 22.0, 33.0, 44.0), 11, 22, 33, 44);
    CHECK_RGBA(pyqtgraph::mkColor(std::array<int, 2>{0, 2}), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor(std::array<int, 2>{1, 2}), 0, 255, 255, 255);
    CHECK_RGBA(pyqtgraph::mkColor(std::array<int, 2>{2, 2}), 255, 0, 0, 255);
    CHECK_RGBA(pyqtgraph::mkColor("#89a"), 136, 153, 170, 255);
    CHECK_RGBA(pyqtgraph::mkColor("#89ab"), 136, 153, 170, 187);
    CHECK_RGBA(pyqtgraph::mkColor("#4488cc"), 68, 136, 204, 255);
    CHECK_RGBA(pyqtgraph::mkColor("#4488cc00"), 68, 136, 204, 0);
    CHECK_RGBA(pyqtgraph::mkColor(QColor(1, 2, 3, 4)), 1, 2, 3, 4);
    CHECK_RGBA(pyqtgraph::mkColor("steelblue"), 70, 130, 180, 255);
    CHECK_RGBA(pyqtgraph::mkColor("lawngreen"), 124, 252, 0, 255);
    return true;
}

bool testColorHelpers()
{
    const QColor source(1, 2, 3, 4);
    CHECK(pyqtgraph::colorTuple(source) == (std::array<int, 4>{1, 2, 3, 4}));
    CHECK(pyqtgraph::colorStr(source) == QString("01020304"));

    const auto gl = pyqtgraph::glColor(source);
    CHECK(std::abs(gl[0] - source.redF()) < 1.0e-12);
    CHECK(std::abs(gl[1] - source.greenF()) < 1.0e-12);
    CHECK(std::abs(gl[2] - source.blueF()) < 1.0e-12);
    CHECK(std::abs(gl[3] - source.alphaF()) < 1.0e-12);

    CHECK_RGBA(pyqtgraph::hsvColor(0.5, 1.0, 1.0, 0.5), 0, 255, 255, 128);

    const pyqtgraph::Color named("r");
    CHECK_RGBA(named, 255, 0, 0, 255);
    const auto namedGl = named.glColor();
    CHECK(std::abs(namedGl[0] - 1.0) < 1.0e-12);
    CHECK(std::abs(namedGl[1]) < 1.0e-12);
    CHECK(std::abs(namedGl[2]) < 1.0e-12);
    CHECK(std::abs(namedGl[3] - 1.0) < 1.0e-12);
    return true;
}

bool testPenOptionsAndBrushEdges()
{
    pyqtgraph::PenOptions hsvOptions;
    hsvOptions.hsv = std::array<double, 4>{0.5, 1.0, 1.0, 0.5};
    hsvOptions.width = 5.0;
    hsvOptions.style = Qt::DashLine;
    hsvOptions.dash = {1.0, 2.0, 3.0, 4.0};
    hsvOptions.hasDash = true;
    hsvOptions.cosmetic = false;

    const QPen hsvPen = pyqtgraph::mkPen(hsvOptions);
    CHECK_RGBA(hsvPen.color(), 0, 255, 255, 128);
    CHECK(hsvPen.widthF() == 5.0);
    CHECK(hsvPen.style() == Qt::CustomDashLine);
    CHECK(!hsvPen.isCosmetic());
    CHECK(hsvPen.capStyle() == Qt::RoundCap);
    CHECK(hsvPen.dashPattern().size() == 4);
    CHECK(hsvPen.dashPattern()[0] == 1.0);
    CHECK(hsvPen.dashPattern()[1] == 2.0);
    CHECK(hsvPen.dashPattern()[2] == 3.0);
    CHECK(hsvPen.dashPattern()[3] == 4.0);

    pyqtgraph::PenOptions colorOptions;
    colorOptions.color = pyqtgraph::mkColor("#ff0");
    colorOptions.width = 2.0;
    colorOptions.style = Qt::DotLine;
    CHECK_PEN(pyqtgraph::mkPen(colorOptions), 255, 255, 0, 255, 2.0, Qt::DotLine, true);

    CHECK(pyqtgraph::mkPen(nullptr).style() == Qt::NoPen);
    CHECK(pyqtgraph::mkBrush(nullptr).style() == Qt::NoBrush);
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testUpstreamMkColorOracleCases() && success;
    success = testColorHelpers() && success;
    success = testPenOptionsAndBrushEdges() && success;
    return success ? 0 : 1;
}
