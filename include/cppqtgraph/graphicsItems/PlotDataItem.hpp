#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotDataItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "PlotCurveItem.hpp"

#include <QtCore/QRectF>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <span>
#include <utility>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class PlotDataItem : public GraphicsObject {
public:
    explicit PlotDataItem(QGraphicsItem* parent = nullptr);
    explicit PlotDataItem(std::span<const double> y, QGraphicsItem* parent = nullptr);
    PlotDataItem(std::span<const double> x, std::span<const double> y, QGraphicsItem* parent = nullptr);
    ~PlotDataItem() override;

    PlotDataItem(const PlotDataItem&) = delete;
    PlotDataItem& operator=(const PlotDataItem&) = delete;
    PlotDataItem(PlotDataItem&&) = delete;
    PlotDataItem& operator=(PlotDataItem&&) = delete;

    void setData();
    void setData(std::span<const double> y);
    void setData(std::span<const double> x, std::span<const double> y);
    void clear();

    [[nodiscard]] bool hasData() const noexcept;
    [[nodiscard]] std::span<const double> xData() const noexcept;
    [[nodiscard]] std::span<const double> yData() const noexcept;
    [[nodiscard]] std::pair<std::span<const double>, std::span<const double>> getData() const noexcept;

    [[nodiscard]] PlotCurveItem* curve() noexcept;
    [[nodiscard]] const PlotCurveItem* curve() const noexcept;

    void setPen(const QPen& pen);
    void setPen(std::nullptr_t);
    [[nodiscard]] QPen pen() const;
    [[nodiscard]] bool lineVisible() const noexcept;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void updateItems();

    PlotCurveItem* curve_ = nullptr;
    std::vector<double> xData_;
    std::vector<double> yData_;
    bool hasData_ = false;
    bool lineVisible_ = true;
    QPen pen_;
};

} // namespace cppqtgraph::graphicsItems
