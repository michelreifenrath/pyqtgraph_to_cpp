#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ROI.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "ImageItem.hpp"

#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <cppqtgraph/Point.hpp>
#include <cppqtgraph/core/ArrayView.hpp>

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QTransform>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

struct ROIState {
    cppqtgraph::Point pos{0.0, 0.0};
    cppqtgraph::Point size{1.0, 1.0};
    qreal angle = 0.0;

    friend bool operator==(const ROIState& lhs, const ROIState& rhs) noexcept
    {
        return lhs.pos == rhs.pos && lhs.size == rhs.size && lhs.angle == rhs.angle;
    }

    friend bool operator!=(const ROIState& lhs, const ROIState& rhs) noexcept { return !(lhs == rhs); }
};

struct ROIAffineSliceParams {
    cppqtgraph::Point shape{0.0, 0.0};
    std::array<cppqtgraph::Point, 2> vectors{cppqtgraph::Point(0.0, 0.0), cppqtgraph::Point(0.0, 0.0)};
    cppqtgraph::Point origin{0.0, 0.0};
};

struct ROIArraySlice {
    std::array<std::pair<std::size_t, std::size_t>, 2> bounds{};
    QTransform transform;
};

struct ROIArrayRegion {
    std::array<std::size_t, 2> shape{};
    std::vector<double> values;

    [[nodiscard]] double operator()(std::size_t axis0, std::size_t axis1) const
    {
        return values.at(axis0 * shape[1] + axis1);
    }
};

class ROI : public GraphicsObject {
    Q_OBJECT

public:
    class Handle : public GraphicsObject, public cppqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    public:
        enum class Type { Scale, Free, Rotate, ScaleRotate };

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
        void hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event) override;
        void mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event) override;

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

    using GraphicsObject::scale;

    [[nodiscard]] ROIState getState() const;
    [[nodiscard]] ROIState stateCopy() const;
    [[nodiscard]] ROIState saveState() const;
    void setState(const ROIState& state, bool update = true);

    void setZValue(qreal z);

    [[nodiscard]] cppqtgraph::Point size() const;
    [[nodiscard]] cppqtgraph::Point pos() const;
    [[nodiscard]] qreal angle() const noexcept;

    void setPos(const QPointF& pos, bool update = true, bool finish = true);
    void setPos(qreal x, qreal y, bool update = true, bool finish = true);
    void setSize(const QPointF& size, bool update = true, bool finish = true);
    void setSize(const QPointF& size,
                 const std::optional<QPointF>& center,
                 const std::optional<QPointF>& centerLocal = std::nullopt,
                 bool snap = false,
                 bool update = true,
                 bool finish = true);
    void setAngle(qreal angle, bool update = true, bool finish = true);
    void setAngle(qreal angle,
                  const std::optional<QPointF>& center,
                  const std::optional<QPointF>& centerLocal = std::nullopt,
                  bool snap = false,
                  bool update = true,
                  bool finish = true);
    void scale(const QPointF& factors,
               const std::optional<QPointF>& center = std::nullopt,
               const std::optional<QPointF>& centerLocal = std::nullopt,
               bool snap = false,
               bool update = true,
               bool finish = true);
    void translate(const QPointF& delta, bool snap = false, bool finish = true, bool update = true);
    void translate(const QPointF& delta, const QPointF& snap, bool finish = true, bool update = true);
    void rotate(qreal angle,
                const std::optional<QPointF>& centerLocal = std::nullopt,
                bool snap = false,
                bool update = true,
                bool finish = true);

    [[nodiscard]] ROIAffineSliceParams getAffineSliceParams(const QGraphicsItem* target, bool fromBoundingRect = false) const;
    [[nodiscard]] ROIAffineSliceParams getAffineSliceParams(std::array<std::size_t, 2> dataShape,
                                                            const ImageItem& image,
                                                            bool fromBoundingRect = false) const;
    [[nodiscard]] std::optional<ROIArraySlice> getArraySlice(std::array<std::size_t, 2> dataShape,
                                                             const ImageItem& image) const;
    template <typename T>
    [[nodiscard]] ROIArrayRegion getArrayRegion(core::ArrayView<const T, 2> data,
                                                const ImageItem& image,
                                                bool fromBoundingRect = false,
                                                int order = 1) const;

    [[nodiscard]] QPointF getSnapPosition(const QPointF& pos, bool snap = true) const;
    [[nodiscard]] QPointF getSnapPosition(const QPointF& pos, const QPointF& snap) const;
    void setSnapSize(qreal snapSize) noexcept;
    [[nodiscard]] qreal snapSize() const noexcept;

    Handle* addScaleHandle(const QPointF& pos,
                           const QPointF& center,
                           Handle* item = nullptr,
                           const QString& name = QString(),
                           bool lockAspect = false);
    Handle* addFreeHandle(const QPointF& pos,
                          Handle* item = nullptr,
                          const QString& name = QString());
    Handle* addRotateHandle(const QPointF& pos,
                            const QPointF& center,
                            Handle* item = nullptr,
                            const QString& name = QString());
    Handle* addScaleRotateHandle(const QPointF& pos,
                                 const QPointF& center,
                                 Handle* item = nullptr,
                                 const QString& name = QString());
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

    void setAspectLocked(bool locked) noexcept;
    [[nodiscard]] bool aspectLocked() const noexcept;

