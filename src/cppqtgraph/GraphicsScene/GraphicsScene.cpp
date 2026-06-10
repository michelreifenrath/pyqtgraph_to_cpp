// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/GraphicsScene.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>

#include <QtCore/QEvent>
#include <QtCore/QLineF>
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtGui/QPainterPath>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsView>

#include <algorithm>
#include <array>
#include <chrono>

namespace {

constexpr std::array<Qt::MouseButton, 3> trackedMouseButtons {
    Qt::LeftButton,
    Qt::MiddleButton,
    Qt::RightButton,
};

qint64 steadyClockNowMilliseconds() noexcept
{
    using Clock = std::chrono::steady_clock;
    return static_cast<qint64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());
}

qreal absoluteZValue(const QGraphicsItem* item) noexcept
{
    qreal value = 0.0;
    const QGraphicsItem* current = item;
    while (current != nullptr) {
        value += current->zValue();
        current = current->parentItem();
    }
    return value;
}

void sortByDescendingAbsoluteZ(QList<QGraphicsItem*>& items)
{
    std::sort(items.begin(), items.end(), [](const QGraphicsItem* lhs, const QGraphicsItem* rhs) {
        return absoluteZValue(lhs) > absoluteZValue(rhs);
    });
}

bool containsItem(const QList<QGraphicsItem*>& items, const QGraphicsItem* item)
{
    return std::find(items.begin(), items.end(), item) != items.end();
}

cppqtgraph::GraphicsScene::GraphicsSceneEventHandler* eventHandlerFor(QGraphicsItem* item)
{
    return dynamic_cast<cppqtgraph::GraphicsScene::GraphicsSceneEventHandler*>(item);
}

QPointF toPointF(const cppqtgraph::Point& point)
{
    return QPointF(point.x(), point.y());
}

} // namespace

namespace cppqtgraph::GraphicsScene {

GraphicsScene::GraphicsScene(int clickRadius, qreal moveDistance, QObject* parent)
    : QGraphicsScene(parent)
{
    setClickRadius(clickRadius);
    setMoveDistance(moveDistance);
}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::setClickRadius(int radius) noexcept
{
    clickRadius_ = radius;
}

int GraphicsScene::clickRadius() const noexcept
{
    return clickRadius_;
}

void GraphicsScene::setMoveDistance(qreal distance) noexcept
{
    moveDistance_ = distance;
}

qreal GraphicsScene::moveDistance() const noexcept
{
    return moveDistance_;
}

QGraphicsView* GraphicsScene::getViewWidget() const
{
    const QList<QGraphicsView*> sceneViews = views();
    if (sceneViews.empty()) {
        return nullptr;
    }

    return sceneViews.front();
}

QList<QGraphicsItem*> GraphicsScene::itemsNearEvent(const MouseClickEvent& event, bool hoverable) const
{
    return itemsNearPoint(toPointF(event.scenePos()), hoverable);
}

QList<QGraphicsItem*> GraphicsScene::itemsNearEvent(const MouseDragEvent& event, bool hoverable) const
{
    return itemsNearPoint(toPointF(event.buttonDownScenePos()), hoverable);
}

QList<QGraphicsItem*> GraphicsScene::itemsNearEvent(const HoverEvent& event, bool hoverable) const
{
    return itemsNearPoint(toPointF(event.scenePos()), hoverable);
}

void GraphicsScene::addItem(QGraphicsItem* item)
{
    QGraphicsScene::addItem(item);
    emit sigItemAdded(item);
}

void GraphicsScene::removeItem(QGraphicsItem* item)
{
    hoverItems_.removeAll(item);
    if (dragItem_ == item) {
        dragItem_ = nullptr;
    }
    QGraphicsScene::removeItem(item);
    emit sigItemRemoved(item);
}

void GraphicsScene::render(QPainter* painter, const QRectF& target, const QRectF& source,
    Qt::AspectRatioMode aspectRatioMode)
{
    prepareForPaint();
    QGraphicsScene::render(painter, target, source, aspectRatioMode);
}

void GraphicsScene::prepareForPaint()
{
    emit sigPrepareForPaint();
}

bool GraphicsScene::event(QEvent* event)
{
    if (event != nullptr && event->type() == QEvent::GraphicsSceneLeave && dragButtons_.empty()) {
        sendHoverEvents(nullptr, true);
    }
    return QGraphicsScene::event(event);
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mousePressEvent(event);
    if (mouseGrabberItem() != nullptr || event == nullptr) {
        return;
    }

    if (lastHoverEvent_ != nullptr && toPointF(lastHoverEvent_->scenePos()) != event->scenePos()) {
        sendHoverEvents(event);
    }

    clickEvents_.append(MouseClickEvent(event));

    const QList<QGraphicsItem*> focusItems = items(event->scenePos());
    for (QGraphicsItem* item : focusItems) {
        if (item != nullptr && item->isEnabled() && item->isVisible()
            && item->flags().testFlag(QGraphicsItem::ItemIsFocusable)) {
            item->setFocus(Qt::MouseFocusReason);
            break;
        }
    }
}

bool GraphicsScene::moveEventIsAllowed() const noexcept
{
    return true;
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event == nullptr) {
        return;
    }

