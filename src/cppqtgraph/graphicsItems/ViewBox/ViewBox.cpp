// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp"
#include "../../../../include/cppqtgraph/GraphicsScene/GraphicsScene.hpp"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGraphicsSceneWheelEvent>
#include <QtWidgets/QGraphicsView>

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace cppqtgraph::graphicsItems {
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

void addSceneItem(QGraphicsScene* scene, QGraphicsItem* item)
{
    if (auto* graphicsScene = qobject_cast<::cppqtgraph::GraphicsScene::GraphicsScene*>(scene)) {
        graphicsScene->addItem(item);
        return;
    }

    scene->addItem(item);
}

void removeSceneItem(QGraphicsScene* scene, QGraphicsItem* item)
{
    if (auto* graphicsScene = qobject_cast<::cppqtgraph::GraphicsScene::GraphicsScene*>(scene)) {
        graphicsScene->removeItem(item);
        return;
    }

    scene->removeItem(item);
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

std::optional<qreal> safeStableCenter(const AxisRange& range)
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
        return std::nullopt;
    }
    return center;
}

std::optional<AxisRange> safeRangeAroundCenter(qreal center, qreal halfSpan)
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

    return std::nullopt;
}

std::optional<AxisRange> safeExpandCollapsedRange(qreal center, const AxisRange& previous)
{
    if (!isFinite(center)) {
        return std::nullopt;
    }

    const qreal previousSpan = previous[1] - previous[0];
    if (isFinite(previousSpan) && previousSpan > 0.0) {
        if (auto range = expandedAroundCenter(center, previousSpan / 2.0)) {
            return *range;
        }
    }

    return safeRangeAroundCenter(center, 0.0);
}

std::optional<AxisRange> safeEnsureQuantizedRange(AxisRange range)
{
    if (!rangeIsFinite(range) || range[1] <= range[0]) {
        return std::nullopt;
    }

    const qreal span = range[1] - range[0];
    if (!isFinite(span)) {
        return std::nullopt;
    }

    const auto center = safeStableCenter(range);
    if (!center.has_value()) {
        return std::nullopt;
    }

    const qreal limit = quantizationLimit(*center);
    if (limit > 0.0 && span < 2.0 * limit) {
        return safeRangeAroundCenter(*center, limit);
    }

    return range;
}

std::optional<AxisRange> safeNormalizeRequestedRange(AxisRange requested, AxisRange previous, qreal padding)
{
    if (!isFinite(requested[0]) || !isFinite(requested[1]) || !isFinite(padding)) {
        return std::nullopt;
    }

    if (requested[1] < requested[0]) {
        std::swap(requested[0], requested[1]);
    }

    qreal span = requested[1] - requested[0];
    bool preservingPreviousSpan = false;
    if (span == 0.0) {
        const auto center = safeStableCenter(requested);
        if (!center.has_value()) {
            return std::nullopt;
        }
        const auto expanded = safeExpandCollapsedRange(*center, previous);
        if (!expanded.has_value()) {
            return std::nullopt;
        }
        requested = *expanded;
        preservingPreviousSpan = true;
    } else {
        const auto quantized = safeEnsureQuantizedRange(requested);
        if (!quantized.has_value()) {
            return std::nullopt;
        }
        requested = *quantized;
    }

    if (padding != 0.0 && !preservingPreviousSpan) {
        span = requested[1] - requested[0];
        if (!isFinite(span)) {
            return std::nullopt;
        }
        const qreal expansion = span * padding;
        if (!isFinite(expansion)) {
            return std::nullopt;
        }
        requested[0] -= expansion;
        requested[1] += expansion;
        if (!rangeIsFinite(requested)) {
            return std::nullopt;
        }
        if (requested[0] == requested[1]) {
            const auto center = safeStableCenter(requested);
            if (!center.has_value()) {
                return std::nullopt;
            }
            const auto expanded = safeExpandCollapsedRange(*center, previous);
            if (!expanded.has_value()) {
                return std::nullopt;
            }
            requested = *expanded;
        } else if (requested[1] < requested[0]) {
            return std::nullopt;
        }
    }

    return safeEnsureQuantizedRange(requested);
}

