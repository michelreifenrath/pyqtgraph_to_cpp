// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

#include "../../../../include/pyqtgraph/graphicsItems/AxisItem.hpp"
#include "../../../../include/pyqtgraph/graphicsItems/LegendItem.hpp"
#include "../../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../../include/pyqtgraph/graphicsItems/PlotDataItem.hpp"

#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtCore/QVariant>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsGridLayout>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pyqtgraph::graphicsItems {

namespace {

struct AxisSpec {
    std::size_t index;
    const char* name;
    const char* orientation;
    int row;
    int column;
    bool visibleByDefault;
};

struct BoundsRange {
    double minimum;
    double maximum;
};

struct PlotBounds {
    BoundsRange x;
    BoundsRange y;
};

constexpr std::array<AxisSpec, 4> axisSpecs{{
    {0, "top", "top", 1, 1, false},
    {1, "bottom", "bottom", 3, 1, true},
    {2, "left", "left", 2, 0, true},
    {3, "right", "right", 2, 2, false},
}};

bool containsItem(const std::vector<QGraphicsItem*>& items, QGraphicsItem* item)
{
    return std::find(items.begin(), items.end(), item) != items.end();
}

void eraseItem(std::vector<QGraphicsItem*>& items, QGraphicsItem* item)
{
    items.erase(std::remove(items.begin(), items.end(), item), items.end());
}

void mergePoint(std::optional<PlotBounds>& bounds, double x, double y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }
    if (!bounds.has_value()) {
        bounds = PlotBounds{BoundsRange{x, x}, BoundsRange{y, y}};
        return;
    }
    bounds->x.minimum = std::min(bounds->x.minimum, x);
    bounds->x.maximum = std::max(bounds->x.maximum, x);
    bounds->y.minimum = std::min(bounds->y.minimum, y);
    bounds->y.maximum = std::max(bounds->y.maximum, y);
}

void mergeData(std::optional<PlotBounds>& bounds, std::span<const double> xData, std::span<const double> yData)
{
    const std::size_t count = std::min(xData.size(), yData.size());
    for (std::size_t index = 0; index < count; ++index) {
        mergePoint(bounds, xData[index], yData[index]);
    }
}

void expandCollapsedBounds(std::optional<PlotBounds>& bounds)
{
    if (!bounds.has_value()) {
        return;
    }
    if (bounds->x.minimum == bounds->x.maximum) {
        bounds->x.minimum -= 0.5;
        bounds->x.maximum += 0.5;
    }
    if (bounds->y.minimum == bounds->y.maximum) {
        bounds->y.minimum -= 0.5;
        bounds->y.maximum += 0.5;
    }
}

std::optional<PlotBounds> dataBounds(const std::vector<QGraphicsItem*>& dataItems,
                                     const std::vector<QGraphicsItem*>& ignoredBoundsItems)
{
    std::optional<PlotBounds> bounds;
    for (QGraphicsItem* item : dataItems) {
        if (item == nullptr || !item->isVisible() || containsItem(ignoredBoundsItems, item)) {
            continue;
        }
        if (const auto* curve = dynamic_cast<const PlotCurveItem*>(item); curve != nullptr) {
            mergeData(bounds, curve->xData(), curve->yData());
            continue;
        }
        if (const auto* data = dynamic_cast<const PlotDataItem*>(item); data != nullptr && data->hasData()) {
            mergeData(bounds, data->xData(), data->yData());
        }
    }

    expandCollapsedBounds(bounds);
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

QRectF legacyPlotRect(const QRectF& bounds)
{
    const qreal scaleX = horizontalScale(bounds);
    const qreal scaleY = verticalScale(bounds);
    return QRectF(bounds.left() + (62.0 * scaleX), bounds.top() + (24.0 * scaleY),
        std::max<qreal>(1.0, 710.0 * scaleX), std::max<qreal>(1.0, 532.0 * scaleY));
}

QRectF targetRectForBounds(const QRectF& fallbackBounds)
{
    if (fallbackBounds.isValid() && fallbackBounds.width() > 1.0 && fallbackBounds.height() > 1.0) {
        return legacyPlotRect(fallbackBounds);
    }
    return QRectF(0.0, 0.0, 1.0, 1.0);
}

QTransform transformForBounds(const PlotBounds& data, const QRectF& target)
{
    const qreal scaleX = target.width() / (data.x.maximum - data.x.minimum);
    const qreal scaleY = target.height() / (data.y.maximum - data.y.minimum);
    const qreal dx = target.left() - (data.x.minimum * scaleX);
    const qreal dy = target.bottom() + (data.y.minimum * scaleY);
    return QTransform(scaleX, 0.0, 0.0, -scaleY, dx, dy);
}

} // namespace

