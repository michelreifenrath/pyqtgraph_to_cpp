// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ScaleBar.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/ScaleBar.hpp"

#include "../../../include/pyqtgraph/graphicsItems/TextItem.hpp"
#include "../../../include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsRectItem>

#include <algorithm>
#include <cmath>
#include <limits>

namespace pyqtgraph::graphicsItems {
namespace {

QPointF scaledBottomRight(const QRectF& rect, const QPointF& anchor)
{
    const QPointF bottomRight = rect.bottomRight();
    return QPointF(bottomRight.x() * anchor.x(), bottomRight.y() * anchor.y());
}

std::pair<double, QString> siScale(double value, double power = 1.0)
{
    if (!std::isfinite(value)) {
        return {1.0, QString{}};
    }

    int magnitude = 0;
    if (std::abs(value) >= 1.0e-25) {
        const double denominator = std::log(1000.0) * power;
        double log1000 = std::log(std::abs(value)) / denominator;
        log1000 = power > 0.0 ? std::floor(log1000) : std::ceil(log1000);
        log1000 = std::clamp(log1000, -9.0, 9.0);
        magnitude = static_cast<int>(log1000);
    }

    QString prefix;
    if (magnitude == 0) {
        prefix = QString{};
    } else if (magnitude < -8 || magnitude > 8) {
        prefix = QStringLiteral("e%1").arg(magnitude * 3);
    } else {
        static const std::array<QString, 17> prefixes = {
            QStringLiteral("y"),
            QStringLiteral("z"),
            QStringLiteral("a"),
            QStringLiteral("f"),
            QStringLiteral("p"),
            QStringLiteral("n"),
            QString::fromUtf8("µ"),
            QStringLiteral("m"),
            QString{},
            QStringLiteral("k"),
            QStringLiteral("M"),
            QStringLiteral("G"),
            QStringLiteral("T"),
            QStringLiteral("P"),
            QStringLiteral("E"),
            QStringLiteral("Z"),
            QStringLiteral("Y"),
        };
        prefix = prefixes.at(static_cast<std::size_t>(magnitude + 8));
    }

    const double scale = std::pow(1000.0, -static_cast<double>(magnitude));
    return {scale, prefix};
}

QString siFormat(qreal value, int precision = 3, const QString& suffix = QString{}, bool space = true)
{
    const auto [scale, prefix] = siScale(static_cast<double>(value));
    QString spacedPrefix = prefix;
    if (!prefix.isEmpty() && !prefix.startsWith(QLatin1Char('e'))) {
        spacedPrefix = (space ? QStringLiteral(" ") : QString{}) + prefix;
    }
    return QString::number(value * scale, 'g', precision) + spacedPrefix + suffix;
}

} // namespace

ScaleBar::ScaleBar(qreal size,
                   qreal width,
                   const QBrush& brush,
                   const QPen& pen,
                   const QString& suffix,
                   const QPointF& offset,
                   QGraphicsItem* parent)
    : GraphicsObject(parent)
    , size_(size)
    , width_(width)
    , brush_(brush)
    , pen_(pen)
    , suffix_(suffix)
    , offset_(offset)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
    setAcceptedMouseButtons(Qt::NoButton);

    bar_ = new QGraphicsRectItem(this);
    bar_->setPen(pen_);
    bar_->setBrush(brush_);

    text_ = new TextItem(siFormat(size_, 3, suffix_), QColor(200, 200, 200), QPointF(0.5, 1.0), this);
}

ScaleBar::~ScaleBar()
{
    disconnectParentView();
}

qreal ScaleBar::size() const noexcept
{
    return size_;
}

void ScaleBar::setSize(qreal size)
{
    if (qFuzzyCompare(size_, size)) {
        return;
    }
    size_ = size;
    updateLabelText();
    updateBar();
}

qreal ScaleBar::barWidth() const noexcept
{
    return width_;
}

