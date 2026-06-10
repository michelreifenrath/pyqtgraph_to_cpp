#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#ifndef CPPQTGRAPH_INTERACTION_FIXTURE_DIR
#define CPPQTGRAPH_INTERACTION_FIXTURE_DIR "oracle/fixtures/interactions"
#endif

namespace {

using ViewBox = cppqtgraph::graphicsItems::ViewBox;

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

class ApplicationGuard {
public:
    ApplicationGuard(int& argc, char** argv)
    {
        if (QApplication::instance() == nullptr) {
            application_ = std::make_unique<QApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QApplication> application_;
};

bool nearlyEqual(qreal lhs, qreal rhs)
{
    return std::abs(lhs - rhs) <= 1.0e-12;
}

bool checkRange(const ViewBox::Range2D& range, qreal xMin, qreal xMax, qreal yMin, qreal yMax)
{
    CHECK(nearlyEqual(range[0][0], xMin));
    CHECK(nearlyEqual(range[0][1], xMax));
    CHECK(nearlyEqual(range[1][0], yMin));
    CHECK(nearlyEqual(range[1][1], yMax));
    return true;
}

ViewBox::AxisRange readAxisRange(const QJsonObject& object, const char* key)
{
    const auto values = object.value(QLatin1String(key)).toArray();
    if (values.size() != 2) {
        throw std::runtime_error("fixture axis range must contain two numbers");
    }
    return ViewBox::AxisRange{values.at(0).toDouble(), values.at(1).toDouble()};
}

QPointF readPoint(const QJsonArray& values)
{
    if (values.size() != 2) {
        throw std::runtime_error("fixture point must contain two numbers");
    }
    return QPointF(values.at(0).toDouble(), values.at(1).toDouble());
}

std::optional<qreal> readOptionalNumber(const QJsonObject& object, const char* key)
{
    if (!object.contains(QLatin1String(key))) {
        return std::nullopt;
    }
    return object.value(QLatin1String(key)).toDouble();
}

bool applyFixtureOperation(ViewBox& viewBox, const QJsonObject& operation)
{
    const auto type = operation.value(QStringLiteral("type")).toString();
    const std::optional<QPointF> center = operation.contains(QStringLiteral("center"))
        ? std::optional<QPointF>{readPoint(operation.value(QStringLiteral("center")).toArray())}
        : std::nullopt;

    if (type == QLatin1String("scaleBy")) {
        if (operation.contains(QStringLiteral("scale"))) {
            viewBox.scaleBy(readPoint(operation.value(QStringLiteral("scale")).toArray()), center);
        } else {
            viewBox.scaleBy(readOptionalNumber(operation, "x"), readOptionalNumber(operation, "y"), center);
        }
        return true;
    }

    if (type == QLatin1String("translateBy")) {
        if (operation.contains(QStringLiteral("offset"))) {
            viewBox.translateBy(readPoint(operation.value(QStringLiteral("offset")).toArray()));
        } else {
            viewBox.translateBy(readOptionalNumber(operation, "x"), readOptionalNumber(operation, "y"));
        }
        return true;
    }

    std::cerr << "unknown fixture operation: " << type.toStdString() << '\n';
    return false;
}

bool testFixturePanZoomCases()
{
    QFile fixture(QStringLiteral(CPPQTGRAPH_INTERACTION_FIXTURE_DIR "/ViewBox_basic_pan_zoom.json"));
    CHECK(fixture.open(QIODevice::ReadOnly));

    const auto document = QJsonDocument::fromJson(fixture.readAll());
    CHECK(document.isObject());

    const auto root = document.object();
    CHECK(root.value(QStringLiteral("version")).toInt() == 1);
    const auto cases = root.value(QStringLiteral("cases")).toArray();
    CHECK(!cases.isEmpty());

    for (const auto& caseValue : cases) {
        const auto caseObject = caseValue.toObject();
        const auto initial = caseObject.value(QStringLiteral("initial")).toObject();
        const auto expected = caseObject.value(QStringLiteral("expected")).toObject();
        const auto initialX = readAxisRange(initial, "x");
        const auto initialY = readAxisRange(initial, "y");
        const auto expectedX = readAxisRange(expected, "x");
        const auto expectedY = readAxisRange(expected, "y");

        ViewBox viewBox;
        viewBox.setRange(initialX, initialY, 0.0);

        for (const auto& operation : caseObject.value(QStringLiteral("operations")).toArray()) {
            CHECK(applyFixtureOperation(viewBox, operation.toObject()));
        }

        CHECK(checkRange(viewBox.targetRange(), expectedX[0], expectedX[1], expectedY[0], expectedY[1]));
        CHECK(checkRange(viewBox.viewRange(), expectedX[0], expectedX[1], expectedY[0], expectedY[1]));
    }

    return true;
}

bool testScaleByDefaultsAndOptionalAxes()
{
    ViewBox viewBox;
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 20.0), 0.0);
    viewBox.scaleBy(QPointF(0.5, 2.0));
    CHECK(checkRange(viewBox.targetRange(), 2.5, 7.5, -10.0, 30.0));
    CHECK(checkRange(viewBox.viewRange(), 2.5, 7.5, -10.0, 30.0));

