#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
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

bool nearlyRelative(qreal lhs, qreal rhs)
{
    const qreal scale = std::abs(rhs) > 1.0 ? std::abs(rhs) : 1.0;
    return std::abs(lhs - rhs) <= scale * 1.0e-12;
}

qreal stableAxisCenter(const pyqtgraph::graphicsItems::ViewBox::AxisRange& range)
{
    const qreal span = range[1] - range[0];
    return std::isfinite(span) ? range[0] + span / 2.0 : range[0] / 2.0 + range[1] / 2.0;
}

bool checkFiniteIncreasingAxis(const pyqtgraph::graphicsItems::ViewBox::AxisRange& range)
{
    CHECK(std::isfinite(range[0]));
    CHECK(std::isfinite(range[1]));
    CHECK(range[0] < range[1]);
    return true;
}

bool checkRange(const pyqtgraph::graphicsItems::ViewBox::Range2D& range,
                qreal xMin,
                qreal xMax,
                qreal yMin,
                qreal yMax)
{
    CHECK(nearlyEqual(range[0][0], xMin));
    CHECK(nearlyEqual(range[0][1], xMax));
    CHECK(nearlyEqual(range[1][0], yMin));
    CHECK(nearlyEqual(range[1][1], yMax));
    return true;
}

bool checkRect(const QRectF& rect, qreal x, qreal y, qreal width, qreal height)
{
    CHECK(nearlyEqual(rect.x(), x));
    CHECK(nearlyEqual(rect.y(), y));
    CHECK(nearlyEqual(rect.width(), width));
    CHECK(nearlyEqual(rect.height(), height));
    return true;
}

bool testDefaultRangesAndCopies()
{
    pyqtgraph::graphicsItems::ViewBox viewBox;

    CHECK(checkRange(viewBox.viewRange(), 0.0, 1.0, 0.0, 1.0));
    CHECK(checkRange(viewBox.targetRange(), 0.0, 1.0, 0.0, 1.0));
    CHECK(checkRect(viewBox.viewRect(), 0.0, 0.0, 1.0, 1.0));
    CHECK(checkRect(viewBox.targetRect(), 0.0, 0.0, 1.0, 1.0));

    auto viewRange = viewBox.viewRange();
    viewRange[0][0] = 99.0;
    auto targetRange = viewBox.targetRange();
    targetRange[1][1] = 99.0;

    CHECK(checkRange(viewBox.viewRange(), 0.0, 1.0, 0.0, 1.0));
    CHECK(checkRange(viewBox.targetRange(), 0.0, 1.0, 0.0, 1.0));

    return true;
}

bool testAxisAndRectSetters()
{
    pyqtgraph::graphicsItems::ViewBox viewBox;

    viewBox.setXRange(2.0, 4.0, 0.0);
    CHECK(checkRange(viewBox.viewRange(), 2.0, 4.0, 0.0, 1.0));
    CHECK(checkRange(viewBox.targetRange(), 2.0, 4.0, 0.0, 1.0));

    viewBox.setYRange(-3.0, 7.0, 0.0);
    CHECK(checkRange(viewBox.viewRange(), 2.0, 4.0, -3.0, 7.0));
    CHECK(checkRange(viewBox.targetRange(), 2.0, 4.0, -3.0, 7.0));

    viewBox.setRange(QRectF(-1.0, 2.0, 4.0, 6.0), 0.0);
    CHECK(checkRange(viewBox.viewRange(), -1.0, 3.0, 2.0, 8.0));
    CHECK(checkRange(viewBox.targetRange(), -1.0, 3.0, 2.0, 8.0));
    CHECK(checkRect(viewBox.viewRect(), -1.0, 2.0, 4.0, 6.0));
    CHECK(checkRect(viewBox.targetRect(), -1.0, 2.0, 4.0, 6.0));

    viewBox.setRange(pyqtgraph::graphicsItems::ViewBox::AxisRange{10.0, 12.0}, std::nullopt, 0.0, false);
    CHECK(checkRange(viewBox.targetRange(), 10.0, 12.0, 2.0, 8.0));
    CHECK(checkRange(viewBox.viewRange(), -1.0, 3.0, 2.0, 8.0));

    return true;
}

