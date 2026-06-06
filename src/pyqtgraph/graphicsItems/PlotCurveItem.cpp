// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"

#include "../../../include/pyqtgraph/functions.hpp"
#include "../../../include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

#include <QtCore/QPointF>
#include <QtCore/QtGlobal>
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
#include <vector>

namespace pyqtgraph::graphicsItems {

namespace {

struct BoundsRange {
    double minimum;
    double maximum;
};

struct ExpandedCurveData {
    std::vector<double> x;
    std::vector<double> y;
};

constexpr qreal defaultCurvePenMargin = 0.5;

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
        bounds->minimum = std::min(bounds->minimum, value);
        bounds->maximum = std::max(bounds->maximum, value);
    }
    return bounds;
}

qreal penMargin(const QPen& pen)
{
    if (pen.style() == Qt::NoPen) {
        return 0.0;
    }
    return std::max(defaultCurvePenMargin, static_cast<qreal>(pen.widthF() / 2.0));
}

QRectF computeBounds(std::span<const double> x, std::span<const double> y, const QPen& pen)
{
    const auto xBounds = finiteBounds(x);
    const auto yBounds = finiteBounds(y);
    if (!xBounds.has_value() || !yBounds.has_value()) {
        return QRectF{};
    }

    const QRectF dataBounds(xBounds->minimum, yBounds->minimum, xBounds->maximum - xBounds->minimum,
        yBounds->maximum - yBounds->minimum);
    const qreal margin = penMargin(pen);
    return dataBounds.adjusted(-margin, -margin, margin, margin);
}

bool dataShapeIsValid(std::size_t xSize, std::size_t ySize, PlotCurveItem::StepMode stepMode)
{
    if (stepMode == PlotCurveItem::StepMode::Center) {
        return xSize == ySize + 1;
    }
    return xSize == ySize;
}

bool isFinitePoint(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

ExpandedCurveData expandStepModeData(PlotCurveItem::StepMode stepMode, std::span<const double> x,
    std::span<const double> y)
{
    if (stepMode == PlotCurveItem::StepMode::None) {
        return ExpandedCurveData{std::vector<double>(x.begin(), x.end()), std::vector<double>(y.begin(), y.end())};
    }
    if (!dataShapeIsValid(x.size(), y.size(), stepMode) || y.empty()) {
        return ExpandedCurveData{};
    }

    const std::size_t xRows = stepMode == PlotCurveItem::StepMode::Center ? x.size() : x.size() + 1;
    std::vector<double> repeatedX(xRows * 2);
    if (stepMode == PlotCurveItem::StepMode::Right) {
        for (std::size_t index = 0; index < x.size(); ++index) {
            repeatedX[index * 2] = x[index];
            repeatedX[index * 2 + 1] = x[index];
        }
        repeatedX[repeatedX.size() - 2] = repeatedX[repeatedX.size() - 4];
        repeatedX[repeatedX.size() - 1] = repeatedX[repeatedX.size() - 3];
    } else if (stepMode == PlotCurveItem::StepMode::Left) {
        for (std::size_t index = 1; index < xRows; ++index) {
            repeatedX[index * 2] = x[index - 1];
            repeatedX[index * 2 + 1] = x[index - 1];
        }
        repeatedX[0] = repeatedX[2];
        repeatedX[1] = repeatedX[3];
    } else {
        for (std::size_t index = 0; index < x.size(); ++index) {
            repeatedX[index * 2] = x[index];
            repeatedX[index * 2 + 1] = x[index];
        }
    }

    ExpandedCurveData expanded;
    expanded.x.assign(repeatedX.begin() + 1, repeatedX.end() - 1);
    expanded.y.reserve(y.size() * 2);
    for (const double value : y) {
        expanded.y.push_back(value);
        expanded.y.push_back(value);
    }
    return expanded;
}

QPainterPath buildCurvePath(std::span<const double> x, std::span<const double> y,
    PlotCurveItem::ConnectMode connectMode, PlotCurveItem::StepMode stepMode)
{
    const ExpandedCurveData data = expandStepModeData(stepMode, x, y);
    QPainterPath path;
    if (data.x.empty() || data.x.size() != data.y.size()) {
        return path;
    }

    if (connectMode == PlotCurveItem::ConnectMode::Pairs) {
        for (std::size_t index = 0; index + 1 < data.x.size(); index += 2) {
            if (!isFinitePoint(data.x[index], data.y[index]) || !isFinitePoint(data.x[index + 1], data.y[index + 1])) {
                continue;
            }
            path.moveTo(QPointF(data.x[index], data.y[index]));
            path.lineTo(QPointF(data.x[index + 1], data.y[index + 1]));
        }
        return path;
    }

    bool hasPoint = false;
    for (std::size_t index = 0; index < data.x.size(); ++index) {
        if (!isFinitePoint(data.x[index], data.y[index])) {
            if (connectMode == PlotCurveItem::ConnectMode::Finite) {
                hasPoint = false;
            }
            continue;
        }
        const QPointF point(data.x[index], data.y[index]);
        if (!hasPoint) {
            path.moveTo(point);
            hasPoint = true;
        } else {
            path.lineTo(point);
        }
    }
    return path;
}

} // namespace

