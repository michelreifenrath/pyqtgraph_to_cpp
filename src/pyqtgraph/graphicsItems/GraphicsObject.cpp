// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsObject.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GraphicsObject.hpp"

#include <QtCore/QVariant>
#include <QtWidgets/QGraphicsItem>

namespace pyqtgraph::graphicsItems {

GraphicsObject::GraphicsObject(QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , GraphicsItem(static_cast<QGraphicsItem*>(this))
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

GraphicsObject::~GraphicsObject() = default;

QVariant GraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemParentHasChanged:
    case QGraphicsItem::ItemSceneHasChanged:
        forgetViewWidget();
        break;
    default:
        break;
    }

    return QGraphicsObject::itemChange(change, value);
}

} // namespace pyqtgraph::graphicsItems
