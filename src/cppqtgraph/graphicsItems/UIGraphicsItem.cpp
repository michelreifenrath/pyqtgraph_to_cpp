// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/UIGraphicsItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/UIGraphicsItem.hpp"

#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

namespace cppqtgraph::graphicsItems {

UIGraphicsItem::UIGraphicsItem(const QRectF& bounds, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , viewBounds_(bounds)
{
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    updateView();
}

UIGraphicsItem::~UIGraphicsItem() = default;

QRectF UIGraphicsItem::viewBounds() const noexcept
{
    return viewBounds_;
}

void UIGraphicsItem::setViewBounds(const QRectF& bounds)
{
    viewBounds_ = bounds;
    updateView();
}

QRectF UIGraphicsItem::boundingRect() const
{
    return cachedBoundingRect_;
}

void UIGraphicsItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
    updateView();
}

void UIGraphicsItem::updateView()
{
    cachedBoundingRect_ = viewBounds_;
    update();
}

} // namespace cppqtgraph::graphicsItems
