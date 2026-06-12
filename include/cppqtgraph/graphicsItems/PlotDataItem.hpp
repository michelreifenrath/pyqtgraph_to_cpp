#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotDataItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "PlotCurveItem.hpp"

#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <array>
#include <optional>
#include <span>
#include <utility>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class ScatterPlotItem;

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

    [[nodiscard]] ScatterPlotItem* scatter() noexcept;
    [[nodiscard]] const ScatterPlotItem* scatter() const noexcept;

    void setSymbol(const QString& symbol);
    void setSymbol(std::nullptr_t);
    [[nodiscard]] QString symbol() const;
    [[nodiscard]] bool symbolsVisible() const noexcept;

    void setSymbolSize(qreal size);
    [[nodiscard]] qreal symbolSize() const noexcept;

    void setSymbolPen(const QPen& pen);
    void setSymbolPen(std::nullptr_t);
    [[nodiscard]] QPen symbolPen() const;

    void setSymbolBrush(const QBrush& brush);
    void setSymbolBrush(std::nullptr_t);
    [[nodiscard]] QBrush symbolBrush() const;

    void setPen(const QPen& pen);
    void setPen(std::nullptr_t);
    [[nodiscard]] QPen pen() const;
    [[nodiscard]] bool lineVisible() const noexcept;

    void setLogMode(bool xEnabled, bool yEnabled);
    [[nodiscard]] std::array<bool, 2> logMode() const noexcept;

    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] std::optional<QRectF> autoRangeBoundsRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void updateMappedData();
    void updateItems();

    PlotCurveItem* curve_ = nullptr;
    ScatterPlotItem* scatter_ = nullptr;
    std::vector<double> xData_;
    std::vector<double> yData_;
    std::vector<double> displayX_;
    std::vector<double> displayY_;
    std::array<bool, 2> logMode_{{false, false}};
    bool hasData_ = false;
    bool lineVisible_ = true;
    bool symbolsVisible_ = false;
    QPen pen_;
    QString symbol_;
    qreal symbolSize_ = 7.0;
    QPen symbolPen_;
    QBrush symbolBrush_;
};

} // namespace cppqtgraph::graphicsItems
