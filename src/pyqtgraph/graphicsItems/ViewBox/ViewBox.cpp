// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../../include/pyqtgraph/GraphicsScene/GraphicsScene.hpp"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGraphicsView>

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

void addSceneItem(QGraphicsScene* scene, QGraphicsItem* item)
{
    if (auto* graphicsScene = qobject_cast<::pyqtgraph::GraphicsScene::GraphicsScene*>(scene)) {
        graphicsScene->addItem(item);
        return;
    }

    scene->addItem(item);
}

void removeSceneItem(QGraphicsScene* scene, QGraphicsItem* item)
{
    if (auto* graphicsScene = qobject_cast<::pyqtgraph::GraphicsScene::GraphicsScene*>(scene)) {
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
    childGroup_.setHandlesChildEvents(false);
    updateAutoRangeSceneConnection();
}

ViewBox::~ViewBox()
{
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
        if (xRange.has_value()) {
            autoRange_[xAxis] = false;
        }
        if (yRange.has_value()) {
            autoRange_[yAxis] = false;
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

    const Range2D nextTargetRange = clampRangeToLimits(targetRange_, limits);
    const Range2D nextViewRange = clampRangeToLimits(viewRange_, limits);

    limits_ = limits;
    targetRange_ = nextTargetRange;
    if (viewRange_ != nextViewRange) {
        markMatrixDirty();
    }
    viewRange_ = nextViewRange;
}

void ViewBox::autoRange(std::optional<qreal> padding)
{
    applyAutoRange(padding, std::array<bool, 2>{{true, true}}, true);
    autoRange_[xAxis] = false;
    autoRange_[yAxis] = false;
}

void ViewBox::enableAutoRange(int axis, bool enable)
{
    if (!axisIsValid(axis)) {
        throw std::invalid_argument("axis must be XAxis, YAxis, or XYAxes");
    }

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
}

void ViewBox::disableAutoRange(int axis)
{
    enableAutoRange(axis, false);
}

std::array<bool, 2> ViewBox::autoRangeEnabled() const
{
    return autoRange_;
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
}

QVariant ViewBox::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsWidget::itemChange(change, value);
    if (change == QGraphicsItem::ItemSceneHasChanged) {
        updateAutoRangeSceneConnection();
    }
    return result;
}

bool ViewBox::applyAutoRange(std::optional<qreal> padding, const std::array<bool, 2>& axes, bool disableAutoRange)
{
    const auto bounds = childrenBounds();
    const std::optional<AxisRange> xRange = axes[xAxis] ? bounds[xAxis] : std::nullopt;
    const std::optional<AxisRange> yRange = axes[yAxis] ? bounds[yAxis] : std::nullopt;

    if (!xRange.has_value() && !yRange.has_value()) {
        return false;
    }

    setRange(xRange, yRange, padding.value_or(defaultPadding_), true, disableAutoRange);
    return true;
}

void ViewBox::updateViewRange(bool forceX, bool forceY)
{
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
    if (viewRange_ != nextViewRange) {
        markMatrixDirty();
    }
    viewRange_ = nextViewRange;
    targetRange_ = clampRangeToLimits(targetRange_, limits_);
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

    childGroup_.setTransform(transform);
    matrixNeedsUpdate_ = false;
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

} // namespace pyqtgraph::graphicsItems
