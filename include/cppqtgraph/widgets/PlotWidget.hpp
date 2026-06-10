#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsView.hpp"
#include "../graphicsItems/PlotItem/PlotItem.hpp"

#include <optional>
#include <span>

namespace cppqtgraph::widgets {

class PlotWidget : public GraphicsView {
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr);
    ~PlotWidget() override;

    PlotWidget(const PlotWidget&) = delete;
    PlotWidget& operator=(const PlotWidget&) = delete;
    PlotWidget(PlotWidget&&) = delete;
    PlotWidget& operator=(PlotWidget&&) = delete;

    [[nodiscard]] graphicsItems::PlotItem* getPlotItem() noexcept;
    [[nodiscard]] const graphicsItems::PlotItem* getPlotItem() const noexcept;

    void addItem(QGraphicsItem* item, bool ignoreBounds = false, const QString& name = QString{});
    void removeItem(QGraphicsItem* item);
    void clear();

    graphicsItems::PlotCurveItem* plot(std::span<const double> y, const QString& name = QString{});
    graphicsItems::PlotCurveItem* plot(std::span<const double> x, std::span<const double> y,
                                       const QString& name = QString{});

    graphicsItems::LegendItem* addLegend(std::optional<QPointF> offset = QPointF(30.0, 30.0));
    [[nodiscard]] graphicsItems::AxisItem* getAxis(const QString& name);
    [[nodiscard]] const graphicsItems::AxisItem* getAxis(const QString& name) const;
    void setLabel(const QString& axis, const QString& text = QString{}, const QString& units = QString{},
                  const QString& unitPrefix = QString{});
    void setTitle(const QString& title = QString{});
    void showAxis(const QString& axis, bool show = true);
    void hideAxis(const QString& axis);

    void setRange(const QRectF& rect, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);
    void setXRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void setYRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void autoRange(std::optional<qreal> padding = std::nullopt);
    void setAspectLocked(bool lock = true, std::optional<qreal> ratio = 1.0);
    void setMouseEnabled(std::optional<bool> x = std::nullopt, std::optional<bool> y = std::nullopt);
    void enableAutoRange(int axis = graphicsItems::ViewBox::XYAxes, bool enable = true);
    void disableAutoRange(int axis = graphicsItems::ViewBox::XYAxes);
    void setLimits(const graphicsItems::ViewBox::Limits& limits);
    void setXLink(graphicsItems::ViewBox* view);
    void setYLink(graphicsItems::ViewBox* view);
    [[nodiscard]] graphicsItems::ViewBox::Range2D viewRange() const;
    [[nodiscard]] QRectF viewRect() const;

private:
    graphicsItems::PlotItem* plotItem_ = nullptr;
};

} // namespace cppqtgraph::widgets
