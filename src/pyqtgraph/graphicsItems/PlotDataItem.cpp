// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotDataItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/PlotDataItem.hpp"

#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <numeric>
#include <stdexcept>
#include <vector>

namespace pyqtgraph::graphicsItems {

namespace {

QPen defaultPlotDataPen()
{
    QPen pen(QColor(200, 200, 200), 1.0);
    pen.setCosmetic(true);
    return pen;
}

} // namespace

PlotDataItem::PlotDataItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , curve_(new PlotCurveItem(this))
    , pen_(defaultPlotDataPen())
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
    if (curve_ == nullptr) {
        return;
    }

    if (hasData_ && lineVisible_) {
        curve_->setData(xData_, yData_);
        curve_->show();
        return;
    }

    if (!hasData_) {
        static constexpr std::span<const double> empty;
        curve_->setData(empty);
    }
    curve_->hide();
}

} // namespace pyqtgraph::graphicsItems
