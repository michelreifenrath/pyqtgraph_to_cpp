#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/FillBetweenItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "PlotCurveItem.hpp"
#include "PlotDataItem.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtCore/QTimerEvent>
#include <QtCore/QVariant>
#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPolygonF>
#include <QtWidgets/QGraphicsPathItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

class QGraphicsItem;
class QPainter;
class QWidget;

namespace pyqtgraph::graphicsItems {

class FillBetweenItem : public QObject, public QGraphicsPathItem {
public:
    explicit FillBetweenItem(QGraphicsItem* parent = nullptr);
    FillBetweenItem(PlotCurveItem* curve1, PlotCurveItem* curve2, const QBrush& brush = QBrush(),
        const QPen& pen = QPen(Qt::NoPen), Qt::FillRule fillRule = Qt::OddEvenFill, QGraphicsItem* parent = nullptr);
    FillBetweenItem(PlotDataItem* curve1, PlotDataItem* curve2, const QBrush& brush = QBrush(),
        const QPen& pen = QPen(Qt::NoPen), Qt::FillRule fillRule = Qt::OddEvenFill, QGraphicsItem* parent = nullptr);
    ~FillBetweenItem() override = default;

    FillBetweenItem(const FillBetweenItem&) = delete;
    FillBetweenItem& operator=(const FillBetweenItem&) = delete;
    FillBetweenItem(FillBetweenItem&&) = delete;
    FillBetweenItem& operator=(FillBetweenItem&&) = delete;

    void setCurves(PlotCurveItem* curve1, PlotCurveItem* curve2);
    void setCurves(PlotDataItem* curve1, PlotDataItem* curve2);
    [[nodiscard]] const PlotCurveItem* curve1() const noexcept;
    [[nodiscard]] const PlotCurveItem* curve2() const noexcept;

    void setFillRule(Qt::FillRule fillRule);
    [[nodiscard]] Qt::FillRule fillRule() const noexcept;
    void updatePath();

    [[nodiscard]] QPainterPath path() const;
    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void timerEvent(QTimerEvent* event) override;

private:
    struct CurveFingerprint {
        const PlotCurveItem* curve = nullptr;
        PlotCurveItem::StepMode stepMode = PlotCurveItem::StepMode::None;
        PlotCurveItem::ConnectMode connectMode = PlotCurveItem::ConnectMode::All;
        std::size_t xSize = 0;
        std::size_t ySize = 0;
        std::size_t hash = 0;

        [[nodiscard]] bool operator==(const CurveFingerprint& other) const noexcept = default;
    };

    [[nodiscard]] CurveFingerprint fingerprint(const PlotCurveItem* curve) const;
    void ensurePathCurrent() const;
    void restoreCurveRelativeZValue();
    void ensureSourceRefreshTimer();

