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
#include <vector>

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

struct AxisTick {
    double value;
    bool major;
};

double niceTickStep(double rawStep)
{
    if (!std::isfinite(rawStep) || rawStep <= 0.0) {
        return 0.0;
    }

    const double magnitude = std::pow(10.0, std::floor(std::log10(rawStep)));
    const double normalized = rawStep / magnitude;
    double niceNormalized = 10.0;
    if (normalized <= 1.0) {
        niceNormalized = 1.0;
    } else if (normalized <= 2.0) {
        niceNormalized = 2.0;
    } else if (normalized <= 5.0) {
        niceNormalized = 5.0;
    }
    return niceNormalized * magnitude;
}

std::vector<AxisTick> axisTicks(const BoundsRange& range, qreal pixelLength)
{
    constexpr double minimumMajorPixelSpacing = 90.0;
    constexpr double minimumMinorPixelSpacing = 12.0;
    constexpr int maximumMajorIntervals = 8;
    constexpr int maximumTickCount = 64;

    if (!std::isfinite(range.minimum) || !std::isfinite(range.maximum)
        || !std::isfinite(static_cast<double>(pixelLength)) || pixelLength <= 0.0
        || range.maximum < range.minimum) {
        return {};
    }

    const double span = range.maximum - range.minimum;
    if (!std::isfinite(span) || span <= 0.0) {
        return {};
    }

    const double rawIntervalCount = std::floor(static_cast<double>(pixelLength) / minimumMajorPixelSpacing);
    int majorIntervals = 1;
    if (std::isfinite(rawIntervalCount)) {
        majorIntervals = static_cast<int>(
            std::clamp(rawIntervalCount, 1.0, static_cast<double>(maximumMajorIntervals)));
    }

    const double majorStep = niceTickStep(span / majorIntervals);
    if (!std::isfinite(majorStep) || majorStep <= 0.0) {
        return {};
    }

    const double majorPixelSpacing = static_cast<double>(pixelLength) * majorStep / span;
    int minorDivisions = 5;
    if (!std::isfinite(majorPixelSpacing) || majorPixelSpacing / minorDivisions < minimumMinorPixelSpacing) {
        minorDivisions = majorPixelSpacing >= minimumMinorPixelSpacing * 2.0 ? 2 : 1;
    }

    const double minorStep = majorStep / minorDivisions;
    if (!std::isfinite(minorStep) || minorStep <= 0.0) {
        return {};
    }

    const double firstTick = std::ceil(range.minimum / minorStep) * minorStep;
    if (!std::isfinite(firstTick)) {
        return {};
    }

    const double lastTick = range.maximum + (std::abs(minorStep) * 1.0e-6);
    const double majorEpsilon = std::abs(majorStep) * 1.0e-6;
    std::vector<AxisTick> ticks;
    ticks.reserve(maximumTickCount);
    bool hasMajorTick = false;
    for (int tickIndex = 0; tickIndex < maximumTickCount; ++tickIndex) {
        const double value = firstTick + (tickIndex * minorStep);
        if (!std::isfinite(value) || value > lastTick) {
            break;
        }
        const double nearestMajor = std::round(value / majorStep) * majorStep;
        const bool major = std::abs(value - nearestMajor) <= majorEpsilon;
        hasMajorTick = hasMajorTick || major;
        ticks.push_back(AxisTick{value, major});
    }

    if (!hasMajorTick && !ticks.empty()) {
        ticks.front().major = true;
        ticks.back().major = true;
    }
    return ticks;
}

void drawTicks(QPainter& painter, const QRectF& itemBounds, const PlotBounds& data)
{
    const QRectF target = plotRect(itemBounds);
    const qreal leftAxis = axisLeft(itemBounds);
    const qreal bottomAxis = axisBottom(itemBounds);

    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    painter.setPen(QPen(QColor(150, 150, 150), 1));

    for (const AxisTick& tick : axisTicks(data.x, target.width())) {
        const double x = mapPoint(tick.value, data.y.minimum, data, target).x();
        const double length = tick.major ? 7.0 : 4.0;
        painter.drawLine(QPointF(x, bottomAxis), QPointF(x, bottomAxis + length));
        if (tick.major) {
            painter.drawText(
                QRectF(x - 22.0, bottomAxis + 8.0, 44.0, 18.0), Qt::AlignCenter, tickLabel(tick.value));
        }
    }

    for (const AxisTick& tick : axisTicks(data.y, target.height())) {
        const double y = mapPoint(data.x.minimum, tick.value, data, target).y();
        painter.drawLine(QPointF(leftAxis - (tick.major ? 7.0 : 4.0), y), QPointF(leftAxis, y));
        if (tick.major) {
            painter.drawText(QRectF(itemBounds.left() + 1.0, y - 9.0, leftAxis - 8.0, 18.0),
                Qt::AlignRight | Qt::AlignVCenter, tickLabel(tick.value));
        }
    }
}

void applyCurveTransforms(const QList<QGraphicsItem*>& items, const QRectF& itemBounds)
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

void PlotItem::updateCurveTransforms()
{
    applyCurveTransforms(childItems(), boundingRect());
    update();
}

void PlotItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    updateCurveTransforms();
}

} // namespace pyqtgraph::graphicsItems
