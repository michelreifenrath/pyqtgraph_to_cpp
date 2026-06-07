// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GraphItem.hpp"

#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace pyqtgraph::graphicsItems {
namespace {

QPen defaultGraphPen()
{
    QPen pen(QColor(255, 255, 255), 1.0);
    pen.setCosmetic(true);
    return pen;
}

std::vector<double> xValues(std::span<const QPointF> positions)
{
    std::vector<double> values;
    values.reserve(positions.size());
    for (const QPointF& point : positions) {
        values.push_back(point.x());
    }
    return values;
}

std::vector<double> yValues(std::span<const QPointF> positions)
{
    std::vector<double> values;
    values.reserve(positions.size());
    for (const QPointF& point : positions) {
        values.push_back(point.y());
    }
    return values;
}

} // namespace

GraphItem::GraphItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , scatter_(std::make_unique<ScatterPlotItem>())
    , linePen_(defaultGraphPen())
{
    scatter_->setParentItem(this);
}

GraphItem::~GraphItem() = default;

void GraphItem::setData(std::span<const QPointF> positions)
{
    setPos(positions);
    clearAdjacency();
}

void GraphItem::setData(std::span<const QPointF> positions, std::span<const GraphEdge> adjacency)
{
    setPos(positions);
    setAdjacency(adjacency);
}

void GraphItem::setData(std::span<const QPointF> positions, std::span<const GraphEdge> adjacency, const QPen& pen)
{
    setPos(positions);
    setPen(pen);
    setAdjacency(adjacency);
}

void GraphItem::setData(std::span<const QPointF> positions, std::span<const GraphEdge> adjacency, std::span<const QPen> pens)
{
    setPos(positions);
    setPens(pens);
    setAdjacency(adjacency);
}

void GraphItem::setPos(std::span<const QPointF> positions)
{
    prepareGeometryChange();
    positions_.assign(positions.begin(), positions.end());
    const std::vector<double> x = xValues(positions_);
    const std::vector<double> y = yValues(positions_);
    scatter_->setData(x, y);
    if (!adjacency_.empty()) {
        validateAdjacency(adjacency_);
    }
    resetEdgePicture();
}

void GraphItem::setAdjacency(std::span<const GraphEdge> adjacency)
{
    validateAdjacency(adjacency);
    if (!linePens_.empty() && linePens_.size() != adjacency.size()) {
        throw std::invalid_argument("GraphItem::setPens requires one pen per edge");
    }
    adjacency_.assign(adjacency.begin(), adjacency.end());
    resetEdgePicture();
}

void GraphItem::clearAdjacency()
{
    if (adjacency_.empty()) {
        return;
    }
    adjacency_.clear();
    resetEdgePicture();
}

void GraphItem::setPen(const QPen& pen)
{
    linePen_ = pen;
    linePens_.clear();
    resetEdgePicture();
}

void GraphItem::setPen(std::nullptr_t)
{
    linePen_.reset();
    linePens_.clear();
    resetEdgePicture();
}

std::optional<QPen> GraphItem::pen() const
{
    return linePen_;
}

void GraphItem::setPens(std::span<const QPen> pens)
{
    if (!adjacency_.empty() && pens.size() != adjacency_.size()) {
        throw std::invalid_argument("GraphItem::setPens requires one pen per edge");
    }
    linePens_.assign(pens.begin(), pens.end());
    linePen_.reset();
    resetEdgePicture();
}

std::span<const QPen> GraphItem::pens() const noexcept
{
    return linePens_;
}

ScatterPlotItem* GraphItem::scatter() noexcept
{
    return scatter_.get();
}

const ScatterPlotItem* GraphItem::scatter() const noexcept
{
    return scatter_.get();
}

std::span<const QPointF> GraphItem::positions() const noexcept
{
    return positions_;
}

std::span<const GraphEdge> GraphItem::adjacency() const noexcept
{
    return adjacency_;
}

std::pair<qreal, qreal> GraphItem::dataBounds(int axis) const
{
    return scatter_->dataBounds(axis);
}

qreal GraphItem::pixelPadding() const noexcept
{
    return scatter_->pixelPadding();
}

QRectF GraphItem::boundingRect() const
{
    return scatter_->boundingRect();
}

void GraphItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (painter == nullptr || !edgeDrawingEnabled()) {
        return;
    }

    validateAdjacency(adjacency_);
    for (std::size_t index = 0; index < adjacency_.size(); ++index) {
        const QPen pen = effectivePen(index);
        if (pen.style() == Qt::NoPen) {
            continue;
        }
        const GraphEdge edge = adjacency_[index];
        painter->setPen(pen);
        painter->drawLine(positions_[static_cast<std::size_t>(edge.source)], positions_[static_cast<std::size_t>(edge.target)]);
    }
}

void GraphItem::resetEdgePicture()
{
    update();
}

void GraphItem::validateAdjacency(std::span<const GraphEdge> adjacency) const
{
    for (const GraphEdge edge : adjacency) {
        if (edge.source < 0 || edge.target < 0) {
            throw std::out_of_range("GraphItem adjacency indices must be non-negative");
        }
        if (static_cast<std::size_t>(edge.source) >= positions_.size()
            || static_cast<std::size_t>(edge.target) >= positions_.size()) {
            throw std::out_of_range("GraphItem adjacency index is outside the node position array");
        }
    }
}

QPen GraphItem::effectivePen(std::size_t edgeIndex) const
{
    if (!linePens_.empty()) {
        if (linePens_.size() != adjacency_.size()) {
            throw std::invalid_argument("GraphItem::setPens requires one pen per edge");
        }
        return linePens_[edgeIndex];
    }
    if (linePen_.has_value()) {
        return *linePen_;
    }
    return QPen(Qt::NoPen);
}

bool GraphItem::edgeDrawingEnabled() const noexcept
{
    if (positions_.empty() || adjacency_.empty()) {
        return false;
    }
    return linePen_.has_value() || !linePens_.empty();
}

} // namespace pyqtgraph::graphicsItems
