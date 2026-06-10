// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ItemGroup.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/ItemGroup.hpp"

#include <QtWidgets/QGraphicsItem>

namespace cppqtgraph::graphicsItems {

ItemGroup::ItemGroup(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents, true);
}

ItemGroup::~ItemGroup() = default;

void ItemGroup::addItem(QGraphicsItem* item)
{
    if (item != nullptr) {
        item->setParentItem(this);
    }
}

QRectF ItemGroup::boundingRect() const
{
    return QRectF();
}

void ItemGroup::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}

} // namespace cppqtgraph::graphicsItems
