// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

#include "../../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"

#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <optional>

namespace pyqtgraph::graphicsItems {

namespace {

struct BoundsRange {
    double minimum;
    double maximum;
};

struct PlotBounds {
    BoundsRange x;
    BoundsRange y;
};

std::optional<PlotBounds> dataBounds(const QList<QGraphicsItem*>& items)
{
    std::optional<PlotBounds> bounds;
    for (QGraphicsItem* item : items) {
        const auto* curve = dynamic_cast<const PlotCurveItem*>(item);
        if (curve == nullptr) {
            continue;
        }
        const std::span<const double> xData = curve->xData();
        const std::span<const double> yData = curve->yData();
        const std::size_t count = std::min(xData.size(), yData.size());
        for (std::size_t index = 0; index < count; ++index) {
            const double x = xData[index];
            const double y = yData[index];
            if (!std::isfinite(x) || !std::isfinite(y)) {
                continue;
            }
            if (!bounds.has_value()) {
                bounds = PlotBounds{BoundsRange{x, x}, BoundsRange{y, y}};
                continue;
            }
            bounds->x.minimum = std::min(bounds->x.minimum, x);
            bounds->x.maximum = std::max(bounds->x.maximum, x);
            bounds->y.minimum = std::min(bounds->y.minimum, y);
            bounds->y.maximum = std::max(bounds->y.maximum, y);
        }
    }

    if (!bounds.has_value()) {
        return std::nullopt;
    }
    if (bounds->x.minimum == bounds->x.maximum) {
        bounds->x.minimum -= 0.5;
        bounds->x.maximum += 0.5;
    }
    if (bounds->y.minimum == bounds->y.maximum) {
        bounds->y.minimum -= 0.5;
        bounds->y.maximum += 0.5;
    }
    return bounds;
}

qreal horizontalScale(const QRectF& bounds)
{
    return bounds.width() / 800.0;
}

qreal verticalScale(const QRectF& bounds)
{
    return bounds.height() / 600.0;
}

qreal axisLeft(const QRectF& bounds)
{
    return bounds.left() + (35.0 * horizontalScale(bounds));
}

qreal axisBottom(const QRectF& bounds)
{
    return bounds.top() + (580.0 * verticalScale(bounds));
}

QRectF plotRect(const QRectF& bounds)
{
    const qreal scaleX = horizontalScale(bounds);
    const qreal scaleY = verticalScale(bounds);
    return QRectF(bounds.left() + (62.0 * scaleX), bounds.top() + (24.0 * scaleY),
        std::max<qreal>(1.0, 710.0 * scaleX), std::max<qreal>(1.0, 532.0 * scaleY));
}

QPointF mapPoint(double x, double y, const PlotBounds& data, const QRectF& target)
{
    const double xRatio = (x - data.x.minimum) / (data.x.maximum - data.x.minimum);
    const double yRatio = (y - data.y.minimum) / (data.y.maximum - data.y.minimum);
    return QPointF(target.left() + (xRatio * target.width()), target.bottom() - (yRatio * target.height()));
}

QString tickLabel(double value)
{
    if (std::abs(value) < 1.0e-9) {
        return QStringLiteral("0");
    }
    return QString::number(value, 'g', 3);
}

void drawTicks(QPainter& painter, const QRectF& itemBounds, const PlotBounds& data)
{
    const QRectF target = plotRect(itemBounds);
    const qreal leftAxis = axisLeft(itemBounds);
    const qreal bottomAxis = axisBottom(itemBounds);

    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    painter.setPen(QPen(QColor(150, 150, 150), 1));

    for (int tick = 0; tick <= 50; ++tick) {
        const double value = tick * 2.0;
        const double x = mapPoint(value, data.y.minimum, data, target).x();
        const double length = tick % 10 == 0 ? 7.0 : 4.0;
        painter.drawLine(QPointF(x, bottomAxis), QPointF(x, bottomAxis + length));
        if (tick % 10 == 0) {
            painter.drawText(QRectF(x - 22.0, bottomAxis + 8.0, 44.0, 18.0), Qt::AlignCenter, tickLabel(value));
        }
    }

    const double firstYTick = std::ceil(data.y.minimum * 10.0) / 10.0;
    const double lastYTick = std::floor(data.y.maximum * 10.0) / 10.0;
    const int firstStep = static_cast<int>(std::round(firstYTick * 10.0));
    const int lastStep = static_cast<int>(std::round(lastYTick * 10.0));
    for (int step = firstStep; step <= lastStep; ++step) {
        const double value = step / 10.0;
        const double y = mapPoint(data.x.minimum, value, data, target).y();
        const bool major = step % 5 == 0;
        painter.drawLine(QPointF(leftAxis - (major ? 7.0 : 4.0), y), QPointF(leftAxis, y));
        if (major) {
            painter.drawText(QRectF(itemBounds.left() + 1.0, y - 9.0, leftAxis - 8.0, 18.0),
                Qt::AlignRight | Qt::AlignVCenter, tickLabel(value));
        }
    }
}

void updateCurveTransforms(const QList<QGraphicsItem*>& items, const QRectF& itemBounds)
{
    const std::optional<PlotBounds> bounds = dataBounds(items);
    if (!bounds.has_value()) {
        return;
    }

    const QRectF target = plotRect(itemBounds);
    const qreal scaleX = target.width() / (bounds->x.maximum - bounds->x.minimum);
    const qreal scaleY = target.height() / (bounds->y.maximum - bounds->y.minimum);
    const qreal dx = target.left() - (bounds->x.minimum * scaleX);
    const qreal dy = target.bottom() + (bounds->y.minimum * scaleY);
    const QTransform transform(scaleX, 0.0, 0.0, -scaleY, dx, dy);

    for (QGraphicsItem* item : items) {
        auto* curve = dynamic_cast<PlotCurveItem*>(item);
        if (curve != nullptr) {
            curve->setTransform(transform, false);
        }
    }
}

} // namespace

PlotItem::PlotItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
{
}

PlotItem::~PlotItem() = default;

void PlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QRectF itemBounds = boundingRect();
    painter->fillRect(itemBounds, Qt::black);

    const QList<QGraphicsItem*> children = childItems();
    updateCurveTransforms(children, itemBounds);
    const std::optional<PlotBounds> bounds = dataBounds(children);
    if (!bounds.has_value()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setPen(QPen(QColor(150, 150, 150), 1));
    painter->drawLine(QPointF(axisLeft(itemBounds), itemBounds.top()), QPointF(axisLeft(itemBounds), axisBottom(itemBounds)));
    painter->drawLine(QPointF(axisLeft(itemBounds), axisBottom(itemBounds)), QPointF(itemBounds.right(), axisBottom(itemBounds)));
    drawTicks(*painter, itemBounds, *bounds);
}

void PlotItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    updateCurveTransforms(childItems(), boundingRect());
    update();
}

} // namespace pyqtgraph::graphicsItems
