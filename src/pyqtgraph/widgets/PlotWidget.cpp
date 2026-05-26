// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/PlotWidget.hpp"

#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QFrame>

namespace pyqtgraph::widgets {

namespace {

QRectF sceneRectForSize(const QSize& size)
{
    return QRectF(0.0, 0.0, static_cast<qreal>(size.width()), static_cast<qreal>(size.height()));
}

} // namespace

PlotWidget::PlotWidget(QWidget* parent)
    : QGraphicsView(parent)
    , scene_(std::make_unique<GraphicsScene::GraphicsScene>())
    , plotItem_(new graphicsItems::PlotItem())
{
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setBackgroundBrush(Qt::black);
    setScene(scene_.get());
    scene_->addItem(plotItem_);
    const QRectF initialSceneRect = sceneRectForSize(size());
    scene_->setSceneRect(initialSceneRect);
    plotItem_->setGeometry(initialSceneRect);
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

void PlotWidget::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    const QRectF updatedSceneRect = sceneRectForSize(viewport()->size());
    scene_->setSceneRect(updatedSceneRect);
    plotItem_->setGeometry(updatedSceneRect);
}

} // namespace pyqtgraph::widgets
