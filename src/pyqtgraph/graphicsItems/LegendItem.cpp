// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LegendItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/LegendItem.hpp"

#include "../../../include/pyqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../include/pyqtgraph/graphicsItems/PlotDataItem.hpp"

#include <QtCore/QRectF>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>

namespace pyqtgraph::graphicsItems {

namespace {

int parsePointSize(const QString& size, int fallback)
{
    QString numeric = size.trimmed();
    if (numeric.endsWith(QStringLiteral("pt"), Qt::CaseInsensitive)) {
        numeric.chop(2);
    }
    bool ok = false;
    const int parsed = numeric.toInt(&ok);
    return ok && parsed > 0 ? parsed : fallback;
}

} // namespace

LegendItem::LegendItem(const QSizeF& size, std::optional<QPointF> offset, QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
    , GraphicsWidgetAnchor(this)
    , fixedSize_(size)
    , offset_(offset)
{
    setFlag(QGraphicsItem::GraphicsItemFlag::ItemIgnoresTransformations);
    if (fixedSize_.isValid() && !fixedSize_.isEmpty()) {
        setGeometry(QRectF(QPointF{}, fixedSize_));
    } else {
        updateSizeToContents();
    }
    if (offset_.has_value() && parentItem() != nullptr) {
        setOffset(*offset_);
    }
}

LegendItem::~LegendItem() = default;

void LegendItem::setOffset(const QPointF& offset)
{
    offset_ = offset;
    const QPointF anchorPosition(offset.x() <= 0.0 ? 1.0 : 0.0, offset.y() <= 0.0 ? 1.0 : 0.0);
    anchor(anchorPosition, anchorPosition, offset);
}

std::optional<QPointF> LegendItem::offset() const noexcept
{
    return offset_;
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

std::optional<QColor> LegendItem::labelTextColor() const noexcept
{
    return labelTextColor_;
}

void LegendItem::setLabelTextSize(const QString& size)
{
    labelTextSize_ = size;
    updateSizeToContents();
    update();
}

QString LegendItem::labelTextSize() const
{
    return labelTextSize_;
}

void LegendItem::addItem(QGraphicsItem* item, const QString& name)
{
    if (item == nullptr || name.isEmpty()) {
        return;
    }
    items_.push_back(Entry{item, name, samplePenForItem(item)});
    updateSizeToContents();
    update();
}

void LegendItem::removeItem(QGraphicsItem* item)
{
    const auto oldSize = items_.size();
    items_.erase(std::remove_if(items_.begin(), items_.end(), [item](const Entry& entry) {
        return entry.item == item;
    }), items_.end());
    if (items_.size() != oldSize) {
        updateSizeToContents();
        update();
    }
}

void LegendItem::removeItem(const QString& name)
{
    const auto oldSize = items_.size();
    items_.erase(std::remove_if(items_.begin(), items_.end(), [&name](const Entry& entry) {
        return entry.name == name;
    }), items_.end());
    if (items_.size() != oldSize) {
        updateSizeToContents();
        update();
    }
}

void LegendItem::clear()
{
    if (items_.empty()) {
        return;
    }
    items_.clear();
    updateSizeToContents();
    update();
}

void LegendItem::setColumnCount(int columnCount)
{
    const int normalized = std::max(1, columnCount);
    if (columnCount_ == normalized) {
        return;
    }
    columnCount_ = normalized;
    updateSizeToContents();
    update();
}

int LegendItem::columnCount() const noexcept
{
    return columnCount_;
}

int LegendItem::rowCount() const noexcept
{
    return rowCount_;
}

int LegendItem::itemCount() const noexcept
{
    return static_cast<int>(items_.size());
}

QString LegendItem::labelForItem(QGraphicsItem* item) const
{
    const auto found = std::find_if(items_.begin(), items_.end(), [item](const Entry& entry) {
        return entry.item == item;
    });
    return found == items_.end() ? QString{} : found->name;
}

void LegendItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    const QRectF bounds = boundingRect();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen_);
    painter->setBrush(brush_);
    if (pen_.style() != Qt::NoPen || brush_.style() != Qt::NoBrush) {
        painter->drawRect(bounds.adjusted(0.5, 0.5, -0.5, -0.5));
    }

    painter->setFont(labelFont());
    const QColor textColor = labelTextColor_.value_or(QColor(Qt::white));
    const QFontMetricsF metrics(painter->font());
    const qreal rowHeight = std::max<qreal>(18.0, metrics.height() + verticalSpacing_);
    const qreal columnWidth = std::max<qreal>(1.0, (bounds.width() - (2.0 * horizontalPadding_)) / std::max(1, columnCount_));
    for (std::size_t index = 0; index < items_.size(); ++index) {
        const int column = static_cast<int>(index) % columnCount_;
        const int row = static_cast<int>(index) / columnCount_;
        const qreal left = bounds.left() + horizontalPadding_ + (static_cast<qreal>(column) * columnWidth);
        const qreal top = bounds.top() + verticalPadding_ + (static_cast<qreal>(row) * rowHeight);
        const qreal centerY = top + rowHeight / 2.0;
        QPen samplePen = items_[index].samplePen;
        if (samplePen.style() == Qt::NoPen) {
            samplePen = QPen(textColor);
        }
        samplePen.setCosmetic(true);
        painter->setPen(samplePen);
        painter->drawLine(QPointF(left, centerY), QPointF(left + sampleWidth_, centerY));
        painter->setPen(textColor);
        painter->drawText(QRectF(left + sampleWidth_ + horizontalSpacing_, top, columnWidth - sampleWidth_ - horizontalSpacing_, rowHeight),
            Qt::AlignLeft | Qt::AlignVCenter, items_[index].name);
    }
}

void LegendItem::updateSizeToContents()
{
    rowCount_ = items_.empty() ? 1 : static_cast<int>(std::ceil(static_cast<double>(items_.size()) / static_cast<double>(columnCount_)));
    if (fixedSize_.isValid() && !fixedSize_.isEmpty()) {
        setGeometry(QRectF(pos(), fixedSize_));
        return;
    }

    const QFontMetricsF metrics(labelFont());
    qreal widest = 0.0;
    for (const Entry& entry : items_) {
        widest = std::max(widest, metrics.horizontalAdvance(entry.name));
    }
    const qreal rowHeight = std::max<qreal>(18.0, metrics.height() + verticalSpacing_);
    const qreal columnWidth = sampleWidth_ + horizontalSpacing_ + widest + horizontalPadding_;
    const qreal width = std::max<qreal>(80.0, (2.0 * horizontalPadding_) + (static_cast<qreal>(columnCount_) * columnWidth));
    const qreal height = std::max<qreal>(24.0, (2.0 * verticalPadding_) + (static_cast<qreal>(rowCount_) * rowHeight));
    setGeometry(QRectF(pos(), QSizeF(width, height)));
    updateAnchorPosition();
}

QPen LegendItem::samplePenForItem(QGraphicsItem* item) const
{
    if (const auto* curve = dynamic_cast<const PlotCurveItem*>(item)) {
        return curve->pen();
    }
    if (const auto* data = dynamic_cast<const PlotDataItem*>(item)) {
        return data->pen();
    }
    return QPen(labelTextColor_.value_or(QColor(Qt::white)));
}

QFont LegendItem::labelFont() const
{
    QFont font(QStringLiteral("Sans Serif"));
    font.setPointSize(parsePointSize(labelTextSize_, 9));
    return font;
}

} // namespace pyqtgraph::graphicsItems
