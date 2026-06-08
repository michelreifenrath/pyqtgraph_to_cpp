// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ROI.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/ROI.hpp"

#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

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

ROI::Handle::Handle(qreal radius, Type type, const QPen& pen, const QPen& hoverPen, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , radius_(radius)
    , type_(type)
    , pen_(pen)
    , hoverPen_(hoverPen)
    , currentPen_(pen_)
{
    setAcceptedMouseButtons(Qt::NoButton);
    buildPath();
    setZValue(11.0);
}

ROI::Handle::~Handle() = default;

void ROI::Handle::connectROI(ROI* roi)
{
    if (roi != nullptr && !rois_.contains(roi)) {
        rois_.append(roi);
    }
}

void ROI::Handle::disconnectROI(ROI* roi)
{
    rois_.removeAll(roi);
}

void ROI::Handle::movePoint(const QPointF& pos, Qt::KeyboardModifiers modifiers, bool finish)
{
    for (ROI* roi : rois_) {
        if (roi != nullptr && !roi->checkPointMove(this, pos, modifiers)) {
            return;
        }
    }
    for (ROI* roi : rois_) {
        if (roi != nullptr) {
            roi->movePoint(this, pos, modifiers, finish, ROI::HandleCoordinateSystem::Scene);
        }
    }
}

void ROI::Handle::hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event == nullptr) {
        return;
    }

    const bool hovering = !event->isExit() && event->acceptDrags(Qt::LeftButton, this);
    currentPen_ = hovering ? hoverPen_ : pen_;
    update();
}

void ROI::Handle::mouseDragEvent(pyqtgraph::GraphicsScene::MouseDragEvent* event)
{
    if (event == nullptr || event->button() != Qt::LeftButton) {
        return;
    }
    event->accept(this);

    if (event->isFinish()) {
        if (isMoving_) {
            for (ROI* roi : rois_) {
                if (roi != nullptr) {
                    roi->stateChangeFinished();
                }
            }
        }
        isMoving_ = false;
        currentPen_ = pen_;
        update();
    } else if (event->isStart()) {
        for (ROI* roi : rois_) {
            if (roi != nullptr) {
                roi->handleMoveStarted();
            }
        }
        isMoving_ = true;
        startPos_ = scenePos();
        cursorOffset_ = scenePos() - event->buttonDownScenePos();
        currentPen_ = hoverPen_;
        update();
    }

    if (isMoving_) {
        const QPointF nextPos = event->scenePos() + cursorOffset_;
        currentPen_ = hoverPen_;
        movePoint(nextPos, event->modifiers(), false);
    }
}

QRectF ROI::Handle::boundingRect() const
{
    return path_.boundingRect();
}

QPainterPath ROI::Handle::shape() const
{
    return path_;
}

void ROI::Handle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(currentPen_);
    painter->drawPath(path_);
}

void ROI::Handle::buildPath()
{
    path_ = QPainterPath();
    const int sides = 4;
    constexpr qreal pi = 3.141592653589793238462643383279502884L;
    qreal angle = type_ == Type::Scale ? 0.0 : pi / 4.0;
    const qreal delta = 2.0 * pi / static_cast<qreal>(sides);
    for (int i = 0; i < sides; ++i) {
        const qreal x = radius_ * std::cos(angle);
        const qreal y = radius_ * std::sin(angle);
        angle += delta;
        if (i == 0) {
            path_.moveTo(x, y);
        } else {
            path_.lineTo(x, y);
        }
    }
    path_.closeSubpath();
}

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
    for (const HandleInfo& info : handles_) {
        if (info.item != nullptr) {
            info.item->setZValue(z + 1.0);
        }
    }
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

ROI::Handle* ROI::addHandle(HandleInfo info)
{
    if (info.item == nullptr) {
        info.item = new Handle(5.0, info.type, QPen(QColor(200, 200, 220)), QPen(QColor(255, 255, 0)), this);
    } else if (info.item->parentItem() == nullptr) {
        info.item->setParentItem(this);
    }

    info.item->setPos(info.pos * state_.size);
    info.item->connectROI(this);
    handles_.append(info);
    info.item->setZValue(zValue() + 1.0);
    stateChanged();
    return info.item;
}

int ROI::indexOfHandle(const Handle* handle) const
{
    for (int i = 0; i < handles_.size(); ++i) {
        if (handles_[i].item == handle) {
            return i;
        }
    }
    return -1;
}

