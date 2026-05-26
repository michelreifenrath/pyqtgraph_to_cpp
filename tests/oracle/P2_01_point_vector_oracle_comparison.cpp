#include "pyqtgraph/Point.hpp"
#include "pyqtgraph/Vector.hpp"

#include <QPointF>
#include <QVector3D>

#include <cmath>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P2_01_FIXTURE
#define PYQTGRAPH_CPP_P2_01_FIXTURE "oracle/fixtures/P2_01/point_vector_semantics.json"
#endif

namespace {

constexpr double kPointTolerance = 1.0e-12;
constexpr double kVectorTolerance = 1.0e-5;

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open P2.01 oracle fixture: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

std::size_t findMatchingBracket(const std::string& text, std::size_t openIndex)
{
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t index = openIndex; index < text.size(); ++index) {
        const char character = text[index];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if (character == '"') {
                inString = false;
            }
            continue;
        }
        if (character == '"') {
            inString = true;
        } else if (character == '[') {
            ++depth;
        } else if (character == ']') {
            --depth;
            if (depth == 0) {
                return index;
            }
        }
    }
    throw std::runtime_error("unclosed JSON array");
}

std::string trim(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

double expectedNumericValue(std::string item, const std::string& key)
{
    item = trim(item);
    if (item == "\"Infinity\"") {
        return std::numeric_limits<double>::infinity();
    }
    if (item == "\"-Infinity\"") {
        return -std::numeric_limits<double>::infinity();
    }
    if (item == "\"NaN\"") {
        return std::numeric_limits<double>::quiet_NaN();
    }
    std::size_t parsed = 0;
    const double value = std::stod(item, &parsed);
    if (parsed != item.size()) {
        throw std::runtime_error("non-numeric array entry for " + key + ": " + item);
    }
    return value;
}

std::vector<double> expectedArray(const std::string& fixture, const std::string& key)
{
    std::smatch match;
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\[");
    if (!std::regex_search(fixture, match, pattern)) {
        throw std::runtime_error("missing array fixture key: " + key);
    }
    const auto arrayStart = static_cast<std::size_t>(match.position(0) + match.length(0) - 1);
    const auto arrayEnd = findMatchingBracket(fixture, arrayStart);
    std::stringstream input(fixture.substr(arrayStart + 1, arrayEnd - arrayStart - 1));
    std::vector<double> values;
    std::string item;
    while (std::getline(input, item, ',')) {
        if (trim(item).empty()) {
            continue;
        }
        values.push_back(expectedNumericValue(item, key));
    }
    return values;
}

double expectedNumber(const std::string& fixture, const std::string& key)
{
    std::smatch match;
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:e[+-]?[0-9]+)?)");
    if (!std::regex_search(fixture, match, pattern)) {
        throw std::runtime_error("missing numeric fixture key: " + key);
    }
    return std::stod(match[1].str());
}

int reportMismatch(const std::string& path, const std::string& expected, const std::string& actual)
{
    std::cerr << "P2.01 oracle fixture mismatch\n"
              << "fixture: " << PYQTGRAPH_CPP_P2_01_FIXTURE << '\n'
              << "path: " << path << '\n'
              << "expected fixture value: " << expected << '\n'
              << "actual C++ value: " << actual << '\n'
              << "tolerance point_absolute=" << kPointTolerance << " vector_absolute=" << kVectorTolerance << '\n';
    return EXIT_FAILURE;
}

bool near(double actual, double expected, double tolerance)
{
    if (std::isnan(expected)) {
        return std::isnan(actual);
    }
    if (std::isinf(expected)) {
        return std::isinf(actual) && std::signbit(actual) == std::signbit(expected);
    }
    return std::abs(actual - expected) <= tolerance;
}

int compareArray(
    const std::string& fixture,
    const std::string& key,
    const std::vector<double>& actual,
    double tolerance
)
{
    const auto expected = expectedArray(fixture, key);
    if (expected.size() != actual.size()) {
        return reportMismatch("$.expected." + key + ".size", std::to_string(expected.size()), std::to_string(actual.size()));
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (!near(actual[index], expected[index], tolerance)) {
            return reportMismatch(
                "$.expected." + key + '[' + std::to_string(index) + ']',
                std::to_string(expected[index]),
                std::to_string(actual[index])
            );
        }
    }
    return EXIT_SUCCESS;
}

