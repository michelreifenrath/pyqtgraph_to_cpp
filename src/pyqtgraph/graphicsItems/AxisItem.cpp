// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/AxisItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/AxisItem.hpp"

namespace pyqtgraph::graphicsItems {

AxisItem::AxisItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
{
}

AxisItem::~AxisItem() = default;

} // namespace pyqtgraph::graphicsItems
