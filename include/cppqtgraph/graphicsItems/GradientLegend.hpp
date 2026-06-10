#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GradientLegend.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <cppqtgraph/colormap.hpp>

#include <QtCore/QMap>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QLinearGradient>
#include <QtGui/QPen>

#include <optional>
#include <utility>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class ViewBox;

class GradientLegend : public GraphicsObject {
public:
    GradientLegend(const QPointF& size, const QPointF& offset, QGraphicsItem* parent = nullptr);
    ~GradientLegend() override;

    GradientLegend(const GradientLegend&) = delete;
    GradientLegend& operator=(const GradientLegend&) = delete;
    GradientLegend(GradientLegend&&) = delete;
    GradientLegend& operator=(GradientLegend&&) = delete;

    [[nodiscard]] QPointF size() const noexcept { return size_; }
    [[nodiscard]] QPointF offset() const noexcept { return offset_; }
    [[nodiscard]] QBrush brush() const { return brush_; }
    [[nodiscard]] QPen pen() const { return pen_; }
    [[nodiscard]] QPen textPen() const { return textPen_; }
    [[nodiscard]] QMap<QString, qreal> labels() const { return labels_; }
    [[nodiscard]] QLinearGradient gradient() const { return gradient_; }

    void setGradient(const QLinearGradient& gradient);
    void setColorMap(const cppqtgraph::ColorMap& colorMap);
    void setIntColorScale(int minVal,
                          int maxVal,
                          int values = 1,
                          int maxValue = 255,
                          int minValue = 150,
                          int maxHue = 360,
                          int minHue = 0,
                          int sat = 255,
                          int alpha = 255,
                          std::optional<std::pair<QString, QString>> labels = std::nullopt);
    void setLabels(const QMap<QString, qreal>& labels);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    [[nodiscard]] ViewBox* viewBox() const;
    void invalidateBounds();

    QPointF size_;
    QPointF offset_;
    QBrush brush_;
    QPen pen_;
    QPen textPen_;
    QMap<QString, qreal> labels_;
    QLinearGradient gradient_;
    mutable std::optional<QRectF> boundingRect_;
};

} // namespace cppqtgraph::graphicsItems
