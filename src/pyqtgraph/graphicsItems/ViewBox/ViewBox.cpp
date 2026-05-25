// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace pyqtgraph::graphicsItems {
namespace {

using AxisRange = ViewBox::AxisRange;
using Limits = ViewBox::Limits;
using Range2D = ViewBox::Range2D;

constexpr int xAxis = ViewBox::XAxis;
constexpr int yAxis = ViewBox::YAxis;

bool isFinite(qreal value)
{
    return std::isfinite(static_cast<double>(value));
}

bool rangeIsFinite(const AxisRange& range)
{
    return isFinite(range[0]) && isFinite(range[1]);
}

void validateFiniteRange(const AxisRange& range, const char* message)
{
    if (!rangeIsFinite(range)) {
        throw std::invalid_argument(message);
    }
}

void validateFiniteIncreasingRange(const AxisRange& range, const char* message)
{
    validateFiniteRange(range, message);
    if (range[1] <= range[0]) {
        throw std::invalid_argument(message);
    }
}

void validateFiniteRange(const Range2D& range, const char* message)
{
    validateFiniteRange(range[xAxis], message);
    validateFiniteRange(range[yAxis], message);
}

qreal stableCenter(const AxisRange& range)
{
    const qreal span = range[1] - range[0];
    if (isFinite(span)) {
        const qreal center = range[0] + span / 2.0;
        if (isFinite(center)) {
            return center;
        }
    }

    const qreal center = range[0] / 2.0 + range[1] / 2.0;
    if (!isFinite(center)) {
        throw std::invalid_argument("range center must be finite");
    }
    return center;
}

qreal quantizationLimit(qreal center)
{
    const qreal magnitude = std::abs(center);
    if (!isFinite(magnitude) || magnitude == 0.0) {
        return 0.0;
    }

    const qreal limit = magnitude * 3.0e-15;
    if (!isFinite(limit) || limit <= 0.0) {
        return 0.0;
    }
    return limit;
}

std::optional<AxisRange> expandedAroundCenter(qreal center, qreal halfSpan)
{
    if (!isFinite(center) || !isFinite(halfSpan) || halfSpan <= 0.0) {
        return std::nullopt;
    }

    const AxisRange symmetric{center - halfSpan, center + halfSpan};
    if (rangeIsFinite(symmetric) && symmetric[0] < symmetric[1]) {
        return symmetric;
    }

    if (center > 0.0) {
        const AxisRange below{center - halfSpan, center};
        if (rangeIsFinite(below) && below[0] < below[1]) {
            return below;
        }
    } else if (center < 0.0) {
        const AxisRange above{center, center + halfSpan};
        if (rangeIsFinite(above) && above[0] < above[1]) {
            return above;
        }
    }

    return std::nullopt;
}

AxisRange rangeAroundCenter(qreal center, qreal halfSpan)
{
    if (auto range = expandedAroundCenter(center, halfSpan)) {
        return *range;
    }

    const qreal quantizedHalfSpan = quantizationLimit(center);
    if (auto range = expandedAroundCenter(center, quantizedHalfSpan)) {
        return *range;
    }

    if (auto range = expandedAroundCenter(center, 0.5)) {
        return *range;
    }

    throw std::invalid_argument("cannot construct finite range around center");
}

AxisRange expandCollapsedRange(qreal center, const AxisRange& previous)
{
    const qreal previousSpan = previous[1] - previous[0];
    if (isFinite(previousSpan) && previousSpan > 0.0) {
        if (auto range = expandedAroundCenter(center, previousSpan / 2.0)) {
            return *range;
        }
    }

    return rangeAroundCenter(center, 0.0);
}

AxisRange ensureQuantizedRange(AxisRange range)
{
    validateFiniteRange(range, "range endpoints must be finite");
    if (range[1] <= range[0]) {
        throw std::invalid_argument("range endpoints must be increasing");
    }

    const qreal span = range[1] - range[0];
    if (!isFinite(span)) {
        throw std::invalid_argument("range span must be finite");
    }

    const qreal limit = quantizationLimit(stableCenter(range));
    if (limit > 0.0 && span < 2.0 * limit) {
        return rangeAroundCenter(stableCenter(range), limit);
    }

    return range;
}

void validateFiniteOptional(const std::optional<qreal>& value, const char* name)
{
    if (value.has_value() && !isFinite(*value)) {
        throw std::invalid_argument(name);
    }
}

void validateLimits(const Limits& limits)
{
    validateFiniteOptional(limits.xMin, "xMin must be finite");
    validateFiniteOptional(limits.xMax, "xMax must be finite");
    validateFiniteOptional(limits.yMin, "yMin must be finite");
    validateFiniteOptional(limits.yMax, "yMax must be finite");
    validateFiniteOptional(limits.minXRange, "minXRange must be finite");
    validateFiniteOptional(limits.maxXRange, "maxXRange must be finite");
    validateFiniteOptional(limits.minYRange, "minYRange must be finite");
    validateFiniteOptional(limits.maxYRange, "maxYRange must be finite");

    if (limits.xMin.has_value() && limits.xMax.has_value() && *limits.xMin > *limits.xMax) {
        throw std::invalid_argument("xMin must be <= xMax");
    }
    if (limits.yMin.has_value() && limits.yMax.has_value() && *limits.yMin > *limits.yMax) {
        throw std::invalid_argument("yMin must be <= yMax");
    }

    const std::array rangeValues{limits.minXRange, limits.maxXRange, limits.minYRange, limits.maxYRange};
    for (const auto& value : rangeValues) {
        if (value.has_value() && *value < 0.0) {
            throw std::invalid_argument("range limits must be non-negative");
        }
    }

    if (limits.minXRange.has_value() && limits.maxXRange.has_value() && *limits.minXRange > *limits.maxXRange) {
        throw std::invalid_argument("minXRange must be <= maxXRange");
    }
    if (limits.minYRange.has_value() && limits.maxYRange.has_value() && *limits.minYRange > *limits.maxYRange) {
        throw std::invalid_argument("minYRange must be <= maxYRange");
    }
}

QRectF rectFromRange(const Range2D& range)
{
    return QRectF(range[xAxis][0], range[yAxis][0], range[xAxis][1] - range[xAxis][0], range[yAxis][1] - range[yAxis][0]);
}

AxisRange clampAxisToLimits(AxisRange range,
                            const std::optional<qreal>& lowerLimit,
                            const std::optional<qreal>& upperLimit,
                            const std::optional<qreal>& minSpanLimit,
                            const std::optional<qreal>& maxSpanLimit)
{
    qreal span = range[1] - range[0];
    const qreal center = stableCenter(range);

    if (maxSpanLimit.has_value() && span > *maxSpanLimit) {
        span = *maxSpanLimit;
        range = rangeAroundCenter(center, span / 2.0);
    }
    if (minSpanLimit.has_value() && span < *minSpanLimit) {
        span = *minSpanLimit;
        range = rangeAroundCenter(center, span / 2.0);
    }

    if (lowerLimit.has_value() && upperLimit.has_value()) {
        const qreal boundedSpan = *upperLimit - *lowerLimit;
        span = range[1] - range[0];
        if (span >= boundedSpan) {
            AxisRange boundedRange{*lowerLimit, *upperLimit};
            validateFiniteIncreasingRange(boundedRange, "range limits produced non-increasing range");
            return boundedRange;
        }
    }

    if (lowerLimit.has_value() && range[0] < *lowerLimit) {
        const qreal delta = *lowerLimit - range[0];
        range[0] += delta;
        range[1] += delta;
    }
    if (upperLimit.has_value() && range[1] > *upperLimit) {
        const qreal delta = range[1] - *upperLimit;
        range[0] -= delta;
        range[1] -= delta;
    }
    if (lowerLimit.has_value() && range[0] < *lowerLimit) {
        range[0] = *lowerLimit;
    }
    if (upperLimit.has_value() && range[1] > *upperLimit) {
        range[1] = *upperLimit;
    }

    validateFiniteIncreasingRange(range, "range limits produced non-increasing range");
    return range;
}

AxisRange clampAxisToLimits(AxisRange range, const Limits& limits, int axis)
{
    if (axis == xAxis) {
        return clampAxisToLimits(range, limits.xMin, limits.xMax, limits.minXRange, limits.maxXRange);
    }
    return clampAxisToLimits(range, limits.yMin, limits.yMax, limits.minYRange, limits.maxYRange);
}

Range2D clampRangeToLimits(Range2D range, const Limits& limits)
{
    range[xAxis] = clampAxisToLimits(range[xAxis], limits, xAxis);
    range[yAxis] = clampAxisToLimits(range[yAxis], limits, yAxis);
    return range;
}

AxisRange normalizeRequestedRange(AxisRange requested, AxisRange previous, qreal padding)
{
    if (!isFinite(requested[0]) || !isFinite(requested[1]) || !isFinite(padding)) {
        throw std::invalid_argument("range endpoints and padding must be finite");
    }

    if (requested[1] < requested[0]) {
        std::swap(requested[0], requested[1]);
    }

    qreal span = requested[1] - requested[0];
    const qreal center = stableCenter(requested);
    bool preservingPreviousSpan = false;
    if (span == 0.0) {
        requested = expandCollapsedRange(center, previous);
        preservingPreviousSpan = true;
    } else {
        requested = ensureQuantizedRange(requested);
    }

    if (padding != 0.0 && !preservingPreviousSpan) {
        span = requested[1] - requested[0];
        if (!isFinite(span)) {
            throw std::invalid_argument("range span and padding must produce finite endpoints");
        }
        const qreal expansion = span * padding;
        if (!isFinite(expansion)) {
            throw std::invalid_argument("range padding must produce finite endpoints");
        }
        requested[0] -= expansion;
        requested[1] += expansion;
        validateFiniteRange(requested, "range padding produced non-finite endpoints");
        if (requested[0] == requested[1]) {
            requested = expandCollapsedRange(stableCenter(requested), previous);
        } else if (requested[1] < requested[0]) {
            throw std::invalid_argument("range padding produced inverted endpoints");
        }
    }

    return ensureQuantizedRange(requested);
}

} // namespace

