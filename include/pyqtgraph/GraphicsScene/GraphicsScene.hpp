#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/GraphicsScene.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QGraphicsScene>

class QGraphicsItem;
class QGraphicsView;
class QObject;

namespace pyqtgraph::GraphicsScene {

class GraphicsScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit GraphicsScene(int clickRadius = 2, qreal moveDistance = 5.0, QObject* parent = nullptr);
    ~GraphicsScene() override;

    GraphicsScene(const GraphicsScene&) = delete;
    GraphicsScene& operator=(const GraphicsScene&) = delete;
    GraphicsScene(GraphicsScene&&) = delete;
    GraphicsScene& operator=(GraphicsScene&&) = delete;

    void setClickRadius(int radius) noexcept;
    [[nodiscard]] int clickRadius() const noexcept;

    void setMoveDistance(qreal distance) noexcept;
    [[nodiscard]] qreal moveDistance() const noexcept;

    [[nodiscard]] QGraphicsView* getViewWidget() const;

    void addItem(QGraphicsItem* item);
    void removeItem(QGraphicsItem* item);

public slots:
    void prepareForPaint();

signals:
    void sigPrepareForPaint();
    void sigItemAdded(QGraphicsItem* item);
    void sigItemRemoved(QGraphicsItem* item);

private:
    int clickRadius_ = 2;
    qreal moveDistance_ = 5.0;
};

} // namespace pyqtgraph::GraphicsScene
