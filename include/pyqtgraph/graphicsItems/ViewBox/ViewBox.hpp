#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/Qt>
#include <QtCore/QVariant>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsItemGroup>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGraphicsSceneWheelEvent>

#include <array>
#include <optional>
#include <vector>

namespace pyqtgraph::graphicsItems {

class ViewBox : public GraphicsWidget {
    Q_OBJECT

public:
    static constexpr int PanMode = 3;
    static constexpr int RectMode = 1;
    static constexpr int XAxis = 0;
    static constexpr int YAxis = 1;
    static constexpr int XYAxes = 2;

    using AxisRange = std::array<qreal, 2>;
    using Range2D = std::array<AxisRange, 2>;

    struct Limits {
        std::optional<qreal> xMin;
        std::optional<qreal> xMax;
        std::optional<qreal> yMin;
        std::optional<qreal> yMax;
        std::optional<qreal> minXRange;
        std::optional<qreal> maxXRange;
        std::optional<qreal> minYRange;
        std::optional<qreal> maxYRange;
    };

    explicit ViewBox(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~ViewBox() override;

    ViewBox(const ViewBox&) = delete;
    ViewBox& operator=(const ViewBox&) = delete;
    ViewBox(ViewBox&&) = delete;
    ViewBox& operator=(ViewBox&&) = delete;

    [[nodiscard]] Range2D viewRange() const;
    [[nodiscard]] Range2D targetRange() const;
    [[nodiscard]] QRectF viewRect() const;
    [[nodiscard]] QRectF targetRect() const;
    [[nodiscard]] Limits limits() const;

    void addItem(QGraphicsItem* item, bool ignoreBounds = false);
    void removeItem(QGraphicsItem* item);
    void clear();

    void setRange(const QRectF& rect, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);
    void setRange(std::optional<AxisRange> xRange,
                  std::optional<AxisRange> yRange,
                  qreal padding = 0.02,
                  bool update = true,
                  bool disableAutoRange = true);
    void setXRange(qreal min, qreal max, qreal padding = 0.02, bool update = true);
    void setYRange(qreal min, qreal max, qreal padding = 0.02, bool update = true);
    void scaleBy(std::optional<qreal> x, std::optional<qreal> y, std::optional<QPointF> center = std::nullopt);
    void scaleBy(const QPointF& scale, std::optional<QPointF> center = std::nullopt);
    void translateBy(std::optional<qreal> x, std::optional<qreal> y);
    void translateBy(const QPointF& offset);
    void setLimits(const Limits& limits);

    void setMouseMode(int mode);
    [[nodiscard]] int mouseMode() const;
    void setMouseEnabled(std::optional<bool> x = std::nullopt, std::optional<bool> y = std::nullopt);
    [[nodiscard]] std::array<bool, 2> mouseEnabled() const;
    void setWheelScaleFactor(qreal factor);
    [[nodiscard]] qreal wheelScaleFactor() const;

    void setXLink(ViewBox* view);
    void setYLink(ViewBox* view);
    void linkView(int axis, ViewBox* view);
    [[nodiscard]] ViewBox* linkedView(int axis) const;
    void linkedViewChanged(ViewBox* view, int axis);
    void blockLink(bool block);

    void autoRange(std::optional<qreal> padding = std::nullopt);
    void enableAutoRange(int axis = XYAxes, bool enable = true);
    void disableAutoRange(int axis = XYAxes);
    [[nodiscard]] std::array<bool, 2> autoRangeEnabled() const;
    void setDefaultPadding(qreal padding = 0.02);

    void setAspectLocked(bool lock = true, std::optional<qreal> ratio = 1.0);
    void invertX(bool inverted = true);
    void invertY(bool inverted = true);
    [[nodiscard]] bool xInverted() const;
    [[nodiscard]] bool yInverted() const;

    [[nodiscard]] QTransform childTransform();
    [[nodiscard]] QPointF mapToView(const QPointF& point);
    [[nodiscard]] QRectF mapToView(const QRectF& rect);
    [[nodiscard]] QPointF mapFromView(const QPointF& point);
    [[nodiscard]] QRectF mapFromView(const QRectF& rect);
    [[nodiscard]] QPointF mapSceneToView(const QPointF& point);
    [[nodiscard]] QRectF mapSceneToView(const QRectF& rect);
    [[nodiscard]] QPointF mapViewToScene(const QPointF& point);
    [[nodiscard]] QRectF mapViewToScene(const QRectF& rect);
    [[nodiscard]] std::array<std::optional<AxisRange>, 2> childrenBounds() const;
    [[nodiscard]] QRectF childrenBoundingRect() const;
    [[nodiscard]] QSizeF viewPixelSize();

signals:
    void sigXRangeChanged(ViewBox* view, AxisRange range);
    void sigYRangeChanged(ViewBox* view, AxisRange range);
    void sigRangeChangedManually(std::array<bool, 2> mask);
    void sigRangeChanged(ViewBox* view, Range2D range, std::array<bool, 2> changed);
    void sigStateChanged(ViewBox* view);
    void sigTransformChanged(ViewBox* view);
    void sigResized(ViewBox* view);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    void wheelEvent(QGraphicsSceneWheelEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

private:
    bool applyAutoRange(std::optional<qreal> padding, const std::array<bool, 2>& axes, bool disableAutoRange = false);
    void updateViewRange(bool forceX = false, bool forceY = false);
    void updateMatrix();
    void markMatrixDirty();
    void updateAutoRangeSceneConnection();
    void refreshAutoRangeIfNeeded();
    void pruneAddedItems() const;
    void emitRangeChanges(const std::array<bool, 2>& changed);
    void notifyLinkedViews(const std::array<bool, 2>& changed);
    [[nodiscard]] QRectF screenGeometry() const;
    [[nodiscard]] qreal currentAspectRatio() const;

    QGraphicsItemGroup childGroup_;
    Range2D targetRange_{{AxisRange{0.0, 1.0}, AxisRange{0.0, 1.0}}};
    Range2D viewRange_{{AxisRange{0.0, 1.0}, AxisRange{0.0, 1.0}}};
    Limits limits_;
    std::array<bool, 2> autoRange_{{true, true}};
    mutable std::vector<QGraphicsItem*> addedItems_;
    qreal defaultPadding_ = 0.02;
    bool xInverted_ = false;
    bool yInverted_ = false;
    std::optional<qreal> aspectLocked_;
    bool matrixNeedsUpdate_ = true;
    std::array<bool, 2> mouseEnabled_{{true, true}};
    int mouseMode_ = PanMode;
    qreal wheelScaleFactor_ = -1.0 / 8.0;
    bool dragActive_ = false;
    Qt::MouseButton dragButton_ = Qt::NoButton;
    QPointF dragLastPos_;
    QPointF dragButtonDownPos_;
    QPoint dragLastScreenPos_;
    std::array<QPointer<ViewBox>, 2> linkedViews_{{nullptr, nullptr}};
    std::array<std::array<QMetaObject::Connection, 2>, 2> linkConnections_{};
    bool linksBlocked_ = false;
    QMetaObject::Connection sceneChangedConnection_;
};

} // namespace pyqtgraph::graphicsItems