bool testNormalizationAndValidation()
{
    pyqtgraph::graphicsItems::ViewBox viewBox;

    viewBox.setXRange(5.0, 1.0, 0.0);
    CHECK(checkRange(viewBox.viewRange(), 1.0, 5.0, 0.0, 1.0));

    pyqtgraph::graphicsItems::ViewBox zeroSpanViewBox;
    zeroSpanViewBox.setXRange(5.0, 5.0, 0.0);
    CHECK(checkRange(zeroSpanViewBox.viewRange(), 4.5, 5.5, 0.0, 1.0));

    pyqtgraph::graphicsItems::ViewBox zeroSpanDefaultPaddingX;
    zeroSpanDefaultPaddingX.setXRange(5.0, 5.0);
    CHECK(checkRange(zeroSpanDefaultPaddingX.viewRange(), 4.5, 5.5, 0.0, 1.0));
    CHECK(checkRange(zeroSpanDefaultPaddingX.targetRange(), 4.5, 5.5, 0.0, 1.0));

    pyqtgraph::graphicsItems::ViewBox zeroSpanDefaultPaddingY;
    zeroSpanDefaultPaddingY.setYRange(-2.0, -2.0);
    CHECK(checkRange(zeroSpanDefaultPaddingY.viewRange(), 0.0, 1.0, -2.5, -1.5));
    CHECK(checkRange(zeroSpanDefaultPaddingY.targetRange(), 0.0, 1.0, -2.5, -1.5));

    const auto beforeInvalidX = viewBox.viewRange();
    bool threw = false;
    try {
        viewBox.setXRange(0.0, std::numeric_limits<qreal>::infinity(), 0.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.viewRange() == beforeInvalidX);

    const auto beforeInvalidY = viewBox.viewRange();
    threw = false;
    try {
        viewBox.setYRange(std::numeric_limits<qreal>::quiet_NaN(), 1.0, 0.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.viewRange() == beforeInvalidY);

    return true;
}

bool testLargeFiniteRangeNormalization()
{
    pyqtgraph::graphicsItems::ViewBox largeZeroSpan;
    largeZeroSpan.setXRange(1.0e308, 1.0e308, 0.0);
    const auto largeXRange = largeZeroSpan.viewRange();
    CHECK(checkFiniteIncreasingAxis(largeXRange[0]));
    CHECK(nearlyRelative(stableAxisCenter(largeXRange[0]), 1.0e308));

    pyqtgraph::graphicsItems::ViewBox collapsedAtLargeOffset;
    const qreal largeOffset = 1.0e20;
    collapsedAtLargeOffset.setYRange(largeOffset, largeOffset + 1.0, 0.0);
    const auto largeYRange = collapsedAtLargeOffset.viewRange();
    CHECK(checkFiniteIncreasingAxis(largeYRange[1]));
    CHECK(nearlyRelative(stableAxisCenter(largeYRange[1]), largeOffset));

    pyqtgraph::graphicsItems::ViewBox rejectsOverflow;
    const auto beforeViewRange = rejectsOverflow.viewRange();
    const auto beforeTargetRange = rejectsOverflow.targetRange();
    bool threw = false;
    try {
        rejectsOverflow.setXRange(-1.0e308, 1.0e308, 0.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(rejectsOverflow.viewRange() == beforeViewRange);
    CHECK(rejectsOverflow.targetRange() == beforeTargetRange);

    return true;
}

bool testEmptyOptionalRangeThrowsAndPreservesState()
{
    pyqtgraph::graphicsItems::ViewBox viewBox;
    viewBox.setXRange(10.0, 12.0, 0.0, false);
    const auto beforeViewRange = viewBox.viewRange();
    const auto beforeTargetRange = viewBox.targetRange();

    bool threw = false;
    try {
        viewBox.setRange(std::nullopt, std::nullopt);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.viewRange() == beforeViewRange);
    CHECK(viewBox.targetRange() == beforeTargetRange);

    return true;
}

bool testLimitsClampPanningAndSpan()
{
    using ViewBox = pyqtgraph::graphicsItems::ViewBox;

    ViewBox bounded;
    ViewBox::Limits bounds;
    bounds.xMin = 0.0;
    bounds.xMax = 10.0;
    bounded.setLimits(bounds);
    CHECK(bounded.limits().xMin == 0.0);
    CHECK(bounded.limits().xMax == 10.0);

    bounded.setXRange(-2.0, 3.0, 0.0);
    CHECK(checkRange(bounded.viewRange(), 0.0, 5.0, 0.0, 1.0));
    bounded.setXRange(8.0, 12.0, 0.0);
    CHECK(checkRange(bounded.viewRange(), 6.0, 10.0, 0.0, 1.0));

    ViewBox minSpan;
    ViewBox::Limits minSpanLimits;
    minSpanLimits.minXRange = 4.0;
    minSpan.setLimits(minSpanLimits);
    minSpan.setXRange(1.0, 2.0, 0.0);
    CHECK(checkRange(minSpan.viewRange(), -0.5, 3.5, 0.0, 1.0));

    ViewBox maxSpan;
    ViewBox::Limits maxSpanLimits;
    maxSpanLimits.maxYRange = 4.0;
    maxSpan.setLimits(maxSpanLimits);
    maxSpan.setYRange(-10.0, 10.0, 0.0);
    CHECK(checkRange(maxSpan.viewRange(), 0.0, 1.0, -2.0, 2.0));

    return true;
}

bool testLargeFiniteHardLimitsCannotCollapseRanges()
{
    using ViewBox = pyqtgraph::graphicsItems::ViewBox;

    ViewBox lowerLimited;
    const auto beforeLimits = lowerLimited.limits();
    const auto beforeViewRange = lowerLimited.viewRange();
    const auto beforeTargetRange = lowerLimited.targetRange();

    ViewBox::Limits limits;
    limits.xMin = 1.0e308;

    bool threw = false;
    try {
        lowerLimited.setLimits(limits);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(lowerLimited.limits().xMin == beforeLimits.xMin);
    CHECK(lowerLimited.viewRange() == beforeViewRange);
    CHECK(lowerLimited.targetRange() == beforeTargetRange);

    return true;
}

bool testInvalidLimitsThrowAndPreserveState()
{
    using ViewBox = pyqtgraph::graphicsItems::ViewBox;
    ViewBox viewBox;

    ViewBox::Limits valid;
    valid.xMin = 0.0;
    valid.xMax = 10.0;
    viewBox.setLimits(valid);
    const auto beforeLimits = viewBox.limits();
    const auto beforeRange = viewBox.viewRange();

    ViewBox::Limits invalidBounds;
    invalidBounds.xMin = 10.0;
    invalidBounds.xMax = 0.0;
    bool threw = false;
    try {
        viewBox.setLimits(invalidBounds);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.limits().xMin == beforeLimits.xMin);
    CHECK(viewBox.limits().xMax == beforeLimits.xMax);
    CHECK(viewBox.viewRange() == beforeRange);

    ViewBox::Limits invalidSpan;
    invalidSpan.minYRange = 5.0;
    invalidSpan.maxYRange = 4.0;
    threw = false;
    try {
        viewBox.setLimits(invalidSpan);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.limits().xMin == beforeLimits.xMin);
    CHECK(viewBox.limits().xMax == beforeLimits.xMax);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testDefaultRangesAndCopies()) {
        return 1;
    }
    if (!testAxisAndRectSetters()) {
        return 1;
    }
    if (!testNormalizationAndValidation()) {
        return 1;
    }
    if (!testLargeFiniteRangeNormalization()) {
        return 1;
    }
    if (!testEmptyOptionalRangeThrowsAndPreservesState()) {
        return 1;
    }
    if (!testLimitsClampPanningAndSpan()) {
        return 1;
    }
    if (!testLargeFiniteHardLimitsCannotCollapseRanges()) {
        return 1;
    }
    if (!testInvalidLimitsThrowAndPreserveState()) {
        return 1;
    }

    return 0;
}
