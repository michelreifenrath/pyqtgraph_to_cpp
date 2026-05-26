// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/PlotWidget.hpp"

#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QVBoxLayout>

namespace pyqtgraph::widgets {

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
    , view_(std::make_unique<QGraphicsView>(this))
    , scene_(std::make_unique<GraphicsScene::GraphicsScene>())
    , plotItem_(new graphicsItems::PlotItem())
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_.get());

    view_->setScene(scene_.get());
    scene_->addItem(plotItem_);
}

PlotWidget::~PlotWidget()
{
    view_->setScene(nullptr);
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
