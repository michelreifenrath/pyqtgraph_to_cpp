#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"

#include <QtCore/Qt>
#include <QtWidgets/QGraphicsItem>

namespace pyqtgraph::graphicsItems {

class ViewBox : public GraphicsWidget {
public:
    static constexpr int PanMode = 3;
    static constexpr int RectMode = 1;
    static constexpr int XAxis = 0;
    static constexpr int YAxis = 1;
    static constexpr int XYAxes = 2;

    explicit ViewBox(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~ViewBox() override;

    ViewBox(const ViewBox&) = delete;
    ViewBox& operator=(const ViewBox&) = delete;
    ViewBox(ViewBox&&) = delete;
    ViewBox& operator=(ViewBox&&) = delete;
};

} // namespace pyqtgraph::graphicsItems
