// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/mouseEvents.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>

namespace {

pyqtgraph::Point toPoint(const QPointF& point)
{
    return pyqtgraph::Point(point);
}

pyqtgraph::Point toPoint(const QPoint& point)
{
    return pyqtgraph::Point(point);
}

pyqtgraph::Point mapFromScene(QGraphicsItem* item, const pyqtgraph::Point& scenePos)
{
    if (item == nullptr) {
        return scenePos;
    }
    return pyqtgraph::Point(item->mapFromScene(scenePos));
}

} // namespace

namespace pyqtgraph::GraphicsScene {

MouseDragEvent::MouseDragEvent(QGraphicsSceneMouseEvent* moveEvent, QGraphicsSceneMouseEvent* pressEvent,
    QGraphicsSceneMouseEvent* lastEvent, bool start, bool finish)
    : scenePos_(moveEvent != nullptr ? toPoint(moveEvent->scenePos()) : pyqtgraph::Point())
    , screenPos_(moveEvent != nullptr ? toPoint(moveEvent->screenPos()) : pyqtgraph::Point())
    , buttonDownScenePos_(pressEvent != nullptr ? toPoint(pressEvent->scenePos()) : pyqtgraph::Point())
    , buttonDownScreenPos_(pressEvent != nullptr ? toPoint(pressEvent->screenPos()) : pyqtgraph::Point())
    , lastScenePos_(lastEvent != nullptr ? toPoint(lastEvent->scenePos())
                                         : (pressEvent != nullptr ? toPoint(pressEvent->scenePos())
                                                                  : (moveEvent != nullptr ? toPoint(moveEvent->lastScenePos()) : scenePos_)))
    , lastScreenPos_(lastEvent != nullptr ? toPoint(lastEvent->screenPos())
                                          : (pressEvent != nullptr ? toPoint(pressEvent->screenPos())
                                                                   : (moveEvent != nullptr ? toPoint(moveEvent->lastScreenPos()) : screenPos_)))
    , buttons_(moveEvent != nullptr ? moveEvent->buttons()
                                    : (pressEvent != nullptr ? pressEvent->buttons() : Qt::MouseButtons(Qt::NoButton)))
    , button_(pressEvent != nullptr ? pressEvent->button()
                                    : (moveEvent != nullptr ? moveEvent->button() : Qt::NoButton))
    , modifiers_(moveEvent != nullptr ? moveEvent->modifiers()
                                      : (pressEvent != nullptr ? pressEvent->modifiers() : Qt::KeyboardModifiers(Qt::NoModifier)))
    , start_(start)
    , finish_(finish)
{
    if (pressEvent == nullptr && moveEvent != nullptr && button_ != Qt::NoButton) {
        buttonDownScenePos_ = toPoint(moveEvent->buttonDownScenePos(button_));
        buttonDownScreenPos_ = toPoint(moveEvent->buttonDownScreenPos(button_));
    }
}

void MouseDragEvent::accept(QGraphicsItem* item) noexcept
{
    accepted_ = true;
    acceptedItem_ = item != nullptr ? item : currentItem_;
}

void MouseDragEvent::ignore() noexcept
{
    accepted_ = false;
    acceptedItem_ = nullptr;
}

bool MouseDragEvent::isAccepted() const noexcept { return accepted_; }

void MouseDragEvent::setCurrentItem(QGraphicsItem* item) noexcept { currentItem_ = item; }

QGraphicsItem* MouseDragEvent::currentItem() const noexcept { return currentItem_; }

QGraphicsItem* MouseDragEvent::acceptedItem() const noexcept { return acceptedItem_; }

pyqtgraph::Point MouseDragEvent::scenePos() const { return scenePos_; }

pyqtgraph::Point MouseDragEvent::screenPos() const { return screenPos_; }

pyqtgraph::Point MouseDragEvent::buttonDownScenePos() const { return buttonDownScenePos_; }

pyqtgraph::Point MouseDragEvent::buttonDownScreenPos() const { return buttonDownScreenPos_; }

pyqtgraph::Point MouseDragEvent::lastScenePos() const { return lastScenePos_; }

pyqtgraph::Point MouseDragEvent::lastScreenPos() const { return lastScreenPos_; }

pyqtgraph::Point MouseDragEvent::pos() const { return mapFromScene(currentItem_, scenePos_); }

pyqtgraph::Point MouseDragEvent::lastPos() const { return mapFromScene(currentItem_, lastScenePos_); }

pyqtgraph::Point MouseDragEvent::buttonDownPos() const { return mapFromScene(currentItem_, buttonDownScenePos_); }

Qt::MouseButtons MouseDragEvent::buttons() const noexcept { return buttons_; }

Qt::MouseButton MouseDragEvent::button() const noexcept { return button_; }

bool MouseDragEvent::isStart() const noexcept { return start_; }

bool MouseDragEvent::isFinish() const noexcept { return finish_; }

Qt::KeyboardModifiers MouseDragEvent::modifiers() const noexcept { return modifiers_; }

MouseClickEvent::MouseClickEvent(QGraphicsSceneMouseEvent* pressEvent, bool doubleClick)
    : scenePos_(pressEvent != nullptr ? toPoint(pressEvent->scenePos()) : pyqtgraph::Point())
    , screenPos_(pressEvent != nullptr ? toPoint(pressEvent->screenPos()) : pyqtgraph::Point())
    , buttons_(pressEvent != nullptr ? pressEvent->buttons() : Qt::MouseButtons(Qt::NoButton))
    , button_(pressEvent != nullptr ? pressEvent->button() : Qt::NoButton)
    , modifiers_(pressEvent != nullptr ? pressEvent->modifiers() : Qt::KeyboardModifiers(Qt::NoModifier))
    , time_(pressEvent != nullptr ? static_cast<qint64>(pressEvent->timestamp()) : 0)
    , doubleClick_(doubleClick)
{
}

void MouseClickEvent::accept(QGraphicsItem* item) noexcept
{
    accepted_ = true;
    acceptedItem_ = item != nullptr ? item : currentItem_;
}

void MouseClickEvent::ignore() noexcept
{
    accepted_ = false;
    acceptedItem_ = nullptr;
}

bool MouseClickEvent::isAccepted() const noexcept { return accepted_; }

void MouseClickEvent::setCurrentItem(QGraphicsItem* item) noexcept { currentItem_ = item; }

QGraphicsItem* MouseClickEvent::currentItem() const noexcept { return currentItem_; }

QGraphicsItem* MouseClickEvent::acceptedItem() const noexcept { return acceptedItem_; }

pyqtgraph::Point MouseClickEvent::scenePos() const { return scenePos_; }

pyqtgraph::Point MouseClickEvent::screenPos() const { return screenPos_; }

pyqtgraph::Point MouseClickEvent::pos() const { return mapFromScene(currentItem_, scenePos_); }

pyqtgraph::Point MouseClickEvent::lastPos() const { return pos(); }

Qt::MouseButtons MouseClickEvent::buttons() const noexcept { return buttons_; }

Qt::MouseButton MouseClickEvent::button() const noexcept { return button_; }

bool MouseClickEvent::doubleClick() const noexcept { return doubleClick_; }

Qt::KeyboardModifiers MouseClickEvent::modifiers() const noexcept { return modifiers_; }

qint64 MouseClickEvent::time() const noexcept { return time_; }

HoverEvent::HoverEvent(QGraphicsSceneMouseEvent* moveEvent, bool acceptable)
    : scenePos_(moveEvent != nullptr ? toPoint(moveEvent->scenePos()) : pyqtgraph::Point())
    , screenPos_(moveEvent != nullptr ? toPoint(moveEvent->screenPos()) : pyqtgraph::Point())
    , lastScenePos_(moveEvent != nullptr ? toPoint(moveEvent->lastScenePos()) : pyqtgraph::Point())
    , lastScreenPos_(moveEvent != nullptr ? toPoint(moveEvent->lastScreenPos()) : pyqtgraph::Point())
    , buttons_(moveEvent != nullptr ? moveEvent->buttons() : Qt::MouseButtons(Qt::NoButton))
    , modifiers_(moveEvent != nullptr ? moveEvent->modifiers() : Qt::KeyboardModifiers(Qt::NoModifier))
    , acceptable_(acceptable)
    , enter_(false)
    , exit_(moveEvent == nullptr)
{
}

void HoverEvent::setCurrentItem(QGraphicsItem* item) noexcept { currentItem_ = item; }

QGraphicsItem* HoverEvent::currentItem() const noexcept { return currentItem_; }

void HoverEvent::setEnter(bool enter) noexcept { enter_ = enter; }

bool HoverEvent::isEnter() const noexcept { return enter_; }

bool HoverEvent::isExit() const noexcept { return exit_; }

bool HoverEvent::acceptClicks(Qt::MouseButton button, QGraphicsItem* item)
{
    if (!acceptable_ || clickItems_.contains(button)) {
        return false;
    }
    clickItems_.insert(button, item != nullptr ? item : currentItem_);
    return true;
}

bool HoverEvent::acceptDrags(Qt::MouseButton button, QGraphicsItem* item)
{
    if (!acceptable_ || dragItems_.contains(button)) {
        return false;
    }
    dragItems_.insert(button, item != nullptr ? item : currentItem_);
    return true;
}

pyqtgraph::Point HoverEvent::scenePos() const { return scenePos_; }

pyqtgraph::Point HoverEvent::screenPos() const { return screenPos_; }

pyqtgraph::Point HoverEvent::lastScenePos() const { return lastScenePos_; }

pyqtgraph::Point HoverEvent::lastScreenPos() const { return lastScreenPos_; }

pyqtgraph::Point HoverEvent::pos() const { return mapFromScene(currentItem_, scenePos_); }

pyqtgraph::Point HoverEvent::lastPos() const { return mapFromScene(currentItem_, lastScenePos_); }

Qt::MouseButtons HoverEvent::buttons() const noexcept { return buttons_; }

Qt::KeyboardModifiers HoverEvent::modifiers() const noexcept { return modifiers_; }

QHash<Qt::MouseButton, QGraphicsItem*> HoverEvent::clickItems() const { return clickItems_; }

QHash<Qt::MouseButton, QGraphicsItem*> HoverEvent::dragItems() const { return dragItems_; }

} // namespace pyqtgraph::GraphicsScene
