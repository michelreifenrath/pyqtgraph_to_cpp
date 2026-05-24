#include "pyqtgraph/Vector.hpp"

#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QVector3D>

#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {

constexpr double kTolerance = 1.0e-5;

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
    CHECK(std::abs(actual - expected) <= tolerance);
}

void assertVector(const pyqtgraph::Vector& vector, double x, double y, double z, double tolerance = kTolerance)
{
    assertNear(vector.x(), x, tolerance);
    assertNear(vector.y(), y, tolerance);
    assertNear(vector.z(), z, tolerance);
}

void expectOutOfRangeAt(const pyqtgraph::Vector& vector, qsizetype index)
{
    bool threw = false;
    try {
        static_cast<void>(vector.at(index));
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectOutOfRangeSet(pyqtgraph::Vector& vector, qsizetype index)
{
    bool threw = false;
    try {
        vector.set(index, 1.0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectInvalidInitializerList(std::initializer_list<double> values)
{
    bool threw = false;
    try {
        const pyqtgraph::Vector vector(values);
        static_cast<void>(vector);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

} // namespace

int main()
{
    using pyqtgraph::Vector;

    static_assert(std::is_base_of_v<QVector3D, Vector>);
    static_assert(std::is_convertible_v<Vector*, QVector3D*>);
    static_assert(std::is_same_v<decltype(Vector{1.0, 2.0, 3.0}.copy()), Vector>);
    static_assert(std::is_same_v<decltype(Vector{1.0, 2.0, 3.0}.angle(QVector3D{0.0F, 1.0F, 0.0F})), std::optional<double>>);

    CHECK(Vector::coordinateCount() == 3);

    assertVector(Vector(), 0.0, 0.0, 0.0);
    assertVector(Vector{1.5, -2.25}, 1.5, -2.25, 0.0);
    assertVector(Vector{1.5, -2.25, 3.75}, 1.5, -2.25, 3.75);
    assertVector(Vector({8.0, 9.0}), 8.0, 9.0, 0.0);
    assertVector(Vector({8.0, 9.0, 10.0}), 8.0, 9.0, 10.0);
    assertVector(Vector{QPointF{4.5, -5.5}}, 4.5, -5.5, 0.0);
    assertVector(Vector{QPoint{4, -5}}, 4.0, -5.0, 0.0);
    assertVector(Vector{QSizeF{6.5, 7.5}}, 6.5, 7.5, 0.0);
    assertVector(Vector{QSize{6, 7}}, 6.0, 7.0, 0.0);
    assertVector(Vector{QVector3D{1.0F, 2.0F, 3.0F}}, 1.0, 2.0, 3.0);
    expectInvalidInitializerList({});
    expectInvalidInitializerList({1.0});
    expectInvalidInitializerList({1.0, 2.0, 3.0, 4.0});

    Vector indexed{10.0, 20.0, 30.0};
    assertNear(indexed.at(0), 10.0);
    assertNear(indexed.at(1), 20.0);
    assertNear(indexed.at(2), 30.0);
    indexed.set(0, -1.0);
    indexed.set(1, -2.0);
    indexed.set(2, -3.0);
    assertVector(indexed, -1.0, -2.0, -3.0);
    expectOutOfRangeAt(indexed, -1);
    expectOutOfRangeAt(indexed, 3);
    expectOutOfRangeSet(indexed, -1);
    expectOutOfRangeSet(indexed, 3);

    const Vector original{6.0, 8.0, 10.0};
    Vector copied = original.copy();
    assertVector(copied, 6.0, 8.0, 10.0);
    copied.set(0, -6.0);
    assertVector(original, 6.0, 8.0, 10.0);
    assertVector(copied, -6.0, 8.0, 10.0);

    const std::optional<double> rightAngle = Vector{1.0, 0.0, 0.0}.angle(QVector3D{0.0F, 1.0F, 0.0F});
    CHECK(rightAngle.has_value());
    assertNear(*rightAngle, 90.0);
    const std::optional<double> fortyFive = Vector{1.0, 0.0, 0.0}.angle(QVector3D{1.0F, 1.0F, 0.0F});
    CHECK(fortyFive.has_value());
    assertNear(*fortyFive, 45.0);
    const std::optional<double> parallel = Vector{1.0, 0.0, 0.0}.angle(QVector3D{1.0F, 0.0F, 0.0F});
    CHECK(parallel.has_value());
    assertNear(*parallel, 0.0);
    CHECK(!Vector().angle(QVector3D{1.0F, 0.0F, 0.0F}).has_value());
    const Vector nonZero{1.0, 0.0, 0.0};
    CHECK(!nonZero.angle(QVector3D{}).has_value());

    assertVector(Vector{-1.0, 2.0, -3.0}.abs(), 1.0, 2.0, 3.0);
    assertVector(pyqtgraph::abs(Vector{-4.0, -5.0, 6.0}), 4.0, 5.0, 6.0);

    return 0;
}