PlotItem::PlotItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    initializeLayout();
    initializeAxes();
    connectAxesToViewBox();
}

PlotItem::~PlotItem() = default;

ViewBox* PlotItem::getViewBox() noexcept
{
    return viewBox_;
}

const ViewBox* PlotItem::getViewBox() const noexcept
{
    return viewBox_;
}

AxisItem* PlotItem::getAxis(const QString& name) const
{
    const std::optional<AxisIndex> index = axisIndex(name);
    if (!index.has_value()) {
        throw std::out_of_range("PlotItem::getAxis axis must be one of left, bottom, top, or right");
    }
    return axes_[static_cast<std::size_t>(*index)];
}

void PlotItem::showAxis(const QString& axis, bool show)
{
    AxisItem* item = getAxis(axis);
    if (show) {
        item->show();
    } else {
        item->hide();
    }
}

void PlotItem::hideAxis(const QString& axis)
{
    showAxis(axis, false);
}

void PlotItem::setLabel(const QString& axis, const QString& text, const QString& units, const QString& unitPrefix)
{
    AxisItem* item = getAxis(axis);
    item->setLabel(text, units, unitPrefix);
    showAxis(axis, true);
}

void PlotItem::addItem(QGraphicsItem* item, bool ignoreBounds, const QString& name)
{
    if (item == nullptr) {
        throw std::invalid_argument("PlotItem::addItem requires a non-null item");
    }
    if (containsItem(items_, item)) {
        return;
    }

    items_.push_back(item);
    if (ignoreBounds && !containsItem(ignoredBoundsItems_, item)) {
        ignoredBoundsItems_.push_back(item);
    }
    registerDataItem(item);

    const bool oldRouting = routingDirectChild_;
    routingDirectChild_ = true;
    item->setParentItem(this);
    routingDirectChild_ = oldRouting;

    if (legend_ != nullptr && !name.isEmpty()) {
        legend_->addItem(item, name);
    }
    updateCurveTransforms();
}

void PlotItem::removeItem(QGraphicsItem* item)
{
    if (item == nullptr || !containsItem(items_, item)) {
        return;
    }

    if (legend_ != nullptr) {
        legend_->removeItem(item);
    }
    resetDataItemTransform(item);
    eraseItem(items_, item);
    eraseItem(ignoredBoundsItems_, item);
    unregisterDataItem(item);

    const bool oldRouting = routingDirectChild_;
    routingDirectChild_ = true;
    if (item->parentItem() == this) {
        item->setParentItem(nullptr);
    }
    routingDirectChild_ = oldRouting;
    updateCurveTransforms();
}

void PlotItem::clear()
{
    const std::vector<QGraphicsItem*> currentItems = items_;
    for (QGraphicsItem* item : currentItems) {
        removeItem(item);
    }
}

std::vector<QGraphicsItem*> PlotItem::items() const
{
    return items_;
}

std::vector<QGraphicsItem*> PlotItem::listDataItems() const
{
    return dataItems_;
}

LegendItem* PlotItem::addLegend(const QPointF& offset)
{
    if (legend_ == nullptr) {
        legend_ = new LegendItem(QSizeF{}, offset);
        legend_->setPen(QPen(QColor(180, 180, 180), 1.0));
        legend_->setBrush(QBrush(QColor(0, 0, 0, 180)));
        legend_->setParentItem(viewBox_);
        legend_->setZValue(viewBox_->zValue() + 10.0);
        legend_->setOffset(offset);
    }
    return legend_;
}

LegendItem* PlotItem::legend() const noexcept
{
    return legend_;
}

void PlotItem::setXRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    viewBox_->setXRange(minimum, maximum, padding, update);
    refreshDataItemTransforms(false);
}

void PlotItem::setYRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    viewBox_->setYRange(minimum, maximum, padding, update);
    refreshDataItemTransforms(false);
}

void PlotItem::setRange(const QRectF& rect, qreal padding, bool update, bool disableAutoRange)
{
    viewBox_->setRange(rect, padding, update, disableAutoRange);
    refreshDataItemTransforms(false);
}

void PlotItem::autoRange(std::optional<qreal> padding)
{
    refreshDataItemTransforms(true, true, padding);
}

QRectF PlotItem::viewRect() const
{
    return viewBox_->viewRect();
}

ViewBox::Range2D PlotItem::viewRange() const
{
    return viewBox_->viewRange();
}

