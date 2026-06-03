#include "../../include/pyqtgraph/functions.hpp"

#include <QBrush>
#include <QColor>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>

#include <array>
#include <cmath>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
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

bool almostEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-12;
}

bool checkRect(const QRectF& rect,
               double left,
               double top,
               double width,
               double height,
               std::string_view label)
{
    if (!almostEqual(rect.left(), left) || !almostEqual(rect.top(), top) || !almostEqual(rect.width(), width) ||
        !almostEqual(rect.height(), height)) {
        std::cerr << label << ": expected rect(" << left << ", " << top << ", " << width << ", " << height
                  << ") got rect(" << rect.left() << ", " << rect.top() << ", " << rect.width() << ", "
                  << rect.height() << ")\n";
        return false;
    }
    return true;
}

#define CHECK_RECT(rect, left, top, width, height) \
    do { \
        if (!checkRect((rect), (left), (top), (width), (height), #rect)) { \
            return false; \
        } \
    } while (false)

bool checkPathElement(const QPainterPath& path, int index, double x, double y, std::string_view label)
{
    if (path.elementCount() <= index) {
        std::cerr << label << ": expected at least " << (index + 1) << " path elements, got " << path.elementCount()
                  << '\n';
        return false;
    }
    const auto element = path.elementAt(index);
    if (!almostEqual(element.x, x) || !almostEqual(element.y, y)) {
        std::cerr << label << '[' << index << "]: expected point(" << x << ", " << y << ") got point("
                  << element.x << ", " << element.y << ")\n";
        return false;
    }
    return true;
}

#define CHECK_PATH_ELEMENT(path, index, x, y) \
    do { \
        if (!checkPathElement((path), (index), (x), (y), #path)) { \
            return false; \
        } \
    } while (false)

bool checkPathPrefix(const QPainterPath& path, std::initializer_list<QPointF> points, std::string_view label)
{
    int index = 0;
    for (const QPointF& point : points) {
        if (!checkPathElement(path, index, point.x(), point.y(), label)) {
            return false;
        }
        ++index;
    }
    return true;
}

#define CHECK_PATH_PREFIX(path, ...) \
    do { \
        if (!checkPathPrefix((path), {__VA_ARGS__}, #path)) { \
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

bool checkBrush(const QBrush& brush,
                int red,
                int green,
                int blue,
                int alpha,
                Qt::BrushStyle style,
                std::string_view label)
{
    if (!checkRgba(brush.color(), red, green, blue, alpha, label)) {
        return false;
    }
    if (brush.style() != style) {
        std::cerr << label << ": expected brush style " << static_cast<int>(style) << " got "
                  << static_cast<int>(brush.style()) << '\n';
        return false;
    }
    return true;
}

#define CHECK_BRUSH(brush, red, green, blue, alpha, style) \
    do { \
        if (!checkBrush((brush), (red), (green), (blue), (alpha), (style), #brush)) { \
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
    CHECK_BRUSH(pyqtgraph::mkBrush("b", Qt::Dense4Pattern), 0, 0, 255, 255, Qt::Dense4Pattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(QColor(1, 2, 3, 4)), 1, 2, 3, 4, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(std::array<int, 4>{11, 12, 13, 14}), 11, 12, 13, 14, Qt::SolidPattern);
    return true;
}

bool testSymbolBehavior()
{
    // Complete symbol oracle from pinned pyqtgraph-0.14.0 commit
    // a20028b98294b9cc8770f2015a92eb342224b788,
    // pyqtgraph/graphicsItems/ScatterPlotItem.py name_list and coords. Paths are
    // normalized to the upstream unit-size coordinate contract centered on (0, 0).
    const auto& symbols = pyqtgraph::symbolPaths();
    constexpr std::array<std::string_view, 19> upstreamSymbols = {"o",
                                                                 "s",
                                                                 "t",
                                                                 "t1",
                                                                 "t2",
                                                                 "t3",
                                                                 "d",
                                                                 "+",
                                                                 "x",
                                                                 "p",
                                                                 "h",
                                                                 "star",
                                                                 "|",
                                                                 "_",
                                                                 "arrow_up",
                                                                 "arrow_right",
                                                                 "arrow_down",
                                                                 "arrow_left",
                                                                 "crosshair"};
    CHECK(symbols.size() == upstreamSymbols.size());
    for (const std::string_view symbol : upstreamSymbols) {
        CHECK(symbols.find(QString::fromUtf8(symbol.data(), static_cast<qsizetype>(symbol.size()))) != symbols.end());
        CHECK(!pyqtgraph::symbolPath(symbol).isEmpty());
    }

    const QPainterPath circle = pyqtgraph::symbolPath("o");
    CHECK_RECT(circle.boundingRect(), -0.5, -0.5, 1.0, 1.0);
    CHECK(circle.contains(QPointF(0.0, 0.0)));
    CHECK(!circle.contains(QPointF(0.6, 0.0)));

    const QPainterPath square = pyqtgraph::symbolPath(QString("s"));
    CHECK_RECT(square.boundingRect(), -0.5, -0.5, 1.0, 1.0);
    CHECK(square.contains(QPointF(0.25, 0.25)));

    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("t"), QPointF(-0.5, -0.5), QPointF(0.0, 0.5), QPointF(0.5, -0.5));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("t1"), QPointF(-0.5, 0.5), QPointF(0.0, -0.5), QPointF(0.5, 0.5));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("t2"), QPointF(-0.5, -0.5), QPointF(-0.5, 0.5), QPointF(0.5, 0.0));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("t3"), QPointF(0.5, 0.5), QPointF(0.5, -0.5), QPointF(-0.5, 0.0));

    const QPainterPath diamond = pyqtgraph::symbolPath("d");
    CHECK_RECT(diamond.boundingRect(), -0.4, -0.5, 0.8, 1.0);
    CHECK_PATH_PREFIX(diamond, QPointF(0.0, -0.5), QPointF(-0.4, 0.0), QPointF(0.0, 0.5), QPointF(0.4, 0.0));

    const QPainterPath plus = pyqtgraph::symbolPath("+");
    CHECK_RECT(plus.boundingRect(), -0.5, -0.5, 1.0, 1.0);
    CHECK_PATH_PREFIX(plus,
                      QPointF(-0.5, -0.1),
                      QPointF(-0.5, 0.1),
                      QPointF(-0.1, 0.1),
                      QPointF(-0.1, 0.5),
                      QPointF(0.1, 0.5),
                      QPointF(0.1, 0.1),
                      QPointF(0.5, 0.1),
                      QPointF(0.5, -0.1),
                      QPointF(0.1, -0.1),
                      QPointF(0.1, -0.5),
                      QPointF(-0.1, -0.5),
                      QPointF(-0.1, -0.1));
    CHECK(plus.contains(QPointF(0.0, 0.45)));
    CHECK(plus.contains(QPointF(0.45, 0.0)));
    CHECK(!plus.contains(QPointF(0.45, 0.45)));

    const QPainterPath cross = pyqtgraph::symbolPath("x");
    CHECK(cross.contains(QPointF(0.0, 0.0)));
    const double crossExtent = 0.6 / std::sqrt(2.0);
    CHECK_RECT(cross.boundingRect(), -crossExtent, -crossExtent, 2.0 * crossExtent, 2.0 * crossExtent);

    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("p"),
                      QPointF(0.0, -0.5),
                      QPointF(-0.4755, -0.1545),
                      QPointF(-0.2939, 0.4045),
                      QPointF(0.2939, 0.4045),
                      QPointF(0.4755, -0.1545));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("h"),
                      QPointF(0.433, 0.25),
                      QPointF(0.0, 0.5),
                      QPointF(-0.433, 0.25),
                      QPointF(-0.433, -0.25),
                      QPointF(0.0, -0.5),
                      QPointF(0.433, -0.25));

    const QPainterPath star = pyqtgraph::symbolPath(std::string_view("star"));
    CHECK_PATH_PREFIX(star,
                      QPointF(0.0, -0.5),
                      QPointF(-0.1123, -0.1545),
                      QPointF(-0.4755, -0.1545),
                      QPointF(-0.1816, 0.059),
                      QPointF(-0.2939, 0.4045),
                      QPointF(0.0, 0.1910),
                      QPointF(0.2939, 0.4045),
                      QPointF(0.1816, 0.059),
                      QPointF(0.4755, -0.1545),
                      QPointF(0.1123, -0.1545));
    CHECK(star.contains(QPointF(0.0, 0.0)));

    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("|"), QPointF(-0.1, 0.5), QPointF(0.1, 0.5), QPointF(0.1, -0.5), QPointF(-0.1, -0.5));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("_"), QPointF(-0.5, -0.1), QPointF(-0.5, 0.1), QPointF(0.5, 0.1), QPointF(0.5, -0.1));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("arrow_up"),
                      QPointF(-0.125, 0.125),
                      QPointF(0.0, 0.0),
                      QPointF(0.125, 0.125),
                      QPointF(0.05, 0.125),
                      QPointF(0.05, 0.5),
                      QPointF(-0.05, 0.5),
                      QPointF(-0.05, 0.125));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("arrow_right"),
                      QPointF(-0.125, -0.125),
                      QPointF(0.0, 0.0),
                      QPointF(-0.125, 0.125),
                      QPointF(-0.125, 0.05),
                      QPointF(-0.5, 0.05),
                      QPointF(-0.5, -0.05),
                      QPointF(-0.125, -0.05));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("arrow_down"),
                      QPointF(0.125, -0.125),
                      QPointF(0.0, 0.0),
                      QPointF(-0.125, -0.125),
                      QPointF(-0.05, -0.125),
                      QPointF(-0.05, -0.5),
                      QPointF(0.05, -0.5),
                      QPointF(0.05, -0.125));
    CHECK_PATH_PREFIX(pyqtgraph::symbolPath("arrow_left"),
                      QPointF(0.125, 0.125),
                      QPointF(0.0, 0.0),
                      QPointF(0.125, -0.125),
                      QPointF(0.125, -0.05),
                      QPointF(0.5, -0.05),
                      QPointF(0.5, 0.05),
                      QPointF(0.125, 0.05));

    const QPainterPath crosshair = pyqtgraph::symbolPath("crosshair");
    CHECK_RECT(crosshair.boundingRect(), -1.0, -1.0, 2.0, 2.0);
    CHECK(crosshair.elementCount() >= 17);
    CHECK_PATH_ELEMENT(crosshair, crosshair.elementCount() - 4, -1.0, 0.0);
    CHECK_PATH_ELEMENT(crosshair, crosshair.elementCount() - 3, 1.0, 0.0);
    CHECK_PATH_ELEMENT(crosshair, crosshair.elementCount() - 2, 0.0, -1.0);
    CHECK_PATH_ELEMENT(crosshair, crosshair.elementCount() - 1, 0.0, 1.0);

    bool rejectedUnknown = false;
    try {
        (void)pyqtgraph::symbolPath("not-a-symbol");
    } catch (const std::invalid_argument&) {
        rejectedUnknown = true;
    }
    CHECK(rejectedUnknown);

    bool rejectedNull = false;
    try {
        (void)pyqtgraph::symbolPath(nullptr);
    } catch (const std::invalid_argument&) {
        rejectedNull = true;
    }
    CHECK(rejectedNull);
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testUpstreamMkColorOracleCases() && success;
    success = testColorHelpers() && success;
    success = testPenOptionsAndBrushEdges() && success;
    success = testSymbolBehavior() && success;
    return success ? 0 : 1;
}