std::optional<AxisRange> safeClampAxisToLimits(AxisRange range,
                                               const std::optional<qreal>& lowerLimit,
                                               const std::optional<qreal>& upperLimit,
                                               const std::optional<qreal>& minSpanLimit,
                                               const std::optional<qreal>& maxSpanLimit)
{
    if (!rangeIsFinite(range) || range[1] <= range[0]) {
        return std::nullopt;
    }

    const auto center = safeStableCenter(range);
    if (!center.has_value()) {
        return std::nullopt;
    }

    qreal span = range[1] - range[0];

    if (maxSpanLimit.has_value() && span > *maxSpanLimit) {
        span = *maxSpanLimit;
        const auto bounded = safeRangeAroundCenter(*center, span / 2.0);
        if (!bounded.has_value()) {
            return std::nullopt;
        }
        range = *bounded;
    }
    if (minSpanLimit.has_value() && span < *minSpanLimit) {
        span = *minSpanLimit;
        const auto bounded = safeRangeAroundCenter(*center, span / 2.0);
        if (!bounded.has_value()) {
            return std::nullopt;
        }
        range = *bounded;
    }

    if (lowerLimit.has_value() && upperLimit.has_value()) {
        const qreal boundedSpan = *upperLimit - *lowerLimit;
        span = range[1] - range[0];
        if (span >= boundedSpan) {
            AxisRange boundedRange{*lowerLimit, *upperLimit};
            if (!rangeIsFinite(boundedRange) || boundedRange[1] <= boundedRange[0]) {
                return std::nullopt;
            }
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

    if (!rangeIsFinite(range) || range[1] <= range[0]) {
        return std::nullopt;
    }
    return range;
}

std::optional<AxisRange> safeClampAxisToLimits(AxisRange range, const Limits& limits, int axis)
{
    if (axis == xAxis) {
        return safeClampAxisToLimits(range, limits.xMin, limits.xMax, limits.minXRange, limits.maxXRange);
    }
    return safeClampAxisToLimits(range, limits.yMin, limits.yMax, limits.minYRange, limits.maxYRange);
}

void validateFiniteOptional(const std::optional<qreal>& value, const char* name)
{
    if (value.has_value() && !isFinite(*value)) {
        throw std::invalid_argument(name);
    }
}

void validateFinitePoint(const QPointF& point, const char* message)
{
    if (!isFinite(point.x()) || !isFinite(point.y())) {
        throw std::invalid_argument(message);
    }
}

void validateFiniteOptionalPoint(const std::optional<QPointF>& point, const char* message)
{
    if (point.has_value()) {
        validateFinitePoint(*point, message);
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

qreal axisSpan(const AxisRange& range)
{
    return range[1] - range[0];
}

bool axisIsValid(int axis)
{
    return axis == ViewBox::XAxis || axis == ViewBox::YAxis || axis == ViewBox::XYAxes;
}

bool linkAxisIsValid(int axis)
{
    return axis == ViewBox::XAxis || axis == ViewBox::YAxis;
}

bool axisRangeChanged(const AxisRange& before, const AxisRange& after)
{
    const qreal threshold = std::abs(axisSpan(after)) * 1.0e-9;
    return std::abs(after[0] - before[0]) > threshold || std::abs(after[1] - before[1]) > threshold;
}

AxisRange expandedAxis(AxisRange range, qreal delta)
{
    range[0] -= delta;
    range[1] += delta;
    return range;
}

AxisRange optionalRangeOrTarget(const std::optional<AxisRange>& range, const AxisRange& target)
{
    return range.value_or(target);
}

AxisRange normalizeBoundsAxis(qreal minimum, qreal maximum)
{
    AxisRange range{minimum, maximum};
    if (range[1] < range[0]) {
        std::swap(range[0], range[1]);
    }
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
    , childGroup_(this)
{
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    childGroup_.setHandlesChildEvents(false);
    updateAutoRangeSceneConnection();
}

ViewBox::~ViewBox()
{
    for (auto& axisConnections : linkConnections_) {
        for (auto& connection : axisConnections) {
            QObject::disconnect(connection);
        }
    }
    QObject::disconnect(sceneChangedConnection_);
    for (auto* child : childGroup_.childItems()) {
        child->setParentItem(nullptr);
        if (auto* childScene = child->scene(); childScene != nullptr) {
            removeSceneItem(childScene, child);
        }
    }
    addedItems_.clear();
}

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

void ViewBox::addItem(QGraphicsItem* item, bool ignoreBounds)
{
    if (item == nullptr) {
        throw std::invalid_argument("addItem requires a non-null item");
    }

    if (item->zValue() < zValue()) {
        item->setZValue(zValue() + 1.0);
    }

    if (auto* itemScene = item->scene(); itemScene != nullptr && itemScene != scene()) {
        removeSceneItem(itemScene, item);
    }
    if (scene() != nullptr && item->scene() != scene()) {
        addSceneItem(scene(), item);
    }

    item->setParentItem(&childGroup_);
    if (!ignoreBounds && std::find(addedItems_.begin(), addedItems_.end(), item) == addedItems_.end()) {
        addedItems_.push_back(item);
        if (autoRange_[xAxis] || autoRange_[yAxis]) {
            applyAutoRange(defaultPadding_, autoRange_);
        }
    }
}

void ViewBox::removeItem(QGraphicsItem* item)
{
    if (item == nullptr) {
        return;
    }

    const bool tracked = std::find(addedItems_.begin(), addedItems_.end(), item) != addedItems_.end();
    const bool parented = item->parentItem() == &childGroup_;
    if (!tracked && !parented) {
        return;
    }

    addedItems_.erase(std::remove(addedItems_.begin(), addedItems_.end(), item), addedItems_.end());
    if (parented) {
        item->setParentItem(nullptr);
    }
    if (auto* itemScene = item->scene(); itemScene != nullptr && itemScene == scene()) {
        removeSceneItem(itemScene, item);
    }
    if (tracked && (autoRange_[xAxis] || autoRange_[yAxis])) {
        applyAutoRange(defaultPadding_, autoRange_);
    }
}

void ViewBox::clear()
{
    pruneAddedItems();
    const auto items = addedItems_;
    for (auto* item : items) {
        removeItem(item);
    }
    for (auto* child : childGroup_.childItems()) {
        child->setParentItem(nullptr);
        if (auto* childScene = child->scene(); childScene != nullptr) {
            removeSceneItem(childScene, child);
        }
    }
    addedItems_.clear();
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
        const auto previousAutoRange = autoRange_;
        if (xRange.has_value()) {
            autoRange_[xAxis] = false;
        }
        if (yRange.has_value()) {
            autoRange_[yAxis] = false;
        }
        if (previousAutoRange != autoRange_) {
            emit sigStateChanged(this);
        }
    }

    if (update) {
        const bool lockX = xRange.has_value() && !yRange.has_value();
        const bool lockY = yRange.has_value() && !xRange.has_value();
        updateViewRange(lockX, lockY);
    } else {
        markMatrixDirty();
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

void ViewBox::scaleBy(std::optional<qreal> x, std::optional<qreal> y, std::optional<QPointF> center)
{
    if (!x.has_value() && !y.has_value()) {
        return;
    }
    validateFiniteOptional(x, "x scale must be finite");
    validateFiniteOptional(y, "y scale must be finite");
    validateFiniteOptionalPoint(center, "scale center must be finite");

    const QRectF rect = targetRect();
    const QPointF scale{x.value_or(1.0), y.value_or(1.0)};
    const QPointF scaleCenter = center.value_or(rect.center());
    const QPointF topLeft = scaleCenter + QPointF((rect.left() - scaleCenter.x()) * scale.x(), (rect.top() - scaleCenter.y()) * scale.y());
    const QPointF bottomRight = scaleCenter + QPointF((rect.right() - scaleCenter.x()) * scale.x(), (rect.bottom() - scaleCenter.y()) * scale.y());

    if (x.has_value() && y.has_value()) {
        setRange(QRectF(topLeft, bottomRight), 0.0);
    } else if (x.has_value()) {
        setXRange(topLeft.x(), bottomRight.x(), 0.0);
    } else {
        setYRange(topLeft.y(), bottomRight.y(), 0.0);
    }
}

void ViewBox::scaleBy(const QPointF& scale, std::optional<QPointF> center)
{
    validateFinitePoint(scale, "scale must be finite");
    scaleBy(scale.x(), scale.y(), center);
}

void ViewBox::translateBy(std::optional<qreal> x, std::optional<qreal> y)
{
    if (!x.has_value() && !y.has_value()) {
        return;
    }
    validateFiniteOptional(x, "x translation must be finite");
    validateFiniteOptional(y, "y translation must be finite");

    const QRectF rect = targetRect();
    const std::optional<AxisRange> xRange = x.has_value()
        ? std::optional<AxisRange>{AxisRange{rect.left() + *x, rect.right() + *x}}
        : std::nullopt;
    const std::optional<AxisRange> yRange = y.has_value()
        ? std::optional<AxisRange>{AxisRange{rect.top() + *y, rect.bottom() + *y}}
        : std::nullopt;
    setRange(xRange, yRange, 0.0);
}

void ViewBox::translateBy(const QPointF& offset)
{
    validateFinitePoint(offset, "translation offset must be finite");
    setRange(targetRect().translated(offset), 0.0);
}

void ViewBox::setLimits(const Limits& limits)
{
    validateLimits(limits);

    const Range2D previousViewRange = viewRange_;
    const Range2D nextTargetRange = clampRangeToLimits(targetRange_, limits);
    const Range2D nextViewRange = clampRangeToLimits(viewRange_, limits);

    limits_ = limits;
    targetRange_ = nextTargetRange;
    if (viewRange_ != nextViewRange) {
        markMatrixDirty();
    }
    viewRange_ = nextViewRange;

    const std::array<bool, 2> changed{{
        axisRangeChanged(previousViewRange[xAxis], viewRange_[xAxis]),
        axisRangeChanged(previousViewRange[yAxis], viewRange_[yAxis]),
    }};
    if (changed[xAxis] || changed[yAxis]) {
        notifyLinkedViews(changed);
        emitRangeChanges(changed);
    }
}

void ViewBox::setMouseMode(int mode)
{
    if (mode != PanMode && mode != RectMode) {
        throw std::invalid_argument("mouse mode must be PanMode or RectMode");
    }
    if (mouseMode_ == mode) {
        return;
    }
    mouseMode_ = mode;
    emit sigStateChanged(this);
}

int ViewBox::mouseMode() const
{
    return mouseMode_;
}

void ViewBox::setMouseEnabled(std::optional<bool> x, std::optional<bool> y)
{
    bool changed = false;
    if (x.has_value() && mouseEnabled_[xAxis] != *x) {
        mouseEnabled_[xAxis] = *x;
        changed = true;
    }
    if (y.has_value() && mouseEnabled_[yAxis] != *y) {
        mouseEnabled_[yAxis] = *y;
        changed = true;
    }
    if (changed) {
        emit sigStateChanged(this);
    }
}

std::array<bool, 2> ViewBox::mouseEnabled() const
{
    return mouseEnabled_;
}

void ViewBox::setWheelScaleFactor(qreal factor)
{
    if (!isFinite(factor)) {
        throw std::invalid_argument("wheel scale factor must be finite");
    }
    if (wheelScaleFactor_ == factor) {
        return;
    }
    wheelScaleFactor_ = factor;
    emit sigStateChanged(this);
}

qreal ViewBox::wheelScaleFactor() const
{
    return wheelScaleFactor_;
}

void ViewBox::setXLink(ViewBox* view)
{
    linkView(xAxis, view);
}

void ViewBox::setYLink(ViewBox* view)
{
    linkView(yAxis, view);
}

void ViewBox::linkView(int axis, ViewBox* view)
{
    if (!linkAxisIsValid(axis)) {
        throw std::invalid_argument("linked axis must be XAxis or YAxis");
    }
    if (view == this) {
        throw std::invalid_argument("ViewBox cannot link an axis to itself");
    }

    for (auto& connection : linkConnections_[axis]) {
        QObject::disconnect(connection);
        connection = QMetaObject::Connection{};
    }

    linkedViews_[axis] = view;
    if (view != nullptr) {
        if (axis == xAxis) {
            linkConnections_[axis][0] = QObject::connect(view, &ViewBox::sigXRangeChanged, this, [this, view](ViewBox*, AxisRange) {
                linkedViewChanged(view, xAxis);
            });
        } else {
            linkConnections_[axis][0] = QObject::connect(view, &ViewBox::sigYRangeChanged, this, [this, view](ViewBox*, AxisRange) {
                linkedViewChanged(view, yAxis);
            });
        }
        linkConnections_[axis][1] = QObject::connect(view, &ViewBox::sigResized, this, [this, view, axis](ViewBox*) {
            linkedViewChanged(view, axis);
        });
        linkedViewChanged(view, axis);
    }

    emit sigStateChanged(this);
}

ViewBox* ViewBox::linkedView(int axis) const
{
    if (!linkAxisIsValid(axis)) {
        throw std::invalid_argument("linked axis must be XAxis or YAxis");
    }
    return linkedViews_[axis].data();
}

void ViewBox::linkedViewChanged(ViewBox* view, int axis)
{
    if (!linkAxisIsValid(axis)) {
        throw std::invalid_argument("linked axis must be XAxis or YAxis");
    }
    if (linksBlocked_ || view == nullptr) {
        return;
    }

    const QRectF sourceRange = view->viewRect();
    const QRectF sourceGeometry = view->screenGeometry();
    const QRectF targetGeometry = screenGeometry();

    view->blockLink(true);
    try {
        if (axis == xAxis) {
            qreal x1 = sourceRange.left();
            qreal x2 = sourceRange.right();
            if (sourceGeometry.isValid() && targetGeometry.isValid() && sourceGeometry.width() > 0.0 && targetGeometry.width() > 0.0) {
                const qreal overlap = std::min(targetGeometry.right(), sourceGeometry.right()) - std::max(targetGeometry.left(), sourceGeometry.left());
                if (overlap >= std::min(sourceGeometry.width() / 3.0, targetGeometry.width() / 3.0)) {
                    const qreal unitsPerPixel = sourceRange.width() / sourceGeometry.width();
                    x1 = xInverted_ ? sourceRange.left() + (targetGeometry.right() - sourceGeometry.right()) * unitsPerPixel
                                     : sourceRange.left() + (targetGeometry.left() - sourceGeometry.left()) * unitsPerPixel;
                    x2 = x1 + targetGeometry.width() * unitsPerPixel;
                }
            }
            enableAutoRange(xAxis, false);
            setXRange(x1, x2, 0.0);
        } else {
            qreal y1 = sourceRange.top();
            qreal y2 = sourceRange.bottom();
            if (sourceGeometry.isValid() && targetGeometry.isValid() && sourceGeometry.height() > 0.0 && targetGeometry.height() > 0.0) {
                const qreal overlap = std::min(targetGeometry.bottom(), sourceGeometry.bottom()) - std::max(targetGeometry.top(), sourceGeometry.top());
                if (overlap >= std::min(sourceGeometry.height() / 3.0, targetGeometry.height() / 3.0)) {
                    const qreal unitsPerPixel = sourceRange.height() / sourceGeometry.height();
                    y2 = yInverted_ ? sourceRange.bottom() + (targetGeometry.bottom() - sourceGeometry.bottom()) * unitsPerPixel
                                     : sourceRange.bottom() - (targetGeometry.top() - sourceGeometry.top()) * unitsPerPixel;
                    y1 = y2 - targetGeometry.height() * unitsPerPixel;
                }
            }
            enableAutoRange(yAxis, false);
            setYRange(y1, y2, 0.0);
        }
    } catch (...) {
        view->blockLink(false);
        throw;
    }
    view->blockLink(false);
}

void ViewBox::blockLink(bool block)
{
    linksBlocked_ = block;
}

void ViewBox::autoRange(std::optional<qreal> padding)
{
    const qreal effectivePadding = padding.value_or(defaultPadding_);
    if (!isFinite(effectivePadding)) {
        throw std::invalid_argument("padding must be finite");
    }
    applyAutoRange(padding, std::array<bool, 2>{{true, true}}, true);
    const auto previousAutoRange = autoRange_;
    autoRange_[xAxis] = false;
    autoRange_[yAxis] = false;
    if (previousAutoRange != autoRange_) {
        emit sigStateChanged(this);
    }
}

void ViewBox::enableAutoRange(int axis, bool enable)
{
    if (!axisIsValid(axis)) {
        throw std::invalid_argument("axis must be XAxis, YAxis, or XYAxes");
    }

    const auto previousAutoRange = autoRange_;

    if (!enable) {
        const std::array<bool, 2> axes{{
            axis == XYAxes ? autoRange_[xAxis] : axis == XAxis && autoRange_[xAxis],
            axis == XYAxes ? autoRange_[yAxis] : axis == YAxis && autoRange_[yAxis],
        }};
        applyAutoRange(defaultPadding_, axes);
    }

    bool shouldAutoRange = false;
    if (axis == XYAxes) {
        shouldAutoRange = enable && (!autoRange_[xAxis] || !autoRange_[yAxis]);
        autoRange_[xAxis] = enable;
        autoRange_[yAxis] = enable;
    } else {
        shouldAutoRange = enable && !autoRange_[axis];
        autoRange_[axis] = enable;
    }

    if (shouldAutoRange) {
        applyAutoRange(defaultPadding_, autoRange_);
    }

    if (previousAutoRange != autoRange_) {
        emit sigStateChanged(this);
    }
}

void ViewBox::disableAutoRange(int axis)
{
    enableAutoRange(axis, false);
}

std::array<bool, 2> ViewBox::autoRangeEnabled() const
{
    return autoRange_;
}

ViewBoxMenu* ViewBox::menu() noexcept
{
    return menu_.get();
}

const ViewBoxMenu* ViewBox::menu() const noexcept
{
    return menu_.get();
}

void ViewBox::setMenuEnabled(bool enable)
{
    menuEnabled_ = enable;
    if (!enable) {
        menu_.reset();
    }
}

bool ViewBox::menuEnabled() const noexcept
{
    return menuEnabled_;
}

void ViewBox::raiseContextMenu(const QPoint& globalPos)
{
    if (!menuEnabled_) {
        return;
    }
    ensureMenu();
    if (menu_ == nullptr) {
        return;
    }
    ++contextMenuRaiseCount_;
    menu_->updateState();
    menu_->popup(globalPos);
}

int ViewBox::contextMenuRaiseCount() const noexcept
{
    return contextMenuRaiseCount_;
}

void ViewBox::ensureMenu()
{
    if (menu_ == nullptr && menuEnabled_) {
        menu_ = std::make_unique<ViewBoxMenu>(this);
    }
}

int ViewBox::dragThresholdPixels() const
{
    if (QApplication::instance() != nullptr) {
        return QApplication::startDragDistance();
    }
    return 10;
}

void ViewBox::setDefaultPadding(qreal padding)
{
    if (!isFinite(padding)) {
        throw std::invalid_argument("default padding must be finite");
    }
    defaultPadding_ = padding;
}

void ViewBox::setAspectLocked(bool lock, std::optional<qreal> ratio)
{
    if (!lock) {
        aspectLocked_ = std::nullopt;
        updateViewRange();
        return;
    }

    const qreal nextRatio = ratio.value_or(currentAspectRatio());
    if (!isFinite(nextRatio) || nextRatio <= 0.0) {
        throw std::invalid_argument("aspect ratio must be finite and positive");
    }
    aspectLocked_ = nextRatio;
    updateViewRange();
}

void ViewBox::invertX(bool inverted)
{
    if (xInverted_ == inverted) {
        return;
    }
    xInverted_ = inverted;
    markMatrixDirty();
}

void ViewBox::invertY(bool inverted)
{
    if (yInverted_ == inverted) {
        return;
    }
    yInverted_ = inverted;
    markMatrixDirty();
}

bool ViewBox::xInverted() const
{
    return xInverted_;
}

bool ViewBox::yInverted() const
{
    return yInverted_;
}

QTransform ViewBox::childTransform()
{
    updateMatrix();
    return childGroup_.transform();
}

QPointF ViewBox::mapToView(const QPointF& point)
{
    updateMatrix();
    return childGroup_.transform().inverted().map(point);
}

QRectF ViewBox::mapToView(const QRectF& rect)
{
    updateMatrix();
    return childGroup_.transform().inverted().mapRect(rect);
}

QPointF ViewBox::mapFromView(const QPointF& point)
{
    updateMatrix();
    return childGroup_.transform().map(point);
}

QRectF ViewBox::mapFromView(const QRectF& rect)
{
    updateMatrix();
    return childGroup_.transform().mapRect(rect);
}

QPointF ViewBox::mapSceneToView(const QPointF& point)
{
    return mapToView(mapFromScene(point));
}

QRectF ViewBox::mapSceneToView(const QRectF& rect)
{
    return mapToView(mapFromScene(rect).boundingRect());
}

QPointF ViewBox::mapViewToScene(const QPointF& point)
{
    return mapToScene(mapFromView(point));
}

QRectF ViewBox::mapViewToScene(const QRectF& rect)
{
    return mapToScene(mapFromView(rect)).boundingRect();
}

std::array<std::optional<AxisRange>, 2> ViewBox::childrenBounds() const
{
    std::array<std::optional<AxisRange>, 2> bounds{std::nullopt, std::nullopt};
    pruneAddedItems();

    for (auto* item : addedItems_) {
        if (item == nullptr || !item->isVisible() || item->scene() != scene()) {
            continue;
        }
        if (item->flags().testFlag(QGraphicsItem::GraphicsItemFlag::ItemHasNoContents)) {
            continue;
        }

        const QRectF localBounds = item->boundingRect();
        if (localBounds.isNull()) {
            continue;
        }

        const QRectF itemBounds = childGroup_.mapFromItem(item, localBounds).boundingRect();
        if (!isFinite(itemBounds.left()) || !isFinite(itemBounds.right()) || !isFinite(itemBounds.top()) || !isFinite(itemBounds.bottom())) {
            continue;
        }

        const AxisRange xBounds = normalizeBoundsAxis(itemBounds.left(), itemBounds.right());
        const AxisRange yBounds = normalizeBoundsAxis(itemBounds.top(), itemBounds.bottom());
        bounds[xAxis] = bounds[xAxis].has_value()
            ? std::optional<AxisRange>{AxisRange{std::min((*bounds[xAxis])[0], xBounds[0]), std::max((*bounds[xAxis])[1], xBounds[1])}}
            : std::optional<AxisRange>{xBounds};
        bounds[yAxis] = bounds[yAxis].has_value()
            ? std::optional<AxisRange>{AxisRange{std::min((*bounds[yAxis])[0], yBounds[0]), std::max((*bounds[yAxis])[1], yBounds[1])}}
            : std::optional<AxisRange>{yBounds};
    }

    return bounds;
}

QRectF ViewBox::childrenBoundingRect() const
{
    const auto bounds = childrenBounds();
    const AxisRange xRange = optionalRangeOrTarget(bounds[xAxis], targetRange_[xAxis]);
    const AxisRange yRange = optionalRangeOrTarget(bounds[yAxis], targetRange_[yAxis]);
    return rectFromRange(Range2D{{xRange, yRange}});
}

QSizeF ViewBox::viewPixelSize()
{
    updateMatrix();

    QTransform itemToDevice = sceneTransform();
    if (auto* currentScene = scene(); currentScene != nullptr && !currentScene->views().isEmpty()) {
        itemToDevice = deviceTransform(currentScene->views().front()->viewportTransform());
    }

    bool invertible = false;
    const QTransform deviceToItem = itemToDevice.inverted(&invertible);
    if (!invertible) {
        return QSizeF(0.0, 0.0);
    }

    const QPointF localOrigin = deviceToItem.map(QPointF(0.0, 0.0));
    const QPointF viewOrigin = mapToView(localOrigin);
    const QPointF xPixel = mapToView(deviceToItem.map(QPointF(1.0, 0.0))) - viewOrigin;
    const QPointF yPixel = mapToView(deviceToItem.map(QPointF(0.0, 1.0))) - viewOrigin;
    return QSizeF(std::hypot(xPixel.x(), xPixel.y()), std::hypot(yPixel.x(), yPixel.y()));
}

void ViewBox::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    updateMatrix();
    GraphicsWidget::paint(painter, option, widget);
}

void ViewBox::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    wheelEventForAxis(event, XYAxes);
}

void ViewBox::wheelEventForAxis(QGraphicsSceneWheelEvent* event, int axis)
{
    std::array<bool, 2> mask = mouseEnabled_;
    if (axis == XAxis || axis == YAxis) {
        mask[axis == XAxis ? YAxis : XAxis] = false;
    }
    if (!mask[xAxis] && !mask[yAxis]) {
        event->ignore();
        return;
    }

    const qreal scale = std::pow(1.02, static_cast<qreal>(event->delta()) * wheelScaleFactor_);
    const QPointF center = mapToView(mapFromScene(event->scenePos()));
    scaleByInteractive(mask[xAxis] ? std::optional<qreal>{scale} : std::nullopt,
                       mask[yAxis] ? std::optional<qreal>{scale} : std::nullopt,
                       center);
    event->accept();
    emit sigRangeChangedManually(mask);
}

void ViewBox::translateByAxisDrag(const QPointF& itemDiff, int axis)
{
    if (axis != XAxis && axis != YAxis) {
        return;
    }
    if (!mouseEnabled_[axis]) {
        return;
    }

    const QPointF maskedDiff(axis == XAxis ? itemDiff.x() : 0.0, axis == YAxis ? itemDiff.y() : 0.0);
    const QPointF diff = maskedDiff * -1.0;
    const QTransform inverse = childTransform().inverted();
    const QPointF mapped = inverse.map(diff) - inverse.map(QPointF(0.0, 0.0));

    std::array<bool, 2> mask{{false, false}};
    mask[axis] = true;
    translateByInteractive(axis == XAxis ? std::optional<qreal>{mapped.x()} : std::nullopt,
                           axis == YAxis ? std::optional<qreal>{mapped.y()} : std::nullopt);
    emit sigRangeChangedManually(mask);
}

void ViewBox::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!mouseEnabled_[xAxis] && !mouseEnabled_[yAxis]) {
        event->ignore();
        return;
    }

    const Qt::MouseButton button = event->button();
    if (button != Qt::LeftButton && button != Qt::MiddleButton && button != Qt::RightButton) {
        GraphicsWidget::mousePressEvent(event);
        return;
    }

    dragActive_ = true;
    dragButton_ = button;
    rightDragExceededThreshold_ = false;
    dragLastPos_ = event->pos();
    dragButtonDownPos_ = event->buttonDownPos(button).isNull() ? event->pos() : event->buttonDownPos(button);
    dragLastScreenPos_ = event->screenPos();
    dragButtonDownScreenPos_ = event->buttonDownScreenPos(button).isNull() ? event->screenPos() : event->buttonDownScreenPos(button);
    event->accept();
}

void ViewBox::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!dragActive_ || (!mouseEnabled_[xAxis] && !mouseEnabled_[yAxis])) {
        event->ignore();
        return;
    }

    const std::array<bool, 2> mask = mouseEnabled_;
    if ((dragButton_ == Qt::LeftButton || dragButton_ == Qt::MiddleButton) && (mouseMode_ == PanMode || dragButton_ == Qt::MiddleButton)) {
        const QPointF diff = (event->pos() - dragLastPos_) * -1.0;
        const QTransform inverse = childTransform().inverted();
        const QPointF mapped = inverse.map(QPointF(mask[xAxis] ? diff.x() : 0.0, mask[yAxis] ? diff.y() : 0.0)) - inverse.map(QPointF(0.0, 0.0));
        if (mask[xAxis] || mask[yAxis]) {
            translateByInteractive(mask[xAxis] ? std::optional<qreal>{mapped.x()} : std::nullopt,
                                   mask[yAxis] ? std::optional<qreal>{mapped.y()} : std::nullopt);
            emit sigRangeChangedManually(mask);
        }
        dragLastPos_ = event->pos();
        dragLastScreenPos_ = event->screenPos();
        event->accept();
        return;
    }

    if (dragButton_ == Qt::LeftButton && mouseMode_ == RectMode) {
        dragLastPos_ = event->pos();
        dragLastScreenPos_ = event->screenPos();
        event->accept();
        return;
    }

    if (dragButton_ == Qt::RightButton) {
        if (!rightDragExceededThreshold_) {
            const QPoint totalDelta = event->screenPos() - dragButtonDownScreenPos_;
            if (totalDelta.manhattanLength() < dragThresholdPixels()) {
                event->accept();
                return;
            }
            rightDragExceededThreshold_ = true;
        }

        const QPoint screenDelta = event->screenPos() - dragLastScreenPos_;
        const qreal xExponent = -static_cast<qreal>(screenDelta.x());
        const qreal yExponent = static_cast<qreal>(screenDelta.y());
        const std::optional<qreal> xScale = mask[xAxis] ? std::optional<qreal>{std::pow(1.02, xExponent)} : std::nullopt;
        const std::optional<qreal> yScale = mask[yAxis] ? std::optional<qreal>{std::pow(1.02, yExponent)} : std::nullopt;
        if (xScale.has_value() || yScale.has_value()) {
            scaleByInteractive(xScale, yScale, mapToView(dragButtonDownPos_));
            emit sigRangeChangedManually(mask);
        }
        dragLastPos_ = event->pos();
        dragLastScreenPos_ = event->screenPos();
        event->accept();
        return;
    }

    event->ignore();
}

