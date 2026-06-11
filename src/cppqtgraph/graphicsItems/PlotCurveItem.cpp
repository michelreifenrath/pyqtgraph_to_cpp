// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/PlotCurveItem.hpp"

#include "../../../include/cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph::graphicsItems {

namespace {

struct BoundsRange {
    double minimum;
    double maximum;
};

QPen defaultCurvePen()
{
    QPen pen(QColor(255, 255, 255), 1.0);
    pen.setCosmetic(true);
    return pen;
}

bool isFinitePoint(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

std::optional<BoundsRange> finiteBounds(std::span<const double> values)
{
    std::optional<BoundsRange> bounds;
    for (const double value : values) {
        if (!std::isfinite(value)) {
            continue;
        }
        if (!bounds.has_value()) {
            bounds = BoundsRange{value, value};
            continue;
        }
        if (value < bounds->minimum) {
            bounds->minimum = value;
        }
        if (value > bounds->maximum) {
            bounds->maximum = value;
        }
    }
    return bounds;
}

std::pair<std::vector<double>, std::vector<double>> generateStepModeData(
    PlotCurveItem::StepMode stepMode,
    std::span<const double> x,
    std::span<const double> y)
{
    if (stepMode == PlotCurveItem::StepMode::None) {
        return {std::vector<double>(x.begin(), x.end()), std::vector<double>(y.begin(), y.end())};
    }

    std::vector<double> steppedX;
    std::vector<double> steppedY;
    if (stepMode == PlotCurveItem::StepMode::Center) {
        if (x.size() != y.size() + 1) {
            return {{}, {}};
        }
        steppedX.reserve(y.size() * 2);
        steppedY.reserve(y.size() * 2);
        for (std::size_t index = 0; index < y.size(); ++index) {
            steppedX.push_back(x[index]);
            steppedY.push_back(y[index]);
            steppedX.push_back(x[index + 1]);
            steppedY.push_back(y[index]);
        }
        return {steppedX, steppedY};
    }

    if (x.size() != y.size()) {
        return {{}, {}};
    }

    steppedX.reserve(y.size() * 2);
    steppedY.reserve(y.size() * 2);
    for (std::size_t index = 0; index < y.size(); ++index) {
        if (stepMode == PlotCurveItem::StepMode::Right) {
            steppedX.push_back(x[index]);
            steppedY.push_back(y[index]);
            steppedX.push_back(index + 1 < x.size() ? x[index + 1] : x[index]);
            steppedY.push_back(y[index]);
            continue;
        }

        steppedX.push_back(index == 0 ? x[index] : x[index - 1]);
        steppedY.push_back(y[index]);
        steppedX.push_back(x[index]);
        steppedY.push_back(y[index]);
    }
    return {steppedX, steppedY};
}

qreal penMargin(const QPen& pen)
{
    if (pen.style() == Qt::NoPen) {
        return 0.0;
    }
    const qreal width = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
    return width / 2.0;
}

bool fillIsActive(const QBrush& fillBrush, const std::optional<double>& fillLevel)
{
    return fillBrush.style() != Qt::NoBrush && fillLevel.has_value();
}

QRectF computeBounds(
    std::span<const double> x,
    std::span<const double> y,
    PlotCurveItem::StepMode stepMode,
    const QPen& pen,
    const QBrush& fillBrush,
    const std::optional<double>& fillLevel)
{
    auto [boundsXData, boundsYData] = generateStepModeData(stepMode, x, y);
    const auto xBounds = finiteBounds(boundsXData);
    auto yBounds = finiteBounds(boundsYData);
    if (!xBounds.has_value() || !yBounds.has_value()) {
        return QRectF{};
    }

    if (fillIsActive(fillBrush, fillLevel)) {
        if (fillLevel.value() < yBounds->minimum) {
            yBounds->minimum = fillLevel.value();
        }
        if (fillLevel.value() > yBounds->maximum) {
            yBounds->maximum = fillLevel.value();
        }
    }

    const qreal margin = penMargin(pen);
    const QRectF dataBounds(xBounds->minimum, yBounds->minimum, xBounds->maximum - xBounds->minimum,
        yBounds->maximum - yBounds->minimum);
    return dataBounds.adjusted(-margin, -margin, margin, margin);
}

std::optional<std::pair<QPointF, QPointF>> firstAndLastFinitePoints(std::span<const double> x, std::span<const double> y)
{
    std::optional<QPointF> first;
    std::optional<QPointF> last;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        if (!isFinitePoint(x[index], y[index])) {
            continue;
        }
        const QPointF point(x[index], y[index]);
        if (!first.has_value()) {
            first = point;
        }
        last = point;
    }
    if (!first.has_value() || !last.has_value()) {
        return std::nullopt;
    }
    return std::pair{first.value(), last.value()};
}

QPainterPath fillPathForLevel(
    const QPainterPath& curvePath,
    std::span<const double> x,
    std::span<const double> y,
    double fillLevel)
{
    const auto endpoints = firstAndLastFinitePoints(x, y);
    if (!endpoints.has_value() || curvePath.isEmpty()) {
        return {};
    }

    QPainterPath fillPath = curvePath;
    fillPath.lineTo(QPointF(endpoints->second.x(), fillLevel));
    fillPath.lineTo(QPointF(endpoints->first.x(), fillLevel));
    fillPath.closeSubpath();
    return fillPath;
}

void appendAllPath(QPainterPath& path, std::span<const double> x, std::span<const double> y)
{
    bool hasStart = false;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        if (!isFinitePoint(x[index], y[index])) {
            continue;
        }
        const QPointF point(x[index], y[index]);
        if (!hasStart) {
            path.moveTo(point);
            hasStart = true;
        } else {
            path.lineTo(point);
        }
    }
}

