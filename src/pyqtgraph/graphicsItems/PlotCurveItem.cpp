// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"

#include <QtCore/QtGlobal>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <cmath>
#include <numeric>
#include <optional>
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

struct BoundsRange {
    double minimum;
    double maximum;
};

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

QRectF computeBounds(std::span<const double> x, std::span<const double> y)
{
    const auto xBounds = finiteBounds(x);
    const auto yBounds = finiteBounds(y);
    if (!xBounds.has_value() || !yBounds.has_value()) {
        return QRectF{};
    }

    return QRectF(xBounds->minimum, yBounds->minimum, xBounds->maximum - xBounds->minimum,
                  yBounds->maximum - yBounds->minimum);
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

    std::vector<double> newX(x.begin(), x.end());
    std::vector<double> newY(y.begin(), y.end());

    const QRectF newBounds = computeBounds(newX, newY);
    if (newBounds != bounds_) {
        prepareGeometryChange();
    }

    xData_.swap(newX);
    yData_.swap(newY);
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
