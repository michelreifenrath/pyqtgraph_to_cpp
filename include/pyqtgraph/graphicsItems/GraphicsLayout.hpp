#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsLayout.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"

#include <QtCore/QString>

#include <map>
#include <utility>
#include <vector>

class QGraphicsGridLayout;
class QGraphicsWidget;

namespace pyqtgraph::graphicsItems {

class PlotItem;
class ViewBox;

class GraphicsLayout : public GraphicsWidget {
public:
    explicit GraphicsLayout(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~GraphicsLayout() override;

    GraphicsLayout(const GraphicsLayout&) = delete;
    GraphicsLayout& operator=(const GraphicsLayout&) = delete;
    GraphicsLayout(GraphicsLayout&&) = delete;
    GraphicsLayout& operator=(GraphicsLayout&&) = delete;

    void nextRow();
    void nextColumn();
    void nextCol();

    PlotItem* addPlot(int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    ViewBox* addViewBox(int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    GraphicsLayout* addLayout(int row = -1, int col = -1, int rowspan = 1, int colspan = 1);
    QGraphicsWidget* addLabel(const QString& text = QStringLiteral(" "), int row = -1, int col = -1,
                              int rowspan = 1, int colspan = 1);
    void addItem(QGraphicsWidget* item, int row = -1, int col = -1, int rowspan = 1, int colspan = 1);

    [[nodiscard]] QGraphicsWidget* getItem(int row, int col) const;
    [[nodiscard]] int itemIndex(const QGraphicsWidget* item) const;
    void removeItem(QGraphicsWidget* item);
    void clear();

    void setContentsMargins(qreal left, qreal top, qreal right, qreal bottom);
    void setSpacing(qreal spacing);

    [[nodiscard]] int currentRow() const noexcept;
    [[nodiscard]] int currentColumn() const noexcept;
    [[nodiscard]] int currentCol() const noexcept;
    [[nodiscard]] QGraphicsGridLayout* gridLayout() noexcept;
    [[nodiscard]] const QGraphicsGridLayout* gridLayout() const noexcept;

private:
    using Cell = std::pair<int, int>;

    [[nodiscard]] Cell resolveCell(int row, int col) const;
    void rememberCells(QGraphicsWidget* item, int row, int col, int rowspan, int colspan);
    void forgetCells(QGraphicsWidget* item);

    QGraphicsGridLayout* layout_ = nullptr;
    std::map<Cell, QGraphicsWidget*> rows_;
    std::map<QGraphicsWidget*, std::vector<Cell>> items_;
    int currentRow_ = 0;
    int currentCol_ = 0;
};

} // namespace pyqtgraph::graphicsItems
