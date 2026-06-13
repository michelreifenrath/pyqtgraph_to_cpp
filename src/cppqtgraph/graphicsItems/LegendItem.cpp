// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LegendItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/LegendItem.hpp"

#include "../../../include/cppqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/PlotDataItem.hpp"

#include <QtCore/QSizeF>
#include <QtCore/Qt>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cppqtgraph::graphicsItems {
namespace {

constexpr qreal kSampleWidth = 26.0;
constexpr qreal kSampleHeight = 18.0;
constexpr qreal kOuterMargin = 6.0;

int normalizedColumnCount(int columnCount)
{
    return std::max(1, columnCount);
}

} // namespace

LegendItem::LegendItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : LegendItem(Options{}, parent, flags)
{
}

LegendItem::LegendItem(std::optional<QPointF> offset, QGraphicsItem* parent)
    : LegendItem([&]() {
          Options options;
          options.offset = offset;
          return options;
      }(), parent)
{
}

LegendItem::LegendItem(const Options& options, QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
    , GraphicsWidgetAnchor(this)
    , fixedSize_(options.size)
    , offset_(options.offset)
    , horizontalSpacing_(options.horizontalSpacing)
    , verticalSpacing_(options.verticalSpacing)
    , pen_(options.pen)
    , brush_(options.brush)
    , labelTextColor_(options.labelTextColor)
    , frame_(options.frame)
    , labelTextPointSize_(options.labelTextPointSize)
    , columnCount_(normalizedColumnCount(options.columnCount))
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    if (fixedSize_.has_value()) {
        setGeometry(QRectF(QPointF(0.0, 0.0), *fixedSize_));
    } else {
        updateSize();
    }
    anchorToOffsetIfReady();
}

LegendItem::~LegendItem() = default;

std::optional<QPointF> LegendItem::offset() const
{
    return offset_;
}

void LegendItem::setOffset(std::optional<QPointF> offset)
{
    offset_ = offset;
    anchorToOffsetIfReady();
}

QPen LegendItem::pen() const
{
    return pen_;
}

void LegendItem::setPen(const QPen& pen)
{
    pen_ = pen;
    update();
}

QBrush LegendItem::brush() const
{
    return brush_;
}

void LegendItem::setBrush(const QBrush& brush)
{
    brush_ = brush;
    update();
}

QColor LegendItem::labelTextColor() const
{
    return labelTextColor_;
}

void LegendItem::setLabelTextColor(const QColor& color)
{
    labelTextColor_ = color;
    update();
}

qreal LegendItem::labelTextPointSize() const noexcept
{
    return labelTextPointSize_;
}

void LegendItem::setLabelTextPointSize(qreal pointSize)
{
    if (!std::isfinite(pointSize) || pointSize <= 0.0) {
        throw std::invalid_argument("LegendItem label text size must be positive and finite");
    }
    labelTextPointSize_ = pointSize;
    updateSize();
    update();
}

void LegendItem::addItem(QGraphicsItem* item, const QString& name)
{
    if (item == nullptr) {
        throw std::invalid_argument("LegendItem::addItem requires a non-null item");
    }
    items_.push_back(Entry{item, name});
    updateSize();
    update();
}

void LegendItem::removeItem(QGraphicsItem* item)
{
    items_.erase(std::remove_if(items_.begin(), items_.end(), [item](const Entry& entry) {
        return entry.item == item;
    }), items_.end());
    updateSize();
    update();
}

void LegendItem::removeItem(const QString& name)
{
    items_.erase(std::remove_if(items_.begin(), items_.end(), [&name](const Entry& entry) {
        return entry.name == name;
    }), items_.end());
    updateSize();
    update();
}

void LegendItem::clear()
{
    items_.clear();
    updateSize();
    update();
}

void LegendItem::setColumnCount(int columnCount)
{
    columnCount_ = normalizedColumnCount(columnCount);
    updateSize();
    update();
}

int LegendItem::columnCount() const noexcept
{
    return columnCount_;
}

int LegendItem::itemCount() const noexcept
{
    return static_cast<int>(items_.size());
}

QString LegendItem::getLabel(QGraphicsItem* item) const
{
    const auto found = std::find_if(items_.begin(), items_.end(), [item](const Entry& entry) {
        return entry.item == item;
    });
    return found == items_.end() ? QString{} : found->name;
}

