#include "pyqtgraph/SRTTransform.hpp"
#include "pyqtgraph/SRTTransform3D.hpp"
#include "pyqtgraph/Transform3D.hpp"

#include <QMatrix4x4>
#include <QPoint>
#include <QPointF>
#include <QTransform>
#include <QVector3D>

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P2_02_FIXTURE
#define PYQTGRAPH_CPP_P2_02_FIXTURE "oracle/fixtures/P2_02/transform_srt_oracle.json"
#endif

namespace {

constexpr double kMatrixTolerance = 1.0e-5;
constexpr double kPointTolerance = 1.0e-5;
constexpr double kVectorTolerance = 1.0e-5;

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
    std::ifstream input(std::filesystem::path{PYQTGRAPH_CPP_P2_02_FIXTURE});
    CHECK(input.good());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(std::string_view text, std::string_view needle)
{
    return text.find(needle) != std::string_view::npos;
}

void requireOracleFixture()
{
    const std::string fixture = readOracleFixture();
    CHECK(contains(fixture, "\"issue\": \"P2.02\""));
    CHECK(contains(fixture, "\"commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\""));
    CHECK(contains(fixture, "\"pyqtgraph/Transform3D.py\""));
    CHECK(contains(fixture, "\"pyqtgraph/SRTTransform.py\""));
    CHECK(contains(fixture, "\"pyqtgraph/SRTTransform3D.py\""));
    CHECK(contains(fixture, "\"tests/test_srttransform3d.py\""));
    CHECK(contains(fixture, "\"matrix_absolute\": 1e-05"));
    CHECK(contains(fixture, "\"two_value_scale_defaults_z\""));
    CHECK(contains(fixture, "C++ exposes matrix3D()/matrix2D()"));
}

void assertNear(double actual, double expected, double tolerance = kMatrixTolerance)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "actual=" << actual << " expected=" << expected << " tolerance=" << tolerance << '\n';
        CHECK(false);
    }
}

void assertPoint(const QPointF& point, double x, double y, double tolerance = kPointTolerance)
{
    assertNear(point.x(), x, tolerance);
    assertNear(point.y(), y, tolerance);
}

void assertVector(const QVector3D& vector, double x, double y, double z, double tolerance = kVectorTolerance)
{
    assertNear(vector.x(), x, tolerance);
    assertNear(vector.y(), y, tolerance);
    assertNear(vector.z(), z, tolerance);
}

void assertMatrix3D(const std::array<double, 16>& matrix)
{
    const double c = std::sqrt(0.5);
    const std::array<double, 16> expected{
        0.2 * c, -0.4 * c, 0.0, 10.0,
        0.2 * c, 0.4 * c, 0.0, 20.0,
        0.0, 0.0, 1.0, 40.0,
        0.0, 0.0, 0.0, 1.0,
    };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        assertNear(matrix[index], expected[index]);
    }
}

void assertMatrix2D(const std::array<double, 9>& matrix)
{
    const double c = std::sqrt(0.5);
    const std::array<double, 9> expected{
        0.2 * c, 0.2 * c, 0.0,
        -0.4 * c, 0.4 * c, 0.0,
        10.0, 20.0, 1.0,
    };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        assertNear(matrix[index], expected[index]);
    }
}

void assertProjectedMatrix2D(const std::array<double, 9>& matrix)
{
    const double c = std::sqrt(0.5);
    const std::array<double, 9> expected{
        0.2 * c, -0.4 * c, 10.0,
        0.2 * c, 0.4 * c, 20.0,
        0.0, 0.0, 1.0,
    };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        assertNear(matrix[index], expected[index]);
    }
}

} // namespace