    QPointer<PlotCurveItem> curve1_;
    QPointer<PlotCurveItem> curve2_;
    Qt::FillRule fillRule_ = Qt::OddEvenFill;
    mutable CurveFingerprint curve1Fingerprint_;
    mutable CurveFingerprint curve2Fingerprint_;
    int sourceRefreshTimerId_ = 0;
};

namespace detail_fillbetweenitem {

inline bool finitePoint(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

using SampleSegment = std::vector<std::size_t>;

inline std::size_t sampleCount(const PlotCurveItem* curve)
{
    if (curve == nullptr) {
        return 0;
    }
    const auto x = curve->xData();
    const auto y = curve->yData();
    if (curve->stepMode() == PlotCurveItem::StepMode::Center) {
        return x.size() == y.size() + 1 ? y.size() : 0;
    }
    return x.size() == y.size() ? y.size() : 0;
}

inline bool finiteSample(const PlotCurveItem* curve, std::size_t index)
{
    const auto x = curve->xData();
    const auto y = curve->yData();
    if (index >= y.size()) {
        return false;
    }
    if (curve->stepMode() == PlotCurveItem::StepMode::Center) {
        return index + 1 < x.size() && finitePoint(x[index], y[index]) && finitePoint(x[index + 1], y[index]);
    }
    return index < x.size() && finitePoint(x[index], y[index]);
}

inline bool segmentDrawable(const PlotCurveItem* curve, const SampleSegment& segment)
{
    return segment.size() >= 2U
        || (segment.size() == 1U && curve != nullptr && curve->stepMode() != PlotCurveItem::StepMode::None);
}

inline void appendSegmentIfDrawable(std::vector<SampleSegment>& segments, SampleSegment& segment, const PlotCurveItem* curve)
{
    if (segmentDrawable(curve, segment)) {
        segments.push_back(segment);
    }
    segment.clear();
}

inline std::vector<SampleSegment> connectedSampleSegments(const PlotCurveItem* curve)
{
    std::vector<SampleSegment> segments;
    const std::size_t count = sampleCount(curve);
    if (count == 0U) {
        return segments;
    }

    switch (curve->connectMode()) {
    case PlotCurveItem::ConnectMode::All: {
        SampleSegment segment;
        segment.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            if (finiteSample(curve, index)) {
                segment.push_back(index);
            }
        }
        appendSegmentIfDrawable(segments, segment, curve);
        break;
    }
    case PlotCurveItem::ConnectMode::Finite: {
        SampleSegment segment;
        for (std::size_t index = 0; index < count; ++index) {
            if (finiteSample(curve, index)) {
                segment.push_back(index);
            } else {
                appendSegmentIfDrawable(segments, segment, curve);
            }
        }
        appendSegmentIfDrawable(segments, segment, curve);
        break;
    }
    case PlotCurveItem::ConnectMode::Pairs:
        for (std::size_t index = 0; index + 1U < count; index += 2U) {
            if (finiteSample(curve, index) && finiteSample(curve, index + 1U)) {
                segments.push_back(SampleSegment{index, index + 1U});
            }
        }
        break;
    }
    return segments;
}

inline void appendPointIfFinite(std::vector<QPointF>& points, double x, double y)
{
    if (finitePoint(x, y)) {
        points.emplace_back(x, y);
    }
}

inline std::vector<QPointF> steppedPointsForSamples(const PlotCurveItem* curve, const SampleSegment& samples)
{
    std::vector<QPointF> points;
    if (curve == nullptr || samples.empty()) {
        return points;
    }
    const auto x = curve->xData();
    const auto y = curve->yData();
    points.reserve(curve->stepMode() == PlotCurveItem::StepMode::None ? samples.size() : samples.size() * 2U);
    for (const std::size_t index : samples) {
        if (!finiteSample(curve, index)) {
            continue;
        }
        switch (curve->stepMode()) {
        case PlotCurveItem::StepMode::None:
            appendPointIfFinite(points, x[index], y[index]);
            break;
        case PlotCurveItem::StepMode::Center:
            appendPointIfFinite(points, x[index], y[index]);
            appendPointIfFinite(points, x[index + 1U], y[index]);
            break;
        case PlotCurveItem::StepMode::Right:
            appendPointIfFinite(points, x[index], y[index]);
            appendPointIfFinite(points, index + 1U < x.size() ? x[index + 1U] : x[index], y[index]);
            break;
        case PlotCurveItem::StepMode::Left:
            appendPointIfFinite(points, index == 0U ? x[index] : x[index - 1U], y[index]);
            appendPointIfFinite(points, x[index], y[index]);
            break;
        }
    }
    return points;
}

inline void hashCombine(std::size_t& seed, std::size_t value)
{
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

inline std::size_t hashValues(std::span<const double> values)
{
    std::size_t seed = values.size();
    const std::hash<double> hashDouble;
    for (const double value : values) {
        hashCombine(seed, hashDouble(value));
    }
    return seed;
}

inline PlotCurveItem* requireCurve(PlotDataItem* item, const char* name)
{
    if (item == nullptr || item->curve() == nullptr) {
        throw std::invalid_argument(std::string("FillBetweenItem requires a non-null ") + name);
    }
    return item->curve();
}

inline void requireTwoCurves(PlotCurveItem* curve1, PlotCurveItem* curve2)
{
    if (curve1 == nullptr || curve2 == nullptr) {
        throw std::invalid_argument("FillBetweenItem requires two curves to fill between");
    }
}

} // namespace detail_fillbetweenitem

inline FillBetweenItem::FillBetweenItem(QGraphicsItem* parent)
    : QObject()
    , QGraphicsPathItem(parent)
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    QGraphicsPathItem::setPen(QPen(Qt::NoPen));
    QGraphicsPathItem::setBrush(QBrush());
    updatePath();
}

inline FillBetweenItem::FillBetweenItem(PlotCurveItem* curve1, PlotCurveItem* curve2, const QBrush& brush,
    const QPen& pen, Qt::FillRule fillRule, QGraphicsItem* parent)
    : FillBetweenItem(parent)
{
    fillRule_ = fillRule;
    QGraphicsPathItem::setBrush(brush);
    QGraphicsPathItem::setPen(pen);
    setCurves(curve1, curve2);
}

inline FillBetweenItem::FillBetweenItem(PlotDataItem* curve1, PlotDataItem* curve2, const QBrush& brush,
    const QPen& pen, Qt::FillRule fillRule, QGraphicsItem* parent)
    : FillBetweenItem(detail_fillbetweenitem::requireCurve(curve1, "curve1"),
          detail_fillbetweenitem::requireCurve(curve2, "curve2"), brush, pen, fillRule, parent)
{
}

inline void FillBetweenItem::setCurves(PlotCurveItem* curve1, PlotCurveItem* curve2)
{
    detail_fillbetweenitem::requireTwoCurves(curve1, curve2);
    curve1_ = curve1;
    curve2_ = curve2;
    restoreCurveRelativeZValue();
    ensureSourceRefreshTimer();
    updatePath();
}

inline void FillBetweenItem::setCurves(PlotDataItem* curve1, PlotDataItem* curve2)
{
    setCurves(detail_fillbetweenitem::requireCurve(curve1, "curve1"),
        detail_fillbetweenitem::requireCurve(curve2, "curve2"));
}

inline const PlotCurveItem* FillBetweenItem::curve1() const noexcept { return curve1_.data(); }
inline const PlotCurveItem* FillBetweenItem::curve2() const noexcept { return curve2_.data(); }

inline void FillBetweenItem::setFillRule(Qt::FillRule fillRule)
{
    fillRule_ = fillRule;
    updatePath();
}

inline Qt::FillRule FillBetweenItem::fillRule() const noexcept { return fillRule_; }

inline FillBetweenItem::CurveFingerprint FillBetweenItem::fingerprint(const PlotCurveItem* curve) const
{
    CurveFingerprint result;
    result.curve = curve;
    if (curve == nullptr) {
        return result;
    }
    const auto x = curve->xData();
    const auto y = curve->yData();
    result.stepMode = curve->stepMode();
    result.connectMode = curve->connectMode();
    result.xSize = x.size();
    result.ySize = y.size();
    result.hash = detail_fillbetweenitem::hashValues(x);
    detail_fillbetweenitem::hashCombine(result.hash, detail_fillbetweenitem::hashValues(y));
    detail_fillbetweenitem::hashCombine(result.hash, static_cast<std::size_t>(result.stepMode));
    detail_fillbetweenitem::hashCombine(result.hash, static_cast<std::size_t>(result.connectMode));
    return result;
}

inline void FillBetweenItem::ensurePathCurrent() const
{
    const CurveFingerprint current1 = fingerprint(curve1_.data());
    const CurveFingerprint current2 = fingerprint(curve2_.data());
    if (!(current1 == curve1Fingerprint_) || !(current2 == curve2Fingerprint_)) {
        const_cast<FillBetweenItem*>(this)->updatePath();
    }
}

inline void FillBetweenItem::restoreCurveRelativeZValue()
{
    if (!curve1_.isNull() && !curve2_.isNull()) {
        setZValue(std::min(curve1_->zValue(), curve2_->zValue()) - 1.0);
    }
}

inline void FillBetweenItem::ensureSourceRefreshTimer()
{
    if (sourceRefreshTimerId_ != 0 || QCoreApplication::instance() == nullptr) {
        return;
    }
    sourceRefreshTimerId_ = startTimer(16, Qt::CoarseTimer);
}

inline QVariant FillBetweenItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = QGraphicsPathItem::itemChange(change, value);
    if (change == QGraphicsItem::ItemParentHasChanged) {
        restoreCurveRelativeZValue();
    }
    return result;
}

