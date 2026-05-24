#include "../../include/pyqtgraph/functions.hpp"

#include <QBrush>
#include <QColor>
#include <QPen>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
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
        std::cerr << label << ": expected style " << static_cast<int>(style) << " got "
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

bool testPenDefaultsCopiesAndAttributes()
{
    CHECK_PEN(pyqtgraph::mkPen(), 200, 200, 200, 255, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(nullptr), 200, 200, 200, 255, 1.0, Qt::NoPen, true);
    CHECK_PEN(pyqtgraph::mkPen(nullptr, 2.5, Qt::DashLine, false), 200, 200, 200, 255, 2.5, Qt::NoPen, false);

    QPen source(QColor(12, 34, 56, 78));
    source.setWidthF(2.5);
    source.setStyle(Qt::DashLine);
    source.setCosmetic(false);
    const QPen copied = pyqtgraph::mkPen(source);
    CHECK(copied == source);
    source.setColor(QColor(1, 2, 3, 4));
    CHECK_RGBA(copied.color(), 12, 34, 56, 78);

    CHECK_PEN(pyqtgraph::mkPen("r", 2.5, Qt::DashLine, false), 255, 0, 0, 255, 2.5, Qt::DashLine, false);
    CHECK(pyqtgraph::mkPen("r", 4.0).capStyle() == Qt::SquareCap);
    CHECK(pyqtgraph::mkPen("r", 5.0).capStyle() == Qt::RoundCap);
    return true;
}

bool testPenColorDelegation()
{
    CHECK_PEN(pyqtgraph::mkPen(QColor(1, 2, 3, 4)), 1, 2, 3, 4, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(QString("steelblue")), 70, 130, 180, 255, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(std::string_view("#33669980")), 51, 102, 153, 128, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen('g'), 0, 255, 0, 255, 1.0, Qt::SolidLine, true);
    CHECK(pyqtgraph::mkPen(static_cast<signed char>(-1)).color() == pyqtgraph::intColor(-1));
    CHECK(pyqtgraph::mkPen(static_cast<unsigned char>(1)).color() == pyqtgraph::intColor(1));
    CHECK(pyqtgraph::mkPen(0).color() == pyqtgraph::intColor(0));
    CHECK(pyqtgraph::mkPen(std::uint32_t{3}).color() == pyqtgraph::intColor(3));
    CHECK_PEN(pyqtgraph::mkPen(0.5), 127, 127, 127, 255, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(1.0, 2.0, 3.0), 1, 2, 3, 255, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(1, 2, 3), 1, 2, 3, 255, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(1, 2, 3, 4), 1, 2, 3, 4, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(4.0, 5.0, 6.0, 7.0, 2.0, Qt::DotLine, false),
              4,
              5,
              6,
              7,
              2.0,
              Qt::DotLine,
              false);
    CHECK_PEN(pyqtgraph::mkPen({8.0, 9.0, 10.0}), 8, 9, 10, 255, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(std::array<int, 4>{11, 12, 13, 14}), 11, 12, 13, 14, 1.0, Qt::SolidLine, true);
    CHECK_PEN(pyqtgraph::mkPen(std::tuple<int, int, int>{15, 16, 17}), 15, 16, 17, 255, 1.0, Qt::SolidLine, true);
    return true;
}

bool testBrushCopiesStylesAndDelegation()
{
    CHECK(pyqtgraph::mkBrush(nullptr).style() == Qt::NoBrush);

    QBrush source(QColor(12, 34, 56, 78), Qt::Dense2Pattern);
    const QBrush copied = pyqtgraph::mkBrush(source);
    CHECK(copied == source);
    source.setColor(QColor(1, 2, 3, 4));
    CHECK_RGBA(copied.color(), 12, 34, 56, 78);

    CHECK_BRUSH(pyqtgraph::mkBrush("b", Qt::Dense4Pattern), 0, 0, 255, 255, Qt::Dense4Pattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(QColor(1, 2, 3, 4)), 1, 2, 3, 4, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(QString("steelblue")), 70, 130, 180, 255, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(std::string_view("#33669980")), 51, 102, 153, 128, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush('g'), 0, 255, 0, 255, Qt::SolidPattern);
    CHECK(pyqtgraph::mkBrush(static_cast<signed char>(-1)).color() == pyqtgraph::intColor(-1));
    CHECK(pyqtgraph::mkBrush(static_cast<unsigned char>(1)).color() == pyqtgraph::intColor(1));
    CHECK(pyqtgraph::mkBrush(0).color() == pyqtgraph::intColor(0));
    CHECK(pyqtgraph::mkBrush(std::uint32_t{3}).color() == pyqtgraph::intColor(3));
    CHECK_BRUSH(pyqtgraph::mkBrush(0.5), 127, 127, 127, 255, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(1.0, 2.0, 3.0), 1, 2, 3, 255, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(4.0, 5.0, 6.0, 7.0, Qt::Dense5Pattern), 4, 5, 6, 7, Qt::Dense5Pattern);
    CHECK_BRUSH(pyqtgraph::mkBrush({8.0, 9.0, 10.0}), 8, 9, 10, 255, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(std::array<int, 4>{11, 12, 13, 14}), 11, 12, 13, 14, Qt::SolidPattern);
    CHECK_BRUSH(pyqtgraph::mkBrush(std::tuple<int, int, int>{15, 16, 17}), 15, 16, 17, 255, Qt::SolidPattern);
    return true;
}

bool testInvalidInputs()
{
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen("q"); }, "pen unknown short name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen('q'); }, "pen unknown char short name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen("not-a-real-color-name"); }, "pen invalid color name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen("#12"); }, "pen invalid hex length"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen({1.0}); }, "pen unsupported sequence length 1"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen(std::array<int, 5>{1, 2, 3, 4, 5}); },
                                     "pen unsupported array length 5"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkPen(static_cast<const char*>(nullptr)); },
                                     "pen null const char pointer"));

    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush("q"); }, "brush unknown short name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush('q'); }, "brush unknown char short name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush("not-a-real-color-name"); },
                                     "brush invalid color name"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush("#12"); }, "brush invalid hex length"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush({1.0}); }, "brush unsupported sequence length 1"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush(std::array<int, 5>{1, 2, 3, 4, 5}); },
                                     "brush unsupported array length 5"));
    CHECK(checkThrowsInvalidArgument([] { (void)pyqtgraph::mkBrush(static_cast<const char*>(nullptr)); },
                                     "brush null const char pointer"));
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testPenDefaultsCopiesAndAttributes() && success;
    success = testPenColorDelegation() && success;
    success = testBrushCopiesStylesAndDelegation() && success;
    success = testInvalidInputs() && success;
    return success ? 0 : 1;
}