void ViewBox::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (dragActive_ && event->button() == dragButton_) {
        if (dragButton_ == Qt::RightButton && !rightDragExceededThreshold_ && menuEnabled_) {
            raiseContextMenu(event->screenPos());
            dragActive_ = false;
            dragButton_ = Qt::NoButton;
            rightDragExceededThreshold_ = false;
            event->accept();
            return;
        }
        if (dragButton_ == Qt::LeftButton && mouseMode_ == RectMode) {
            const QRectF zoomRect = QRectF(mapToView(dragButtonDownPos_), mapToView(event->pos())).normalized();
            if (!zoomRect.isEmpty()) {
                applyInteractiveRange(AxisRange{zoomRect.left(), zoomRect.right()},
                                      AxisRange{zoomRect.top(), zoomRect.bottom()},
                                      0.0);
                emit sigRangeChangedManually(mouseEnabled_);
            }
        }
        dragActive_ = false;
        dragButton_ = Qt::NoButton;
        rightDragExceededThreshold_ = false;
        event->accept();
        return;
    }
    GraphicsWidget::mouseReleaseEvent(event);
}

void ViewBox::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    GraphicsWidget::resizeEvent(event);
    markMatrixDirty();
    if ((autoRange_[xAxis] || autoRange_[yAxis]) && !addedItems_.empty()) {
        if (!applyAutoRange(defaultPadding_, autoRange_)) {
            updateViewRange();
        }
    } else {
        updateViewRange();
    }
    notifyLinkedViews(std::array<bool, 2>{{true, true}});
    emit sigResized(this);
}

