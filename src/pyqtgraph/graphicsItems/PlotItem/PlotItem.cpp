// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

#include "../../../../include/pyqtgraph/graphicsItems/AxisItem.hpp"
#include "../../../../include/pyqtgraph/graphicsItems/LegendItem.hpp"
#include "../../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"

#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtCore/QVariant>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QTextDocument>
#include <QtGui/QTextOption>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsGridLayout>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace pyqtgraph::graphicsItems {
namespace {

QPen defaultPlotPen()
{
    QPen pen(QColor(200, 200, 200), 1.0);
    pen.setCosmetic(true);
    return pen;
}

bool isDataItem(QGraphicsItem* item)
{
    return dynamic_cast<PlotCurveItem*>(item) != nullptr;
}

struct BoundsRange {
    double minimum;
    double maximum;
};

struct PlotBounds {
    BoundsRange x;
    BoundsRange y;
};

std::optional<PlotBounds> directCurveBounds(const QList<QGraphicsItem*>& children)
{
    std::optional<PlotBounds> bounds;
    for (QGraphicsItem* item : children) {
        const auto* curve = dynamic_cast<const PlotCurveItem*>(item);
        if (curve == nullptr) {
            continue;
        }
        const auto xData = curve->xData();
        const auto yData = curve->yData();
        const std::size_t count = std::min(xData.size(), yData.size());
        for (std::size_t index = 0; index < count; ++index) {
            const double x = xData[index];
            const double y = yData[index];
            if (!std::isfinite(x) || !std::isfinite(y)) {
                continue;
            }
            if (!bounds.has_value()) {
                bounds = PlotBounds{BoundsRange{x, x}, BoundsRange{y, y}};
            } else {
                bounds->x.minimum = std::min(bounds->x.minimum, x);
                bounds->x.maximum = std::max(bounds->x.maximum, x);
                bounds->y.minimum = std::min(bounds->y.minimum, y);
                bounds->y.maximum = std::max(bounds->y.maximum, y);
            }
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

    layout_ = new QGraphicsGridLayout();
    layout_->setContentsMargins(1.0, 24.0, 1.0, 1.0);
    layout_->setHorizontalSpacing(0.0);
    layout_->setVerticalSpacing(0.0);
    setLayout(layout_);

    viewBox_ = new ViewBox();
    viewBox_->setParentItem(this);
    layout_->addItem(viewBox_, 1, 1);

    axes_[static_cast<std::size_t>(AxisSlot::Top)] = new AxisItem(QStringLiteral("top"));
    axes_[static_cast<std::size_t>(AxisSlot::Bottom)] = new AxisItem(QStringLiteral("bottom"));
    axes_[static_cast<std::size_t>(AxisSlot::Left)] = new AxisItem(QStringLiteral("left"));
    axes_[static_cast<std::size_t>(AxisSlot::Right)] = new AxisItem(QStringLiteral("right"));

    layout_->addItem(axis(AxisSlot::Top), 0, 1);
    layout_->addItem(axis(AxisSlot::Bottom), 2, 1);
    layout_->addItem(axis(AxisSlot::Left), 1, 0);
    layout_->addItem(axis(AxisSlot::Right), 1, 2);
    layout_->setRowStretchFactor(1, 100);
    layout_->setColumnStretchFactor(1, 100);

    axis(AxisSlot::Left)->show();
    axis(AxisSlot::Bottom)->show();
    axis(AxisSlot::Top)->hide();
    axis(AxisSlot::Right)->hide();

    titleItem_ = new QGraphicsTextItem();
    titleItem_->setParentItem(this);
    titleItem_->setDefaultTextColor(Qt::white);
    titleItem_->setFont(QFont(QStringLiteral("Sans Serif"), 11));
    titleItem_->hide();
    updateTitleGeometry();

    QObject::connect(viewBox_, &ViewBox::sigRangeChanged, this, [this](ViewBox*, ViewBox::Range2D, std::array<bool, 2>) {
        updateAxisRanges();
    });
    updateAxisRanges();
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

AxisItem* PlotItem::getAxis(const QString& name)
{
    return axis(axisSlot(name));
}

const AxisItem* PlotItem::getAxis(const QString& name) const
{
    return axis(axisSlot(name));
}

void PlotItem::addItem(QGraphicsItem* item, bool ignoreBounds, const QString& name)
{
    if (item == nullptr) {
        throw std::invalid_argument("PlotItem::addItem requires a non-null item");
    }
    if (isChromeItem(item)) {
        return;
    }
    if (std::find(items_.begin(), items_.end(), item) != items_.end()) {
        return;
    }

    items_.push_back(item);
    if (isDataItem(item)) {
        dataItems_.push_back(item);
    }
    recordItemName(item, name);

    viewBox_->addItem(item, ignoreBounds);

    const QString effectiveName = itemName(item);
    if (legend_ != nullptr && !effectiveName.isEmpty()) {
        legend_->addItem(item, effectiveName);
    }
    updateAxisRanges();
}

void PlotItem::removeItem(QGraphicsItem* item)
{
    if (item == nullptr) {
        return;
    }
    const auto itemIt = std::find(items_.begin(), items_.end(), item);
    if (itemIt == items_.end()) {
        return;
    }

    items_.erase(itemIt);
    dataItems_.erase(std::remove(dataItems_.begin(), dataItems_.end(), item), dataItems_.end());
    itemNames_.erase(std::remove_if(itemNames_.begin(), itemNames_.end(), [item](const auto& entry) {
                         return entry.first == item;
                     }),
        itemNames_.end());

    if (legend_ != nullptr) {
        legend_->removeItem(item);
    }
    viewBox_->removeItem(item);
    updateAxisRanges();
}

void PlotItem::clear()
{
    const auto items = items_;
    for (QGraphicsItem* item : items) {
        removeItem(item);
    }
    if (legend_ != nullptr) {
        legend_->clear();
    }
}

std::vector<QGraphicsItem*> PlotItem::listDataItems() const
{
    return dataItems_;
}

PlotCurveItem* PlotItem::plot(std::span<const double> y, const QString& name, const QPen& pen)
{
    auto* item = new PlotCurveItem();
    item->setPen(pen == QPen() ? defaultPlotPen() : pen);
    item->setData(y);
    addItem(item, false, name);
    return item;
}

PlotCurveItem* PlotItem::plot(std::span<const double> x, std::span<const double> y, const QString& name, const QPen& pen)
{
    auto* item = new PlotCurveItem();
    item->setPen(pen == QPen() ? defaultPlotPen() : pen);
    item->setData(x, y);
    addItem(item, false, name);
    return item;
}

LegendItem* PlotItem::addLegend(const QPointF& offset)
{
    if (legend_ == nullptr) {
        legend_ = new LegendItem(nullptr, offset);
        legend_->setParentItem(viewBox_);
        for (QGraphicsItem* item : items_) {
            const QString name = itemName(item);
            if (!name.isEmpty()) {
                legend_->addItem(item, name);
            }
        }
    }
    return legend_;
}

LegendItem* PlotItem::legend() noexcept
{
    return legend_;
}

const LegendItem* PlotItem::legend() const noexcept
{
    return legend_;
}

void PlotItem::setLabel(const QString& axisName, const QString& text, const QString& units, const QString& unitPrefix)
{
    AxisItem* targetAxis = getAxis(axisName);
    targetAxis->setLabel(text, units, unitPrefix);
    showAxis(axisName, true);
}

void PlotItem::setTitle(const QString& title)
{
    if (titleItem_ == nullptr) {
        return;
    }
    if (title.isEmpty()) {
        titleItem_->hide();
    } else {
        titleItem_->setPlainText(title);
        titleItem_->show();
    }
    updateTitleGeometry();
}

void PlotItem::showAxis(const QString& axisName, bool show)
{
    AxisItem* targetAxis = getAxis(axisName);
    if (show) {
        targetAxis->show();
    } else {
        targetAxis->hide();
    }
}

void PlotItem::hideAxis(const QString& axisName)
{
    showAxis(axisName, false);
}

void PlotItem::setXRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    viewBox_->setXRange(minimum, maximum, padding, update);
    updateAxisRanges();
}

void PlotItem::setYRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    viewBox_->setYRange(minimum, maximum, padding, update);
    updateAxisRanges();
}

void PlotItem::setRange(std::optional<ViewBox::AxisRange> xRange,
                        std::optional<ViewBox::AxisRange> yRange,
                        qreal padding,
                        bool update,
                        bool disableAutoRange)
{
    viewBox_->setRange(std::move(xRange), std::move(yRange), padding, update, disableAutoRange);
    updateAxisRanges();
}

void PlotItem::autoRange(std::optional<qreal> padding)
{
    viewBox_->autoRange(padding);
    updateAxisRanges();
}

QRectF PlotItem::viewRect() const
{
    return viewBox_->viewRect();
}

ViewBox::Range2D PlotItem::viewRange() const
{
    return viewBox_->viewRange();
}

void PlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }
    painter->fillRect(boundingRect(), Qt::black);
}

