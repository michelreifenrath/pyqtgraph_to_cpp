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
    qreal center = (range[0] + range[1]) / 2.0;

    if (maxSpanLimit.has_value() && span > *maxSpanLimit) {
        span = *maxSpanLimit;
        range = AxisRange{center - span / 2.0, center + span / 2.0};
    }
    if (minSpanLimit.has_value() && span < *minSpanLimit) {
        span = *minSpanLimit;
        range = AxisRange{center - span / 2.0, center + span / 2.0};
    }

    if (lowerLimit.has_value() && upperLimit.has_value()) {
        const qreal boundedSpan = *upperLimit - *lowerLimit;
        span = range[1] - range[0];
        if (span >= boundedSpan) {
            return AxisRange{*lowerLimit, *upperLimit};
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
    const qreal center = (requested[0] + requested[1]) / 2.0;
    if (span == 0.0) {
        span = previous[1] - previous[0];
        requested = AxisRange{center - span / 2.0, center + span / 2.0};
    }

    if (padding != 0.0) {
        span = requested[1] - requested[0];
        const qreal expansion = span * padding;
        requested[0] -= expansion;
        requested[1] += expansion;
    }

    return requested;
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
    Range2D nextTargetRange = targetRange_;

    if (xRange.has_value()) {
        nextTargetRange[xAxis] = clampAxisToLimits(normalizeRequestedRange(*xRange, targetRange_[xAxis], padding), limits_, xAxis);
    }
    if (yRange.has_value()) {
        nextTargetRange[yAxis] = clampAxisToLimits(normalizeRequestedRange(*yRange, targetRange_[yAxis], padding), limits_, yAxis);
    }

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