    ViewBox xOnly;
    xOnly.setRange(QRectF(0.0, 0.0, 10.0, 20.0), 0.0);
    xOnly.scaleBy(0.5, std::nullopt, QPointF(0.0, 0.0));
    CHECK(checkRange(xOnly.targetRange(), 0.0, 5.0, 0.0, 20.0));
    CHECK(checkRange(xOnly.viewRange(), 0.0, 5.0, 0.0, 20.0));

    ViewBox yOnly;
    yOnly.setRange(QRectF(0.0, -10.0, 10.0, 20.0), 0.0);
    yOnly.scaleBy(std::nullopt, 0.25, QPointF(0.0, 0.0));
    CHECK(checkRange(yOnly.targetRange(), 0.0, 10.0, -2.5, 2.5));
    CHECK(checkRange(yOnly.viewRange(), 0.0, 10.0, -2.5, 2.5));

    return true;
}

bool testTranslateByPointAndOptionalAxes()
{
    ViewBox bothAxes;
    bothAxes.setRange(QRectF(-2.0, 3.0, 5.0, 7.0), 0.0);
    bothAxes.translateBy(QPointF(4.0, -6.0));
    CHECK(checkRange(bothAxes.targetRange(), 2.0, 7.0, -3.0, 4.0));
    CHECK(checkRange(bothAxes.viewRange(), 2.0, 7.0, -3.0, 4.0));

    ViewBox optionalAxes;
    optionalAxes.setRange(QRectF(1.0, 2.0, 3.0, 4.0), 0.0);
    optionalAxes.translateBy(std::nullopt, 5.0);
    CHECK(checkRange(optionalAxes.targetRange(), 1.0, 4.0, 7.0, 11.0));
    optionalAxes.translateBy(-2.0, std::nullopt);
    CHECK(checkRange(optionalAxes.viewRange(), -1.0, 2.0, 7.0, 11.0));

    return true;
}

bool testLimitClampingDelegatesThroughSetRange()
{
    ViewBox viewBox;
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    ViewBox::Limits limits;
    limits.xMin = -2.0;
    limits.xMax = 8.0;
    limits.yMin = -1.0;
    limits.yMax = 12.0;
    viewBox.setLimits(limits);

    viewBox.translateBy(QPointF(5.0, 5.0));
    CHECK(checkRange(viewBox.targetRange(), -2.0, 8.0, 2.0, 12.0));
    CHECK(checkRange(viewBox.viewRange(), -2.0, 8.0, 2.0, 12.0));

    viewBox.scaleBy(QPointF(0.01, 0.01));
    CHECK(checkRange(viewBox.targetRange(), 2.95, 3.05, 6.95, 7.05));

    return true;
}

bool testInvalidInputsAndNoAxisCallsPreserveState()
{
    ViewBox viewBox;
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 20.0), 0.0);

    const auto beforeScale = viewBox.targetRange();
    bool threw = false;
    try {
        viewBox.scaleBy(std::numeric_limits<qreal>::quiet_NaN(), 1.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.targetRange() == beforeScale);
    CHECK(viewBox.viewRange() == beforeScale);

    threw = false;
    try {
        viewBox.scaleBy(1.0, 1.0, QPointF(std::numeric_limits<qreal>::infinity(), 0.0));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.targetRange() == beforeScale);

    threw = false;
    try {
        viewBox.translateBy(QPointF(0.0, std::numeric_limits<qreal>::quiet_NaN()));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.targetRange() == beforeScale);

    viewBox.translateBy(std::nullopt, std::nullopt);
    CHECK(viewBox.targetRange() == beforeScale);
    CHECK(viewBox.viewRange() == beforeScale);

    viewBox.scaleBy(std::nullopt, std::nullopt);
    CHECK(viewBox.targetRange() == beforeScale);
    CHECK(viewBox.viewRange() == beforeScale);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard app(argc, argv);

    if (!testFixturePanZoomCases()) {
        return 1;
    }
    if (!testScaleByDefaultsAndOptionalAxes()) {
        return 1;
    }
    if (!testTranslateByPointAndOptionalAxes()) {
        return 1;
    }
    if (!testLimitClampingDelegatesThroughSetRange()) {
        return 1;
    }
    if (!testInvalidInputsAndNoAxisCallsPreserveState()) {
        return 1;
    }

    return 0;
}
