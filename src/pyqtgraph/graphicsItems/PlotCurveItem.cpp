// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"

#include <QtCore/QtGlobal>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

class QPainter;

namespace pyqtgraph::graphicsItems {

PlotCurveItem::PlotCurveItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
}

PlotCurveItem::~PlotCurveItem() = default;

QRectF PlotCurveItem::boundingRect() const
{
    return QRectF{};
}

void PlotCurveItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

} // namespace pyqtgraph::graphicsItems