QVariant PlotItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsWidget::itemChange(change, value);
    if (routingDirectChild_) {
        return result;
    }

    switch (change) {
    case QGraphicsItem::ItemChildAddedChange:
        if (auto* item = value.value<QGraphicsItem*>(); item != nullptr && !isManagedInternalItem(item)
            && !isPlotDataCurve(item) && !containsItem(items_, item)
            && (dynamic_cast<PlotCurveItem*>(item) != nullptr || dynamic_cast<PlotDataItem*>(item) != nullptr)) {
            items_.push_back(item);
            registerDataItem(item);
        }
        registerDirectDataChildren();
        updateCurveTransforms();
        break;
    case QGraphicsItem::ItemChildRemovedChange:
        if (auto* item = value.value<QGraphicsItem*>(); item != nullptr && !isManagedInternalItem(item)) {
            if (legend_ != nullptr) {
                legend_->removeItem(item);
            }
            resetDataItemTransform(item);
            eraseItem(items_, item);
            eraseItem(ignoredBoundsItems_, item);
            unregisterDataItem(item);
            updateCurveTransforms();
        }
        break;
    default:
        break;
    }

    return result;
}

void PlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->fillRect(boundingRect(), Qt::black);
}

void PlotItem::updateCurveTransforms()
{
    registerDirectDataChildren();
    const auto autoRange = viewBox_ != nullptr ? viewBox_->autoRangeEnabled() : std::array<bool, 2>{{false, false}};
    refreshDataItemTransforms(autoRange[ViewBox::XAxis] || autoRange[ViewBox::YAxis], false);
    update();
}

void PlotItem::initializeLayout()
{
    auto* grid = new QGraphicsGridLayout();
    grid->setContentsMargins(1.0, 1.0, 1.0, 1.0);
    grid->setHorizontalSpacing(0.0);
    grid->setVerticalSpacing(0.0);
    setLayout(grid);

    viewBox_ = new ViewBox(this);
    grid->addItem(viewBox_, 2, 1);

    for (int row = 0; row < 4; ++row) {
        grid->setRowPreferredHeight(row, 0.0);
        grid->setRowMinimumHeight(row, 0.0);
        grid->setRowSpacing(row, 0.0);
        grid->setRowStretchFactor(row, 1);
    }
    for (int column = 0; column < 3; ++column) {
        grid->setColumnPreferredWidth(column, 0.0);
        grid->setColumnMinimumWidth(column, 0.0);
        grid->setColumnSpacing(column, 0.0);
        grid->setColumnStretchFactor(column, 1);
    }
    grid->setRowStretchFactor(2, 100);
    grid->setColumnStretchFactor(1, 100);
}

void PlotItem::initializeAxes()
{
    auto* grid = static_cast<QGraphicsGridLayout*>(layout());
    for (const AxisSpec& spec : axisSpecs) {
        auto* axis = new AxisItem(QString::fromLatin1(spec.orientation), this);
        axis->setZValue(0.5);
        axis->setFlag(QGraphicsItem::GraphicsItemFlag::ItemNegativeZStacksBehindParent);
        axes_[spec.index] = axis;
        grid->addItem(axis, spec.row, spec.column);
        showAxis(QString::fromLatin1(spec.name), spec.visibleByDefault);
    }
}

void PlotItem::connectAxesToViewBox()
{
    QObject::connect(viewBox_, &ViewBox::sigXRangeChanged, this, [this](ViewBox*, ViewBox::AxisRange range) {
        axes_[Top]->setRange(range[0], range[1]);
        axes_[Bottom]->setRange(range[0], range[1]);
    });
    QObject::connect(viewBox_, &ViewBox::sigYRangeChanged, this, [this](ViewBox*, ViewBox::AxisRange range) {
        axes_[Left]->setRange(range[0], range[1]);
        axes_[Right]->setRange(range[0], range[1]);
    });
}

std::optional<PlotItem::AxisIndex> PlotItem::axisIndex(const QString& name)
{
    if (name == QStringLiteral("top")) {
        return Top;
    }
    if (name == QStringLiteral("bottom")) {
        return Bottom;
    }
    if (name == QStringLiteral("left")) {
        return Left;
    }
    if (name == QStringLiteral("right")) {
        return Right;
    }
    return std::nullopt;
}

bool PlotItem::isManagedInternalItem(QGraphicsItem* item) const noexcept
{
    if (item == nullptr || item == viewBox_ || item == legend_) {
        return true;
    }
    return std::any_of(axes_.begin(), axes_.end(), [item](AxisItem* axis) {
        return item == axis;
    });
}

bool PlotItem::ignoresBounds(QGraphicsItem* item) const noexcept
{
    return containsItem(ignoredBoundsItems_, item);
}

