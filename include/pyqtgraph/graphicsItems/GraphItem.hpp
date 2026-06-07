#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "ScatterPlotItem.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

struct GraphEdge {
    int source = 0;
    int target = 0;

    friend bool operator==(const GraphEdge&, const GraphEdge&) = default;
};

class GraphItem : public GraphicsObject {
public:
    explicit GraphItem(QGraphicsItem* parent = nullptr);
    ~GraphItem() override;

    GraphItem(const GraphItem&) = delete;
    GraphItem& operator=(const GraphItem&) = delete;
    GraphItem(GraphItem&&) = delete;
    GraphItem& operator=(GraphItem&&) = delete;

    void setData(std::span<const QPointF> positions);
    void setData(std::span<const QPointF> positions, std::span<const GraphEdge> adjacency);
    void setData(std::span<const QPointF> positions, std::span<const GraphEdge> adjacency, const QPen& pen);
    void setData(std::span<const QPointF> positions, std::span<const GraphEdge> adjacency, std::span<const QPen> pens);

    using GraphicsObject::setPos;
    void setPos(std::span<const QPointF> positions);
    void setAdjacency(std::span<const GraphEdge> adjacency);
    void clearAdjacency();

    void setPen(const QPen& pen);
    void setPen(std::nullptr_t);
    [[nodiscard]] std::optional<QPen> pen() const;
    void setPens(std::span<const QPen> pens);
    [[nodiscard]] std::span<const QPen> pens() const noexcept;

    [[nodiscard]] ScatterPlotItem* scatter() noexcept;
    [[nodiscard]] const ScatterPlotItem* scatter() const noexcept;
    [[nodiscard]] std::span<const QPointF> positions() const noexcept;
    [[nodiscard]] std::span<const GraphEdge> adjacency() const noexcept;

    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;
    [[nodiscard]] qreal pixelPadding() const noexcept;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void resetEdgePicture();
    void validateAdjacency(std::span<const GraphEdge> adjacency) const;
    [[nodiscard]] QPen effectivePen(std::size_t edgeIndex) const;
    [[nodiscard]] bool edgeDrawingEnabled() const noexcept;

    std::unique_ptr<ScatterPlotItem> scatter_;
    std::vector<QPointF> positions_;
    std::vector<GraphEdge> adjacency_;
    std::optional<QPen> linePen_;
    std::vector<QPen> linePens_;
};

} // namespace pyqtgraph::graphicsItems
