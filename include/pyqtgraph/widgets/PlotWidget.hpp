#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PlotWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsScene/GraphicsScene.hpp"
#include "../graphicsItems/PlotItem/PlotItem.hpp"

#include <QtWidgets/QGraphicsView>

#include <memory>

namespace pyqtgraph::widgets {

class PlotWidget : public QGraphicsView {
public:
    explicit PlotWidget(QWidget* parent = nullptr);
    ~PlotWidget() override;

    PlotWidget(const PlotWidget&) = delete;
    PlotWidget& operator=(const PlotWidget&) = delete;
    PlotWidget(PlotWidget&&) = delete;
    PlotWidget& operator=(PlotWidget&&) = delete;

    graphicsItems::PlotItem* getPlotItem() noexcept;
    const graphicsItems::PlotItem* getPlotItem() const noexcept;

private:
    std::unique_ptr<GraphicsScene::GraphicsScene> scene_;
    graphicsItems::PlotItem* plotItem_ = nullptr;
};

} // namespace pyqtgraph::widgets
