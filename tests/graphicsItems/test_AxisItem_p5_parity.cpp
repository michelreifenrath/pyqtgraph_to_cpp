#define CPPQTGRAPH_PLOTTING_NO_MAIN
#include "../../examples/Plotting.cpp"

#include <cppqtgraph/graphicsItems/AxisItem.hpp>

#include <QtCore/QRectF>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace {

// Pinned from deterministic Plotting.cpp p5 data (seed 0x504C5454) and
// pyqtgraph-0.14.0 AxisItem.py @ a20028b98294b9cc8770f2015a92eb342224b788.
constexpr double kP5LogXMin = -7.388271382801615;
constexpr double kP5LogXMax = -4.509981088978167;
constexpr double kP5LinearYMin = 1.02795;
constexpr double kP5LinearYMax = 1.0679;
constexpr double kAxisPixelLengthBottom = 340.0;
constexpr double kAxisPixelLengthLeft = 240.0;

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }
    return true;
}

bool checkClose(double actual, double expected, double tolerance, std::string_view expression, std::string_view file, int line)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << file << ':' << line << ": check failed: " << expression << " actual=" << actual
                  << " expected=" << expected << " tolerance=" << tolerance << '\n';
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

#define CHECK_CLOSE(actual, expected, tolerance) \
    do { \
        if (!checkClose((actual), (expected), (tolerance), #actual, __FILE__, __LINE__)) { \
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

bool containsSemanticUnits(const QString& labelHtml, const QString& prefix, const QString& units)
{
    return labelHtml.contains(QStringLiteral("(%1%2)").arg(prefix, units));
}

bool tickPositionsIncreaseAlongBottomAxis(const cppqtgraph::graphicsItems::AxisItem::DrawSpecs& specs)
{
    std::vector<double> centers;
    centers.reserve(specs.text.size());
    for (const auto& textSpec : specs.text) {
        centers.push_back(textSpec.rect.center().x());
    }
    if (centers.size() < 2U) {
        return false;
    }
    std::sort(centers.begin(), centers.end());
    for (std::size_t index = 1; index < centers.size(); ++index) {
        if (centers[index] <= centers[index - 1]) {
            return false;
        }
    }
    return true;
}

bool testStandaloneP5LikeAxes()
{
    using cppqtgraph::graphicsItems::AxisItem;

    AxisItem left(AxisItem::Orientation::Left);
    left.setGeometry(QRectF(0.0, 0.0, 70.0, kAxisPixelLengthLeft));
    left.setWidth(70.0);
    left.setLabel(QStringLiteral("Y Axis"), QStringLiteral("A"));
    left.setRange(kP5LinearYMin, kP5LinearYMax);
    left.setLogMode(false, false);

    CHECK(left.labelText() == QStringLiteral("Y Axis"));
    CHECK(left.labelUnits() == QStringLiteral("A"));
    CHECK(left.labelUnitPrefix().isEmpty());
    CHECK_CLOSE(left.autoSIPrefixScale(), 1.0, 1.0e-15);
    CHECK(left.labelString().contains(QStringLiteral("Y Axis (A)")));

    const auto leftLevels = left.tickValues(kP5LinearYMin, kP5LinearYMax, kAxisPixelLengthLeft);
    CHECK(!leftLevels.empty());
    CHECK((leftLevels[0].values == std::vector<double>{1.03, 1.04, 1.05, 1.06}));
    const auto leftStrings = left.tickStrings(leftLevels[0].values, left.autoSIPrefixScale(), leftLevels[0].spacing);
    CHECK((leftStrings == std::vector<QString>{
              QStringLiteral("1.03"),
              QStringLiteral("1.04"),
              QStringLiteral("1.05"),
              QStringLiteral("1.06"),
          }));

    AxisItem bottom(AxisItem::Orientation::Bottom);
    bottom.setGeometry(QRectF(0.0, 0.0, kAxisPixelLengthBottom, 60.0));
    bottom.setHeight(60.0);
    bottom.setLabel(QStringLiteral("Y Axis"), QStringLiteral("s"));
    bottom.setRange(kP5LogXMin, kP5LogXMax);
    bottom.setLogMode(true, false);

    CHECK(bottom.labelText() == QStringLiteral("Y Axis"));
    CHECK(bottom.labelUnits() == QStringLiteral("s"));
    CHECK(containsSemanticUnits(bottom.labelString(), QString::fromUtf8("µ"), QStringLiteral("s")));
    CHECK(bottom.labelUnitPrefix() == QString::fromUtf8("µ"));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0e6, 1.0e-9);

    const auto bottomLevels = bottom.tickValues(kP5LogXMin, kP5LogXMax, kAxisPixelLengthBottom);
    CHECK(bottomLevels.size() >= 2U);
    CHECK((bottomLevels[0].values == std::vector<double>{-7.0, -6.0, -5.0}));
    const auto bottomStrings = bottom.tickStrings(
        bottomLevels[0].values,
        bottom.autoSIPrefixScale(),
        bottomLevels[0].spacing);
    CHECK((bottomStrings == std::vector<QString>{
              QStringLiteral("0.1"),
              QStringLiteral("1"),
              QString::fromUtf8("10¹"),
          }));

    const double logTwo = std::log10(2.0);
    CHECK(std::any_of(bottomLevels[1].values.begin(), bottomLevels[1].values.end(), [logTwo](double value) {
        return std::abs(value - (-6.0 + logTwo)) < 1.0e-12;
    }));

    QImage probe(400, 320, QImage::Format_ARGB32_Premultiplied);
    probe.fill(Qt::black);
    QPainter painter(&probe);
    const auto leftSpecs = left.generateDrawSpecs(painter);
    const auto bottomSpecs = bottom.generateDrawSpecs(painter);
    painter.end();
    CHECK(leftSpecs.has_value());
    CHECK(bottomSpecs.has_value());
    CHECK(!leftSpecs->text.empty());
    CHECK(!bottomSpecs->text.empty());
    CHECK(std::any_of(leftSpecs->text.begin(), leftSpecs->text.end(), [](const AxisItem::TextSpec& spec) {
        return spec.text == QStringLiteral("1.04");
    }));
    CHECK(std::any_of(bottomSpecs->text.begin(), bottomSpecs->text.end(), [](const AxisItem::TextSpec& spec) {
        return spec.text == QString::fromUtf8("10¹");
    }));
    CHECK(tickPositionsIncreaseAlongBottomAxis(*bottomSpecs));

    return true;
}

bool testPlottingP5Wiring()
{
    using cppqtgraph::graphicsItems::AxisItem;

    auto example = cppqtgraph::examples::createPlottingExample();
    example.widget->resize(1000, 600);
    example.widget->show();
    QApplication::processEvents();

    example.plots[4]->setXRange(kP5LogXMin, kP5LogXMax, 0.0);
    example.plots[4]->setYRange(kP5LinearYMin, kP5LinearYMax, 0.0);
    QApplication::processEvents();

    auto* left = example.plots[4]->getAxis(QStringLiteral("left"));
    auto* bottom = example.plots[4]->getAxis(QStringLiteral("bottom"));
    CHECK(left != nullptr);
    CHECK(bottom != nullptr);

    CHECK_CLOSE(left->range().first, kP5LinearYMin, 1.0e-12);
    CHECK_CLOSE(left->range().second, kP5LinearYMax, 1.0e-12);
    CHECK_CLOSE(bottom->range().first, kP5LogXMin, 1.0e-12);
    CHECK_CLOSE(bottom->range().second, kP5LogXMax, 1.0e-12);

    CHECK(left->labelUnitPrefix().isEmpty());
    CHECK(bottom->labelUnitPrefix() == QString::fromUtf8("µ"));
    CHECK_CLOSE(left->autoSIPrefixScale(), 1.0, 1.0e-15);
    CHECK_CLOSE(bottom->autoSIPrefixScale(), 1.0e6, 1.0e-9);

    const auto bottomLevels = bottom->tickValues(kP5LogXMin, kP5LogXMax, kAxisPixelLengthBottom);
    CHECK((bottomLevels[0].values == std::vector<double>{-7.0, -6.0, -5.0}));
    const auto bottomStrings = bottom->tickStrings(
        bottomLevels[0].values,
        bottom->autoSIPrefixScale(),
        bottomLevels[0].spacing);
    CHECK((bottomStrings == std::vector<QString>{
              QStringLiteral("0.1"),
              QStringLiteral("1"),
              QString::fromUtf8("10¹"),
          }));

    left->setGeometry(QRectF(0.0, 0.0, 70.0, kAxisPixelLengthLeft));
    left->setWidth(70.0);
    bottom->setGeometry(QRectF(0.0, 0.0, kAxisPixelLengthBottom, 60.0));
    bottom->setHeight(60.0);

    QImage probe(400, 320, QImage::Format_ARGB32_Premultiplied);
    probe.fill(Qt::black);
    QPainter painter(&probe);
    const auto bottomSpecs = bottom->generateDrawSpecs(painter);
    painter.end();
    CHECK(bottomSpecs.has_value());
    CHECK(!bottomSpecs->ticks.empty());
    CHECK(!bottomSpecs->text.empty());
    CHECK(tickPositionsIncreaseAlongBottomAxis(*bottomSpecs));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testStandaloneP5LikeAxes()) {
        return 1;
    }
    if (!testPlottingP5Wiring()) {
        return 1;
    }

    return 0;
}
