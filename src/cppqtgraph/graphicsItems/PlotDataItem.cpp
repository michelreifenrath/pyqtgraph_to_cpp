// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotDataItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/PlotDataItem.hpp"

#include "../../../include/cppqtgraph/graphicsItems/ScatterPlotItem.hpp"

#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace cppqtgraph::graphicsItems {

namespace {

QString defaultPlotDataSymbol()
{
    return QStringLiteral("o");
}

double mapLogAxisValue(double value)
{
    if (!(value > 0.0) || !std::isfinite(value)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::log10(value);
}

QPen defaultPlotDataPen()
{
    QPen pen(QColor(200, 200, 200), 1.0);
    pen.setCosmetic(true);
    return pen;
}

QPen defaultPlotDataSymbolPen()
{
    QPen pen(QColor(200, 200, 200), 1.0);
    pen.setCosmetic(true);
    return pen;
}

QPen normalizeSymbolPen(const QPen& pen)
{
    if (pen.style() == Qt::NoPen) {
        return pen;
    }
    QPen normalized(pen);
    normalized.setCosmetic(true);
    return normalized;
}

QBrush defaultPlotDataSymbolBrush()
{
    return QBrush(QColor(50, 50, 150));
}

} // namespace

PlotDataItem::PlotDataItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , curve_(new PlotCurveItem(this))
    , scatter_(new ScatterPlotItem(this))
    , pen_(defaultPlotDataPen())
    , symbolPen_(defaultPlotDataSymbolPen())
    , symbolBrush_(defaultPlotDataSymbolBrush())
{
    setFlag(QGraphicsItem::ItemHasNoContents);
    updateItems();
}

PlotDataItem::PlotDataItem(std::span<const double> y, QGraphicsItem* parent)
    : PlotDataItem(parent)
{
    setData(y);
}

PlotDataItem::PlotDataItem(std::span<const double> x, std::span<const double> y, QGraphicsItem* parent)
    : PlotDataItem(parent)
{
    setData(x, y);
}

PlotDataItem::~PlotDataItem() = default;

void PlotDataItem::setData()
{
    clear();
}

void PlotDataItem::setData(std::span<const double> y)
{
    if (y.empty()) {
        clear();
        return;
    }

    std::vector<double> x(y.size());
    std::iota(x.begin(), x.end(), 0.0);
    setData(x, y);
}

void PlotDataItem::setData(std::span<const double> x, std::span<const double> y)
{
    if (x.empty() || y.empty()) {
        clear();
        return;
    }

    if (x.size() != y.size()) {
        throw std::invalid_argument("PlotDataItem::setData requires x and y to have the same length");
    }

    std::vector<double> newX(x.begin(), x.end());
    std::vector<double> newY(y.begin(), y.end());

    xData_.swap(newX);
    yData_.swap(newY);
    hasData_ = true;
    updateItems();
}

void PlotDataItem::clear()
{
    xData_.clear();
    yData_.clear();
    hasData_ = false;
    updateItems();
}

bool PlotDataItem::hasData() const noexcept
{
    return hasData_;
}

std::span<const double> PlotDataItem::xData() const noexcept
{
    return xData_;
}

std::span<const double> PlotDataItem::yData() const noexcept
{
    return yData_;
}

std::pair<std::span<const double>, std::span<const double>> PlotDataItem::getData() const noexcept
{
    return {xData(), yData()};
}

PlotCurveItem* PlotDataItem::curve() noexcept
{
    return curve_;
}

const PlotCurveItem* PlotDataItem::curve() const noexcept
{
    return curve_;
}

ScatterPlotItem* PlotDataItem::scatter() noexcept
{
    return scatter_;
}

const ScatterPlotItem* PlotDataItem::scatter() const noexcept
{
    return scatter_;
}

void PlotDataItem::setSymbol(const QString& symbol)
{
    scatter_->setSymbol(symbol);
    symbol_ = symbol;
    symbolsVisible_ = true;
    updateItems();
}

void PlotDataItem::setSymbol(std::nullptr_t)
{
    symbol_.clear();
    symbolsVisible_ = false;
    updateItems();
}

QString PlotDataItem::symbol() const
{
    return symbol_;
}

bool PlotDataItem::symbolsVisible() const noexcept
{
    return symbolsVisible_;
}

void PlotDataItem::setSymbolSize(qreal size)
{
    symbolSize_ = size;
    if (symbol_.isEmpty()) {
        symbol_ = defaultPlotDataSymbol();
        symbolsVisible_ = true;
    }
    updateItems();
}

qreal PlotDataItem::symbolSize() const noexcept
{
    return symbolSize_;
}

void PlotDataItem::setSymbolPen(const QPen& pen)
{
    symbolPen_ = normalizeSymbolPen(pen);
    if (symbol_.isEmpty()) {
        symbol_ = defaultPlotDataSymbol();
        symbolsVisible_ = true;
    }
    updateItems();
}

void PlotDataItem::setSymbolPen(std::nullptr_t)
{
    symbolPen_ = QPen(Qt::NoPen);
    if (symbol_.isEmpty()) {
        symbol_ = defaultPlotDataSymbol();
        symbolsVisible_ = true;
    }
    updateItems();
}

QPen PlotDataItem::symbolPen() const
{
    return symbolPen_;
}

void PlotDataItem::setSymbolBrush(const QBrush& brush)
{
    symbolBrush_ = brush;
    if (symbol_.isEmpty()) {
        symbol_ = defaultPlotDataSymbol();
        symbolsVisible_ = true;
    }
    updateItems();
}

void PlotDataItem::setSymbolBrush(std::nullptr_t)
{
    symbolBrush_ = QBrush(Qt::NoBrush);
    if (symbol_.isEmpty()) {
        symbol_ = defaultPlotDataSymbol();
        symbolsVisible_ = true;
    }
    updateItems();
}

QBrush PlotDataItem::symbolBrush() const
{
    return symbolBrush_;
}

void PlotDataItem::setPen(const QPen& pen)
{
    pen_ = pen;
    lineVisible_ = pen.style() != Qt::NoPen;
    updateItems();
}

void PlotDataItem::setPen(std::nullptr_t)
{
    pen_ = QPen(Qt::NoPen);
    lineVisible_ = false;
    updateItems();
}

QPen PlotDataItem::pen() const
{
    return pen_;
}

bool PlotDataItem::lineVisible() const noexcept
{
    return lineVisible_;
}

void PlotDataItem::setLogMode(bool xEnabled, bool yEnabled)
{
    if (logMode_[0] == xEnabled && logMode_[1] == yEnabled) {
        return;
    }
    logMode_ = {xEnabled, yEnabled};
    updateItems();
}

std::array<bool, 2> PlotDataItem::logMode() const noexcept
{
    return logMode_;
}

void PlotDataItem::updateMappedData()
{
    if (!hasData_) {
        displayX_.clear();
        displayY_.clear();
        return;
    }

    displayX_.assign(xData_.begin(), xData_.end());
    displayY_.assign(yData_.begin(), yData_.end());
    if (logMode_[0]) {
        for (double& value : displayX_) {
            value = mapLogAxisValue(value);
        }
    }
    if (logMode_[1]) {
        for (double& value : displayY_) {
            value = mapLogAxisValue(value);
        }
    }
}

std::optional<QRectF> PlotDataItem::autoRangeBoundsRect() const
{
    if (!hasData_ || (!lineVisible_ && !symbolsVisible_)) {
        return std::nullopt;
    }

    std::optional<QRectF> bounds;
    if (curve_ != nullptr && lineVisible_) {
        const QRectF curveBounds = curve_->boundingRect();
        if (!curveBounds.isNull()) {
            bounds = curveBounds;
        }
    }
    if (scatter_ != nullptr && symbolsVisible_) {
        if (const std::optional<QRectF> scatterBounds = scatter_->autoRangeBoundsRect(); scatterBounds.has_value()) {
            bounds = bounds.has_value() ? bounds->united(*scatterBounds) : scatterBounds;
        }
    }
    if (bounds.has_value()) {
        return bounds;
    }

    bool havePoint = false;
    double xMin = 0.0;
    double xMax = 0.0;
    double yMin = 0.0;
    double yMax = 0.0;

    const std::size_t count = std::min(xData_.size(), yData_.size());
    for (std::size_t index = 0; index < count; ++index) {
        double x = xData_[index];
        double y = yData_[index];
        if (logMode_[0]) {
            x = mapLogAxisValue(x);
        }
        if (logMode_[1]) {
            y = mapLogAxisValue(y);
        }
        if (!std::isfinite(x) || !std::isfinite(y)) {
            continue;
        }
        if (!havePoint) {
            xMin = xMax = x;
            yMin = yMax = y;
            havePoint = true;
            continue;
        }
        xMin = std::min(xMin, x);
        xMax = std::max(xMax, x);
        yMin = std::min(yMin, y);
        yMax = std::max(yMax, y);
    }

    if (!havePoint) {
        return std::nullopt;
    }

    constexpr double kMinimumBoundsSpan = 1.0e-12;
    const double width = std::max(xMax - xMin, kMinimumBoundsSpan);
    const double height = std::max(yMax - yMin, kMinimumBoundsSpan);
    const double xCenter = (xMin + xMax) * 0.5;
    const double yCenter = (yMin + yMax) * 0.5;
    return QRectF(xCenter - width * 0.5, yCenter - height * 0.5, width, height);
}

QRectF PlotDataItem::boundingRect() const
{
    return QRectF{};
}

void PlotDataItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void PlotDataItem::updateItems()
{
    static constexpr std::span<const double> empty;

    updateMappedData();
    const std::span<const double> displayX = displayX_;
    const std::span<const double> displayY = displayY_;

    if (curve_ != nullptr) {
        curve_->setPen(pen_);
        if (hasData_ && lineVisible_) {
            curve_->setData(displayX, displayY);
            curve_->show();
        } else {
            if (!hasData_) {
                curve_->setData(empty);
            }
            curve_->hide();
        }
    }

    if (scatter_ == nullptr) {
        return;
    }

    if (hasData_ && symbolsVisible_) {
        scatter_->setSymbol(symbol_);
        scatter_->setSize(symbolSize_);
        scatter_->setPen(symbolPen_);
        scatter_->setBrush(symbolBrush_);
        scatter_->setData(displayX, displayY);
        scatter_->show();
        return;
    }

    if (!hasData_) {
        scatter_->setData(empty);
    }
    scatter_->hide();
}

} // namespace cppqtgraph::graphicsItems