void ScaleBar::setBarWidth(qreal width)
{
    if (qFuzzyCompare(width_, width)) {
        return;
    }
    width_ = width;
    updateBar();
}

QBrush ScaleBar::brush() const
{
    return brush_;
}

void ScaleBar::setBrush(const QBrush& brush)
{
    brush_ = brush;
    bar_->setBrush(brush_);
}

QPen ScaleBar::pen() const
{
    return pen_;
}

void ScaleBar::setPen(const QPen& pen)
{
    pen_ = pen;
    bar_->setPen(pen_);
}

QString ScaleBar::suffix() const
{
    return suffix_;
}

void ScaleBar::setSuffix(const QString& suffix)
{
    if (suffix_ == suffix) {
        return;
    }
    suffix_ = suffix;
    updateLabelText();
}

QPointF ScaleBar::offset() const noexcept
{
    return offset_;
}

void ScaleBar::setOffset(const QPointF& offset)
{
    offset_ = offset;
    applyAnchor();
}

QRectF ScaleBar::boundingRect() const
{
    return QRectF();
}

void ScaleBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

QVariant ScaleBar::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemParentHasChanged) {
        changeParent();
    }
    return GraphicsObject::itemChange(change, value);
}

void ScaleBar::changeParent()
{
    disconnectParentView();
    connectParentView();
    applyAnchor();
    updateBar();
}

void ScaleBar::connectParentView()
{
    auto* view = dynamic_cast<ViewBox*>(parentItem());
    if (view == nullptr) {
        return;
    }

    rangeChangedConnection_ = QObject::connect(view, &ViewBox::sigRangeChanged, view, [this](ViewBox*, ViewBox::Range2D, std::array<bool, 2>) {
        updateBar();
    });
}

void ScaleBar::disconnectParentView()
{
    if (rangeChangedConnection_) {
        QObject::disconnect(rangeChangedConnection_);
        rangeChangedConnection_ = {};
    }
}

void ScaleBar::applyAnchor()
{
    QGraphicsItem* parent = parentItem();
    if (parent == nullptr) {
        hasAnchor_ = false;
        return;
    }

    const qreal anchorX = offset_.x() <= 0.0 ? 1.0 : 0.0;
    const qreal anchorY = offset_.y() <= 0.0 ? 1.0 : 0.0;
    itemAnchor_ = QPointF(anchorX, anchorY);
    parentAnchor_ = QPointF(anchorX, anchorY);
    hasAnchor_ = true;
    updateAnchorPosition();
}

void ScaleBar::updateAnchorPosition()
{
    QGraphicsItem* parent = parentItem();
    if (parent == nullptr || !hasAnchor_) {
        return;
    }

    const QPointF originInParent = mapToParent(QPointF(0.0, 0.0));
    const QPointF itemAnchorLocal = scaledBottomRight(boundingRect(), itemAnchor_);
    const QPointF itemAnchorInParent = mapToParent(itemAnchorLocal);
    const QPointF parentAnchorPoint = scaledBottomRight(parent->boundingRect(), parentAnchor_);
    const QPointF newPos = parentAnchorPoint + (originInParent - itemAnchorInParent) + offset_;
    setPos(newPos);
}

void ScaleBar::updateBar()
{
    auto* view = dynamic_cast<ViewBox*>(parentItem());
    if (view == nullptr) {
        return;
    }

    const QPointF p1 = mapFromParent(view->mapFromView(QPointF(0.0, 0.0)));
    const QPointF p2 = mapFromParent(view->mapFromView(QPointF(size_, 0.0)));
    const qreal mappedWidth = p2.x() - p1.x();
    bar_->setRect(QRectF(-mappedWidth, 0.0, mappedWidth, width_));
    text_->setPos(-mappedWidth * 0.5, 0.0);
}

void ScaleBar::updateLabelText()
{
    text_->setPlainText(siFormat(size_, 3, suffix_));
}

} // namespace pyqtgraph::graphicsItems