int main()
{
    using pyqtgraph::SRTTransform;
    using pyqtgraph::SRTTransform3D;
    using pyqtgraph::Transform3D;

    static_assert(std::is_base_of_v<QMatrix4x4, Transform3D>);
    static_assert(std::is_base_of_v<QTransform, SRTTransform>);
    static_assert(std::is_base_of_v<Transform3D, SRTTransform3D>);
    static_assert(std::is_same_v<decltype(Transform3D{}.inverted()), std::pair<Transform3D, bool>>);
    static_assert(std::is_same_v<decltype(SRTTransform3D{}.as2D()), SRTTransform>);

    requireOracleFixture();

    const Transform3D identity;
    const std::array<double, 16> identity3D = identity.matrix3D();
    CHECK(identity3D[0] == 1.0);
    CHECK(identity3D[5] == 1.0);
    CHECK(identity3D[10] == 1.0);
    CHECK(identity3D[15] == 1.0);
    const std::array<double, 9> identity2D = identity.matrix2D();
    CHECK(identity2D[0] == 1.0);
    CHECK(identity2D[4] == 1.0);
    CHECK(identity2D[8] == 1.0);
    CHECK(identity.matrix(3).size() == 16);
    CHECK(identity.matrix(2).size() == 9);

    bool invalidLengthThrew = false;
    try {
        const Transform3D invalid(std::vector<double>{1.0, 2.0, 3.0});
        static_cast<void>(invalid);
    } catch (const std::invalid_argument&) {
        invalidLengthThrew = true;
    }
    CHECK(invalidLengthThrew);

    SRTTransform srt;
    srt.setRotate(45.0);
    srt.setScale(0.2, 0.4);
    srt.setTranslate(10.0, 20.0);
    CHECK(srt.getRotation() == 45.0);
    assertPoint(srt.getScale(), 0.2, 0.4);
    assertPoint(srt.getTranslation(), 10.0, 20.0);
    assertMatrix2D(srt.matrix());
    assertPoint(srt.map(QPointF{1.0, 0.0}), 10.0 + 0.2 * std::sqrt(0.5), 20.0 + 0.2 * std::sqrt(0.5), kMatrixTolerance);
    assertPoint(srt.map(QPointF{0.0, 1.0}), 10.0 - 0.4 * std::sqrt(0.5), 20.0 + 0.4 * std::sqrt(0.5), kMatrixTolerance);
    CHECK((srt.map(QPoint{0, 0}) == QPoint{10, 20}));
    const SRTTransform::State saved2D = srt.saveState();
    SRTTransform restored2D;
    restored2D.restoreState(saved2D);
    assertMatrix2D(restored2D.matrix());
    const SRTTransform fromQTransform(QTransform{srt});
    assertPoint(fromQTransform.getTranslation(), 10.0, 20.0);
    assertPoint(fromQTransform.getScale(), 0.2, 0.4);
    assertNear(fromQTransform.getRotation(), 45.0);
    const SRTTransform divided = srt.dividedBy(SRTTransform{});
    assertMatrix2D(divided.matrix());
    const SRTTransform multiplied = SRTTransform{} * srt;
    assertMatrix2D(multiplied.matrix());

    SRTTransform3D srt3d;
    srt3d.setRotate(45.0, QVector3D{0.0F, 0.0F, 1.0F});
    srt3d.setScale(0.2, 0.4, 1.0);
    srt3d.setTranslate(10.0, 20.0, 40.0);
    const auto rotation = srt3d.getRotation();
    assertNear(rotation.angle, 45.0);
    assertVector(rotation.axis, 0.0, 0.0, 1.0);
    assertVector(srt3d.getScale(), 0.2, 0.4, 1.0);
    assertVector(srt3d.getTranslation(), 10.0, 20.0, 40.0);
    assertMatrix3D(srt3d.matrix3D());
    assertProjectedMatrix2D(srt3d.matrix2D());
    assertVector(srt3d.map(pyqtgraph::Vector{1.0, 0.0, 0.0}), 10.0 + 0.2 * std::sqrt(0.5), 20.0 + 0.2 * std::sqrt(0.5), 40.0);
    assertVector(srt3d.map(QVector3D{0.0F, 1.0F, 0.0F}), 10.0 - 0.4 * std::sqrt(0.5), 20.0 + 0.4 * std::sqrt(0.5), 40.0);
    assertPoint(srt3d.map(QPointF{1.0, 0.0}), 10.0 + 0.2 * std::sqrt(0.5), 20.0 + 0.2 * std::sqrt(0.5), kVectorTolerance);
    const auto mappedArray = srt3d.map(std::array<double, 3>{1.0, 0.0, 0.0});
    assertNear(mappedArray[0], 10.0 + 0.2 * std::sqrt(0.5), kVectorTolerance);
    assertNear(mappedArray[1], 20.0 + 0.2 * std::sqrt(0.5), kVectorTolerance);
    assertNear(mappedArray[2], 40.0, kVectorTolerance);
    const std::vector<double> mappedVector = srt3d.map(std::vector<double>{0.0, 1.0});
    CHECK(mappedVector.size() == 2);
    assertNear(mappedVector[0], 10.0 - 0.4 * std::sqrt(0.5), kVectorTolerance);
    assertNear(mappedVector[1], 20.0 + 0.4 * std::sqrt(0.5), kVectorTolerance);

    const Transform3D copied3D(srt3d);
    assertMatrix3D(copied3D.matrix3D());
    const SRTTransform3D roundTrip(copied3D);
    assertMatrix3D(roundTrip.matrix3D());
    assertNear(roundTrip.getRotation().angle, 45.0, kVectorTolerance);
    assertVector(roundTrip.getRotation().axis, 0.0, 0.0, 1.0);
    assertVector(roundTrip.getScale(), 0.2, 0.4, 1.0);
    assertVector(roundTrip.getTranslation(), 10.0, 20.0, 40.0);

    const SRTTransform::State as2DState = srt3d.as2D().saveState();
    assertPoint(as2DState.pos, 10.0, 20.0);
    assertPoint(as2DState.scale, 0.2, 0.4);
    assertNear(as2DState.angle, 45.0, kVectorTolerance);

    const SRTTransform3D::State saved3D = srt3d.saveState();
    SRTTransform3D restored3D;
    restored3D.restoreState(saved3D);
    assertMatrix3D(restored3D.matrix3D());

    SRTTransform3D twoValueScale;
    twoValueScale.setScale(2.0, 3.0);
    assertVector(twoValueScale.getScale(), 2.0, 3.0, 1.0);
    twoValueScale.scale(0.5, 0.25);
    assertVector(twoValueScale.getScale(), 1.0, 0.75, 1.0);
    twoValueScale.setRotate(0.0, QVector3D{1.0F, 0.0F, 0.0F});
    const SRTTransform3D zeroAngleRoundTrip(Transform3D{twoValueScale});
    assertNear(zeroAngleRoundTrip.getRotation().angle, 0.0, kVectorTolerance);
    assertVector(zeroAngleRoundTrip.getRotation().axis, 0.0, 0.0, 1.0);

    SRTTransform3D nonZ;
    nonZ.setRotate(30.0, QVector3D{1.0F, 0.0F, 0.0F});
    bool nonZRejected = false;
    try {
        static_cast<void>(nonZ.as2D());
    } catch (const std::invalid_argument&) {
        nonZRejected = true;
    }
    CHECK(nonZRejected);

    const auto inverted = srt3d.inverted();
    CHECK(inverted.second);
    const QVector3D point{3.0F, -2.0F, 5.0F};
    const QVector3D roundTripPoint = inverted.first.map(srt3d.map(point));
    assertVector(roundTripPoint, point.x(), point.y(), point.z(), 2.0e-5);

    return EXIT_SUCCESS;
}
