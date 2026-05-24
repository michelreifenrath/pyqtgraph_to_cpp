#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsItem.hpp"

#include <QtWidgets/QGraphicsWidget>

namespace pyqtgraph::graphicsItems {

class GraphicsWidget : public QGraphicsWidget, public GraphicsItem {
public:
    explicit GraphicsWidget(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~GraphicsWidget() override;

    GraphicsWidget(const GraphicsWidget&) = delete;
    GraphicsWidget& operator=(const GraphicsWidget&) = delete;
    GraphicsWidget(GraphicsWidget&&) = delete;
    GraphicsWidget& operator=(GraphicsWidget&&) = delete;

    [[nodiscard]] QGraphicsItem* graphicsItem() const noexcept;

    void setFixedHeight(qreal height);
    void setFixedWidth(qreal width);
    [[nodiscard]] qreal height() const;
    [[nodiscard]] qreal width() const;
};

} // namespace pyqtgraph::graphicsItems
