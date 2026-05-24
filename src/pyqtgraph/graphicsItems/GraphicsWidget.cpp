// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GraphicsWidget.hpp"

#include <QtCore/QRectF>
#include <QtCore/QVariant>
#include <QtWidgets/QGraphicsItem>

namespace pyqtgraph::graphicsItems {

GraphicsWidget::GraphicsWidget(QGraphicsItem* parent, Qt::WindowFlags flags)
    : QGraphicsWidget(parent, flags)
    , GraphicsItem(static_cast<QGraphicsItem*>(this))
{
}

GraphicsWidget::~GraphicsWidget() = default;

QGraphicsItem* GraphicsWidget::graphicsItem() const noexcept
{
    return GraphicsItem::graphicsItem();
}

void GraphicsWidget::setFixedHeight(qreal height)
{
    setMaximumHeight(height);
    setMinimumHeight(height);
}

void GraphicsWidget::setFixedWidth(qreal width)
{
    setMaximumWidth(width);
    setMinimumWidth(width);
}

qreal GraphicsWidget::height() const
{
    return geometry().height();
}

qreal GraphicsWidget::width() const
{
    return geometry().width();
}

QVariant GraphicsWidget::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemParentHasChanged:
    case QGraphicsItem::ItemSceneHasChanged:
        forgetViewWidget();
        break;
    default:
        break;
    }

    return QGraphicsWidget::itemChange(change, value);
}

} // namespace pyqtgraph::graphicsItems
