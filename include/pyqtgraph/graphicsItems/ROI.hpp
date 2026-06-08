#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ROI.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <pyqtgraph/Point.hpp>

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

struct ROIState {
    pyqtgraph::Point pos{0.0, 0.0};
    pyqtgraph::Point size{1.0, 1.0};
    qreal angle = 0.0;

    friend bool operator==(const ROIState& lhs, const ROIState& rhs) noexcept
    {
        return lhs.pos == rhs.pos && lhs.size == rhs.size && lhs.angle == rhs.angle;
    }

    friend bool operator!=(const ROIState& lhs, const ROIState& rhs) noexcept { return !(lhs == rhs); }
};

class ROI : public GraphicsObject {
    Q_OBJECT

public:
    class Handle : public GraphicsObject, public pyqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    public:
        enum class Type { Scale };

        explicit Handle(qreal radius = 5.0,
                        Type type = Type::Scale,
                        const QPen& pen = QPen(Qt::white),
                        const QPen& hoverPen = QPen(Qt::yellow),
                        QGraphicsItem* parent = nullptr);
        ~Handle() override;

        void connectROI(ROI* roi);
        void disconnectROI(ROI* roi);
        void movePoint(const QPointF& pos,
                       Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                       bool finish = true);
        void hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event) override;
        void mouseDragEvent(pyqtgraph::GraphicsScene::MouseDragEvent* event) override;

        [[nodiscard]] QRectF boundingRect() const override;
        [[nodiscard]] QPainterPath shape() const override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    private:
        void buildPath();

        qreal radius_ = 5.0;
        Type type_ = Type::Scale;
        QPen pen_;
        QPen hoverPen_;
        QPen currentPen_;
        QPainterPath path_;
        QList<ROI*> rois_;
        bool isMoving_ = false;
        QPointF startPos_;
        QPointF cursorOffset_;
    };

    enum class HandleCoordinateSystem { Parent, Scene };

    explicit ROI(const QPointF& pos, const QPointF& size = QPointF(1.0, 1.0), qreal angle = 0.0, QGraphicsItem* parent = nullptr);
    ~ROI() override;

    [[nodiscard]] ROIState getState() const;
    [[nodiscard]] ROIState stateCopy() const;
    [[nodiscard]] ROIState saveState() const;
    void setState(const ROIState& state, bool update = true);

    void setZValue(qreal z);

    [[nodiscard]] pyqtgraph::Point size() const;
    [[nodiscard]] pyqtgraph::Point pos() const;
    [[nodiscard]] qreal angle() const noexcept;

    void setPos(const QPointF& pos, bool update = true, bool finish = true);
    void setPos(qreal x, qreal y, bool update = true, bool finish = true);
    void setSize(const QPointF& size, bool update = true, bool finish = true);
    void setAngle(qreal angle, bool update = true, bool finish = true);
    void translate(const QPointF& delta, bool snap = false, bool finish = true, bool update = true);

    [[nodiscard]] QPointF getSnapPosition(const QPointF& pos, bool snap = true) const;
    [[nodiscard]] QPointF getSnapPosition(const QPointF& pos, const QPointF& snap) const;
    void setSnapSize(qreal snapSize) noexcept;
    [[nodiscard]] qreal snapSize() const noexcept;

    Handle* addScaleHandle(const QPointF& pos,
                           const QPointF& center,
                           Handle* item = nullptr,
                           const QString& name = QString(),
                           bool lockAspect = false);
    [[nodiscard]] QList<Handle*> getHandles() const;
    void handleMoveStarted();
    [[nodiscard]] bool checkPointMove(const Handle* handle,
                                      const QPointF& pos,
                                      Qt::KeyboardModifiers modifiers = Qt::NoModifier) const;
    void movePoint(Handle* handle,
                   const QPointF& pos,
                   Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                   bool finish = true,
                   HandleCoordinateSystem coords = HandleCoordinateSystem::Parent);

    void stateChanged(bool finish = true);
    void stateChangeFinished();

    [[nodiscard]] QRectF parentBounds() const;
    [[nodiscard]] QRectF stateRect(const ROIState& state) const;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;
    void setHoverPen(const QPen& pen);
    [[nodiscard]] QPen hoverPen() const;

signals:
    void sigRegionChangeFinished(pyqtgraph::graphicsItems::ROI* roi);
    void sigRegionChangeStarted(pyqtgraph::graphicsItems::ROI* roi);
    void sigRegionChanged(pyqtgraph::graphicsItems::ROI* roi);

private:
    struct HandleInfo {
        QString name;
        Handle::Type type = Handle::Type::Scale;
        pyqtgraph::Point pos{0.0, 0.0};
        pyqtgraph::Point center{0.0, 0.0};
        Handle* item = nullptr;
        bool lockAspect = false;
        bool xOff = false;
        bool yOff = false;
    };

    Handle* addHandle(HandleInfo info);
    [[nodiscard]] int indexOfHandle(const Handle* handle) const;

    ROIState state_;
    ROIState lastState_;
    ROIState preMoveState_;
    bool haveLastState_ = false;
    bool freeHandleMoved_ = false;
    qreal snapSize_ = 1.0;
    bool scaleSnap_ = false;
    qreal scaleSnapSize_ = 1.0;
    bool invertible_ = true;
    bool aspectLocked_ = false;
    QVector<HandleInfo> handles_;
    QPen pen_;
    QPen hoverPen_;
    QPen currentPen_;
};

} // namespace pyqtgraph::graphicsItems