void LegendItem::setParentItem(QGraphicsItem* parent)
{
    GraphicsWidget::setParentItem(parent);
    anchorToOffsetIfReady();
}

QRectF LegendItem::boundingRect() const
{
    QRectF rect = QGraphicsWidget::boundingRect();
    if (rect.width() <= 0.0 || rect.height() <= 0.0) {
        rect = QRectF(QPointF(0.0, 0.0), geometry().size());
    }
    return rect;
}

void LegendItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    const QRectF rect = boundingRect();
    if (frame_) {
        painter->setPen(pen_);
        painter->setBrush(brush_);
        painter->drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
    }

    QFont font = painter->font();
    font.setPointSizeF(labelTextPointSize_);
    painter->setFont(font);
    painter->setPen(QPen(labelTextColor_));

    const int columns = normalizedColumnCount(columnCount_);
    const int rows = std::max(1, static_cast<int>(std::ceil(static_cast<double>(items_.size()) / columns)));
    const qreal cellWidth = std::max<qreal>(1.0, (rect.width() - 2.0 * kOuterMargin) / columns);
    const qreal cellHeight = std::max<qreal>(kSampleHeight, (rect.height() - 2.0 * kOuterMargin) / rows);

    for (std::size_t index = 0; index < items_.size(); ++index) {
        const int row = static_cast<int>(index) / columns;
        const int column = static_cast<int>(index) % columns;
        const qreal left = rect.left() + kOuterMargin + column * cellWidth;
        const qreal top = rect.top() + kOuterMargin + row * cellHeight;
        const qreal centerY = top + cellHeight / 2.0;

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(samplePen(items_[index]));
        painter->drawLine(QPointF(left, centerY), QPointF(left + kSampleWidth, centerY));
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setPen(QPen(labelTextColor_));
        painter->drawText(QRectF(left + kSampleWidth + horizontalSpacing_, top, cellWidth - kSampleWidth - horizontalSpacing_, cellHeight),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          items_[index].name);
    }
}

void LegendItem::updateSize()
{
    if (fixedSize_.has_value()) {
        setGeometry(QRectF(QPointF(0.0, 0.0), *fixedSize_));
        anchorToOffsetIfReady();
        return;
    }

    QFont font;
    font.setPointSizeF(labelTextPointSize_);
    const QFontMetricsF metrics(font);
    qreal widestLabel = 0.0;
    for (const Entry& entry : items_) {
        widestLabel = std::max(widestLabel, metrics.horizontalAdvance(entry.name));
    }

    const int columns = normalizedColumnCount(columnCount_);
    const int rows = std::max(1, static_cast<int>(std::ceil(static_cast<double>(items_.size()) / columns)));
    const qreal cellWidth = kSampleWidth + horizontalSpacing_ + widestLabel + kOuterMargin;
    const qreal cellHeight = std::max(kSampleHeight, metrics.height()) + verticalSpacing_;
    const QSizeF size(2.0 * kOuterMargin + columns * cellWidth,
                      2.0 * kOuterMargin + rows * cellHeight - verticalSpacing_);
    setGeometry(QRectF(QPointF(0.0, 0.0), size));
    setPreferredSize(size);
    anchorToOffsetIfReady();
}

void LegendItem::anchorToOffsetIfReady()
{
    if (!offset_.has_value() || parentItem() == nullptr) {
        return;
    }
    const qreal anchorX = offset_->x() <= 0.0 ? 1.0 : 0.0;
    const qreal anchorY = offset_->y() <= 0.0 ? 1.0 : 0.0;
    anchor(QPointF(anchorX, anchorY), QPointF(anchorX, anchorY), *offset_);
}

QPen LegendItem::samplePen(const Entry& entry) const
{
    if (auto* curve = dynamic_cast<PlotCurveItem*>(entry.item)) {
        QPen curvePen = curve->pen();
        if (curvePen.style() != Qt::NoPen) {
            curvePen.setCosmetic(true);
            return curvePen;
        }
    }
    if (auto* data = dynamic_cast<PlotDataItem*>(entry.item)) {
        QPen dataPen = data->pen();
        if (dataPen.style() != Qt::NoPen) {
            dataPen.setCosmetic(true);
            return dataPen;
        }
    }
    QPen fallback(labelTextColor_, 1.5);
    fallback.setCosmetic(true);
    return fallback;
}

} // namespace cppqtgraph::graphicsItems