int compareNumber(const std::string& fixture, const std::string& key, double actual, double tolerance)
{
    const double expected = expectedNumber(fixture, key);
    if (!near(actual, expected, tolerance)) {
        return reportMismatch("$.expected." + key, std::to_string(expected), std::to_string(actual));
    }
    return EXIT_SUCCESS;
}

template <typename Callable>
int compareDomainErrorMapping(
    const std::string& fixture,
    const std::string& key,
    std::string_view upstreamError,
    Callable callable
)
{
    const std::string expectedEntry = "\"" + key + "\": \"" + std::string(upstreamError) + "\"";
    if (!contains(fixture, expectedEntry)) {
        return reportMismatch("$.expected." + key, std::string(upstreamError), "missing");
    }
    try {
        static_cast<void>(callable());
    } catch (const std::domain_error&) {
        return EXIT_SUCCESS;
    } catch (...) {
        return reportMismatch("$.expected." + key, "std::domain_error", "different exception");
    }
    return reportMismatch("$.expected." + key, "std::domain_error", "no exception");
}

template <typename Callable>
int compareOverflowErrorMapping(
    const std::string& fixture,
    const std::string& key,
    std::string_view upstreamError,
    Callable callable
)
{
    const std::string expectedEntry = "\"" + key + "\": \"" + std::string(upstreamError) + "\"";
    if (!contains(fixture, expectedEntry)) {
        return reportMismatch("$.expected." + key, std::string(upstreamError), "missing");
    }
    try {
        static_cast<void>(callable());
    } catch (const std::overflow_error&) {
        return EXIT_SUCCESS;
    } catch (...) {
        return reportMismatch("$.expected." + key, "std::overflow_error", "different exception");
    }
    return reportMismatch("$.expected." + key, "std::overflow_error", "no exception");
}

std::vector<double> pointValues(const pyqtgraph::Point& point)
{
    return {point.x(), point.y()};
}

std::vector<double> vectorValues(const pyqtgraph::Vector& vector)
{
    return {static_cast<double>(vector.x()), static_cast<double>(vector.y()), static_cast<double>(vector.z())};
}

int comparePoint(const std::string& fixture, const std::string& key, const pyqtgraph::Point& point)
{
    return compareArray(fixture, key, pointValues(point), kPointTolerance);
}

int compareVector(const std::string& fixture, const std::string& key, const pyqtgraph::Vector& vector)
{
    return compareArray(fixture, key, vectorValues(vector), kVectorTolerance);
}

} // namespace

