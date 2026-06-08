// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ROI.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/ROI.hpp"

#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <cmath>

namespace pyqtgraph::graphicsItems {
namespace {

qreal snappedCoordinate(qreal value, qreal snap)
{
    if (snap == 0.0) {
        return value;
    }
    return std::round(value / snap) * snap;
}

} // namespace

ROI::ROI(const QPointF& pos, const QPointF& size, qreal angle, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , state_{pyqtgraph::Point(0.0, 0.0), pyqtgraph::Point(1.0, 1.0), 0.0}
    , lastState_(state_)
    , pen_(QColor(255, 255, 255))
    , hoverPen_(QColor(255, 255, 0))
    , currentPen_(pen_)
{
    setAcceptedMouseButtons(Qt::NoButton);
    setPos(pos);
    setAngle(angle);
    setSize(size);
    setZValue(10.0);
}

ROI::~ROI() = default;

ROIState ROI::getState() const
{
    return stateCopy();
}

ROIState ROI::stateCopy() const
{
    return state_;
}

ROIState ROI::saveState() const
{
    return stateCopy();
}

void ROI::setState(const ROIState& state, bool update)
{
    setPos(state.pos, false, false);
    setSize(state.size, false, false);
    setAngle(state.angle, update, true);
}

void ROI::setZValue(qreal z)
{
    QGraphicsItem::setZValue(z);
}

pyqtgraph::Point ROI::size() const
{
    return getState().size;
}

pyqtgraph::Point ROI::pos() const
{
    return getState().pos;
}

qreal ROI::angle() const noexcept
{
    return state_.angle;
}

void ROI::setPos(const QPointF& pos, bool update, bool finish)
{
    state_.pos = pyqtgraph::Point(pos);
    GraphicsObject::setPos(state_.pos);
    if (update) {
        stateChanged(finish);
    }
}

void ROI::setPos(qreal x, qreal y, bool update, bool finish)
{
    setPos(QPointF(x, y), update, finish);
}

void ROI::setSize(const QPointF& size, bool update, bool finish)
{
    prepareGeometryChange();
    state_.size = pyqtgraph::Point(size);
    if (update) {
        stateChanged(finish);
    }
}

void ROI::setAngle(qreal angle, bool update, bool finish)
{
    state_.angle = angle;
    QTransform transform;
    transform.rotate(angle);
    setTransform(transform);
    if (update) {
        stateChanged(finish);
    }
}

void ROI::translate(const QPointF& delta, bool snap, bool finish, bool update)
{
    QPointF next = state_.pos + delta;
    if (snap) {
        next = getSnapPosition(next, true);
    }
    setPos(next, update, finish);
}

QPointF ROI::getSnapPosition(const QPointF& pos, bool snap) const
{
    if (!snap) {
        return pos;
    }
    return getSnapPosition(pos, QPointF(snapSize_, snapSize_));
}

QPointF ROI::getSnapPosition(const QPointF& pos, const QPointF& snap) const
{
    return QPointF(snappedCoordinate(pos.x(), snap.x()), snappedCoordinate(pos.y(), snap.y()));
}

void ROI::setSnapSize(qreal snapSize) noexcept
{
    snapSize_ = snapSize;
}

qreal ROI::snapSize() const noexcept
{
    return snapSize_;
}

void ROI::stateChanged(bool finish)
{
    bool changed = !haveLastState_ || state_ != lastState_;

    prepareGeometryChange();
    if (changed || freeHandleMoved_) {
        update();
        emit sigRegionChanged(this);
    }

    freeHandleMoved_ = false;
    lastState_ = getState();
    haveLastState_ = true;

    if (finish) {
        stateChangeFinished();
    }
}

void ROI::stateChangeFinished()
{
    emit sigRegionChangeFinished(this);
}

QRectF ROI::parentBounds() const
{
    return mapToParent(boundingRect()).boundingRect();
}

QRectF ROI::stateRect(const ROIState& state) const
{
    QRectF rect(0.0, 0.0, state.size.x(), state.size.y());
    QTransform transform;
    transform.rotate(-state.angle);
    rect = transform.mapRect(rect);
    return rect.adjusted(state.pos.x(), state.pos.y(), state.pos.x(), state.pos.y());
}

QRectF ROI::boundingRect() const
{
    return QRectF(0.0, 0.0, state_.size.x(), state_.size.y()).normalized();
}

void ROI::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QRectF rect = QRectF(0.0, 0.0, state_.size.x(), state_.size.y()).normalized();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(currentPen_);
    painter->save();
    painter->translate(rect.left(), rect.top());
    painter->scale(rect.width(), rect.height());
    painter->drawRect(QRectF(0.0, 0.0, 1.0, 1.0));
    painter->restore();
}

void ROI::setPen(const QPen& pen)
{
    pen_ = pen;
    currentPen_ = pen_;
    update();
}

QPen ROI::pen() const
{
    return pen_;
}

void ROI::setHoverPen(const QPen& pen)
{
    hoverPen_ = pen;
}

QPen ROI::hoverPen() const
{
    return hoverPen_;
}

} // namespace pyqtgraph::graphicsItems
