#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/GraphicsScene.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>

#include <memory>

class QEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsView;
class QObject;
class QPainter;

namespace cppqtgraph::GraphicsScene {

class GraphicsSceneEventHandler {
public:
    virtual ~GraphicsSceneEventHandler() = default;

    virtual void hoverEvent(HoverEvent* event) { Q_UNUSED(event); }
    virtual void mouseClickEvent(MouseClickEvent* event) { Q_UNUSED(event); }
    virtual void mouseDragEvent(MouseDragEvent* event) { Q_UNUSED(event); }
};

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

    [[nodiscard]] QList<QGraphicsItem*> itemsNearEvent(const MouseClickEvent& event, bool hoverable = false) const;
    [[nodiscard]] QList<QGraphicsItem*> itemsNearEvent(const MouseDragEvent& event, bool hoverable = false) const;
    [[nodiscard]] QList<QGraphicsItem*> itemsNearEvent(const HoverEvent& event, bool hoverable = false) const;

    // Item (if any) that claimed the drag for `button` during the most recent hover
    // (via HoverEvent::acceptDrags). Lets a native-event panner such as ViewBox yield
    // the press to a custom GraphicsSceneEventHandler item (LinearRegionItem, ROI, ...)
    // instead of grabbing the mouse and short-circuiting the custom drag dispatch.
    [[nodiscard]] QGraphicsItem* dragClaimantForButton(Qt::MouseButton button) const;

    void addItem(QGraphicsItem* item);
    void removeItem(QGraphicsItem* item);

    void render(QPainter* painter, const QRectF& target = QRectF(), const QRectF& source = QRectF(),
        Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);

public slots:
    void prepareForPaint();

signals:
    void sigMouseHover(const QList<QGraphicsItem*>& items);
    void sigMouseMoved(const QPointF& scenePos);
    void sigMouseClicked(MouseClickEvent* event);
    void sigPrepareForPaint();
    void sigItemAdded(QGraphicsItem* item);
    void sigItemRemoved(QGraphicsItem* item);

protected:
    bool event(QEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    [[nodiscard]] bool moveEventIsAllowed() const noexcept;
    void sendHoverEvents(QGraphicsSceneMouseEvent* event, bool exitOnly = false);
    [[nodiscard]] bool sendDragEvent(QGraphicsSceneMouseEvent* event, bool init = false, bool final = false);
    [[nodiscard]] bool sendClickEvent(MouseClickEvent& event);
    [[nodiscard]] QList<QGraphicsItem*> itemsNearPoint(const QPointF& point, bool hoverable) const;

    int clickRadius_ = 2;
    qreal moveDistance_ = 5.0;
    QList<MouseClickEvent> clickEvents_;
    QList<Qt::MouseButton> dragButtons_;
    QGraphicsItem* dragItem_ = nullptr;
    std::unique_ptr<MouseDragEvent> lastDrag_;
    QList<QGraphicsItem*> hoverItems_;
    std::unique_ptr<HoverEvent> lastHoverEvent_;
    qint64 minDragTimeMilliseconds_ = 500;
};

} // namespace cppqtgraph::GraphicsScene