bool PlotItem::isPlotDataCurve(QGraphicsItem* item) const noexcept
{
    return std::any_of(dataItems_.begin(), dataItems_.end(), [item](QGraphicsItem* dataItem) {
        const auto* data = dynamic_cast<const PlotDataItem*>(dataItem);
        return data != nullptr && data->curve() == item;
    });
}

void PlotItem::registerDirectDataChildren()
{
    const QList<QGraphicsItem*> children = childItems();
    for (QGraphicsItem* child : children) {
        if (isManagedInternalItem(child) || isPlotDataCurve(child) || containsItem(items_, child)) {
            continue;
        }
        if (dynamic_cast<PlotCurveItem*>(child) != nullptr || dynamic_cast<PlotDataItem*>(child) != nullptr) {
            items_.push_back(child);
            registerDataItem(child);
        }
    }
}

void PlotItem::registerDataItem(QGraphicsItem* item)
{
    if (item == nullptr) {
        return;
    }
    if (dynamic_cast<PlotCurveItem*>(item) != nullptr) {
        if (!containsItem(dataItems_, item)) {
            dataItems_.push_back(item);
        }
        return;
    }

    if (auto* data = dynamic_cast<PlotDataItem*>(item); data != nullptr) {
        if (!containsItem(dataItems_, item)) {
            dataItems_.push_back(item);
        }
        if (data->curve() != nullptr && data->curve()->parentItem() != this) {
            const bool oldRouting = routingDirectChild_;
            routingDirectChild_ = true;
            data->curve()->setParentItem(this);
            routingDirectChild_ = oldRouting;
        }
    }
}

void PlotItem::unregisterDataItem(QGraphicsItem* item)
{
    if (auto* data = dynamic_cast<PlotDataItem*>(item); data != nullptr && data->curve() != nullptr) {
        data->curve()->setTransform(QTransform{}, false);
        const bool oldRouting = routingDirectChild_;
        routingDirectChild_ = true;
        if (data->curve()->parentItem() == this) {
            data->curve()->setParentItem(data);
        }
        routingDirectChild_ = oldRouting;
    }
    eraseItem(dataItems_, item);
}

void PlotItem::setDataItemTransform(QGraphicsItem* item, const QTransform& transform)
{
    if (auto* curve = dynamic_cast<PlotCurveItem*>(item); curve != nullptr) {
        curve->setTransform(transform, false);
        return;
    }
    if (auto* data = dynamic_cast<PlotDataItem*>(item); data != nullptr && data->curve() != nullptr) {
        data->curve()->setTransform(transform, false);
    }
}

void PlotItem::resetDataItemTransform(QGraphicsItem* item)
{
    setDataItemTransform(item, QTransform{});
}

void PlotItem::refreshDataItemTransforms(bool applyAutoRange, bool disableAutoRange, std::optional<qreal> padding)
{
    if (viewBox_ == nullptr) {
        return;
    }

    const std::optional<PlotBounds> bounds = dataBounds(dataItems_, ignoredBoundsItems_);
    if (bounds.has_value() && applyAutoRange) {
        const auto autoRange = viewBox_->autoRangeEnabled();
        const std::optional<ViewBox::AxisRange> xRange = autoRange[ViewBox::XAxis] || disableAutoRange
            ? std::optional<ViewBox::AxisRange>{ViewBox::AxisRange{bounds->x.minimum, bounds->x.maximum}}
            : std::nullopt;
        const std::optional<ViewBox::AxisRange> yRange = autoRange[ViewBox::YAxis] || disableAutoRange
            ? std::optional<ViewBox::AxisRange>{ViewBox::AxisRange{bounds->y.minimum, bounds->y.maximum}}
            : std::nullopt;
        if (xRange.has_value() || yRange.has_value()) {
            viewBox_->setRange(xRange, yRange, padding.value_or(0.02), true, disableAutoRange);
        }
    }

    const QRectF visible = viewBox_->viewRect();
    PlotBounds transformBounds{BoundsRange{visible.left(), visible.right()}, BoundsRange{visible.top(), visible.bottom()}};
    if (bounds.has_value() && applyAutoRange) {
        transformBounds = *bounds;
    } else if (!visible.isValid() || visible.width() <= 0.0 || visible.height() <= 0.0) {
        if (!bounds.has_value()) {
            return;
        }
        transformBounds = *bounds;
    }

    const QRectF target = targetRectForBounds(boundingRect());
    const QTransform transform = transformForBounds(transformBounds, target);
    for (QGraphicsItem* item : dataItems_) {
        setDataItemTransform(item, transform);
    }
}

void PlotItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    updateCurveTransforms();
}

} // namespace pyqtgraph::graphicsItems
