// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

namespace pyqtgraph::graphicsItems {

PlotItem::PlotItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
{
}

PlotItem::~PlotItem() = default;

} // namespace pyqtgraph::graphicsItems
