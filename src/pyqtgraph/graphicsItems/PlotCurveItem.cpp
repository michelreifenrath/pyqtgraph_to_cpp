// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"

#include <QtCore/QtGlobal>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

class QPainter;

namespace pyqtgraph::graphicsItems {

PlotCurveItem::PlotCurveItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
}

PlotCurveItem::~PlotCurveItem() = default;

namespace {

QRectF computeBounds(std::span<const double> x, std::span<const double> y)
{
    if (x.empty()) {
        return QRectF{};
    }

    const auto [minX, maxX] = std::minmax_element(x.begin(), x.end());
    const auto [minY, maxY] = std::minmax_element(y.begin(), y.end());
    return QRectF(*minX, *minY, *maxX - *minX, *maxY - *minY);
}

} // namespace

void PlotCurveItem::setData(std::span<const double> y)
{
    std::vector<double> x(y.size());
    std::iota(x.begin(), x.end(), 0.0);
    setData(x, y);
}

void PlotCurveItem::setData(std::span<const double> x, std::span<const double> y)
{
    if (x.size() != y.size()) {
        throw std::invalid_argument("PlotCurveItem::setData requires x and y to have the same length");
    }

    const QRectF newBounds = computeBounds(x, y);
    if (newBounds != bounds_) {
        prepareGeometryChange();
    }

    xData_.assign(x.begin(), x.end());
    yData_.assign(y.begin(), y.end());
    bounds_ = newBounds;
    update();
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
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

} // namespace pyqtgraph::graphicsItems