QVariant ViewBox::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsWidget::itemChange(change, value);
    if (change == QGraphicsItem::ItemSceneHasChanged) {
        updateAutoRangeSceneConnection();
    }
    return result;
}

bool ViewBox::applyInteractiveRange(std::optional<AxisRange> xRange,
                                    std::optional<AxisRange> yRange,
                                    qreal padding,
                                    bool disableAutoRange)
{
    if (!xRange.has_value() && !yRange.has_value()) {
        return false;
    }

    Range2D nextTargetRange = targetRange_;
    bool applied = false;

    if (xRange.has_value()) {
        if (const auto normalized = safeNormalizeRequestedRange(*xRange, targetRange_[xAxis], padding)) {
            if (const auto clamped = safeClampAxisToLimits(*normalized, limits_, xAxis)) {
                nextTargetRange[xAxis] = *clamped;
                applied = true;
            }
        }
    }
    if (yRange.has_value()) {
        if (const auto normalized = safeNormalizeRequestedRange(*yRange, targetRange_[yAxis], padding)) {
            if (const auto clamped = safeClampAxisToLimits(*normalized, limits_, yAxis)) {
                nextTargetRange[yAxis] = *clamped;
                applied = true;
            }
        }
    }

    if (!applied || !rangeIsFinite(nextTargetRange[xAxis]) || !rangeIsFinite(nextTargetRange[yAxis])
        || nextTargetRange[xAxis][1] <= nextTargetRange[xAxis][0]
        || nextTargetRange[yAxis][1] <= nextTargetRange[yAxis][0]) {
        return false;
    }

    const Range2D previousTargetRange = targetRange_;
    targetRange_ = nextTargetRange;

    if (disableAutoRange) {
        const auto previousAutoRange = autoRange_;
        if (xRange.has_value()) {
            autoRange_[xAxis] = false;
        }
        if (yRange.has_value()) {
            autoRange_[yAxis] = false;
        }
        if (previousAutoRange != autoRange_) {
            emit sigStateChanged(this);
        }
    }

    const bool lockX = xRange.has_value() && !yRange.has_value();
    const bool lockY = yRange.has_value() && !xRange.has_value();
    try {
        updateViewRange(lockX, lockY);
    } catch (...) {
        targetRange_ = previousTargetRange;
        return false;
    }
    return true;
}

