#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>
#include <QtWidgets/QGraphicsItem>

#include <span>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class PlotCurveItem : public GraphicsObject {
public:
    explicit PlotCurveItem(QGraphicsItem* parent = nullptr);
    ~PlotCurveItem() override;

    PlotCurveItem(const PlotCurveItem&) = delete;
    PlotCurveItem& operator=(const PlotCurveItem&) = delete;
    PlotCurveItem(PlotCurveItem&&) = delete;
    PlotCurveItem& operator=(PlotCurveItem&&) = delete;

    void setData(std::span<const double> y);
    void setData(std::span<const double> x, std::span<const double> y);

    [[nodiscard]] std::span<const double> xData() const noexcept;
    [[nodiscard]] std::span<const double> yData() const noexcept;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    std::vector<double> xData_;
    std::vector<double> yData_;
    QRectF bounds_;
};

} // namespace pyqtgraph::graphicsItems
