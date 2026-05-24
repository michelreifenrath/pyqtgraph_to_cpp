#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/AxisItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"

#include <QtCore/Qt>
#include <QtWidgets/QGraphicsItem>

namespace pyqtgraph::graphicsItems {

class AxisItem : public GraphicsWidget {
public:
    explicit AxisItem(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~AxisItem() override;

    AxisItem(const AxisItem&) = delete;
    AxisItem& operator=(const AxisItem&) = delete;
    AxisItem(AxisItem&&) = delete;
    AxisItem& operator=(AxisItem&&) = delete;
};

} // namespace pyqtgraph::graphicsItems