void ViewBox::scaleByInteractive(std::optional<qreal> x, std::optional<qreal> y, std::optional<QPointF> center)
{
    if (!x.has_value() && !y.has_value()) {
        return;
    }
    if ((x.has_value() && !isFinite(*x)) || (y.has_value() && !isFinite(*y))) {
        return;
    }
    if (center.has_value() && (!isFinite(center->x()) || !isFinite(center->y()))) {
        return;
    }

    const QRectF rect = targetRect();
    const QPointF scale{x.value_or(1.0), y.value_or(1.0)};
    const QPointF scaleCenter = center.value_or(rect.center());
    const QPointF topLeft = scaleCenter
        + QPointF((rect.left() - scaleCenter.x()) * scale.x(), (rect.top() - scaleCenter.y()) * scale.y());
    const QPointF bottomRight = scaleCenter
        + QPointF((rect.right() - scaleCenter.x()) * scale.x(), (rect.bottom() - scaleCenter.y()) * scale.y());

    if (!isFinite(topLeft.x()) || !isFinite(topLeft.y()) || !isFinite(bottomRight.x()) || !isFinite(bottomRight.y())) {
        return;
    }

    if (x.has_value() && y.has_value()) {
        applyInteractiveRange(AxisRange{topLeft.x(), bottomRight.x()}, AxisRange{topLeft.y(), bottomRight.y()}, 0.0);
    } else if (x.has_value()) {
        applyInteractiveRange(AxisRange{topLeft.x(), bottomRight.x()}, std::nullopt, 0.0);
    } else {
        applyInteractiveRange(std::nullopt, AxisRange{topLeft.y(), bottomRight.y()}, 0.0);
    }
}

