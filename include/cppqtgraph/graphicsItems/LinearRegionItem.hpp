#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LinearRegionItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "InfiniteLine.hpp"

#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>

#include <QtCore/QRectF>
#include <QtGui/QBrush>
#include <QtGui/QPen>

#include <array>
#include <optional>
#include <utility>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class LinearRegionItem : public GraphicsObject, public cppqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    Q_OBJECT

public:
    enum class Orientation { Vertical, Horizontal };
    enum class SwapMode { Sort, Block, Push, NoSwap };

    explicit LinearRegionItem(std::pair<qreal, qreal> values = {0.0, 1.0},
                              Orientation orientation = Orientation::Vertical,
                              bool movable = true,
                              std::optional<InfiniteLine::Bounds> bounds = std::nullopt,
                              QGraphicsItem* parent = nullptr);
    ~LinearRegionItem() override;

    [[nodiscard]] std::pair<qreal, qreal> getRegion() const;
    void setRegion(std::pair<qreal, qreal> region);

    void setBounds(std::optional<InfiniteLine::Bounds> bounds);
    void setBounds(std::pair<qreal, qreal> bounds);
    void setMovable(bool movable = true);
    [[nodiscard]] bool movable() const noexcept;

    void setSpan(qreal min, qreal max);
    [[nodiscard]] std::pair<qreal, qreal> span() const noexcept;

    void setSwapMode(SwapMode mode);
    [[nodiscard]] SwapMode swapMode() const noexcept;

    void setBrush(const QBrush& brush);
    [[nodiscard]] QBrush brush() const;
    void setHoverBrush(const QBrush& brush);
    [[nodiscard]] QBrush hoverBrush() const;

    [[nodiscard]] InfiniteLine* line(int index) const;
    [[nodiscard]] Orientation orientation() const noexcept;

    void lineMoved(int index);
    void lineMoveFinished();

    void setMouseHover(bool hover);
    [[nodiscard]] bool mouseHovering() const noexcept;

    [[nodiscard]] std::optional<QRectF> autoRangeBoundsRect() const override;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event) override;
    void mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event) override;
    void mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event) override;

signals:
    void sigRegionChangeFinished(cppqtgraph::graphicsItems::LinearRegionItem* region);
    void sigRegionChanged(cppqtgraph::graphicsItems::LinearRegionItem* region);

private:
    [[nodiscard]] QPointF valuePoint(qreal value) const;
    [[nodiscard]] qreal axisDelta(const QPointF& point) const;

    Orientation orientation_ = Orientation::Vertical;
    SwapMode swapMode_ = SwapMode::Sort;
    bool movable_ = true;
    bool moving_ = false;
    bool mouseHovering_ = false;
    bool blockLineSignal_ = false;
    std::array<InfiniteLine*, 2> lines_{{nullptr, nullptr}};
    std::array<QPointF, 2> cursorOffsets_{};
    std::array<QPointF, 2> startPositions_{};
    std::pair<qreal, qreal> span_{0.0, 1.0};
    QBrush brush_;
    QBrush hoverBrush_;
    QBrush currentBrush_;
};

} // namespace cppqtgraph::graphicsItems