PlotItem::AxisSlot PlotItem::axisSlot(const QString& name) const
{
    const QString lower = name.toLower();
    if (lower == QStringLiteral("left")) {
        return AxisSlot::Left;
    }
    if (lower == QStringLiteral("bottom")) {
        return AxisSlot::Bottom;
    }
    if (lower == QStringLiteral("right")) {
        return AxisSlot::Right;
    }
    if (lower == QStringLiteral("top")) {
        return AxisSlot::Top;
    }
    throw std::invalid_argument("PlotItem axis must be left, bottom, right, or top");
}

AxisItem* PlotItem::axis(AxisSlot slot) noexcept
{
    return axes_[static_cast<std::size_t>(slot)];
}

const AxisItem* PlotItem::axis(AxisSlot slot) const noexcept
{
    return axes_[static_cast<std::size_t>(slot)];
}

bool PlotItem::isChromeItem(const QGraphicsItem* item) const noexcept
{
    if (item == nullptr) {
        return true;
    }
    if (item == viewBox_ || item == titleItem_ || item == legend_) {
        return true;
    }
    return std::any_of(axes_.cbegin(), axes_.cend(), [item](const AxisItem* axisItem) {
        return item == axisItem;
    });
}

QString PlotItem::itemName(QGraphicsItem* item) const
{
    const auto it = std::find_if(itemNames_.cbegin(), itemNames_.cend(), [item](const auto& entry) {
        return entry.first == item;
    });
    return it == itemNames_.cend() ? QString{} : it->second;
}

