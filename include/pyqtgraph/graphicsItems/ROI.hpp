#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ROI.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <pyqtgraph/Point.hpp>

#include <QtCore/QPointF>
#include <QtCore/QRectF>
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
    ROIState state_;
    ROIState lastState_;
    bool haveLastState_ = false;
    bool freeHandleMoved_ = false;
    qreal snapSize_ = 1.0;
    QPen pen_;
    QPen hoverPen_;
    QPen currentPen_;
};

} // namespace pyqtgraph::graphicsItems