int main()
{
    using pyqtgraph::Point;
    using pyqtgraph::Vector;

    const auto fixture = readFile(PYQTGRAPH_CPP_P2_01_FIXTURE);
    if (!contains(fixture, "\"issue\": \"P2.01\"") || !contains(fixture, "\"pyqtgraph_version\": \"0.14.0\"")) {
        return reportMismatch("$.reference", "P2.01 PyQtGraph 0.14.0 metadata", "missing");
    }
    if (!contains(fixture, "operator^ is intentionally unsupported")) {
        return reportMismatch("$.cpp_equivalences", "operator^ documentation", "missing");
    }

#define COMPARE_ARRAY(key, actual, tolerance) \
    do { \
        const auto actualValues = (actual); \
        const int result = compareArray(fixture, (key), actualValues, (tolerance)); \
        if (result != EXIT_SUCCESS) { \
            return result; \
        } \
    } while (false)
#define COMPARE_POINT(key, ...) \
    do { \
        const int result = comparePoint(fixture, (key), (__VA_ARGS__)); \
        if (result != EXIT_SUCCESS) { \
            return result; \
        } \
    } while (false)
#define COMPARE_VECTOR(key, ...) \
    do { \
        const int result = compareVector(fixture, (key), (__VA_ARGS__)); \
        if (result != EXIT_SUCCESS) { \
            return result; \
        } \
    } while (false)
#define COMPARE_NUMBER(key, actual, tolerance) \
    do { \
        const int result = compareNumber(fixture, (key), (actual), (tolerance)); \
        if (result != EXIT_SUCCESS) { \
            return result; \
        } \
    } while (false)

    const Point point{6.0, 8.0};
    const Point other{2.0, 4.0};
    COMPARE_POINT("point_construct_scalar", Point(3.0));
    COMPARE_POINT("point_construct_sequence", Point({8.0, 9.0}));
    COMPARE_NUMBER("point_len", static_cast<double>(Point::coordinateCount()), 0.0);
    COMPARE_ARRAY("point_initial_index_values", (std::vector<double>{Point{10.0, 20.0}.at(0), Point{10.0, 20.0}.at(1)}), kPointTolerance);
    Point indexed{10.0, 20.0};
    indexed.set(0, -1.0);
    indexed.set(1, -2.0);
    COMPARE_POINT("point_set_values", indexed);
    COMPARE_ARRAY("point_iteration_values", (std::vector<double>{Point{10.0, 20.0}.at(0), Point{10.0, 20.0}.at(1)}), kPointTolerance);
    COMPARE_POINT("point_add", point + other);
    COMPARE_POINT("point_sub", point - other);
    COMPARE_POINT("point_mul", point * other);
    COMPARE_POINT("point_div", point / other);
    if (const int result = compareDomainErrorMapping(fixture, "point_div_zero_coordinate_error", "ZeroDivisionError", [] {
            return Point{1.0, 0.0} / QPointF{0.0, 2.0};
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    if (const int result = compareDomainErrorMapping(fixture, "point_div_scalar_zero_error", "ZeroDivisionError", [] {
            return Point{1.0, 0.0} / 0.0;
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    COMPARE_POINT("point_reflected_add", 2.0 + point);
    COMPARE_POINT("point_reflected_sub", 20.0 - point);
    COMPARE_POINT("point_reflected_mul", 2.0 * point);
    COMPARE_POINT("point_reflected_div", 24.0 / point);
    if (const int result = compareDomainErrorMapping(fixture, "point_reflected_div_zero_coordinate_error", "ZeroDivisionError", [] {
            return 2.0 / Point{0.0, 2.0};
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    COMPARE_POINT("point_pow_point", Point{2.0, 3.0}.pow(QPointF{4.0, 2.0}));
    COMPARE_POINT("point_pow_scalar", Point{4.0, 9.0}.pow(0.5));
    COMPARE_POINT("point_pow_negative_infinity_fractional", Point{-std::numeric_limits<double>::infinity(), 9.0}.pow(0.5));
    if (const int result = compareOverflowErrorMapping(fixture, "point_pow_point_overflow_error", "OverflowError", [] {
            return Point{2.0, 2.0}.pow(QPointF{1024.0, 2.0});
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    if (const int result = compareOverflowErrorMapping(fixture, "point_pow_scalar_overflow_error", "OverflowError", [] {
            return Point{2.0, 2.0}.pow(1024.0);
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    COMPARE_POINT("point_reflected_pow", pyqtgraph::pow(2.0, Point{3.0, 4.0}));
    if (const int result = compareOverflowErrorMapping(fixture, "point_reflected_pow_overflow_error", "OverflowError", [] {
            return pyqtgraph::pow(2.0, Point{1024.0, 1024.0});
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    if (const int result = compareDomainErrorMapping(fixture, "point_pow_zero_negative_error", "ZeroDivisionError", [] {
            return Point{0.0, 2.0}.pow(QPointF{-1.0, 2.0});
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    if (const int result = compareDomainErrorMapping(fixture, "point_pow_scalar_zero_negative_error", "ZeroDivisionError", [] {
            return Point{0.0, 2.0}.pow(-1.0);
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    if (const int result = compareDomainErrorMapping(fixture, "point_reflected_pow_zero_negative_error", "ZeroDivisionError", [] {
            return pyqtgraph::pow(0.0, Point{-1.0, 2.0});
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    if (const int result = compareDomainErrorMapping(fixture, "point_pow_negative_fractional_error", "TypeError", [] {
            return Point{-1.0, 2.0}.pow(0.5);
        });
        result != EXIT_SUCCESS) {
        return result;
    }
    COMPARE_NUMBER("point_length", point.length(), kPointTolerance);
    COMPARE_POINT("point_norm", point.norm());
    if (!contains(fixture, "\"point_zero_norm_error\": \"ZeroDivisionError\"")) {
        return reportMismatch("$.expected.point_zero_norm_error", "ZeroDivisionError", "missing");
    }
    bool zeroNormThrew = false;
    try {
        static_cast<void>(Point().norm());
    } catch (const std::domain_error&) {
        zeroNormThrew = true;
    }
    if (!zeroNormThrew) {
        return reportMismatch("$.expected.point_zero_norm_error", "std::domain_error", "no exception");
    }
    COMPARE_NUMBER("point_angle_degrees", Point(1.0, 0.0).angle(QPointF(0.0, 1.0)), kPointTolerance);
    COMPARE_NUMBER("point_angle_radians", Point(1.0, 0.0).angle(QPointF(0.0, 1.0), QStringView{u"radians"}), kPointTolerance);
    COMPARE_NUMBER("point_dot", Point(3.0, 4.0).dot(QPointF(5.0, 6.0)), kPointTolerance);
    COMPARE_NUMBER("point_cross", Point(3.0, 4.0).cross(QPointF(5.0, 6.0)), kPointTolerance);
    COMPARE_POINT("point_proj", Point{3.0, 4.0}.proj(QPointF{10.0, 0.0}));
    COMPARE_NUMBER("point_min", Point(3.0, -4.0).min(), kPointTolerance);
    COMPARE_NUMBER("point_max", Point(3.0, -4.0).max(), kPointTolerance);
    COMPARE_POINT("point_copy", point.copy());
    const QPoint qpoint = Point{3.0, 4.0}.toQPoint();
    COMPARE_ARRAY("point_to_qpoint", (std::vector<double>{static_cast<double>(qpoint.x()), static_cast<double>(qpoint.y())}), kPointTolerance);

    COMPARE_VECTOR("vector_construct_2", Vector{1.5, -2.25});
    COMPARE_VECTOR("vector_construct_3", Vector{1.5, -2.25, 3.75});
    COMPARE_VECTOR("vector_construct_sequence_2", Vector({8.0, 9.0}));
    COMPARE_VECTOR("vector_construct_sequence_3", Vector({8.0, 9.0, 10.0}));
    COMPARE_NUMBER("vector_len", static_cast<double>(Vector::coordinateCount()), 0.0);
    COMPARE_ARRAY("vector_initial_index_values", (std::vector<double>{Vector{10.0, 20.0, 30.0}.at(0), Vector{10.0, 20.0, 30.0}.at(1), Vector{10.0, 20.0, 30.0}.at(2)}), kVectorTolerance);
    Vector vector{10.0, 20.0, 30.0};
    vector.set(0, -1.0);
    vector.set(1, -2.0);
    vector.set(2, -3.0);
    COMPARE_VECTOR("vector_set_values", vector);
    COMPARE_ARRAY("vector_iteration_values", (std::vector<double>{Vector{10.0, 20.0, 30.0}.at(0), Vector{10.0, 20.0, 30.0}.at(1), Vector{10.0, 20.0, 30.0}.at(2)}), kVectorTolerance);
    const std::optional<double> angle = Vector{1.0, 0.0, 0.0}.angle(QVector3D{0.0F, 1.0F, 0.0F});
    if (!angle.has_value()) {
        return reportMismatch("$.expected.vector_angle_right", "90", "nullopt");
    }
    COMPARE_NUMBER("vector_angle_right", *angle, kVectorTolerance);
    if (Vector().angle(QVector3D{1.0F, 0.0F, 0.0F}).has_value() || !contains(fixture, "\"vector_angle_zero\": null")) {
        return reportMismatch("$.expected.vector_angle_zero", "null", "non-null");
    }
    COMPARE_VECTOR("vector_abs", Vector{-1.0, 2.0, -3.0}.abs());

    return EXIT_SUCCESS;
}
