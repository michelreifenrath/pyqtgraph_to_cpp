// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/mouseEvents.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtCore/QVariant>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <array>
#include <chrono>
#include <memory>

using PyQtGraphItemLifetimeToken = std::shared_ptr<void>;
Q_DECLARE_METATYPE(PyQtGraphItemLifetimeToken)

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

qint64 steadyClockNowMilliseconds() noexcept
{
    using Clock = std::chrono::steady_clock;
    return static_cast<qint64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());
}

constexpr std::array<Qt::MouseButton, 3> trackedMouseButtons {
    Qt::LeftButton,
    Qt::MiddleButton,
    Qt::RightButton,
};

void copyButtonDownPositions(QGraphicsSceneMouseEvent* moveEvent,
    QHash<Qt::MouseButton, pyqtgraph::Point>& scenePositions,
    QHash<Qt::MouseButton, pyqtgraph::Point>& screenPositions)
{
    if (moveEvent == nullptr) {
        return;
    }

    for (Qt::MouseButton button : trackedMouseButtons) {
        scenePositions.insert(button, toPoint(moveEvent->buttonDownScenePos(button)));
        screenPositions.insert(button, toPoint(moveEvent->buttonDownScreenPos(button)));
    }
}

void insertButtonDownPosition(Qt::MouseButton button,
    const pyqtgraph::Point& scenePos,
    const pyqtgraph::Point& screenPos,
    QHash<Qt::MouseButton, pyqtgraph::Point>& scenePositions,
    QHash<Qt::MouseButton, pyqtgraph::Point>& screenPositions)
{
    if (button == Qt::NoButton) {
        return;
    }

    scenePositions.insert(button, scenePos);
    screenPositions.insert(button, screenPos);
}

pyqtgraph::Point pointForButton(
    const QHash<Qt::MouseButton, pyqtgraph::Point>& positions,
    Qt::MouseButton requestedButton,
    Qt::MouseButton defaultButton)
{
    const Qt::MouseButton resolvedButton = requestedButton == Qt::NoButton ? defaultButton : requestedButton;
    const auto position = positions.constFind(resolvedButton);
    if (position == positions.constEnd()) {
        return pyqtgraph::Point();
    }
    return position.value();
}

PyQtGraphItemLifetimeToken lifetimeTokenFor(QGraphicsItem* item)
{
    // Plain QGraphicsItem has no QObject destruction signal. Store a shared token
    // in item data so HoverEvent can keep only a weak reference to item lifetime.
    constexpr int claimLifetimeTokenKey = 0x70716734;

    const QVariant storedToken = item->data(claimLifetimeTokenKey);
    if (storedToken.isValid()) {
        PyQtGraphItemLifetimeToken token = storedToken.value<PyQtGraphItemLifetimeToken>();
        if (token != nullptr) {
            return token;
        }
    }

    PyQtGraphItemLifetimeToken token = std::make_shared<char>();
    item->setData(claimLifetimeTokenKey, QVariant::fromValue(token));
    return token;
}

} // namespace

namespace pyqtgraph::GraphicsScene {

MouseDragEvent::MouseDragEvent(QGraphicsSceneMouseEvent* moveEvent, QGraphicsSceneMouseEvent* pressEvent,
    QGraphicsSceneMouseEvent* lastEvent, bool start, bool finish)
    : scenePos_(moveEvent != nullptr ? toPoint(moveEvent->scenePos()) : pyqtgraph::Point())
    , screenPos_(moveEvent != nullptr ? toPoint(moveEvent->screenPos()) : pyqtgraph::Point())
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
    copyButtonDownPositions(moveEvent, buttonDownScenePos_, buttonDownScreenPos_);
    if (moveEvent == nullptr && pressEvent != nullptr) {
        insertButtonDownPosition(button_, toPoint(pressEvent->scenePos()), toPoint(pressEvent->screenPos()),
            buttonDownScenePos_, buttonDownScreenPos_);
    }
}