signals:
    void sigRegionChangeFinished(cppqtgraph::graphicsItems::ROI* roi);
    void sigRegionChangeStarted(cppqtgraph::graphicsItems::ROI* roi);
    void sigRegionChanged(cppqtgraph::graphicsItems::ROI* roi);

private:
    struct HandleInfo {
        QString name;
        Handle::Type type = Handle::Type::Scale;
        cppqtgraph::Point pos{0.0, 0.0};
        cppqtgraph::Point center{0.0, 0.0};
        Handle* item = nullptr;
        bool lockAspect = false;
        bool xOff = false;
        bool yOff = false;
    };

    Handle* addHandle(HandleInfo info, int index = -1);
    [[nodiscard]] int indexOfHandle(const Handle* handle) const;
    [[nodiscard]] bool handleUsesAbsolutePosition(Handle::Type type) const noexcept;

protected:
    void clearHandles();

    ROIState state_;
    ROIState lastState_;
    ROIState preMoveState_;
    bool haveLastState_ = false;
    bool freeHandleMoved_ = false;
    qreal snapSize_ = 1.0;
    bool scaleSnap_ = false;
    qreal scaleSnapSize_ = 1.0;
    qreal rotateSnapAngle_ = 15.0;
    bool invertible_ = true;
    bool aspectLocked_ = false;
    QVector<HandleInfo> handles_;
    QPen pen_;
    QPen hoverPen_;
    QPen currentPen_;
};

class RectROI : public ROI {
public:
    explicit RectROI(const QPointF& pos,
                     const QPointF& size,
                     bool centered = false,
                     bool sideScalers = false,
                     QGraphicsItem* parent = nullptr);
};

class EllipseROI : public ROI {
public:
    explicit EllipseROI(const QPointF& pos, const QPointF& size, QGraphicsItem* parent = nullptr);

    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    virtual void addShapeHandles();
    void invalidateShapePath() noexcept;

private:
    mutable QPainterPath shapePath_;
    mutable bool shapePathValid_ = false;
};

class CircleROI : public EllipseROI {
public:
    explicit CircleROI(const QPointF& pos,
                       const QPointF& size,
                       QGraphicsItem* parent = nullptr);
    explicit CircleROI(const QPointF& pos,
                       qreal radius,
                       QGraphicsItem* parent = nullptr);

protected:
    void addShapeHandles() override;
};

class LineROI : public ROI {
public:
    explicit LineROI(const QPointF& pos1, const QPointF& pos2, qreal width, QGraphicsItem* parent = nullptr);
};

