#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ArrowItem.py
// and pyqtgraph/functions.py makeArrowPath
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QPointF>
#include <QtCore/QtGlobal>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsPathItem>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

namespace detail_arrowitem {

inline constexpr qreal pi = 3.141592653589793238462643383279502884L;
inline constexpr qreal penPaddingFactor = 0.7072;

inline qreal degreesToRadians(qreal degrees)
{
    return degrees * pi / 180.0;
}

inline QPen defaultArrowPen()
{
    QPen pen(QColor(200, 200, 200), 1.0);
    pen.setCosmetic(true);
    return pen;
}

inline QBrush defaultArrowBrush()
{
    return QBrush(QColor(50, 50, 200));
}

} // namespace detail_arrowitem

struct ArrowItemOptions {
    bool pxMode = true;
    qreal angle = -150.0;
    qreal headLen = 20.0;
    std::optional<qreal> headWidth;
    qreal tipAngle = 25.0;
    qreal baseAngle = 0.0;
    std::optional<qreal> tailLen;
    qreal tailWidth = 3.0;
    QPen pen = detail_arrowitem::defaultArrowPen();
    QBrush brush = detail_arrowitem::defaultArrowBrush();
};

class ArrowItem : public QGraphicsPathItem {
public:
    explicit ArrowItem(QGraphicsItem* parent = nullptr);
    explicit ArrowItem(const ArrowItemOptions& options, QGraphicsItem* parent = nullptr);
    explicit ArrowItem(const QPointF& pos, QGraphicsItem* parent = nullptr);
    ArrowItem(const QPointF& pos, const ArrowItemOptions& options, QGraphicsItem* parent = nullptr);
    ~ArrowItem() override = default;

    ArrowItem(const ArrowItem&) = delete;
    ArrowItem& operator=(const ArrowItem&) = delete;
    ArrowItem(ArrowItem&&) = delete;
    ArrowItem& operator=(ArrowItem&&) = delete;

    void setStyle(const ArrowItemOptions& options);
    [[nodiscard]] ArrowItemOptions style() const;
    [[nodiscard]] const ArrowItemOptions& options() const noexcept;

    void setPxMode(bool enabled);
    [[nodiscard]] bool pxMode() const noexcept;
    void setAngle(qreal angle);
    [[nodiscard]] qreal angle() const noexcept;

    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;
    [[nodiscard]] qreal pixelPadding() const;
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    [[nodiscard]] static QPainterPath makeArrowPath(const ArrowItemOptions& options);
    void rebuildPath();

    ArrowItemOptions options_;
    QPainterPath path_;
};

inline ArrowItem::ArrowItem(QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
{
    setStyle(options_);
}

inline ArrowItem::ArrowItem(const ArrowItemOptions& options, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
{
    setStyle(options);
}

inline ArrowItem::ArrowItem(const QPointF& pos, QGraphicsItem* parent)
    : ArrowItem(parent)
{
    setPos(pos);
}

inline ArrowItem::ArrowItem(const QPointF& pos, const ArrowItemOptions& options, QGraphicsItem* parent)
    : ArrowItem(options, parent)
{
    setPos(pos);
}

inline void ArrowItem::setStyle(const ArrowItemOptions& options)
{
    prepareGeometryChange();
    options_ = options;
    rebuildPath();
    QGraphicsPathItem::setPen(options_.pen);
    QGraphicsPathItem::setBrush(options_.brush);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, options_.pxMode);
    update();
}

inline ArrowItemOptions ArrowItem::style() const
{
    return options_;
}

inline const ArrowItemOptions& ArrowItem::options() const noexcept
{
    return options_;
}

inline void ArrowItem::setPxMode(bool enabled)
{
    ArrowItemOptions updated = options_;
    updated.pxMode = enabled;
    setStyle(updated);
}

inline bool ArrowItem::pxMode() const noexcept
{
    return options_.pxMode;
}

inline void ArrowItem::setAngle(qreal angle)
{
    ArrowItemOptions updated = options_;
    updated.angle = angle;
    setStyle(updated);
}

inline qreal ArrowItem::angle() const noexcept
{
    return options_.angle;
}

inline std::pair<qreal, qreal> ArrowItem::dataBounds(int axis) const
{
    if (options_.pxMode) {
        return {0.0, 0.0};
    }

    qreal penWidth = 0.0;
    const QPen currentPen = pen();
    if (!currentPen.isCosmetic()) {
        penWidth = currentPen.widthF() * detail_arrowitem::penPaddingFactor;
    }

    const QRectF bounds = boundingRect();
    if (axis == 0) {
        return {bounds.left() - penWidth, bounds.right() + penWidth};
    }
    return {bounds.top() - penWidth, bounds.bottom() + penWidth};
}

inline qreal ArrowItem::pixelPadding() const
{
    qreal pad = 0.0;
    if (options_.pxMode) {
        const QRectF bounds = boundingRect();
        pad += std::hypot(bounds.width(), bounds.height());
    }

    const QPen currentPen = pen();
    if (currentPen.isCosmetic()) {
        pad += std::max<qreal>(1.0, currentPen.widthF()) * detail_arrowitem::penPaddingFactor;
    }
    return pad;
}

inline QPainterPath ArrowItem::shape() const
{
    return path_;
}

inline void ArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (painter == nullptr) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    QGraphicsPathItem::paint(painter, option, widget);
}

inline QPainterPath ArrowItem::makeArrowPath(const ArrowItemOptions& options)
{
    const qreal headWidth = options.headWidth.value_or(
        options.headLen * std::tan(detail_arrowitem::degreesToRadians(options.tipAngle * 0.5)));

    QPainterPath path;
    path.moveTo(0.0, 0.0);
    path.lineTo(options.headLen, -headWidth);
    if (!options.tailLen.has_value()) {
        const qreal innerY = options.headLen - headWidth * std::tan(detail_arrowitem::degreesToRadians(options.baseAngle));
        path.lineTo(innerY, 0.0);
    } else {
        const qreal halfTailWidth = options.tailWidth * 0.5;
        const qreal innerY = options.headLen
            - (headWidth - halfTailWidth) * std::tan(detail_arrowitem::degreesToRadians(options.baseAngle));
        path.lineTo(innerY, -halfTailWidth);
        path.lineTo(options.headLen + *options.tailLen, -halfTailWidth);
        path.lineTo(options.headLen + *options.tailLen, halfTailWidth);
        path.lineTo(innerY, halfTailWidth);
    }
    path.lineTo(options.headLen, headWidth);
    path.lineTo(0.0, 0.0);
    return path;
}

inline void ArrowItem::rebuildPath()
{
    QTransform transform;
    transform.rotate(options_.angle);
    path_ = transform.map(makeArrowPath(options_));
    QGraphicsPathItem::setPath(path_);
}

} // namespace cppqtgraph::graphicsItems
