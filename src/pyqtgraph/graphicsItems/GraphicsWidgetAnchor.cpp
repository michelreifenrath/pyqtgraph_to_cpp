// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsWidgetAnchor.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GraphicsWidgetAnchor.hpp"

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsWidget>

#include <cmath>
#include <stdexcept>

namespace pyqtgraph::graphicsItems {
namespace {

QPointF scaledBottomRight(const QRectF& rect, const QPointF& anchor)
{
    const QPointF bottomRight = rect.bottomRight();
    return QPointF(bottomRight.x() * anchor.x(), bottomRight.y() * anchor.y());
}

} // namespace

GraphicsWidgetAnchor::GraphicsWidgetAnchor(QGraphicsWidget* item)
    : item_(item)
{
    connectItemGeometryChanged();
}

GraphicsWidgetAnchor::~GraphicsWidgetAnchor()
{
    disconnectParentGeometryChanged();
    disconnectItemGeometryChanged();
}

void GraphicsWidgetAnchor::setAnchorItem(QGraphicsWidget* item)
{
    if (item_.data() == item) {
        return;
    }

    disconnectParentGeometryChanged();
    disconnectItemGeometryChanged();
    item_ = item;
    hasAnchor_ = false;
    connectItemGeometryChanged();
}

QGraphicsWidget* GraphicsWidgetAnchor::anchorItem() const noexcept
{
    return item_.data();
}

void GraphicsWidgetAnchor::anchor(const QPointF& itemPos, const QPointF& parentPos, const QPointF& offset)
{
    QGraphicsWidget* item = item_.data();
    if (item == nullptr) {
        throw std::runtime_error("Cannot anchor; item is not set.");
    }

    QGraphicsItem* parentItem = item->parentItem();
    if (parentItem == nullptr) {
        throw std::runtime_error("Cannot anchor; parent is not set.");
    }

    auto* parentWidget = dynamic_cast<QGraphicsWidget*>(parentItem);
    if (parentWidget == nullptr) {
        throw std::runtime_error("Cannot anchor; parent is not a QGraphicsWidget.");
    }

    if (parent_.data() != parentWidget) {
        disconnectParentGeometryChanged();
        parent_ = parentWidget;
        connectParentGeometryChanged(parentWidget);
    }

    itemAnchor_ = itemPos;
    parentAnchor_ = parentPos;
    offset_ = offset;
    hasAnchor_ = true;
    updateAnchorPosition();
}

void GraphicsWidgetAnchor::autoAnchor(const QPointF& pos, bool relative)
{
    QGraphicsWidget* item = item_.data();
    if (item == nullptr) {
        throw std::runtime_error("Cannot anchor; item is not set.");
    }

    QGraphicsItem* parentItem = item->parentItem();
    if (parentItem == nullptr) {
        throw std::runtime_error("Cannot anchor; parent is not set.");
    }

    QRectF childRect = item->mapRectToParent(item->boundingRect());
    childRect.translate(pos - item->pos());
    const QRectF parentRect = parentItem->boundingRect();

    QPointF anchorPos;
    QPointF parentPos;
    QPointF itemPos;
    if (std::abs(childRect.left() - parentRect.left()) < std::abs(childRect.right() - parentRect.right())) {
        anchorPos.setX(0.0);
        parentPos.setX(parentRect.left());
        itemPos.setX(childRect.left());
    } else {
        anchorPos.setX(1.0);
        parentPos.setX(parentRect.right());
        itemPos.setX(childRect.right());
    }

    if (std::abs(childRect.top() - parentRect.top()) < std::abs(childRect.bottom() - parentRect.bottom())) {
        anchorPos.setY(0.0);
        parentPos.setY(parentRect.top());
        itemPos.setY(childRect.top());
    } else {
        anchorPos.setY(1.0);
        parentPos.setY(parentRect.bottom());
        itemPos.setY(childRect.bottom());
    }

    if (relative) {
        const qreal relativeX = (itemPos.x() - parentRect.left()) / parentRect.width();
        const qreal relativeY = (itemPos.y() - parentRect.top()) / parentRect.height();
        anchor(anchorPos, QPointF(relativeX, relativeY));
    } else {
        anchor(anchorPos, anchorPos, itemPos - parentPos);
    }
}

void GraphicsWidgetAnchor::updateAnchorPosition()
{
    QGraphicsWidget* item = item_.data();
    QGraphicsWidget* parent = parent_.data();
    if (item == nullptr || parent == nullptr || !hasAnchor_) {
        return;
    }

    const QPointF originInParent = item->mapToParent(QPointF(0.0, 0.0));
    const QPointF itemAnchorLocal = scaledBottomRight(item->boundingRect(), itemAnchor_);
    const QPointF itemAnchorInParent = item->mapToParent(itemAnchorLocal);
    const QPointF parentAnchorPoint = scaledBottomRight(parent->boundingRect(), parentAnchor_);
    const QPointF newPos = parentAnchorPoint + (originInParent - itemAnchorInParent) + offset_;
    item->setPos(newPos);
}

void GraphicsWidgetAnchor::connectItemGeometryChanged()
{
    QGraphicsWidget* item = item_.data();
    if (item == nullptr) {
        return;
    }

    itemGeometryChangedConnection_ = QObject::connect(item, &QGraphicsWidget::geometryChanged, item, [this]() {
        updateAnchorPosition();
    });
}

void GraphicsWidgetAnchor::disconnectItemGeometryChanged() noexcept
{
    if (itemGeometryChangedConnection_) {
        QObject::disconnect(itemGeometryChangedConnection_);
        itemGeometryChangedConnection_ = QMetaObject::Connection{};
    }
}

void GraphicsWidgetAnchor::connectParentGeometryChanged(QGraphicsWidget* parent)
{
    if (parent == nullptr) {
        return;
    }

    parentGeometryChangedConnection_ = QObject::connect(parent, &QGraphicsWidget::geometryChanged, parent, [this]() {
        updateAnchorPosition();
    });
}

void GraphicsWidgetAnchor::disconnectParentGeometryChanged() noexcept
{
    if (parentGeometryChangedConnection_) {
        QObject::disconnect(parentGeometryChangedConnection_);
        parentGeometryChangedConnection_ = QMetaObject::Connection{};
    }
    parent_.clear();
}

} // namespace pyqtgraph::graphicsItems
