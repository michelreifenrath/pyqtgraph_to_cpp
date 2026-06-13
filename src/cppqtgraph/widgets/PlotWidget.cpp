// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/PlotWidget.hpp"

#include <QtWidgets/QSizePolicy>

namespace cppqtgraph::widgets {

PlotWidget::PlotWidget(QWidget* parent)
    : GraphicsView(parent)
    , plotItem_(new graphicsItems::PlotItem())
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    enableMouse(false);
    setCentralItem(plotItem_);
}

PlotWidget::~PlotWidget() = default;

graphicsItems::PlotItem* PlotWidget::getPlotItem() noexcept
{
    return plotItem_;
}

const graphicsItems::PlotItem* PlotWidget::getPlotItem() const noexcept
{
    return plotItem_;
}

void PlotWidget::addItem(QGraphicsItem* item, bool ignoreBounds, const QString& name)
{
    plotItem_->addItem(item, ignoreBounds, name);
}

void PlotWidget::removeItem(QGraphicsItem* item)
{
    plotItem_->removeItem(item);
}

void PlotWidget::clear()
{
    plotItem_->clear();
}

graphicsItems::PlotDataItem* PlotWidget::plot(std::span<const double> y, const QString& name)
{
    return plotItem_->plot(y, name);
}

graphicsItems::PlotDataItem* PlotWidget::plot(std::span<const double> x, std::span<const double> y, const QString& name)
{
    return plotItem_->plot(x, y, name);
}

graphicsItems::PlotDataItem* PlotWidget::plot(std::span<const double> y, graphicsItems::PlotItem::PlotOptions options)
{
    return plotItem_->plot(y, options);
}

graphicsItems::PlotDataItem* PlotWidget::plot(std::span<const double> x, std::span<const double> y,
                                              graphicsItems::PlotItem::PlotOptions options)
{
    return plotItem_->plot(x, y, options);
}

graphicsItems::LegendItem* PlotWidget::addLegend(std::optional<QPointF> offset)
{
    return plotItem_->addLegend(offset);
}

graphicsItems::AxisItem* PlotWidget::getAxis(const QString& name)
{
    return plotItem_->getAxis(name);
}

const graphicsItems::AxisItem* PlotWidget::getAxis(const QString& name) const
{
    return plotItem_->getAxis(name);
}

void PlotWidget::setLabel(const QString& axis, const QString& text, const QString& units, const QString& unitPrefix)
{
    plotItem_->setLabel(axis, text, units, unitPrefix);
}

void PlotWidget::setTitle(const QString& title)
{
    plotItem_->setTitle(title);
}

void PlotWidget::showAxis(const QString& axis, bool show)
{
    plotItem_->showAxis(axis, show);
}

void PlotWidget::hideAxis(const QString& axis)
{
    plotItem_->hideAxis(axis);
}

void PlotWidget::setRange(const QRectF& rect, qreal padding, bool update, bool disableAutoRange)
{
    plotItem_->setRange(rect, padding, update, disableAutoRange);
}

void PlotWidget::setXRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    plotItem_->setXRange(minimum, maximum, padding, update);
}

void PlotWidget::setYRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    plotItem_->setYRange(minimum, maximum, padding, update);
}

void PlotWidget::autoRange(std::optional<qreal> padding)
{
    plotItem_->autoRange(padding);
}

void PlotWidget::setAspectLocked(bool lock, std::optional<qreal> ratio)
{
    plotItem_->getViewBox()->setAspectLocked(lock, ratio);
}

void PlotWidget::setMouseEnabled(std::optional<bool> x, std::optional<bool> y)
{
    plotItem_->getViewBox()->setMouseEnabled(x, y);
}

void PlotWidget::enableAutoRange(int axis, bool enable)
{
    plotItem_->getViewBox()->enableAutoRange(axis, enable);
}

void PlotWidget::disableAutoRange(int axis)
{
    plotItem_->getViewBox()->disableAutoRange(axis);
}

void PlotWidget::setLimits(const graphicsItems::ViewBox::Limits& limits)
{
    plotItem_->getViewBox()->setLimits(limits);
}

void PlotWidget::setXLink(graphicsItems::ViewBox* view)
{
    plotItem_->getViewBox()->setXLink(view);
}

void PlotWidget::setYLink(graphicsItems::ViewBox* view)
{
    plotItem_->getViewBox()->setYLink(view);
}

graphicsItems::ViewBox::Range2D PlotWidget::viewRange() const
{
    return plotItem_->viewRange();
}

QRectF PlotWidget::viewRect() const
{
    return plotItem_->viewRect();
}

} // namespace cppqtgraph::widgets
