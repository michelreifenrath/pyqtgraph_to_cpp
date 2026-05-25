// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/PlotWidget.hpp"

#include <QtWidgets/QGraphicsScene>

namespace pyqtgraph::widgets {

PlotWidget::PlotWidget(QWidget* parent)
    : QGraphicsView(parent)
    , scene_(std::make_unique<QGraphicsScene>())
    , plotItem_(new graphicsItems::PlotItem())
{
    setScene(scene_.get());
    scene_->addItem(plotItem_);
}

PlotWidget::~PlotWidget()
{
    setScene(nullptr);
}

graphicsItems::PlotItem* PlotWidget::getPlotItem() noexcept
{
    return plotItem_;
}

const graphicsItems::PlotItem* PlotWidget::getPlotItem() const noexcept
{
    return plotItem_;
}

} // namespace pyqtgraph::widgets