    if (!moveEventIsAllowed()) {
        QGraphicsScene::mouseMoveEvent(event);
        event->accept();
        return;
    }

    emit sigMouseMoved(event->scenePos());

    QGraphicsScene::mouseMoveEvent(event);
    sendHoverEvents(event);

    if (!event->buttons() || mouseGrabberItem() != nullptr) {
        return;
    }

    bool init = false;
    const qint64 now = steadyClockNowMilliseconds();
    for (Qt::MouseButton button : trackedMouseButtons) {
        if (!event->buttons().testFlag(button) || dragButtons_.contains(button)) {
            continue;
        }

        auto clickEvent = std::find_if(clickEvents_.begin(), clickEvents_.end(), [button](const MouseClickEvent& click) {
            return click.button() == button;
        });
        if (clickEvent == clickEvents_.end()) {
            continue;
        }

        const qreal distance = QLineF(event->scenePos(), toPointF(clickEvent->scenePos())).length();
        if (distance == 0.0
            || (distance < moveDistance_ && now - clickEvent->time() < minDragTimeMilliseconds_)) {
            continue;
        }

        init = init || dragButtons_.empty();
        dragButtons_.append(button);
    }

    if (!dragButtons_.empty() && sendDragEvent(event, init)) {
        event->accept();
    }
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event == nullptr) {
        return;
    }

    if (mouseGrabberItem() == nullptr) {
        if (dragButtons_.contains(event->button())) {
            if (sendDragEvent(event, false, true)) {
                event->accept();
            }
            dragButtons_.removeAll(event->button());
        } else {
            for (qsizetype index = 0; index < clickEvents_.size(); ++index) {
                if (clickEvents_[index].button() != event->button()) {
                    continue;
                }
                if (sendClickEvent(clickEvents_[index])) {
                    event->accept();
                }
                clickEvents_.removeAt(index);
                break;
            }
        }
    }

    if (!event->buttons()) {
        dragItem_ = nullptr;
        dragButtons_.clear();
        clickEvents_.clear();
        lastDrag_.reset();
    }

    QGraphicsScene::mouseReleaseEvent(event);
    sendHoverEvents(event);
}

void GraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mouseDoubleClickEvent(event);
    if (mouseGrabberItem() == nullptr && event != nullptr) {
        clickEvents_.append(MouseClickEvent(event, true));
    }
}