ViewBox::ViewBox(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
{
}

ViewBox::~ViewBox() = default;

Range2D ViewBox::viewRange() const
{
    return viewRange_;
}

Range2D ViewBox::targetRange() const
{
    return targetRange_;
}

QRectF ViewBox::viewRect() const
{
    return rectFromRange(viewRange_);
}

QRectF ViewBox::targetRect() const
{
    return rectFromRange(targetRange_);
}

Limits ViewBox::limits() const
{
    return limits_;
}

void ViewBox::setRange(const QRectF& rect, qreal padding, bool update, bool disableAutoRange)
{
    setRange(AxisRange{rect.left(), rect.right()}, AxisRange{rect.top(), rect.bottom()}, padding, update, disableAutoRange);
}

void ViewBox::setRange(std::optional<AxisRange> xRange,
                       std::optional<AxisRange> yRange,
                       qreal padding,
                       bool update,
                       bool disableAutoRange)
{
    if (!xRange.has_value() && !yRange.has_value()) {
        throw std::invalid_argument("at least one axis range must be provided");
    }

    Range2D nextTargetRange = targetRange_;

    if (xRange.has_value()) {
        nextTargetRange[xAxis] = clampAxisToLimits(normalizeRequestedRange(*xRange, targetRange_[xAxis], padding), limits_, xAxis);
    }
    if (yRange.has_value()) {
        nextTargetRange[yAxis] = clampAxisToLimits(normalizeRequestedRange(*yRange, targetRange_[yAxis], padding), limits_, yAxis);
    }

    validateFiniteRange(nextTargetRange, "setRange produced non-finite target range");
    targetRange_ = nextTargetRange;

    if (disableAutoRange) {
        if (xRange.has_value()) {
            autoRange_[xAxis] = false;
        }
        if (yRange.has_value()) {
            autoRange_[yAxis] = false;
        }
    }

    if (update) {
        viewRange_ = targetRange_;
    }
}

void ViewBox::setXRange(qreal min, qreal max, qreal padding, bool update)
{
    setRange(AxisRange{min, max}, std::nullopt, padding, update);
}

void ViewBox::setYRange(qreal min, qreal max, qreal padding, bool update)
{
    setRange(std::nullopt, AxisRange{min, max}, padding, update);
}

void ViewBox::setLimits(const Limits& limits)
{
    validateLimits(limits);

    const Range2D nextTargetRange = clampRangeToLimits(targetRange_, limits);
    const Range2D nextViewRange = clampRangeToLimits(viewRange_, limits);

    limits_ = limits;
    targetRange_ = nextTargetRange;
    viewRange_ = nextViewRange;
}

} // namespace pyqtgraph::graphicsItems
