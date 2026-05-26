#include "pyqtgraph/Point.hpp"

#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>

#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {

constexpr double kTolerance = 1.0e-12;

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
            std::exit(EXIT_FAILURE); \
        } \
    } while (false)

void assertNear(double actual, double expected, double tolerance = kTolerance)
{
    if (std::isinf(expected)) {
        CHECK(std::isinf(actual));
        CHECK(std::signbit(actual) == std::signbit(expected));
        return;
    }
    CHECK(std::abs(actual - expected) <= tolerance);
}

void assertPoint(const pyqtgraph::Point& point, double x, double y, double tolerance = kTolerance)
{
    assertNear(point.x(), x, tolerance);
    assertNear(point.y(), y, tolerance);
}

void expectOutOfRangeAt(const pyqtgraph::Point& point, qsizetype index)
{
    bool threw = false;
    try {
        static_cast<void>(point.at(index));
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectOutOfRangeSet(pyqtgraph::Point& point, qsizetype index)
{
    bool threw = false;
    try {
        point.set(index, 1.0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectInvalidInitializerList(std::initializer_list<double> values)
{
    bool threw = false;
    try {
        const pyqtgraph::Point point(values);
        static_cast<void>(point);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

} // namespace

int main()
{
    using pyqtgraph::Point;

    static_assert(std::is_same_v<decltype(Point{1.0, 2.0} + Point{3.0, 4.0}), Point>);
    static_assert(std::is_same_v<decltype(Point{1.0, 2.0} * QPointF{3.0, 4.0}), Point>);
    static_assert(std::is_same_v<decltype(2.0 / Point{1.0, 2.0}), Point>);
    static_assert(std::is_same_v<decltype(Point{2.0, 3.0}.pow(QPointF{4.0, 2.0})), Point>);
    static_assert(std::is_same_v<decltype(Point{4.0, 9.0}.pow(0.5)), Point>);
    static_assert(std::is_same_v<decltype(pyqtgraph::pow(2.0, Point{3.0, 4.0})), Point>);

    CHECK(Point::coordinateCount() == 2);
    const Point sequenceEquivalent{10.0, 20.0};
    assertNear(sequenceEquivalent.at(0), 10.0);
    assertNear(sequenceEquivalent.at(1), 20.0);

    assertPoint(Point(), 0.0, 0.0);
    assertPoint(Point{1.5, -2.25}, 1.5, -2.25);
    assertPoint(Point(3.0), 3.0, 3.0);
    assertPoint(Point{3.0}, 3.0, 3.0);
    assertPoint(Point{QPointF{4.5, -5.5}}, 4.5, -5.5);
    assertPoint(Point{QPoint{4, -5}}, 4.0, -5.0);
    assertPoint(Point{QSizeF{6.5, 7.5}}, 6.5, 7.5);
    assertPoint(Point{QSize{6, 7}}, 6.0, 7.0);
    assertPoint(Point({8.0, 9.0}), 8.0, 9.0);
    expectInvalidInitializerList({});
    expectInvalidInitializerList({1.0, 2.0, 3.0});

    Point indexed{10.0, 20.0};
    assertNear(indexed.at(0), 10.0);
    assertNear(indexed.at(1), 20.0);
    indexed.set(0, -1.0);
    indexed.set(1, -2.0);
    assertPoint(indexed, -1.0, -2.0);
    expectOutOfRangeAt(indexed, -1);
    expectOutOfRangeAt(indexed, 2);
    expectOutOfRangeSet(indexed, -1);
    expectOutOfRangeSet(indexed, 2);

    const Point point{6.0, 8.0};
    const Point other{2.0, 4.0};
    assertPoint(point + other, 8.0, 12.0);
    assertPoint(point - other, 4.0, 4.0);
    assertPoint(point * other, 12.0, 32.0);
    assertPoint(point / other, 3.0, 2.0);
    assertPoint(point + QPointF{1.0, -1.0}, 7.0, 7.0);
    assertPoint(Point{2.0, 3.0}.pow(QPointF{4.0, 2.0}), 16.0, 9.0);
    assertPoint(Point{4.0, 9.0}.pow(0.5), 2.0, 3.0);
    assertPoint(pyqtgraph::pow(2.0, Point{3.0, 4.0}), 8.0, 16.0);

    assertPoint(point + 2.0, 8.0, 10.0);
    assertPoint(point - 2.0, 4.0, 6.0);
    assertPoint(point * 2.0, 12.0, 16.0);
    assertPoint(point / 2.0, 3.0, 4.0);
    assertPoint(2.0 + point, 8.0, 10.0);
    assertPoint(20.0 - point, 14.0, 12.0);
    assertPoint(2.0 * point, 12.0, 16.0);
    assertPoint(24.0 / point, 4.0, 3.0);

    Point compound{3.0, 6.0};
    compound += QPointF{1.0, 2.0};
    assertPoint(compound, 4.0, 8.0);
    compound -= QPointF{2.0, 3.0};
    assertPoint(compound, 2.0, 5.0);
    compound *= QPointF{4.0, 2.0};
    assertPoint(compound, 8.0, 10.0);
    compound /= QPointF{2.0, 5.0};
    assertPoint(compound, 4.0, 2.0);
    compound += 1.0;
    compound -= 2.0;
    compound *= 3.0;
    compound /= 3.0;
    assertPoint(compound, 3.0, 1.0);

    assertPoint(point.copy(), 6.0, 8.0);
    assertNear(point.length(), 10.0);
    assertPoint(point.norm(), 0.6, 0.8);
    assertNear(point.norm().length(), 1.0);
    CHECK(std::isnan(Point().norm().x()));
    CHECK(std::isnan(Point().norm().y()));

    assertNear(Point{1.0, 0.0}.angle(QPointF{0.0, 1.0}), -90.0);
    assertNear(Point{1.0, 0.0}.angle(QPointF{0.0, 1.0}, QStringView{u"radians"}), -std::acos(-1.0) / 2.0);
    assertNear(Point{3.0, 4.0}.dot(QPointF{5.0, 6.0}), 39.0);
    assertNear(Point{3.0, 4.0}.cross(QPointF{5.0, 6.0}), -2.0);
    assertPoint(Point{3.0, 4.0}.proj(QPointF{10.0, 0.0}), 3.0, 0.0);
    assertPoint(Point{1.0, 0.0}.proj(QPointF{1.0e308, 0.0}), 1.0, 0.0);
    assertPoint(Point{1.0, 0.0}.proj(QPointF{1.0e-308, 0.0}), 1.0, 0.0);
    assertNear(Point{3.0, -4.0}.min(), -4.0);
    assertNear(Point{3.0, -4.0}.max(), 3.0);

    assertPoint(Point{1.0, 0.0} / Point{0.0, 2.0}, std::numeric_limits<double>::infinity(), 0.0);
    const QPoint qpoint = Point{3.0, 4.0}.toQPoint();
    CHECK(qpoint == QPoint(3, 4));

    return 0;
}