void ViewBox::translateByInteractive(std::optional<qreal> x, std::optional<qreal> y)
{
    if (!x.has_value() && !y.has_value()) {
        return;
    }
    if ((x.has_value() && !isFinite(*x)) || (y.has_value() && !isFinite(*y))) {
        return;
    }

    const QRectF rect = targetRect();
    const std::optional<AxisRange> xRange = x.has_value()
        ? std::optional<AxisRange>{AxisRange{rect.left() + *x, rect.right() + *x}}
        : std::nullopt;
    const std::optional<AxisRange> yRange = y.has_value()
        ? std::optional<AxisRange>{AxisRange{rect.top() + *y, rect.bottom() + *y}}
        : std::nullopt;
    applyInteractiveRange(xRange, yRange, 0.0);
}

bool ViewBox::applyAutoRange(std::optional<qreal> padding, const std::array<bool, 2>& axes, bool disableAutoRange)
{
    const auto bounds = childrenBounds();
    const std::optional<AxisRange> xRange = axes[xAxis] ? bounds[xAxis] : std::nullopt;
    const std::optional<AxisRange> yRange = axes[yAxis] ? bounds[yAxis] : std::nullopt;

    if (!xRange.has_value() && !yRange.has_value()) {
        return false;
    }

    return applyInteractiveRange(xRange, yRange, padding.value_or(defaultPadding_), disableAutoRange);
}

