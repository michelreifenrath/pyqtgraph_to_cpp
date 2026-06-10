#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GraphicsLayoutWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsView.hpp"
#include "../graphicsItems/GraphicsLayout.hpp"

#include <QtCore/QSize>
#include <QtCore/QString>

#include <optional>

class QGraphicsWidget;

namespace cppqtgraph::widgets {

class GraphicsLayoutWidget : public GraphicsView {
    Q_OBJECT

public:
    explicit GraphicsLayoutWidget(QWidget* parent = nullptr);
    GraphicsLayoutWidget(QWidget* parent, bool showWidget, std::optional<QSize> size = std::nullopt,
                         const QString& title = QString{});
    ~GraphicsLayoutWidget() override;

    GraphicsLayoutWidget(const GraphicsLayoutWidget&) = delete;
    GraphicsLayoutWidget& operator=(const GraphicsLayoutWidget&) = delete;
    GraphicsLayoutWidget(GraphicsLayoutWidget&&) = delete;
    GraphicsLayoutWidget& operator=(GraphicsLayoutWidget&&) = delete;

    graphicsItems::GraphicsLayout* ci = nullptr;

    [[nodiscard]] graphicsItems::GraphicsLayout* graphicsLayout() noexcept;
    [[nodiscard]] const graphicsItems::GraphicsLayout* graphicsLayout() const noexcept;

    void nextRow();
    void nextColumn();
    void nextCol();

    graphicsItems::PlotItem* addPlot(int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    graphicsItems::ViewBox* addViewBox(int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    graphicsItems::GraphicsLayout* addLayout(int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    QGraphicsWidget* addLabel(const QString& text = QStringLiteral(" "), int row = -1, int col = -1,
                              int rowspan = 1, int colspan = 1);
    void addItem(QGraphicsWidget* item, int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    [[nodiscard]] QGraphicsWidget* getItem(int row, int col) const;
    [[nodiscard]] int itemIndex(const QGraphicsWidget* item) const;
    void removeItem(QGraphicsWidget* item);
    void clear();
};

} // namespace cppqtgraph::widgets
