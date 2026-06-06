// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LegendItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/LegendItem.hpp"

#include "../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../include/pyqtgraph/graphicsItems/PlotDataItem.hpp"

#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>

namespace pyqtgraph::graphicsItems {
namespace {

constexpr qreal kMargin = 6.0;
constexpr qreal kSampleWidth = 28.0;
constexpr qreal kEntryHeight = 20.0;
constexpr qreal kColumnSpacing = 14.0;
constexpr qreal kSampleTextSpacing = 5.0;

QPen samplePenFor(QGraphicsItem* item)
{
    if (auto* curve = dynamic_cast<PlotCurveItem*>(item); curve != nullptr) {
        return curve->pen();
    }
    if (auto* data = dynamic_cast<PlotDataItem*>(item); data != nullptr && data->curve() != nullptr) {
        return data->curve()->pen();
    }
    QPen pen(Qt::white);
    pen.setCosmetic(true);
    return pen;
}

} // namespace

LegendItem::LegendItem(QGraphicsItem* parent,
                       QPointF offset,
                       QPen pen,
                       QBrush brush,
                       QColor labelTextColor,
                       bool frame,
                       int columnCount,
                       Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
    , GraphicsWidgetAnchor(this)
    , offset_(offset)
    , pen_(std::move(pen))
    , brush_(std::move(brush))
    , labelTextColor_(std::move(labelTextColor))
    , frame_(frame)
    , columnCount_(std::max(1, columnCount))
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    updateSize();
    if (parent != nullptr) {
        setOffset(offset_);
    }
}

LegendItem::~LegendItem() = default;

void LegendItem::addItem(QGraphicsItem* item, const QString& name)
{
    if (item == nullptr || name.isEmpty()) {
        return;
    }
    const auto existing = std::find_if(entries_.cbegin(), entries_.cend(), [item, &name](const Entry& entry) {
        return entry.item == item || entry.name == name;
    });
    if (existing != entries_.cend()) {
        return;
    }

    entries_.push_back(Entry{item, name, samplePenFor(item)});
    updateSize();
}

void LegendItem::removeItem(QGraphicsItem* item)
{
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [item](const Entry& entry) {
                       return entry.item == item;
                   }),
                   entries_.end());
    updateSize();
}

void LegendItem::removeItem(const QString& name)
{
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [&name](const Entry& entry) {
                       return entry.name == name;
                   }),
                   entries_.end());
    updateSize();
}

void LegendItem::clear()
{
    entries_.clear();
    updateSize();
}

std::size_t LegendItem::count() const noexcept
{
    return entries_.size();
}

bool LegendItem::contains(const QString& name) const
{
    return std::any_of(entries_.cbegin(), entries_.cend(), [&name](const Entry& entry) {
        return entry.name == name;
    });
}

QStringList LegendItem::names() const
{
    QStringList result;
    for (const Entry& entry : entries_) {
        result.push_back(entry.name);
    }
    return result;
}

void LegendItem::setColumnCount(int columnCount)
{
    columnCount_ = std::max(1, columnCount);
    updateSize();
}

int LegendItem::columnCount() const noexcept
{
    return columnCount_;
}

void LegendItem::setPen(const QPen& pen)
{
    pen_ = pen;
    update();
}

QPen LegendItem::pen() const
{
    return pen_;
}

void LegendItem::setBrush(const QBrush& brush)
{
    brush_ = brush;
    update();
}

QBrush LegendItem::brush() const
{
    return brush_;
}

void LegendItem::setLabelTextColor(const QColor& color)
{
    labelTextColor_ = color;
    update();
}

QColor LegendItem::labelTextColor() const
{
    return labelTextColor_;
}

void LegendItem::setFrame(bool frame)
{
    frame_ = frame;
    update();
}

bool LegendItem::frame() const noexcept
{
    return frame_;
}

void LegendItem::setOffset(const QPointF& offset)
{
    offset_ = offset;
    if (parentItem() == nullptr) {
        return;
    }
    const QPointF itemPos(offset_.x() <= 0.0 ? 1.0 : 0.0, offset_.y() <= 0.0 ? 1.0 : 0.0);
    anchor(itemPos, itemPos, offset_);
}

QPointF LegendItem::offset() const noexcept
{
    return offset_;
}

void LegendItem::setParentItem(QGraphicsItem* parent)
{
    GraphicsWidget::setParentItem(parent);
    if (parent != nullptr) {
        setOffset(offset_);
    }
}

QRectF LegendItem::boundingRect() const
{
    return QRectF(QPointF(0.0, 0.0), contentSize_);
}

void LegendItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    if (frame_) {
        painter->setPen(pen_);
        painter->setBrush(brush_);
        painter->drawRect(boundingRect().adjusted(0.5, 0.5, -0.5, -0.5));
    }

    painter->setBrush(Qt::NoBrush);
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        const Entry& entry = entries_.at(index);
        const QRectF rect = entryRect(index);
        QPen samplePen = entry.samplePen;
        if (samplePen.style() == Qt::NoPen) {
            samplePen = QPen(labelTextColor_);
            samplePen.setCosmetic(true);
        }
        painter->setPen(samplePen);
        const qreal y = std::floor(rect.center().y()) + 0.5;
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.left() + kSampleWidth, y));

        painter->setPen(QPen(labelTextColor_));
        painter->drawText(QRectF(rect.left() + kSampleWidth + kSampleTextSpacing, rect.top(),
                              rect.width() - kSampleWidth - kSampleTextSpacing, rect.height()),
            Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, entry.name);
    }
}

void LegendItem::updateSize()
{
    QFontMetricsF metrics(QFont{});
    qreal maxTextWidth = 38.0;
    for (const Entry& entry : entries_) {
        maxTextWidth = std::max(maxTextWidth, metrics.horizontalAdvance(entry.name));
    }

    const int columns = std::max(1, std::min(columnCount_, std::max(1, static_cast<int>(entries_.size()))));
    const int rows = entries_.empty() ? 1 : static_cast<int>(std::ceil(static_cast<double>(entries_.size()) / columns));
    const qreal columnWidth = kSampleWidth + kSampleTextSpacing + maxTextWidth;
    const qreal width = (2.0 * kMargin) + (columns * columnWidth) + ((columns - 1) * kColumnSpacing);
    const qreal height = (2.0 * kMargin) + (rows * kEntryHeight);
    contentSize_ = QSizeF(width, height);
    resize(contentSize_);
    updateGeometry();
    updateAnchorPosition();
    update();
}

QRectF LegendItem::entryRect(std::size_t index) const
{
    const int columns = std::max(1, std::min(columnCount_, std::max(1, static_cast<int>(entries_.size()))));
    const int row = static_cast<int>(index) / columns;
    const int column = static_cast<int>(index) % columns;
    const qreal columnWidth = (contentSize_.width() - (2.0 * kMargin) - ((columns - 1) * kColumnSpacing)) / columns;
    return QRectF(kMargin + (column * (columnWidth + kColumnSpacing)), kMargin + (row * kEntryHeight), columnWidth, kEntryHeight);
}

} // namespace pyqtgraph::graphicsItems