void ViewBox::updateViewRange(bool forceX, bool forceY)
{
    const Range2D previousViewRange = viewRange_;
    Range2D nextViewRange = targetRange_;

    if (aspectLocked_.has_value()) {
        const QRectF bounds = rect();
        const qreal aspect = *aspectLocked_;
        const qreal targetHeight = axisSpan(targetRange_[yAxis]);
        if (bounds.width() != 0.0 && bounds.height() != 0.0 && targetHeight != 0.0 && aspect != 0.0) {
            const qreal targetRatio = axisSpan(targetRange_[xAxis]) / targetHeight;
            qreal viewRatio = (bounds.width() / bounds.height()) / aspect;
            if (viewRatio == 0.0) {
                viewRatio = 1.0;
            }

            const qreal dy = 0.5 * (axisSpan(targetRange_[xAxis]) / viewRatio - axisSpan(targetRange_[yAxis]));
            const qreal dx = 0.5 * (axisSpan(targetRange_[yAxis]) * viewRatio - axisSpan(targetRange_[xAxis]));
            const AxisRange rangeX = expandedAxis(targetRange_[xAxis], dx);
            const AxisRange rangeY = expandedAxis(targetRange_[yAxis], dy);

            int fixedAxis;
            if (forceX) {
                fixedAxis = xAxis;
            } else if (forceY) {
                fixedAxis = yAxis;
            } else {
                fixedAxis = targetRatio > viewRatio ? xAxis : yAxis;
            }

            if (fixedAxis == xAxis) {
                nextViewRange[yAxis] = rangeY;
            } else {
                nextViewRange[xAxis] = rangeX;
            }
        }
    }

    nextViewRange = clampRangeToLimits(nextViewRange, limits_);
    validateFiniteRange(nextViewRange, "updateViewRange produced non-finite view range");
    const std::array<bool, 2> changed{{
        axisRangeChanged(previousViewRange[xAxis], nextViewRange[xAxis]),
        axisRangeChanged(previousViewRange[yAxis], nextViewRange[yAxis]),
    }};
    if (changed[xAxis] || changed[yAxis]) {
        markMatrixDirty();
    }
    viewRange_ = nextViewRange;
    targetRange_ = clampRangeToLimits(targetRange_, limits_);
    if (changed[xAxis] || changed[yAxis]) {
        notifyLinkedViews(changed);
        emitRangeChanges(changed);
    }
}