class PolyLineROI : public ROI {
public:
    explicit PolyLineROI(const QVector<QPointF>& positions,
                         bool closed = false,
                         const QPointF& pos = QPointF(0.0, 0.0),
                         QGraphicsItem* parent = nullptr);

    void setPoints(const QVector<QPointF>& points, std::optional<bool> closed = std::nullopt);
    [[nodiscard]] bool closed() const noexcept;
    [[nodiscard]] QVector<QPointF> pointPositions() const;

    [[nodiscard]] QPainterPath shape() const override;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    bool closed_ = false;
};

template <typename T>
ROIArrayRegion ROI::getArrayRegion(core::ArrayView<const T, 2> data,
                                   const ImageItem& image,
                                   bool fromBoundingRect,
                                   int order) const
{
    if (order != 0 && order != 1) {
        throw std::invalid_argument("ROI::getArrayRegion supports interpolation order 0 or 1");
    }

    const ROIAffineSliceParams params = getAffineSliceParams(data.shape(), image, fromBoundingRect);
    const auto out0 = static_cast<std::size_t>(std::max<qreal>(0.0, std::ceil(params.shape.x())));
    const auto out1 = static_cast<std::size_t>(std::max<qreal>(0.0, std::ceil(params.shape.y())));

    ROIArrayRegion result;
    result.shape = {out0, out1};
    result.values.assign(out0 * out1, 0.0);
    if (data.shape()[0] == 0 || data.shape()[1] == 0) {
        return result;
    }

    const auto sampleNearest = [&data](qreal axis0, qreal axis1) -> double {
        const auto i0 = static_cast<long long>(std::nearbyint(axis0));
        const auto i1 = static_cast<long long>(std::nearbyint(axis1));
        if (i0 < 0 || i1 < 0 || i0 >= static_cast<long long>(data.shape()[0]) || i1 >= static_cast<long long>(data.shape()[1])) {
            return 0.0;
        }
        return static_cast<double>(data(static_cast<std::size_t>(i0), static_cast<std::size_t>(i1)));
    };

    const auto sampleLinear = [&data](qreal axis0, qreal axis1) -> double {
        if (axis0 < 0.0 || axis1 < 0.0 || axis0 > static_cast<qreal>(data.shape()[0] - 1)
            || axis1 > static_cast<qreal>(data.shape()[1] - 1)) {
            return 0.0;
        }

        const auto i0 = static_cast<long long>(std::floor(axis0));
        const auto i1 = static_cast<long long>(std::floor(axis1));
        const qreal d0 = axis0 - static_cast<qreal>(i0);
        const qreal d1 = axis1 - static_cast<qreal>(i1);
        double value = 0.0;
        for (int f0 = 0; f0 <= 1; ++f0) {
            for (int f1 = 0; f1 <= 1; ++f1) {
                long long n0 = i0 + f0;
                long long n1 = i1 + f1;
                if (n0 < 0 || n1 < 0 || n0 >= static_cast<long long>(data.shape()[0])
                    || n1 >= static_cast<long long>(data.shape()[1])) {
                    n0 = 0;
                    n1 = 0;
                }
                const qreal w0 = f0 == 0 ? (1.0 - d0) : d0;
                const qreal w1 = f1 == 0 ? (1.0 - d1) : d1;
                value += static_cast<double>(data(static_cast<std::size_t>(n0), static_cast<std::size_t>(n1))) * w0 * w1;
            }
        }
        return value;
    };

    for (std::size_t axis0 = 0; axis0 < out0; ++axis0) {
        for (std::size_t axis1 = 0; axis1 < out1; ++axis1) {
            const cppqtgraph::Point sample = params.origin + params.vectors[0] * static_cast<qreal>(axis0)
                + params.vectors[1] * static_cast<qreal>(axis1);
            result.values[axis0 * out1 + axis1] = order == 0 ? sampleNearest(sample.x(), sample.y()) : sampleLinear(sample.x(), sample.y());
        }
    }

    return result;
}

} // namespace cppqtgraph::graphicsItems