void appendFinitePath(QPainterPath& path, std::span<const double> x, std::span<const double> y)
{
    QPainterPath segment;
    QPointF segmentStart;
    bool hasStart = false;
    bool hasLine = false;

    const auto flushSegment = [&]() {
        if (hasLine) {
            path.addPath(segment);
        }
        segment = QPainterPath();
        hasStart = false;
        hasLine = false;
    };

    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        if (!isFinitePoint(x[index], y[index])) {
            flushSegment();
            continue;
        }
        const QPointF point(x[index], y[index]);
        if (!hasStart) {
            segmentStart = point;
            segment.moveTo(point);
            hasStart = true;
        } else {
            if (!hasLine) {
                segment = QPainterPath(segmentStart);
            }
            segment.lineTo(point);
            hasLine = true;
        }
    }
    flushSegment();
}

void appendPairsPath(QPainterPath& path, std::span<const double> x, std::span<const double> y)
{
    const std::size_t count = std::min(x.size(), y.size());
    for (std::size_t index = 0; index + 1 < count; index += 2) {
        if (!isFinitePoint(x[index], y[index]) || !isFinitePoint(x[index + 1], y[index + 1])) {
            continue;
        }
        path.moveTo(QPointF(x[index], y[index]));
        path.lineTo(QPointF(x[index + 1], y[index + 1]));
    }
}

QPainterPath generatePath(
    std::span<const double> x,
    std::span<const double> y,
    PlotCurveItem::StepMode stepMode,
    PlotCurveItem::ConnectMode connectMode)
{
    auto [pathXStorage, pathYStorage] = generateStepModeData(stepMode, x, y);
    const std::span<const double> pathX(pathXStorage);
    const std::span<const double> pathY(pathYStorage);

    QPainterPath path;
    switch (connectMode) {
    case PlotCurveItem::ConnectMode::All:
        appendAllPath(path, pathX, pathY);
        break;
    case PlotCurveItem::ConnectMode::Finite:
        appendFinitePath(path, pathX, pathY);
        break;
    case PlotCurveItem::ConnectMode::Pairs:
        appendPairsPath(path, pathX, pathY);
        break;
    }
    return path;
}

bool stepDataLengthsAreValid(PlotCurveItem::StepMode stepMode, std::size_t xSize, std::size_t ySize)
{
    if (stepMode == PlotCurveItem::StepMode::Center) {
        return xSize == ySize + 1;
    }
    return xSize == ySize;
}

} // namespace

PlotCurveItem::PlotCurveItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , pen_(defaultCurvePen())
{
}

PlotCurveItem::~PlotCurveItem() = default;

void PlotCurveItem::setData(std::span<const double> y)
{
    std::vector<double> x(y.size());
    std::iota(x.begin(), x.end(), 0.0);
    setData(x, y);
}

