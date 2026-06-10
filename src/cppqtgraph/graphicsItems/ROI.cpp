// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ROI.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/ROI.hpp"

#include <cppqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtCore/QStringView>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cppqtgraph::graphicsItems {
namespace {

qreal snappedCoordinate(qreal value, qreal snap)
{
    if (snap == 0.0) {
        return value;
    }
    return std::round(value / snap) * snap;
}

cppqtgraph::Point imageLocalToData(const ImageItem& image, const QPointF& local)
{
    if (image.axisOrder() == ImageItem::AxisOrder::RowMajor) {
        return cppqtgraph::Point(local.y(), local.x());
    }
    return cppqtgraph::Point(local);
}

void requireSameScene(const QGraphicsItem& roi, const QGraphicsItem& image)
{
    if (roi.scene() != nullptr && image.scene() != nullptr && roi.scene() != image.scene()) {
        throw std::runtime_error("ROI and target item must be members of the same scene");
    }
}

std::pair<std::size_t, std::size_t> clippedBounds(qreal rawMin, qreal rawMax, std::size_t extent)
{
    if (extent == 0) {
        return {0, 0};
    }
    const qreal low = std::clamp(std::min(rawMin, rawMax), 0.0, static_cast<qreal>(extent));
    const qreal high = std::clamp(std::max(rawMin, rawMax), 0.0, static_cast<qreal>(extent));
    if (high <= low) {
        const auto point = static_cast<std::size_t>(std::clamp<qreal>(std::trunc(low), 0.0, static_cast<qreal>(extent)));
        return {point, point};
    }
    const auto start = static_cast<std::size_t>(std::clamp<qreal>(std::trunc(low), 0.0, static_cast<qreal>(extent)));
    const auto stop = static_cast<std::size_t>(std::clamp<qreal>(std::trunc(1.0 + high), 0.0, static_cast<qreal>(extent)));
    return {start, std::max(start, stop)};
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

void ROI::Handle::hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event == nullptr) {
        return;
    }

    const bool hovering = !event->isExit() && event->acceptDrags(Qt::LeftButton, this);
    currentPen_ = hovering ? hoverPen_ : pen_;
    update();
}

void ROI::Handle::mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event)
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
    int sides = 4;
    constexpr qreal pi = 3.141592653589793238462643383279502884L;
    qreal angle = 0.0;
    if (type_ == Type::Rotate || type_ == Type::ScaleRotate) {
        sides = 12;
    } else if (type_ == Type::Free) {
        angle = pi / 4.0;
    }
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
    , state_{cppqtgraph::Point(0.0, 0.0), cppqtgraph::Point(1.0, 1.0), 0.0}
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

cppqtgraph::Point ROI::size() const
{
    return getState().size;
}

cppqtgraph::Point ROI::pos() const
{
    return getState().pos;
}

qreal ROI::angle() const noexcept
{
    return state_.angle;
}

void ROI::setPos(const QPointF& pos, bool update, bool finish)
{
    state_.pos = cppqtgraph::Point(pos);
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
    state_.size = cppqtgraph::Point(size);
    if (update) {
        stateChanged(finish);
    }
}