void GraphicsScene::sendHoverEvents(QGraphicsSceneMouseEvent* event, bool exitOnly)
{
    std::unique_ptr<HoverEvent> hoverEvent;
    QList<QGraphicsItem*> nearItems;

    if (exitOnly) {
        hoverEvent = std::make_unique<HoverEvent>(nullptr, false);
    } else {
        if (event == nullptr) {
            return;
        }
        hoverEvent = std::make_unique<HoverEvent>(event, !event->buttons());
        nearItems = itemsNearEvent(*hoverEvent, true);
        emit sigMouseHover(nearItems);
    }

    QList<QGraphicsItem*> previousItems = hoverItems_;
    for (QGraphicsItem* item : nearItems) {
        GraphicsSceneEventHandler* handler = eventHandlerFor(item);
        if (handler == nullptr) {
            continue;
        }

        hoverEvent->setCurrentItem(item);
        if (!hoverItems_.contains(item)) {
            hoverItems_.append(item);
            hoverEvent->setEnter(true);
        } else {
            previousItems.removeAll(item);
            hoverEvent->setEnter(false);
        }
        hoverEvent->setExit(false);
        handler->hoverEvent(hoverEvent.get());
    }

    hoverEvent->setEnter(false);
    hoverEvent->setExit(true);
    for (QGraphicsItem* item : previousItems) {
        GraphicsSceneEventHandler* handler = eventHandlerFor(item);
        if (handler != nullptr && item->scene() == this) {
            hoverEvent->setCurrentItem(item);
            handler->hoverEvent(hoverEvent.get());
        }
        hoverItems_.removeAll(item);
    }

    if (!exitOnly && event != nullptr
        && (event->type() == QEvent::GraphicsSceneMousePress
            || (event->type() == QEvent::GraphicsSceneMouseMove && !event->buttons()))) {
        lastHoverEvent_ = std::move(hoverEvent);
    }
}

bool GraphicsScene::sendDragEvent(QGraphicsSceneMouseEvent* event, bool init, bool final)
{
    if (event == nullptr || clickEvents_.empty()) {
        return false;
    }

    MouseDragEvent dragEvent(event, clickEvents_.front(), lastDrag_.get(), init, final);

    if (init && dragItem_ == nullptr) {
        QGraphicsItem* acceptedItem = nullptr;
        if (lastHoverEvent_ != nullptr) {
            acceptedItem = lastHoverEvent_->dragItems().value(dragEvent.button(), nullptr);
        }

        if (acceptedItem != nullptr && acceptedItem->scene() == this && eventHandlerFor(acceptedItem) != nullptr) {
            dragItem_ = acceptedItem;
            dragEvent.setCurrentItem(dragItem_);
            eventHandlerFor(dragItem_)->mouseDragEvent(&dragEvent);
        } else {
            const QList<QGraphicsItem*> nearItems = itemsNearEvent(dragEvent);
            for (QGraphicsItem* item : nearItems) {
                if (item == nullptr || !item->isVisible() || !item->isEnabled()) {
                    continue;
                }
                GraphicsSceneEventHandler* handler = eventHandlerFor(item);
                if (handler == nullptr) {
                    continue;
                }
                dragEvent.setCurrentItem(item);
                handler->mouseDragEvent(&dragEvent);
                if (dragEvent.isAccepted()) {
                    dragItem_ = item;
                    if (item->flags().testFlag(QGraphicsItem::ItemIsFocusable)) {
                        item->setFocus(Qt::MouseFocusReason);
                    }
                    break;
                }
            }
        }
    } else if (dragItem_ != nullptr && eventHandlerFor(dragItem_) != nullptr) {
        dragEvent.setCurrentItem(dragItem_);
        eventHandlerFor(dragItem_)->mouseDragEvent(&dragEvent);
    }

    lastDrag_ = std::make_unique<MouseDragEvent>(dragEvent);
    return dragEvent.isAccepted();
}

