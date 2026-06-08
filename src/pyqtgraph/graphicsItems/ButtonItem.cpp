// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ButtonItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/ButtonItem.hpp"

#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>

namespace pyqtgraph::graphicsItems {
namespace {

qreal pixmapLogicalWidth(const QPixmap& pixmap)
{
    const qreal ratio = pixmap.devicePixelRatio();
    if (ratio <= 0.0) {
        return static_cast<qreal>(pixmap.width());
    }
    return static_cast<qreal>(pixmap.width()) / ratio;
}

} // namespace

ButtonItem::ButtonItem(const QPixmap& pixmap, std::optional<qreal> width, QGraphicsItem* parentItem)
    : GraphicsObject(parentItem)
    , pixmap_(pixmap)
{
    setWidthFromPixmapIfNeeded(width);
    setOpacity(0.7);
}

ButtonItem::ButtonItem(const QString& imageFile, std::optional<qreal> width, QGraphicsItem* parentItem)
    : ButtonItem(QPixmap(imageFile), width, parentItem)
{
}

ButtonItem::~ButtonItem() = default;

void ButtonItem::setImageFile(const QString& imageFile)
{
    setPixmap(QPixmap(imageFile));
}

void ButtonItem::setPixmap(const QPixmap& pixmap)
{
    prepareGeometryChange();
    pixmap_ = pixmap;
    update();
}

QPixmap ButtonItem::pixmap() const
{
    return pixmap_;
}

bool ButtonItem::enabled() const noexcept
{
    return enabled_;
}

void ButtonItem::disable()
{
    enabled_ = false;
    setOpacity(0.4);
}

void ButtonItem::enable()
{
    enabled_ = true;
    setOpacity(0.7);
}

void ButtonItem::hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event)
{
    if (!enabled_ || event == nullptr) {
        return;
    }

    if (event->isEnter()) {
        setOpacity(1.0);
    } else if (event->isExit()) {
        setOpacity(0.7);
    }
}

void ButtonItem::mouseClickEvent(pyqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (!enabled_ || event == nullptr || event->button() != Qt::LeftButton) {
        return;
    }

    event->accept(this);
    emit clicked(this);
}

QRectF ButtonItem::boundingRect() const
{
    return QRectF(0.0, 0.0, width_, width_);
}

void ButtonItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr || pixmap_.isNull()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawPixmap(QRectF(0.0, 0.0, width_, width_), pixmap_, QRectF(pixmap_.rect()));
}

void ButtonItem::setWidthFromPixmapIfNeeded(std::optional<qreal> width)
{
    width_ = width.value_or(pixmapLogicalWidth(pixmap_));
    width_ = std::max<qreal>(0.0, width_);
}

} // namespace pyqtgraph::graphicsItems
