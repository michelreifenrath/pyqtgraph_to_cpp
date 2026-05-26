#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"

#include <QtCore/Qt>
#include <QtWidgets/QGraphicsItem>

class QGraphicsSceneResizeEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class PlotItem : public GraphicsWidget {
public:
    explicit PlotItem(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~PlotItem() override;

    PlotItem(const PlotItem&) = delete;
    PlotItem& operator=(const PlotItem&) = delete;
    PlotItem(PlotItem&&) = delete;
    PlotItem& operator=(PlotItem&&) = delete;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
};

} // namespace pyqtgraph::graphicsItems
