#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GridItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>
#include <QtGui/QPen>

#include <array>
#include <optional>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class ViewBox;

class GridItem : public GraphicsObject {
public:
    struct TickSpacing {
        std::vector<std::optional<qreal>> x{std::nullopt, std::nullopt, std::nullopt};
        std::vector<std::optional<qreal>> y{std::nullopt, std::nullopt, std::nullopt};
    };

    explicit GridItem(QGraphicsItem* parent = nullptr);
    explicit GridItem(const QPen& pen, std::optional<QPen> textPen = std::nullopt, QGraphicsItem* parent = nullptr);
    ~GridItem() override;

    GridItem(const GridItem&) = delete;
    GridItem& operator=(const GridItem&) = delete;
    GridItem(GridItem&&) = delete;
    GridItem& operator=(GridItem&&) = delete;

    [[nodiscard]] QPen pen() const;
    void setPen(const QPen& pen);
    [[nodiscard]] std::optional<QPen> textPen() const;
    void setTextPen(std::optional<QPen> pen);
    [[nodiscard]] TickSpacing tickSpacing() const;
    void setTickSpacing(std::optional<std::vector<std::optional<qreal>>> x = std::nullopt,
                        std::optional<std::vector<std::optional<qreal>>> y = std::nullopt);
    void setTickSpacing(const TickSpacing& spacing);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    [[nodiscard]] ViewBox* viewBox() const;
    [[nodiscard]] int gridDepth() const;

    QPen pen_;
    std::optional<QPen> textPen_;
    TickSpacing tickSpacing_;
};

} // namespace cppqtgraph::graphicsItems