bool GraphicsScene::sendClickEvent(MouseClickEvent& event)
{
    if (dragItem_ != nullptr && eventHandlerFor(dragItem_) != nullptr) {
        event.setCurrentItem(dragItem_);
        eventHandlerFor(dragItem_)->mouseClickEvent(&event);
    } else {
        QGraphicsItem* acceptedItem = nullptr;
        if (lastHoverEvent_ != nullptr) {
            acceptedItem = lastHoverEvent_->clickItems().value(event.button(), nullptr);
        }

        if (acceptedItem != nullptr && acceptedItem->scene() == this && eventHandlerFor(acceptedItem) != nullptr) {
            event.setCurrentItem(acceptedItem);
            eventHandlerFor(acceptedItem)->mouseClickEvent(&event);
        } else {
            const QList<QGraphicsItem*> nearItems = itemsNearEvent(event);
            for (QGraphicsItem* item : nearItems) {
                if (item == nullptr || !item->isVisible() || !item->isEnabled()) {
                    continue;
                }
                GraphicsSceneEventHandler* handler = eventHandlerFor(item);
                if (handler == nullptr) {
                    continue;
                }
                event.setCurrentItem(item);
                handler->mouseClickEvent(&event);
                if (event.isAccepted()) {
                    if (item->flags().testFlag(QGraphicsItem::ItemIsFocusable)) {
                        item->setFocus(Qt::MouseFocusReason);
                    }
                    break;
                }
            }
        }
    }

    emit sigMouseClicked(&event);
    return event.isAccepted();
}

QList<QGraphicsItem*> GraphicsScene::itemsNearPoint(const QPointF& point, bool hoverable) const
{
    QTransform viewportTransform;
    QRectF radiusRect;
    bool hasRadiusRect = false;

    if (QGraphicsView* view = getViewWidget(); view != nullptr) {
        viewportTransform = view->viewportTransform();
        if (clickRadius_ > 0) {
            const QRectF mappedRect = view->mapToScene(QRect(0, 0, 2 * clickRadius_, 2 * clickRadius_)).boundingRect();
            radiusRect = QRectF(point.x() - mappedRect.width() / 2.0, point.y() - mappedRect.height() / 2.0,
                mappedRect.width(), mappedRect.height());
            hasRadiusRect = true;
        }
    } else if (clickRadius_ > 0) {
        const qreal diameter = static_cast<qreal>(2 * clickRadius_);
        radiusRect = QRectF(point.x() - diameter / 2.0, point.y() - diameter / 2.0, diameter, diameter);
        hasRadiusRect = true;
    }

    QList<QGraphicsItem*> itemsAtPoint = items(point, Qt::IntersectsItemShape, Qt::DescendingOrder, viewportTransform);
    sortByDescendingAbsoluteZ(itemsAtPoint);

    QList<QGraphicsItem*> itemsWithinRadius;
    if (hasRadiusRect) {
        itemsWithinRadius = items(radiusRect, Qt::IntersectsItemShape, Qt::DescendingOrder, viewportTransform);
        sortByDescendingAbsoluteZ(itemsWithinRadius);
        for (QGraphicsItem* item : itemsAtPoint) {
            itemsWithinRadius.removeAll(item);
        }
    }

    QList<QGraphicsItem*> candidates = itemsAtPoint;
    for (QGraphicsItem* item : itemsWithinRadius) {
        if (!containsItem(candidates, item)) {
            candidates.append(item);
        }
    }

    QList<QGraphicsItem*> selectedItems;
    for (QGraphicsItem* item : candidates) {
        if (item == nullptr || item->scene() != this) {
            continue;
        }
        if (hoverable && eventHandlerFor(item) == nullptr) {
            continue;
        }

        const QPainterPath shape = item->shape();
        if (shape.isEmpty()) {
            continue;
        }

        bool containsPoint = shape.contains(item->mapFromScene(point));
        bool intersectsRadius = false;
        if (hasRadiusRect) {
            intersectsRadius = shape.intersects(item->mapFromScene(radiusRect).boundingRect());
        }

        if (containsPoint || intersectsRadius) {
            selectedItems.append(item);
        }
    }

    return selectedItems;
}

} // namespace cppqtgraph::GraphicsScene