void ViewBox::markMatrixDirty()
{
    matrixNeedsUpdate_ = true;
    update();
}

void ViewBox::updateAutoRangeSceneConnection()
{
    QObject::disconnect(sceneChangedConnection_);
    sceneChangedConnection_ = QMetaObject::Connection{};

    auto* currentScene = scene();
    if (currentScene == nullptr) {
        return;
    }

    sceneChangedConnection_ = QObject::connect(currentScene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        if ((autoRange_[xAxis] || autoRange_[yAxis]) && !addedItems_.empty()) {
            applyAutoRange(defaultPadding_, autoRange_);
        }
    });
}

void ViewBox::refreshAutoRangeIfNeeded()
{
    if ((autoRange_[xAxis] || autoRange_[yAxis]) && !addedItems_.empty()) {
        applyAutoRange(defaultPadding_, autoRange_);
    }
}

void ViewBox::pruneAddedItems() const
{
    const auto childItems = childGroup_.childItems();
    addedItems_.erase(std::remove_if(addedItems_.begin(), addedItems_.end(), [&childItems](QGraphicsItem* item) {
                          return item == nullptr || std::find(childItems.cbegin(), childItems.cend(), item) == childItems.cend();
                      }),
                      addedItems_.end());
}

void ViewBox::emitRangeChanges(const std::array<bool, 2>& changed)
{
    if (changed[xAxis]) {
        emit sigXRangeChanged(this, viewRange_[xAxis]);
    }
    if (changed[yAxis]) {
        emit sigYRangeChanged(this, viewRange_[yAxis]);
    }
    emit sigRangeChanged(this, viewRange_, changed);
}

void ViewBox::notifyLinkedViews(const std::array<bool, 2>& changed)
{
    if (linksBlocked_) {
        return;
    }
    for (int axis : {xAxis, yAxis}) {
        if (!changed[axis]) {
            continue;
        }
        if (auto* link = linkedViews_[axis].data(); link != nullptr) {
            link->linkedViewChanged(this, axis);
        }
    }
}

QRectF ViewBox::screenGeometry() const
{
    QRectF geometry = sceneBoundingRect();
    if (!geometry.isValid()) {
        return QRectF{};
    }
    if (auto* currentScene = scene(); currentScene != nullptr && !currentScene->views().isEmpty()) {
        geometry = currentScene->views().front()->mapFromScene(sceneBoundingRect()).boundingRect();
    }
    return geometry;
}

void ViewBox::updateMatrix()
{
    refreshAutoRangeIfNeeded();
    if (!matrixNeedsUpdate_) {
        return;
    }

    const QRectF bounds = rect();
    const QRectF visible = viewRect();
    if (visible.width() == 0.0 || visible.height() == 0.0) {
        return;
    }

    QPointF scale(bounds.width() / visible.width(), bounds.height() / visible.height());
    if (!yInverted_) {
        scale.setY(-scale.y());
    }
    if (xInverted_) {
        scale.setX(-scale.x());
    }

    QTransform transform;
    const QPointF center = bounds.center();
    transform.translate(center.x(), center.y());
    transform.scale(scale.x(), scale.y());
    const QPointF viewCenter = visible.center();
    transform.translate(-viewCenter.x(), -viewCenter.y());

    const bool transformChanged = childGroup_.transform() != transform;
    childGroup_.setTransform(transform);
    matrixNeedsUpdate_ = false;
    if (transformChanged) {
        emit sigTransformChanged(this);
    }
}

qreal ViewBox::currentAspectRatio() const
{
    const QRectF bounds = rect();
    const qreal viewHeight = axisSpan(viewRange_[yAxis]);
    if (bounds.height() == 0.0 || viewHeight == 0.0) {
        return 1.0;
    }
    const qreal viewRatio = axisSpan(viewRange_[xAxis]) / viewHeight;
    if (viewRatio == 0.0) {
        return 1.0;
    }
    return (bounds.width() / bounds.height()) / viewRatio;
}

} // namespace cppqtgraph::graphicsItems
