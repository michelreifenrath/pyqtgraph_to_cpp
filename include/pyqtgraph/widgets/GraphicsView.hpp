#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GraphicsView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsScene/GraphicsScene.hpp"

#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtWidgets/QGraphicsView>

#include <memory>

class QGraphicsWidget;
class QResizeEvent;
class QWheelEvent;

namespace pyqtgraph::widgets {

class GraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphicsView(QWidget* parent = nullptr);
    ~GraphicsView() override;

    GraphicsView(const GraphicsView&) = delete;
    GraphicsView& operator=(const GraphicsView&) = delete;
    GraphicsView(GraphicsView&&) = delete;
    GraphicsView& operator=(GraphicsView&&) = delete;

    [[nodiscard]] GraphicsScene::GraphicsScene* graphicsScene() noexcept;
    [[nodiscard]] const GraphicsScene::GraphicsScene* graphicsScene() const noexcept;

    void setCentralItem(QGraphicsWidget* item);
    void setCentralWidget(QGraphicsWidget* item);
    [[nodiscard]] QGraphicsWidget* centralItem() const noexcept;
    [[nodiscard]] QGraphicsWidget* centralWidget() const noexcept;

    void addItem(QGraphicsItem* item);
    void removeItem(QGraphicsItem* item);

    void enableMouse(bool enabled = true);
    [[nodiscard]] bool mouseEnabled() const noexcept;

    void setRange(const QRectF& rect, qreal padding = 0.05, bool lockAspect = false, bool propagate = true,
                  bool disableAutoPixel = true);
    void setXRange(qreal left, qreal right, qreal padding = 0.05);
    void setYRange(qreal top, qreal bottom, qreal padding = 0.05);
    void translateRange(qreal dx, qreal dy);
    void scaleRange(qreal sx, qreal sy, QPointF center = QPointF{});
    void setAspectLocked(bool locked = true);

    [[nodiscard]] QRectF range() const noexcept;
    [[nodiscard]] QRectF viewRect() const;
    [[nodiscard]] QRectF visibleRange() const;

signals:
    void sigDeviceRangeChanged(pyqtgraph::widgets::GraphicsView* view, const QRectF& range);
    void sigDeviceTransformChanged(pyqtgraph::widgets::GraphicsView* view);
    void sigMouseReleased(QMouseEvent* event);
    void sigSceneMouseMoved(const QPointF& scenePos);
    void sigScaleChanged(pyqtgraph::widgets::GraphicsView* view);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateMatrix(bool propagate = true);
    void updateCentralGeometry();
    [[nodiscard]] QRectF paddedRect(const QRectF& rect, qreal padding) const;

    std::unique_ptr<GraphicsScene::GraphicsScene> scene_;
    QPointer<QGraphicsWidget> centralWidget_;
    QPointer<QGraphicsWidget> internallyOwnedDefaultCentral_;
    QRectF range_{0.0, 0.0, 1.0, 1.0};
    bool autoPixelRange_ = true;
    bool mouseEnabled_ = false;
    bool aspectLocked_ = false;
    QPointF lastMousePos_;
    bool hasLastMousePos_ = false;
};

} // namespace pyqtgraph::widgets