ROI::Handle* ROI::addScaleHandle(const QPointF& pos,
                           const QPointF& center,
                           Handle* item,
                           const QString& name,
                           bool lockAspect)
{
    const pyqtgraph::Point handlePos(pos);
    const pyqtgraph::Point handleCenter(center);
    HandleInfo info;
    info.name = name;
    info.type = Handle::Type::Scale;
    info.pos = handlePos;
    info.center = handleCenter;
    info.item = item;
    info.lockAspect = lockAspect;
    info.xOff = handlePos.x() == handleCenter.x();
    info.yOff = handlePos.y() == handleCenter.y();
    return addHandle(info);
}

QList<ROI::Handle*> ROI::getHandles() const
{
    QList<Handle*> result;
    result.reserve(handles_.size());
    for (const HandleInfo& info : handles_) {
        if (info.item != nullptr) {
            result.append(info.item);
        }
    }
    return result;
}

void ROI::handleMoveStarted()
{
    preMoveState_ = getState();
    emit sigRegionChangeStarted(this);
}

bool ROI::checkPointMove(const Handle* handle, const QPointF& pos, Qt::KeyboardModifiers modifiers) const
{
    Q_UNUSED(handle);
    Q_UNUSED(pos);
    Q_UNUSED(modifiers);
    return true;
}

void ROI::movePoint(Handle* handle, const QPointF& pos, Qt::KeyboardModifiers modifiers, bool finish, HandleCoordinateSystem coords)
{
    const int index = indexOfHandle(handle);
    if (index < 0) {
        return;
    }

    HandleInfo& info = handles_[index];
    if (info.type != Handle::Type::Scale) {
        return;
    }

    ROIState newState = stateCopy();
    const pyqtgraph::Point p0 = pyqtgraph::Point(mapToParent(info.pos * state_.size));
    pyqtgraph::Point p1(pos);
    if (coords == HandleCoordinateSystem::Scene) {
        p1 = pyqtgraph::Point(mapFromScene(p1));
        p1 = pyqtgraph::Point(mapToParent(p1));
    }

    const pyqtgraph::Point center = info.center;
    const pyqtgraph::Point centerScaled = center * state_.size;
    const pyqtgraph::Point localP0 = pyqtgraph::Point(mapFromParent(p0)) - centerScaled;
    pyqtgraph::Point localP1 = pyqtgraph::Point(mapFromParent(p1)) - centerScaled;

    if (info.xOff) {
        localP1.setX(0.0);
    }
    if (info.yOff) {
        localP1.setY(0.0);
    }

    if (scaleSnap_ || (modifiers & Qt::ControlModifier)) {
        localP1.setX(snappedCoordinate(localP1.x(), scaleSnapSize_));
        localP1.setY(snappedCoordinate(localP1.y(), scaleSnapSize_));
    }

    if (info.lockAspect || (modifiers & Qt::AltModifier)) {
        localP1 = localP1.proj(localP0);
    }

    pyqtgraph::Point handleScale = info.pos - center;
    if (handleScale.x() == 0.0) {
        handleScale.setX(1.0);
    }
    if (handleScale.y() == 0.0) {
        handleScale.setY(1.0);
    }
    pyqtgraph::Point newSize = localP1 / handleScale;
    if (newSize.x() == 0.0) {
        newSize.setX(newState.size.x());
    }
    if (newSize.y() == 0.0) {
        newSize.setY(newState.size.y());
    }
    if (!invertible_) {
        if (newSize.x() < 0.0) {
            newSize.setX(newState.size.x());
        }
        if (newSize.y() < 0.0) {
            newSize.setY(newState.size.y());
        }
    }
    if (aspectLocked_) {
        newSize.setX(newSize.y());
    }

    const pyqtgraph::Point oldCenterScaled = center * state_.size;
    const pyqtgraph::Point newCenterScaled = center * newSize;
    const pyqtgraph::Point correction = pyqtgraph::Point(mapToParent(oldCenterScaled - newCenterScaled))
        - pyqtgraph::Point(mapToParent(QPointF(0.0, 0.0)));

    newState.size = newSize;
    newState.pos += correction;
    setPos(newState.pos, false, false);
    setSize(newState.size, false, false);
    stateChanged(finish);
}

void ROI::stateChanged(bool finish)
{
    bool changed = !haveLastState_ || state_ != lastState_;

    prepareGeometryChange();
    if (changed) {
        for (const HandleInfo& info : handles_) {
            if (info.item != nullptr && info.item->parentItem() == this) {
                info.item->setPos(info.pos * state_.size);
            }
        }
    }
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
