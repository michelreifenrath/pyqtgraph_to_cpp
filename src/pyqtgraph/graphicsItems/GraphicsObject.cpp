// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsObject.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GraphicsObject.hpp"

#include <QtWidgets/QGraphicsItem>

namespace pyqtgraph::graphicsItems {

GraphicsObject::GraphicsObject(QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , GraphicsItem(static_cast<QGraphicsItem*>(this))
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

GraphicsObject::~GraphicsObject() = default;

} // namespace pyqtgraph::graphicsItems