void PlotItem::recordItemName(QGraphicsItem* item, const QString& name)
{
    if (name.isEmpty()) {
        return;
    }
    auto it = std::find_if(itemNames_.begin(), itemNames_.end(), [item](const auto& entry) {
        return entry.first == item;
    });
    if (it == itemNames_.end()) {
        itemNames_.push_back({item, name});
    } else {
        it->second = name;
    }
}

void PlotItem::updateCurveTransforms()
{
    if (viewBox_ == nullptr) {
        return;
    }

    const auto bounds = directCurveBounds(childItems());
    if (!bounds.has_value()) {
        updateAxisRanges();
        update();
        return;
    }

    viewBox_->setRange(ViewBox::AxisRange{bounds->x.minimum, bounds->x.maximum},
        ViewBox::AxisRange{bounds->y.minimum, bounds->y.maximum}, 0.02, true, false);
    const QTransform transform = transformForBounds(*bounds, viewBox_->geometry());
    for (QGraphicsItem* item : childItems()) {
        if (auto* curve = dynamic_cast<PlotCurveItem*>(item); curve != nullptr) {
            curve->setTransform(transform, false);
        }
    }
    updateAxisRanges();
    update();
}

void PlotItem::updateAxisRanges()
{
    if (viewBox_ == nullptr || std::any_of(axes_.cbegin(), axes_.cend(), [](const AxisItem* item) { return item == nullptr; })) {
        return;
    }
    const ViewBox::Range2D range = viewBox_->viewRange();
    axis(AxisSlot::Bottom)->setRange(range[ViewBox::XAxis][0], range[ViewBox::XAxis][1]);
    axis(AxisSlot::Top)->setRange(range[ViewBox::XAxis][0], range[ViewBox::XAxis][1]);
    axis(AxisSlot::Left)->setRange(range[ViewBox::YAxis][0], range[ViewBox::YAxis][1]);
    axis(AxisSlot::Right)->setRange(range[ViewBox::YAxis][0], range[ViewBox::YAxis][1]);
}

void PlotItem::updateTitleGeometry()
{
    if (titleItem_ == nullptr) {
        return;
    }
    const QRectF bounds = boundingRect();
    const QRectF textBounds = titleItem_->boundingRect();
    titleItem_->setTextWidth(std::max<qreal>(1.0, bounds.width()));
    titleItem_->setPos(bounds.left(), bounds.top() + 1.0);
    titleItem_->setDefaultTextColor(Qt::white);
    titleItem_->document()->setDefaultTextOption(QTextOption(Qt::AlignHCenter));
    if (titleItem_->isVisible()) {
        layout_->setContentsMargins(1.0, std::max<qreal>(24.0, textBounds.height() + 2.0), 1.0, 1.0);
    } else {
        layout_->setContentsMargins(1.0, 1.0, 1.0, 1.0);
    }
}

QVariant PlotItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsWidget::itemChange(change, value);
    if (change == QGraphicsItem::ItemChildAddedChange) {
        updateCurveTransforms();
    } else if (change == QGraphicsItem::ItemChildRemovedChange) {
        if (auto* curve = dynamic_cast<PlotCurveItem*>(value.value<QGraphicsItem*>()); curve != nullptr) {
            curve->setTransform(QTransform{}, false);
        }
        updateCurveTransforms();
    }
    return result;
}

void PlotItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    GraphicsWidget::resizeEvent(event);
    updateTitleGeometry();
    updateAxisRanges();
}

} // namespace pyqtgraph::graphicsItems
