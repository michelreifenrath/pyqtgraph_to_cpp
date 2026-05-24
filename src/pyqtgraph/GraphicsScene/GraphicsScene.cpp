// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/GraphicsScene.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/GraphicsScene/GraphicsScene.hpp"

#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsView>

namespace pyqtgraph::GraphicsScene {

GraphicsScene::GraphicsScene(int clickRadius, qreal moveDistance, QObject* parent)
    : QGraphicsScene(parent)
{
    setClickRadius(clickRadius);
    setMoveDistance(moveDistance);
}

GraphicsScene::~GraphicsScene() = default;

void GraphicsScene::setClickRadius(int radius) noexcept
{
    clickRadius_ = radius;
}

int GraphicsScene::clickRadius() const noexcept
{
    return clickRadius_;
}

void GraphicsScene::setMoveDistance(qreal distance) noexcept
{
    moveDistance_ = distance;
}

qreal GraphicsScene::moveDistance() const noexcept
{
    return moveDistance_;
}

QGraphicsView* GraphicsScene::getViewWidget() const
{
    const QList<QGraphicsView*> sceneViews = views();
    if (sceneViews.empty()) {
        return nullptr;
    }

    return sceneViews.front();
}

void GraphicsScene::addItem(QGraphicsItem* item)
{
    QGraphicsScene::addItem(item);
    emit sigItemAdded(item);
}

void GraphicsScene::removeItem(QGraphicsItem* item)
{
    QGraphicsScene::removeItem(item);
    emit sigItemRemoved(item);
}

void GraphicsScene::prepareForPaint()
{
    emit sigPrepareForPaint();
}

} // namespace pyqtgraph::GraphicsScene