void ROI::setSize(const QPointF& size,
                  const std::optional<QPointF>& center,
                  const std::optional<QPointF>& centerLocal,
                  bool snap,
                  bool update,
                  bool finish)
{
    cppqtgraph::Point newSize(size);
    if (snap) {
        newSize.setX(snappedCoordinate(newSize.x(), scaleSnapSize_));
        newSize.setY(snappedCoordinate(newSize.y(), scaleSnapSize_));
    }

    std::optional<cppqtgraph::Point> normalizedCenter;
    if (centerLocal.has_value()) {
        cppqtgraph::Point oldSize(state_.size);
        if (oldSize.x() == 0.0) {
            oldSize.setX(1.0);
        }
        if (oldSize.y() == 0.0) {
            oldSize.setY(1.0);
        }
        normalizedCenter = cppqtgraph::Point(centerLocal.value()) / oldSize;
    } else if (center.has_value()) {
        normalizedCenter = cppqtgraph::Point(center.value());
    }

    if (normalizedCenter.has_value()) {
        const cppqtgraph::Point oldLocal = normalizedCenter.value() * state_.size;
        const cppqtgraph::Point newLocal = normalizedCenter.value() * newSize;
        const cppqtgraph::Point oldParent = cppqtgraph::Point(mapToParent(oldLocal));
        const cppqtgraph::Point newParent = cppqtgraph::Point(mapToParent(newLocal));
        setPos(state_.pos + oldParent - newParent, false, false);
    }

    setSize(newSize, update, finish);
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

void ROI::setAngle(qreal angle,
                   const std::optional<QPointF>& center,
                   const std::optional<QPointF>& centerLocal,
                   bool snap,
                   bool update,
                   bool finish)
{
    if (snap) {
        angle = snappedCoordinate(angle, rotateSnapAngle_);
    }

    QTransform nextTransform;
    nextTransform.rotate(angle);

    std::optional<cppqtgraph::Point> localCenter;
    if (center.has_value()) {
        localCenter = cppqtgraph::Point(center.value()) * state_.size;
    } else if (centerLocal.has_value()) {
        localCenter = cppqtgraph::Point(centerLocal.value());
    }

    if (localCenter.has_value()) {
        const cppqtgraph::Point oldParent = cppqtgraph::Point(mapToParent(localCenter.value()));
        const cppqtgraph::Point nextParentAtCurrentPos = cppqtgraph::Point(nextTransform.map(localCenter.value())) + state_.pos;
        setPos(state_.pos + oldParent - nextParentAtCurrentPos, false, false);
    }

    state_.angle = angle;
    setTransform(nextTransform);
    if (update) {
        stateChanged(finish);
    }
}

void ROI::scale(const QPointF& factors,
                const std::optional<QPointF>& center,
                const std::optional<QPointF>& centerLocal,
                bool snap,
                bool update,
                bool finish)
{
    setSize(state_.size * factors, center, centerLocal, snap, update, finish);
}

void ROI::translate(const QPointF& delta, bool snap, bool finish, bool update)
{
    QPointF next = state_.pos + delta;
    if (snap) {
        next = getSnapPosition(next, true);
    }
    setPos(next, update, finish);
}

void ROI::translate(const QPointF& delta, const QPointF& snap, bool finish, bool update)
{
    setPos(getSnapPosition(state_.pos + delta, snap), update, finish);
}

void ROI::rotate(qreal angle, const std::optional<QPointF>& centerLocal, bool snap, bool update, bool finish)
{
    setAngle(state_.angle + angle, std::nullopt, centerLocal, snap, update, finish);
}

ROIAffineSliceParams ROI::getAffineSliceParams(const QGraphicsItem* target, bool fromBoundingRect) const
{
    if (target == nullptr) {
        throw std::invalid_argument("ROI::getAffineSliceParams target must not be null");
    }
    if (scene() != nullptr && target->scene() != nullptr && scene() != target->scene()) {
        throw std::runtime_error("ROI and target item must be members of the same scene");
    }

    const QPointF localOrigin = fromBoundingRect ? boundingRect().topLeft() : QPointF(0.0, 0.0);
    const cppqtgraph::Point origin(mapToItem(target, localOrigin));
    const cppqtgraph::Point vx = cppqtgraph::Point(mapToItem(target, localOrigin + QPointF(1.0, 0.0))) - origin;
    const cppqtgraph::Point vy = cppqtgraph::Point(mapToItem(target, localOrigin + QPointF(0.0, 1.0))) - origin;

    const qreal lx = std::hypot(vx.x(), vx.y());
    const qreal ly = std::hypot(vy.x(), vy.y());
    const cppqtgraph::Point vectorX = lx == 0.0 ? cppqtgraph::Point(0.0, 0.0) : vx / lx;
    const cppqtgraph::Point vectorY = ly == 0.0 ? cppqtgraph::Point(0.0, 0.0) : vy / ly;

    cppqtgraph::Point shape = fromBoundingRect ? cppqtgraph::Point(boundingRect().size()) : state_.size;
    shape.setX(std::abs(shape.x() * lx));
    shape.setY(std::abs(shape.y() * ly));

    return ROIAffineSliceParams{shape, {vectorX, vectorY}, origin};
}

ROIAffineSliceParams ROI::getAffineSliceParams(std::array<std::size_t, 2> dataShape,
                                               const ImageItem& image,
                                               bool fromBoundingRect) const
{
    Q_UNUSED(dataShape);
    requireSameScene(*this, image);

    const QPointF localOrigin = fromBoundingRect ? boundingRect().topLeft() : QPointF(0.0, 0.0);
    cppqtgraph::Point origin = imageLocalToData(image, mapToItem(&image, localOrigin));
    cppqtgraph::Point vx = imageLocalToData(image, mapToItem(&image, localOrigin + QPointF(1.0, 0.0))) - origin;
    cppqtgraph::Point vy = imageLocalToData(image, mapToItem(&image, localOrigin + QPointF(0.0, 1.0))) - origin;

    const qreal lx = std::hypot(vx.x(), vx.y());
    const qreal ly = std::hypot(vy.x(), vy.y());
    cppqtgraph::Point vectorX = lx == 0.0 ? cppqtgraph::Point(0.0, 0.0) : vx / lx;
    cppqtgraph::Point vectorY = ly == 0.0 ? cppqtgraph::Point(0.0, 0.0) : vy / ly;

    cppqtgraph::Point shape = fromBoundingRect ? cppqtgraph::Point(boundingRect().size()) : state_.size;
    shape.setX(std::abs(shape.x() * lx));
    shape.setY(std::abs(shape.y() * ly));

    if (image.axisOrder() == ImageItem::AxisOrder::RowMajor) {
        std::swap(vectorX, vectorY);
        shape = cppqtgraph::Point(shape.y(), shape.x());
    }

    return ROIAffineSliceParams{shape, {vectorX, vectorY}, origin};
}

std::optional<ROIArraySlice> ROI::getArraySlice(std::array<std::size_t, 2> dataShape, const ImageItem& image) const
{
    requireSameScene(*this, image);
    if (!image.hasImage() || image.width() == 0 || image.height() == 0 || dataShape[0] == 0 || dataShape[1] == 0) {
        return std::nullopt;
    }

    bool invertible = false;
    const QTransform inverseImageScene = image.sceneTransform().inverted(&invertible);
    if (!invertible) {
        return std::nullopt;
    }
    QTransform transform = sceneTransform() * inverseImageScene;
    if (image.axisOrder() == ImageItem::AxisOrder::RowMajor) {
        transform.scale(static_cast<qreal>(dataShape[1]) / static_cast<qreal>(image.width()),
                        static_cast<qreal>(dataShape[0]) / static_cast<qreal>(image.height()));
    } else {
        transform.scale(static_cast<qreal>(dataShape[0]) / static_cast<qreal>(image.width()),
                        static_cast<qreal>(dataShape[1]) / static_cast<qreal>(image.height()));
    }

    const QRectF bounds = boundingRect();
    const std::array<QPointF, 4> corners{bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()};
    qreal minX = std::numeric_limits<qreal>::infinity();
    qreal maxX = -std::numeric_limits<qreal>::infinity();
    qreal minY = std::numeric_limits<qreal>::infinity();
    qreal maxY = -std::numeric_limits<qreal>::infinity();
    for (const QPointF& corner : corners) {
        const QPointF mapped = mapToItem(&image, corner);
        minX = std::min(minX, mapped.x());
        maxX = std::max(maxX, mapped.x());
        minY = std::min(minY, mapped.y());
        maxY = std::max(maxY, mapped.y());
    }

    ROIArraySlice result;
    result.transform = transform;
    if (image.axisOrder() == ImageItem::AxisOrder::RowMajor) {
        const qreal scaleX = static_cast<qreal>(dataShape[1]) / static_cast<qreal>(image.width());
        const qreal scaleY = static_cast<qreal>(dataShape[0]) / static_cast<qreal>(image.height());
        result.bounds = {clippedBounds(minY * scaleY, maxY * scaleY, dataShape[0]),
                         clippedBounds(minX * scaleX, maxX * scaleX, dataShape[1])};
    } else {
        const qreal scaleX = static_cast<qreal>(dataShape[0]) / static_cast<qreal>(image.width());
        const qreal scaleY = static_cast<qreal>(dataShape[1]) / static_cast<qreal>(image.height());
        result.bounds = {clippedBounds(minX * scaleX, maxX * scaleX, dataShape[0]),
                         clippedBounds(minY * scaleY, maxY * scaleY, dataShape[1])};
    }
    return result;
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

bool ROI::handleUsesAbsolutePosition(Handle::Type type) const noexcept
{
    return type == Handle::Type::Free;
}

ROI::Handle* ROI::addHandle(HandleInfo info, int index)
{
    if (info.item == nullptr) {
        info.item = new Handle(5.0, info.type, QPen(QColor(200, 200, 220)), QPen(QColor(255, 255, 0)), this);
    } else if (info.item->parentItem() == nullptr) {
        info.item->setParentItem(this);
    }

    if (handleUsesAbsolutePosition(info.type)) {
        info.item->setPos(info.pos);
    } else {
        info.item->setPos(info.pos * state_.size);
    }
    info.item->connectROI(this);
    if (index < 0 || index >= handles_.size()) {
        handles_.append(info);
    } else {
        handles_.insert(index, info);
    }
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

void ROI::clearHandles()
{
    while (!handles_.isEmpty()) {
        HandleInfo info = handles_.takeLast();
        if (info.item != nullptr) {
            info.item->disconnectROI(this);
            if (info.item->parentItem() == this) {
                delete info.item;
            }
        }
    }
    stateChanged();
}

ROI::Handle* ROI::addScaleHandle(const QPointF& pos,
                                 const QPointF& center,
                                 Handle* item,
                                 const QString& name,
                                 bool lockAspect)
{
    const cppqtgraph::Point handlePos(pos);
    const cppqtgraph::Point handleCenter(center);
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

ROI::Handle* ROI::addFreeHandle(const QPointF& pos, Handle* item, const QString& name)
{
    HandleInfo info;
    info.name = name;
    info.type = Handle::Type::Free;
    info.pos = cppqtgraph::Point(pos);
    info.item = item;
    return addHandle(info);
}

ROI::Handle* ROI::addRotateHandle(const QPointF& pos, const QPointF& center, Handle* item, const QString& name)
{
    HandleInfo info;
    info.name = name;
    info.type = Handle::Type::Rotate;
    info.pos = cppqtgraph::Point(pos);
    info.center = cppqtgraph::Point(center);
    info.item = item;
    return addHandle(info);
}

ROI::Handle* ROI::addScaleRotateHandle(const QPointF& pos, const QPointF& center, Handle* item, const QString& name)
{
    const cppqtgraph::Point handlePos(pos);
    const cppqtgraph::Point handleCenter(center);
    if (handlePos == handleCenter) {
        throw std::invalid_argument("Scale/rotate handles cannot be at their center point");
    }
    HandleInfo info;
    info.name = name;
    info.type = Handle::Type::ScaleRotate;
    info.pos = handlePos;
    info.center = handleCenter;
    info.item = item;
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
    ROIState newState = stateCopy();
    cppqtgraph::Point p1(pos);
    if (coords == HandleCoordinateSystem::Scene) {
        p1 = cppqtgraph::Point(mapFromScene(p1));
        p1 = cppqtgraph::Point(mapToParent(p1));
    }

    if (info.type == Handle::Type::Free) {
        const cppqtgraph::Point newPos = cppqtgraph::Point(mapFromParent(p1));
        info.item->setPos(newPos);
        info.pos = newPos;
        freeHandleMoved_ = true;
        stateChanged(finish);
        return;
    }

    const cppqtgraph::Point p0 = cppqtgraph::Point(mapToParent(info.pos * state_.size));
    const cppqtgraph::Point center = info.center;
    const cppqtgraph::Point centerScaled = center * state_.size;
    const cppqtgraph::Point localP0 = cppqtgraph::Point(mapFromParent(p0)) - centerScaled;
    cppqtgraph::Point localP1 = cppqtgraph::Point(mapFromParent(p1)) - centerScaled;

    if (info.type == Handle::Type::Scale) {
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

        cppqtgraph::Point handleScale = info.pos - center;
        if (handleScale.x() == 0.0) {
            handleScale.setX(1.0);
        }
        if (handleScale.y() == 0.0) {
            handleScale.setY(1.0);
        }
        cppqtgraph::Point newSize = localP1 / handleScale;
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

        const cppqtgraph::Point oldCenterScaled = center * state_.size;
        const cppqtgraph::Point newCenterScaled = center * newSize;
        const cppqtgraph::Point correction = cppqtgraph::Point(mapToParent(oldCenterScaled - newCenterScaled))
            - cppqtgraph::Point(mapToParent(QPointF(0.0, 0.0)));

        newState.size = newSize;
        newState.pos += correction;
        setPos(newState.pos, false, false);
        setSize(newState.size, false, false);
        stateChanged(finish);
        return;
    }

    if (localP0.length() == 0.0 || localP1.length() == 0.0) {
        return;
    }

    if (info.type == Handle::Type::Rotate) {
        qreal ang = newState.angle - localP0.angle(localP1);
        if (scaleSnap_ || (modifiers & Qt::ControlModifier)) {
            ang = snappedCoordinate(ang, rotateSnapAngle_);
        }

        QTransform nextTransform;
        nextTransform.rotate(ang);

        const cppqtgraph::Point oldParent = cppqtgraph::Point(mapToParent(centerScaled));
        const cppqtgraph::Point nextParentAtCurrentPos = cppqtgraph::Point(nextTransform.map(centerScaled)) + newState.pos;
        newState.pos += oldParent - nextParentAtCurrentPos;
        newState.angle = ang;
        setPos(newState.pos, false, false);
        setAngle(ang, false, false);
        stateChanged(finish);
        return;
    }

    if (info.type == Handle::Type::ScaleRotate) {
        qreal ang = newState.angle - localP0.angle(localP1);
        if (scaleSnap_ || (modifiers & Qt::ControlModifier)) {
            ang = snappedCoordinate(ang, rotateSnapAngle_);
        }

        cppqtgraph::Point newSize = newState.size;
        const qreal lengthRatio = localP1.length() / localP0.length();
        if (aspectLocked_ || info.center.x() != info.pos.x()) {
            newSize.setX(state_.size.x() * lengthRatio);
            if (scaleSnap_) {
                newSize.setX(snappedCoordinate(newSize.x(), snapSize_));
            }
        }
        if (aspectLocked_ || info.center.y() != info.pos.y()) {
            newSize.setY(state_.size.y() * lengthRatio);
            if (scaleSnap_) {
                newSize.setY(snappedCoordinate(newSize.y(), snapSize_));
            }
        }
        if (newSize.x() == 0.0) {
            newSize.setX(1.0);
        }
        if (newSize.y() == 0.0) {
            newSize.setY(1.0);
        }

        const cppqtgraph::Point centerAtNewSize = center * newSize;
        QTransform nextTransform;
        nextTransform.rotate(ang);

        const cppqtgraph::Point oldParent = cppqtgraph::Point(mapToParent(centerScaled));
        const cppqtgraph::Point nextParentAtCurrentPos = cppqtgraph::Point(nextTransform.map(centerAtNewSize)) + newState.pos;
        newState.pos += oldParent - nextParentAtCurrentPos;
        newState.angle = ang;
        newState.size = newSize;
        setPos(newState.pos, false, false);
        setSize(newState.size, false, false);
        setAngle(ang, false, false);
        stateChanged(finish);
    }
}

void ROI::stateChanged(bool finish)
{
    bool changed = !haveLastState_ || state_ != lastState_;

    prepareGeometryChange();
    if (changed) {
        for (const HandleInfo& info : handles_) {
            if (info.item != nullptr && info.item->parentItem() == this) {
                if (handleUsesAbsolutePosition(info.type)) {
                    info.item->setPos(info.pos);
                } else {
                    info.item->setPos(info.pos * state_.size);
                }
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

void ROI::setAspectLocked(bool locked) noexcept
{
    aspectLocked_ = locked;
}

bool ROI::aspectLocked() const noexcept
{
    return aspectLocked_;
}

RectROI::RectROI(const QPointF& pos, const QPointF& size, bool centered, bool sideScalers, QGraphicsItem* parent)
    : ROI(pos, size, 0.0, parent)
{
    const QPointF center = centered ? QPointF(0.5, 0.5) : QPointF(0.0, 0.0);
    addScaleHandle(QPointF(1.0, 1.0), center);
    if (sideScalers) {
        addScaleHandle(QPointF(1.0, 0.5), QPointF(center.x(), 0.5));
        addScaleHandle(QPointF(0.5, 1.0), QPointF(0.5, center.y()));
    }
}

EllipseROI::EllipseROI(const QPointF& pos, const QPointF& size, QGraphicsItem* parent)
    : ROI(pos, size, 0.0, parent)
{
    QObject::connect(this, &ROI::sigRegionChanged, this, [this](ROI*) { invalidateShapePath(); });
    addShapeHandles();
}

void EllipseROI::invalidateShapePath() noexcept
{
    shapePathValid_ = false;
}

void EllipseROI::addShapeHandles()
{
    addRotateHandle(QPointF(1.0, 0.5), QPointF(0.5, 0.5));
    const qreal offset = 0.5 * std::pow(2.0, -0.5) + 0.5;
    addScaleHandle(QPointF(offset, offset), QPointF(0.5, 0.5));
}

QPainterPath EllipseROI::shape() const
{
    if (!shapePathValid_) {
        const QRectF rect = boundingRect();
        const QPointF center = rect.center();
        const qreal radiusX = rect.width() / 2.0;
        const qreal radiusY = rect.height() / 2.0;
        constexpr qreal pi = 3.141592653589793238462643383279502884L;
        shapePath_ = QPainterPath();
        constexpr int segments = 24;
        for (int i = 0; i <= segments; ++i) {
            const qreal theta = 2.0 * pi * static_cast<qreal>(i) / static_cast<qreal>(segments);
            const QPointF point(center.x() + radiusX * std::cos(theta), center.y() + radiusY * std::sin(theta));
            if (i == 0) {
                shapePath_.moveTo(point);
            } else {
                shapePath_.lineTo(point);
            }
        }
        shapePathValid_ = true;
    }
    return shapePath_;
}

void EllipseROI::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QRectF rect = QRectF(0.0, 0.0, state_.size.x(), state_.size.y()).normalized();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen());
    painter->save();
    painter->translate(rect.left(), rect.top());
    painter->scale(rect.width(), rect.height());
    painter->drawEllipse(QRectF(0.0, 0.0, 1.0, 1.0));
    painter->restore();
}

CircleROI::CircleROI(const QPointF& pos, const QPointF& size, QGraphicsItem* parent)
    : EllipseROI(pos, size, parent)
{
    setAspectLocked(true);
    clearHandles();
    addShapeHandles();
}

CircleROI::CircleROI(const QPointF& pos, qreal radius, QGraphicsItem* parent)
    : CircleROI(pos, QPointF(radius * 2.0, radius * 2.0), parent)
{
}

void CircleROI::addShapeHandles()
{
    const qreal offset = 0.5 * std::pow(2.0, -0.5) + 0.5;
    addScaleHandle(QPointF(offset, offset), QPointF(0.5, 0.5));
}

LineROI::LineROI(const QPointF& pos1, const QPointF& pos2, qreal width, QGraphicsItem* parent)
    : ROI(QPointF(), QPointF(1.0, 1.0), 0.0, parent)
{
    const cppqtgraph::Point start(pos1);
    const cppqtgraph::Point end(pos2);
    const cppqtgraph::Point delta = end - start;
    const qreal length = delta.length();
    const qreal radians = delta.angle(cppqtgraph::Point(1.0, 0.0), QStringView(u"radians"));
    constexpr qreal pi = 3.141592653589793238462643383279502884L;
    const cppqtgraph::Point offset(width / 2.0 * std::sin(radians), -width / 2.0 * std::cos(radians));
    const cppqtgraph::Point origin = start + offset;

    setPos(origin, false, false);
    setSize(QPointF(length, width), false, false);
    setAngle(radians * 180.0 / pi, true, true);

    addScaleRotateHandle(QPointF(0.0, 0.5), QPointF(1.0, 0.5));
    addScaleRotateHandle(QPointF(1.0, 0.5), QPointF(0.0, 0.5));
    addScaleHandle(QPointF(0.5, 1.0), QPointF(0.5, 0.5));
}

PolyLineROI::PolyLineROI(const QVector<QPointF>& positions, bool closed, const QPointF& pos, QGraphicsItem* parent)
    : ROI(pos, QPointF(1.0, 1.0), 0.0, parent)
    , closed_(closed)
{
    setPoints(positions, closed);
}

void PolyLineROI::setPoints(const QVector<QPointF>& points, std::optional<bool> closed)
{
    if (closed.has_value()) {
        closed_ = *closed;
    }

    clearHandles();
    for (const QPointF& point : points) {
        addFreeHandle(point);
    }
    prepareGeometryChange();
    stateChanged(true);
}

bool PolyLineROI::closed() const noexcept
{
    return closed_;
}

QVector<QPointF> PolyLineROI::pointPositions() const
{
    QVector<QPointF> points;
    const QList<Handle*> handles = getHandles();
    points.reserve(handles.size());
    for (const Handle* handle : handles) {
        if (handle != nullptr) {
            points.append(handle->pos());
        }
    }
    return points;
}

QPainterPath PolyLineROI::shape() const
{
    QPainterPath path;
    const QVector<QPointF> points = pointPositions();
    if (points.isEmpty()) {
        return path;
    }

    path.moveTo(points.front());
    for (int i = 1; i < points.size(); ++i) {
        path.lineTo(points.at(i));
    }
    if (closed_ && points.size() > 1) {
        path.lineTo(points.front());
    }
    return path;
}

QRectF PolyLineROI::boundingRect() const
{
    return shape().boundingRect();
}

void PolyLineROI::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QPainterPath outline = shape();
    if (outline.isEmpty()) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen());
    painter->drawPath(outline);
}

} // namespace cppqtgraph::graphicsItems