PlotCurveItem::PlotCurveItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , pen_(pyqtgraph::mkPen('w'))
{
}

PlotCurveItem::~PlotCurveItem() = default;

void PlotCurveItem::refreshDataBoundsCache()
{
    dataBoundsYData_.clear();
    if (stepMode_ != StepMode::Center || !dataShapeIsValid(xData_.size(), yData_.size(), stepMode_) || yData_.empty()) {
        return;
    }

    // PlotItem currently derives auto-range by zipping xData() and yData().
    // Center step mode has one more x bin edge than y sample, so expose the
    // final edge to that zip by duplicating the nearest y value for bounds only.
    dataBoundsYData_.reserve(xData_.size());
    for (std::size_t index = 0; index < xData_.size(); ++index) {
        dataBoundsYData_.push_back(yData_[std::min(index, yData_.size() - 1)]);
    }
}

void PlotCurveItem::setData(std::span<const double> y)
{
    std::vector<double> x(y.size());
    std::iota(x.begin(), x.end(), 0.0);
    setData(x, y);
}

void PlotCurveItem::setData(std::span<const double> x, std::span<const double> y)
{
    if (!dataShapeIsValid(x.size(), y.size(), stepMode_)) {
        if (stepMode_ == StepMode::Center) {
            throw std::invalid_argument("PlotCurveItem::setData requires len(x) == len(y) + 1 for center step mode");
        }
        throw std::invalid_argument("PlotCurveItem::setData requires x and y to have the same length");
    }

    std::vector<double> newX(x.begin(), x.end());
    std::vector<double> newY(y.begin(), y.end());

    const QRectF newBounds = computeBounds(newX, newY, pen_);
    if (newBounds != bounds_) {
        prepareGeometryChange();
    }

    xData_.swap(newX);
    yData_.swap(newY);
    refreshDataBoundsCache();
    bounds_ = newBounds;
    if (auto* plotItem = dynamic_cast<PlotItem*>(parentItem())) {
        plotItem->updateCurveTransforms();
    }
    update();
}

void PlotCurveItem::setPen(const QPen& pen)
{
    if (pen_ == pen) {
        return;
    }
    const QRectF newBounds = computeBounds(xData_, yData_, pen);
    if (newBounds != bounds_) {
        prepareGeometryChange();
    }
    pen_ = pen;
    bounds_ = newBounds;
    if (auto* plotItem = dynamic_cast<PlotItem*>(parentItem())) {
        plotItem->updateCurveTransforms();
    }
    update();
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
    if (!xData_.empty() && !dataShapeIsValid(xData_.size(), yData_.size(), mode)) {
        throw std::invalid_argument("PlotCurveItem::setStepMode is incompatible with the current x/y data lengths");
    }
    stepMode_ = mode;
    refreshDataBoundsCache();
    if (auto* plotItem = dynamic_cast<PlotItem*>(parentItem())) {
        plotItem->updateCurveTransforms();
    }
    update();
}

PlotCurveItem::StepMode PlotCurveItem::stepMode() const noexcept
{
    return stepMode_;
}

void PlotCurveItem::setSkipFiniteCheck(bool skipFiniteCheck)
{
    if (skipFiniteCheck_ == skipFiniteCheck) {
        return;
    }
    skipFiniteCheck_ = skipFiniteCheck;
    update();
}

bool PlotCurveItem::skipFiniteCheck() const noexcept
{
    return skipFiniteCheck_;
}

std::span<const double> PlotCurveItem::xData() const noexcept
{
    return xData_;
}

std::span<const double> PlotCurveItem::yData() const noexcept
{
    if (stepMode_ == StepMode::Center && dataBoundsYData_.size() == xData_.size()) {
        return dataBoundsYData_;
    }
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

    if (xData_.empty() || yData_.empty() || pen_.style() == Qt::NoPen || !dataShapeIsValid(xData_.size(), yData_.size(), stepMode_)) {
        return;
    }

    const QPainterPath path = buildCurvePath(xData_, yData_, connectMode_, stepMode_);
    if (path.isEmpty()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(pen_);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
}

} // namespace pyqtgraph::graphicsItems