MouseDragEvent::MouseDragEvent(QGraphicsSceneMouseEvent* moveEvent, const MouseClickEvent& pressEvent,
    const MouseDragEvent* lastEvent, bool start, bool finish)
    : scenePos_(moveEvent != nullptr ? toPoint(moveEvent->scenePos()) : pressEvent.scenePos())
    , screenPos_(moveEvent != nullptr ? toPoint(moveEvent->screenPos()) : pressEvent.screenPos())
    , lastScenePos_(lastEvent != nullptr ? lastEvent->scenePos() : pressEvent.scenePos())
    , lastScreenPos_(lastEvent != nullptr ? lastEvent->screenPos() : pressEvent.screenPos())
    , buttons_(moveEvent != nullptr ? moveEvent->buttons() : pressEvent.buttons())
    , button_(pressEvent.button())
    , modifiers_(moveEvent != nullptr ? moveEvent->modifiers() : pressEvent.modifiers())
    , start_(start)
    , finish_(finish)
{
    copyButtonDownPositions(moveEvent, buttonDownScenePos_, buttonDownScreenPos_);
    insertButtonDownPosition(pressEvent.button(), pressEvent.scenePos(), pressEvent.screenPos(),
        buttonDownScenePos_, buttonDownScreenPos_);
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

pyqtgraph::Point MouseDragEvent::buttonDownScenePos() const { return buttonDownScenePos(button_); }

pyqtgraph::Point MouseDragEvent::buttonDownScenePos(Qt::MouseButton button) const
{
    return pointForButton(buttonDownScenePos_, button, button_);
}

pyqtgraph::Point MouseDragEvent::buttonDownScreenPos() const { return buttonDownScreenPos(button_); }

pyqtgraph::Point MouseDragEvent::buttonDownScreenPos(Qt::MouseButton button) const
{
    return pointForButton(buttonDownScreenPos_, button, button_);
}

pyqtgraph::Point MouseDragEvent::lastScenePos() const { return lastScenePos_; }

pyqtgraph::Point MouseDragEvent::lastScreenPos() const { return lastScreenPos_; }

pyqtgraph::Point MouseDragEvent::pos() const { return mapFromScene(currentItem_, scenePos_); }

pyqtgraph::Point MouseDragEvent::lastPos() const { return mapFromScene(currentItem_, lastScenePos_); }

pyqtgraph::Point MouseDragEvent::buttonDownPos() const { return buttonDownPos(button_); }

pyqtgraph::Point MouseDragEvent::buttonDownPos(Qt::MouseButton button) const
{
    return mapFromScene(currentItem_, buttonDownScenePos(button));
}

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
    , time_(pressEvent != nullptr ? steadyClockNowMilliseconds() : 0)
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

HoverEvent::ItemClaim HoverEvent::makeItemClaim(QGraphicsItem* item)
{
    if (item == nullptr) {
        return {};
    }
    return ItemClaim { item, lifetimeTokenFor(item) };
}

QHash<Qt::MouseButton, QGraphicsItem*> HoverEvent::exposedClaims(const QHash<Qt::MouseButton, ItemClaim>& claims)
{
    QHash<Qt::MouseButton, QGraphicsItem*> exposedItems;
    for (auto claim = claims.constBegin(); claim != claims.constEnd(); ++claim) {
        if (claim.value().isLive()) {
            exposedItems.insert(claim.key(), claim.value().item);
        }
    }
    return exposedItems;
}

void HoverEvent::setCurrentItem(QGraphicsItem* item) noexcept { currentItem_ = item; }

QGraphicsItem* HoverEvent::currentItem() const noexcept { return currentItem_; }

void HoverEvent::setEnter(bool enter) noexcept { enter_ = enter; }

void HoverEvent::setExit(bool exit) noexcept { exit_ = exit; }

bool HoverEvent::isEnter() const noexcept { return enter_; }

bool HoverEvent::isExit() const noexcept { return exit_; }

bool HoverEvent::acceptClicks(Qt::MouseButton button, QGraphicsItem* item)
{
    if (!acceptable_) {
        return false;
    }

    auto claim = clickItems_.find(button);
    if (claim != clickItems_.end()) {
        if (claim.value().isLive()) {
            return false;
        }
        clickItems_.erase(claim);
    }

    ItemClaim itemClaim = makeItemClaim(item != nullptr ? item : currentItem_);
    if (!itemClaim.isLive()) {
        return false;
    }
    clickItems_.insert(button, itemClaim);
    return true;
}

bool HoverEvent::acceptDrags(Qt::MouseButton button, QGraphicsItem* item)
{
    if (!acceptable_) {
        return false;
    }

    auto claim = dragItems_.find(button);
    if (claim != dragItems_.end()) {
        if (claim.value().isLive()) {
            return false;
        }
        dragItems_.erase(claim);
    }

    ItemClaim itemClaim = makeItemClaim(item != nullptr ? item : currentItem_);
    if (!itemClaim.isLive()) {
        return false;
    }
    dragItems_.insert(button, itemClaim);
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

QHash<Qt::MouseButton, QGraphicsItem*> HoverEvent::clickItems() const { return exposedClaims(clickItems_); }

QHash<Qt::MouseButton, QGraphicsItem*> HoverEvent::dragItems() const { return exposedClaims(dragItems_); }

} // namespace pyqtgraph::GraphicsScene