inline void FillBetweenItem::timerEvent(QTimerEvent* event)
{
    if (event != nullptr && event->timerId() == sourceRefreshTimerId_) {
        ensurePathCurrent();
        return;
    }
    QObject::timerEvent(event);
}

inline QPainterPath FillBetweenItem::path() const
{
    ensurePathCurrent();
    return QGraphicsPathItem::path();
}

inline QRectF FillBetweenItem::boundingRect() const
{
    ensurePathCurrent();
    return QGraphicsPathItem::boundingRect();
}

inline QPainterPath FillBetweenItem::shape() const
{
    ensurePathCurrent();
    return QGraphicsPathItem::shape();
}

inline void FillBetweenItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    ensurePathCurrent();
    QGraphicsPathItem::paint(painter, option, widget);
}

inline void FillBetweenItem::updatePath()
{
    QPainterPath path;
    path.setFillRule(fillRule_);
    curve1Fingerprint_ = fingerprint(curve1_.data());
    curve2Fingerprint_ = fingerprint(curve2_.data());
    if (curve1_.isNull() || curve2_.isNull()) {
        setPath(path);
        return;
    }
    const std::vector<detail_fillbetweenitem::SampleSegment> firstSegments =
        detail_fillbetweenitem::connectedSampleSegments(curve1_.data());
    const std::vector<detail_fillbetweenitem::SampleSegment> secondSegments =
        detail_fillbetweenitem::connectedSampleSegments(curve2_.data());
    const std::size_t segmentCount = std::min(firstSegments.size(), secondSegments.size());
    for (std::size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
        const std::vector<QPointF> first =
            detail_fillbetweenitem::steppedPointsForSamples(curve1_.data(), firstSegments[segmentIndex]);
        const std::vector<QPointF> second =
            detail_fillbetweenitem::steppedPointsForSamples(curve2_.data(), secondSegments[segmentIndex]);
        if (first.size() < 2U || second.size() < 2U) {
            continue;
        }
        QPolygonF polygon;
        polygon.reserve(static_cast<qsizetype>(first.size() + second.size()));
        for (const QPointF& point : first) {
            polygon << point;
        }
        for (auto it = second.rbegin(); it != second.rend(); ++it) {
            polygon << *it;
        }
        path.addPolygon(polygon);
    }
    setPath(path);
}

} // namespace pyqtgraph::graphicsItems
