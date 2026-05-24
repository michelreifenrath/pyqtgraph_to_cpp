// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GraphicsItem.hpp"

#include <QtCore/QPointer>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>

#include <memory>
#include <utility>

namespace pyqtgraph::graphicsItems {

struct GraphicsItem::Private {
    explicit Private(QGraphicsItem* graphicsItem) noexcept
        : host(graphicsItem)
    {
    }

    QGraphicsItem* host = nullptr;
    mutable QPointer<QGraphicsView> viewWidget;
};

GraphicsItem::GraphicsItem(QGraphicsItem* host)
    : d_(std::make_unique<Private>(host))
{
}

GraphicsItem::~GraphicsItem() = default;

GraphicsItem::GraphicsItem(GraphicsItem&&) noexcept = default;

GraphicsItem& GraphicsItem::operator=(GraphicsItem&&) noexcept = default;

void GraphicsItem::setGraphicsItem(QGraphicsItem* host) noexcept
{
    if (d_->host == host) {
        return;
    }

    d_->host = host;
    forgetViewWidget();
}

QGraphicsItem* GraphicsItem::graphicsItem() const noexcept
{
    return d_->host;
}

QGraphicsView* GraphicsItem::getViewWidget() const
{
    if (!d_->viewWidget.isNull()) {
        return d_->viewWidget.data();
    }

    if (d_->host == nullptr || d_->host->scene() == nullptr) {
        return nullptr;
    }

    const QList<QGraphicsView*> views = d_->host->scene()->views();
    if (views.isEmpty()) {
        return nullptr;
    }

    d_->viewWidget = views.front();
    return d_->viewWidget.data();
}

void GraphicsItem::forgetViewWidget() const noexcept
{
    d_->viewWidget.clear();
}

} // namespace pyqtgraph::graphicsItems
