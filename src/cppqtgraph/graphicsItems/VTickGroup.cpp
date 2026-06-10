// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/VTickGroup.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/VTickGroup.hpp"

#include "../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace cppqtgraph::graphicsItems {
namespace {

QPen defaultTickPen()
{
    QPen pen(QColor(200, 200, 200));
    pen.setCosmetic(true);
    return pen;
}

bool finite(qreal value)
{
    return std::isfinite(static_cast<double>(value));
}

void validateYRange(const std::array<qreal, 2>& values)
{
    if (!finite(values[0]) || !finite(values[1]) || values[1] < values[0]) {
        throw std::invalid_argument("VTickGroup y range must be finite and increasing");
    }
}

} // namespace

VTickGroup::VTickGroup(QGraphicsItem* parent)
    : VTickGroup({}, {0.0, 1.0}, defaultTickPen(), parent)
{
}

VTickGroup::VTickGroup(std::vector<double> xvals, std::array<qreal, 2> yrange, QPen pen, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , pen_(std::move(pen))
    , xvals_(std::move(xvals))
    , yrange_(yrange)
{
    pen_.setCosmetic(true);
    validateYRange(yrange_);
    rebuildTicks();
}

VTickGroup::~VTickGroup() = default;

QPen VTickGroup::pen() const
{
    return pen_;
}

void VTickGroup::setPen(const QPen& pen)
{
    pen_ = pen;
    pen_.setCosmetic(true);
    update();
}

const std::vector<double>& VTickGroup::xValues() const noexcept
{
    return xvals_;
}

void VTickGroup::setXVals(std::span<const double> values)
{
    xvals_.assign(values.begin(), values.end());
    rebuildTicks();
    update();
}

void VTickGroup::setXVals(std::initializer_list<double> values)
{
    xvals_.assign(values.begin(), values.end());
    rebuildTicks();
    update();
}

std::array<qreal, 2> VTickGroup::yRange() const noexcept
{
    return yrange_;
}

void VTickGroup::setYRange(std::array<qreal, 2> values)
{
    validateYRange(values);
    yrange_ = values;
    update();
}

const QPainterPath& VTickGroup::path() const noexcept
{
    return path_;
}

QRectF VTickGroup::boundingRect() const
{
    if (const ViewBox* vb = viewBox(); vb != nullptr) {
        return vb->viewRect().normalized();
    }
    return path_.boundingRect();
}

void VTickGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr || path_.isEmpty()) {
        return;
    }
    const QRectF view = boundingRect();
    if (!view.isValid() || view.isEmpty()) {
        return;
    }

    const qreal height = view.height();
    const qreal y = view.y() + yrange_[0] * height;
    const qreal tickHeight = (yrange_[1] - yrange_[0]) * height;
    if (tickHeight <= 0.0 || !finite(tickHeight)) {
        return;
    }

    painter->save();
    painter->translate(0.0, y);
    painter->scale(1.0, tickHeight);
    painter->setPen(pen_);
    painter->drawPath(path_);
    painter->restore();
}

ViewBox* VTickGroup::viewBox() const
{
    for (QGraphicsItem* item = parentItem(); item != nullptr; item = item->parentItem()) {
        if (auto* vb = dynamic_cast<ViewBox*>(item)) {
            return vb;
        }
    }
    return nullptr;
}

void VTickGroup::rebuildTicks()
{
    QPainterPath path;
    for (double x : xvals_) {
        if (!std::isfinite(x)) {
            continue;
        }
        path.moveTo(x, 0.0);
        path.lineTo(x, 1.0);
    }
    path_ = path;
}

} // namespace cppqtgraph::graphicsItems
