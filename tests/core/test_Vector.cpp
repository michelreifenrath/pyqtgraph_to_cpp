#include "cppqtgraph/Vector.hpp"

#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QVector3D>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#ifndef CPPQTGRAPH_P2_01_FIXTURE
#define CPPQTGRAPH_P2_01_FIXTURE "oracle/fixtures/P2_01/point_vector_oracle.json"
#endif

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

std::string readOracleFixture()
{
    std::ifstream input(std::filesystem::path{CPPQTGRAPH_P2_01_FIXTURE});
    CHECK(input.good());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(std::string_view text, std::string_view needle)
{
    return text.find(needle) != std::string_view::npos;
}

void requireVectorOracleFixture()
{
    const std::string fixture = readOracleFixture();
    CHECK(contains(fixture, "\"issue\": \"P2.01\""));
    CHECK(contains(fixture, "\"commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\""));
    CHECK(contains(fixture, "\"pyqtgraph/Vector.py\""));
    CHECK(contains(fixture, "\"tests/test_Vector.py\""));
    CHECK(contains(fixture, "\"vector_absolute\": 1e-05"));
    CHECK(contains(fixture, "\"angle_zero_vector\": null"));
    CHECK(contains(fixture, "std::nullopt"));
    CHECK(contains(fixture, "QVector3D float coordinates"));
}

void assertNear(double actual, double expected, double tolerance = kTolerance)
{
    CHECK(std::abs(actual - expected) <= tolerance);
}

void assertVector(const cppqtgraph::Vector& vector, double x, double y, double z, double tolerance = kTolerance)
{
    assertNear(vector.x(), x, tolerance);
    assertNear(vector.y(), y, tolerance);
    assertNear(vector.z(), z, tolerance);
}

void expectOutOfRangeAt(const cppqtgraph::Vector& vector, qsizetype index)
{
    bool threw = false;
    try {
        static_cast<void>(vector.at(index));
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectOutOfRangeSet(cppqtgraph::Vector& vector, qsizetype index)
{
    bool threw = false;
    try {
        vector.set(index, 1.0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectOutOfRangeIndexRead(const cppqtgraph::Vector& vector, int index)
{
    bool threw = false;
    try {
        static_cast<void>(vector[index]);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectOutOfRangeIndexWrite(cppqtgraph::Vector& vector, int index)
{
    bool threw = false;
    try {
        vector[index] = 1.0F;
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

void expectInvalidInitializerList(std::initializer_list<double> values)
{
    bool threw = false;
    try {
        const cppqtgraph::Vector vector(values);
        static_cast<void>(vector);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

} // namespace

int main()
{
    using cppqtgraph::Vector;

    static_assert(std::is_base_of_v<QVector3D, Vector>);
    static_assert(std::is_convertible_v<Vector*, QVector3D*>);
    static_assert(std::is_same_v<decltype(Vector{1.0, 2.0, 3.0}.copy()), Vector>);
    static_assert(std::is_same_v<decltype(Vector{1.0, 2.0, 3.0}.angle(QVector3D{0.0F, 1.0F, 0.0F})), std::optional<double>>);

    CHECK(Vector::coordinateCount() == 3);
    requireVectorOracleFixture();

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
    const Vector& constIndexed = indexed;
    assertNear(constIndexed[0], 10.0);
    assertNear(constIndexed[1], 20.0);
    assertNear(constIndexed[2], 30.0);
    indexed[0] = -1.0F;
    indexed[1] = -2.0F;
    indexed[2] = -3.0F;
    assertVector(indexed, -1.0, -2.0, -3.0);
    indexed.set(0, -4.0);
    indexed.set(1, -5.0);
    indexed.set(2, -6.0);
    assertVector(indexed, -4.0, -5.0, -6.0);
    expectOutOfRangeAt(indexed, -1);
    expectOutOfRangeAt(indexed, 3);
    expectOutOfRangeAt(indexed, 4);
    expectOutOfRangeSet(indexed, -1);
    expectOutOfRangeSet(indexed, 3);
    expectOutOfRangeSet(indexed, 4);
    expectOutOfRangeIndexRead(indexed, -1);
    expectOutOfRangeIndexRead(indexed, 3);
    expectOutOfRangeIndexRead(indexed, 4);
    expectOutOfRangeIndexWrite(indexed, -1);
    expectOutOfRangeIndexWrite(indexed, 3);
    expectOutOfRangeIndexWrite(indexed, 4);

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
    assertVector(cppqtgraph::abs(Vector{-4.0, -5.0, 6.0}), 4.0, 5.0, 6.0);

    return 0;
}
