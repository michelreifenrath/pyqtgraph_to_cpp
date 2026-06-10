// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/CurvePoint.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/CurvePoint.hpp"

#include <QtCore/QtGlobal>
#include <QtMath>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsPathItem>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cppqtgraph::graphicsItems {
namespace {

double clipScalar(double value, double minimum, double maximum)
{
    return std::clamp(value, minimum, maximum);
}

std::size_t clipIndex(std::ptrdiff_t value, std::size_t maximum)
{
    const auto clipped = std::clamp<std::ptrdiff_t>(value, 0, static_cast<std::ptrdiff_t>(maximum));
    return static_cast<std::size_t>(clipped);
}

QPainterPath makeArrowPath(const CurveArrowStyle& style)
{
    const double headWidth = style.headWidth.value_or(style.headLen * std::tan(qDegreesToRadians(style.tipAngle * 0.5)));
    QPainterPath path;
    path.moveTo(0.0, 0.0);
    path.lineTo(style.headLen, -headWidth);
    if (!style.tailLen.has_value()) {
        const double innerX = style.headLen - headWidth * std::tan(qDegreesToRadians(style.baseAngle));
        path.lineTo(innerX, 0.0);
    } else {
        const double halfTailWidth = style.tailWidth * 0.5;
        const double innerX = style.headLen - (headWidth - halfTailWidth) * std::tan(qDegreesToRadians(style.baseAngle));
        path.lineTo(innerX, -halfTailWidth);
        path.lineTo(style.headLen + *style.tailLen, -halfTailWidth);
        path.lineTo(style.headLen + *style.tailLen, halfTailWidth);
        path.lineTo(innerX, halfTailWidth);
    }
    path.lineTo(style.headLen, headWidth);
    path.closeSubpath();
    return QTransform().rotate(style.angle).map(path);
}

} // namespace

CurvePoint::CurvePoint(PlotCurveItem* curve, int index, bool rotate, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , curve_(curve)
    , rotate_(rotate)
{
    if (curve_ != nullptr) {
        setParentItem(curve_);
    }
    setFlag(QGraphicsItem::ItemHasNoContents, true);
    setIndex(index);
}

CurvePoint::CurvePoint(PlotCurveItem* curve, double position, bool rotate, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , curve_(curve)
    , rotate_(rotate)
{
    if (curve_ != nullptr) {
        setParentItem(curve_);
    }
    setFlag(QGraphicsItem::ItemHasNoContents, true);
    setPosition(position);
}

CurvePoint::~CurvePoint() = default;

void CurvePoint::setPos(double position)
{
    setPosition(position);
}

void CurvePoint::setPosition(double position)
{
    position_ = position;
    updateMode_ = UpdateMode::Position;
    updateFromCurve(updateMode_);
}

double CurvePoint::position() const noexcept
{
    return position_;
}

void CurvePoint::setIndex(int index)
{
    index_ = index;
    updateMode_ = UpdateMode::Index;
    updateFromCurve(updateMode_);
}

int CurvePoint::index() const noexcept
{
    return index_;
}

PlotCurveItem* CurvePoint::curve() const noexcept
{
    return curve_.data();
}

void CurvePoint::setRotate(bool rotate)
{
    if (rotate_ == rotate) {
        return;
    }
    rotate_ = rotate;
    updateFromCurve(updateMode_);
}

bool CurvePoint::rotate() const noexcept
{
    return rotate_;
}

QPropertyAnimation* CurvePoint::makeAnimation(QByteArray property, double start, double end, int duration, int loop)
{
    auto* animation = new QPropertyAnimation(this, property);
    animation->setDuration(duration);
    animation->setStartValue(start);
    animation->setEndValue(end);
    animation->setLoopCount(loop);
    return animation;
}

QRectF CurvePoint::boundingRect() const
{
    return QRectF();
}

void CurvePoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

bool CurvePoint::updateFromCurve(UpdateMode mode)
{
    PlotCurveItem* currentCurve = curve_.data();
    if (currentCurve == nullptr) {
        return false;
    }

    const auto x = currentCurve->xData();
    const auto y = currentCurve->yData();
    const std::size_t count = std::min(x.size(), y.size());
    if (count == 0) {
        return false;
    }

    double sampleIndex = static_cast<double>(index_);
    if (mode == UpdateMode::Position) {
        sampleIndex = (static_cast<double>(count) - 1.0) * clipScalar(position_, 0.0, 1.0);
    } else if (index_ < 0 || static_cast<std::size_t>(index_) >= count) {
        throw std::out_of_range("CurvePoint index is outside the curve data range");
    }

    QPointF newPosition;
    std::size_t tangentStart = 0;
    std::size_t tangentEnd = 0;
    const double nearestInteger = std::floor(sampleIndex);
    if (!qFuzzyCompare(sampleIndex + 1.0, nearestInteger + 1.0)) {
        const auto first = static_cast<std::size_t>(nearestInteger);
        const auto second = clipIndex(static_cast<std::ptrdiff_t>(first) + 1, count - 1);
        const double secondWeight = sampleIndex - nearestInteger;
        const double firstWeight = 1.0 - secondWeight;
        newPosition = QPointF(
            x[first] * firstWeight + x[second] * secondWeight,
            y[first] * firstWeight + y[second] * secondWeight);
        tangentStart = first;
        tangentEnd = second;
    } else {
        const auto index = static_cast<std::size_t>(sampleIndex);
        tangentStart = clipIndex(static_cast<std::ptrdiff_t>(index) - 1, count - 1);
        tangentEnd = clipIndex(static_cast<std::ptrdiff_t>(index) + 1, count - 1);
        newPosition = QPointF(x[index], y[index]);
    }

    const QPointF p1 = currentCurve->mapToScene(QPointF(x[tangentStart], y[tangentStart]));
    const QPointF p2 = currentCurve->mapToScene(QPointF(x[tangentEnd], y[tangentEnd]));
    const double radians = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
    resetTransform();
    if (rotate_) {
        setRotation(180.0 + qRadiansToDegrees(radians));
    } else {
        setRotation(0.0);
    }
    QGraphicsItem::setPos(newPosition);
    return true;
}

CurveArrow::CurveArrow(PlotCurveItem* curve, int index, QGraphicsItem* parent)
    : CurvePoint(curve, index, true, parent)
{
    createArrow();
}

CurveArrow::CurveArrow(PlotCurveItem* curve, double position, QGraphicsItem* parent)
    : CurvePoint(curve, position, true, parent)
{
    createArrow();
}

CurveArrow::~CurveArrow() = default;

void CurveArrow::setStyle(const CurveArrowStyle& style)
{
    style_ = style;
    applyStyle();
}

const CurveArrowStyle& CurveArrow::style() const noexcept
{
    return style_;
}

QGraphicsPathItem* CurveArrow::arrow() noexcept
{
    return arrow_;
}

const QGraphicsPathItem* CurveArrow::arrow() const noexcept
{
    return arrow_;
}

void CurveArrow::createArrow()
{
    arrow_ = new QGraphicsPathItem(this);
    applyStyle();
}

void CurveArrow::applyStyle()
{
    if (style_.pxMode) {
        setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    } else {
        setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
    }

    if (arrow_ == nullptr) {
        return;
    }
    arrow_->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
    arrow_->setPath(makeArrowPath(style_));
    arrow_->setPen(style_.pen);
    arrow_->setBrush(style_.brush);
}

} // namespace cppqtgraph::graphicsItems