void PlotCurveItem::setData(std::span<const double> x, std::span<const double> y)
{
    if (!stepDataLengthsAreValid(stepMode_, x.size(), y.size())) {
        if (stepMode_ == StepMode::Center) {
            throw std::invalid_argument(
                "PlotCurveItem::setData requires x length to be y length + 1 for center step mode");
        }
        throw std::invalid_argument("PlotCurveItem::setData requires x and y to have the same length");
    }

    std::vector<double> newX(x.begin(), x.end());
    std::vector<double> newY(y.begin(), y.end());

    const QRectF newBounds = computeBounds(newX, newY, stepMode_, pen_, fillBrush_, fillLevel_);
    if (newBounds != bounds_) {
        prepareGeometryChange();
    }

    xData_.swap(newX);
    yData_.swap(newY);
    bounds_ = newBounds;
    if (auto* plotItem = dynamic_cast<PlotItem*>(parentItem())) {
        plotItem->updateCurveTransforms();
    }
    update();
}

void PlotCurveItem::setPen(const QPen& pen)
{
    pen_ = pen;
    refreshBounds();
    update();
}

void PlotCurveItem::setPen(std::nullptr_t)
{
    setPen(QPen(Qt::NoPen));
}

QPen PlotCurveItem::pen() const
{
    return pen_;
}

void PlotCurveItem::setConnectMode(ConnectMode mode)
{
    if (connectMode_ == mode) {
        return;
    }
    connectMode_ = mode;
    update();
}

PlotCurveItem::ConnectMode PlotCurveItem::connectMode() const noexcept
{
    return connectMode_;
}

void PlotCurveItem::setStepMode(StepMode mode)
{
    if (stepMode_ == mode) {
        return;
    }
    stepMode_ = mode;
    refreshBounds();
    update();
}

PlotCurveItem::StepMode PlotCurveItem::stepMode() const noexcept
{
    return stepMode_;
}

void PlotCurveItem::setAntialias(bool enabled)
{
    if (antialias_ == enabled) {
        return;
    }
    antialias_ = enabled;
    update();
}

bool PlotCurveItem::antialias() const noexcept
{
    return antialias_;
}

void PlotCurveItem::setFillLevel(double level)
{
    if (fillLevel_.has_value() && fillLevel_.value() == level) {
        return;
    }
    fillLevel_ = level;
    refreshBounds();
    update();
}

void PlotCurveItem::clearFillLevel()
{
    if (!fillLevel_.has_value()) {
        return;
    }
    fillLevel_.reset();
    refreshBounds();
    update();
}

std::optional<double> PlotCurveItem::fillLevel() const noexcept
{
    return fillLevel_;
}

void PlotCurveItem::setFillBrush(const QBrush& brush)
{
    if (fillBrush_ == brush) {
        return;
    }
    fillBrush_ = brush;
    refreshBounds();
    update();
}

void PlotCurveItem::setFillBrush(std::nullptr_t)
{
    setFillBrush(QBrush(Qt::NoBrush));
}

QBrush PlotCurveItem::fillBrush() const
{
    return fillBrush_;
}

std::span<const double> PlotCurveItem::xData() const noexcept
{
    return xData_;
}

std::span<const double> PlotCurveItem::yData() const noexcept
{
    return yData_;
}

QRectF PlotCurveItem::boundingRect() const
{
    return bounds_;
}

void PlotCurveItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (xData_.empty() || yData_.empty()) {
        return;
    }

    const bool hasStroke = pen_.style() != Qt::NoPen;
    const bool hasFill = fillIsActive(fillBrush_, fillLevel_);
    if (!hasStroke && !hasFill) {
        return;
    }

    const QPainterPath path = generatePath(xData_, yData_, stepMode_, connectMode_);
    if (path.isEmpty()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, antialias_);

    if (hasFill) {
        const QPainterPath fillPath = fillPathForLevel(path, xData_, yData_, fillLevel_.value());
        if (!fillPath.isEmpty()) {
            painter->fillPath(fillPath, fillBrush_);
        }
    }

    if (hasStroke) {
        painter->setPen(pen_);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);
    }
}

void PlotCurveItem::refreshBounds()
{
    const QRectF newBounds = computeBounds(xData_, yData_, stepMode_, pen_, fillBrush_, fillLevel_);
    if (newBounds != bounds_) {
        prepareGeometryChange();
        bounds_ = newBounds;
        if (auto* plotItem = dynamic_cast<PlotItem*>(parentItem())) {
            plotItem->updateCurveTransforms();
        }
    }
}

} // namespace cppqtgraph::graphicsItems
