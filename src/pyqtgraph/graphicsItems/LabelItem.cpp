// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LabelItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/LabelItem.hpp"

#include "../../../include/pyqtgraph/functions.hpp"

#include <QtCore/QVariant>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGraphicsTextItem>

namespace pyqtgraph::graphicsItems {
namespace {

QString justifyToString(const QString& justify)
{
    const QString normalized = justify.trimmed().toLower();
    if (normalized == QStringLiteral("left") || normalized == QStringLiteral("center")
        || normalized == QStringLiteral("right")) {
        return normalized;
    }
    return QStringLiteral("center");
}

} // namespace

LabelItem::LabelItem(const QString& text, QGraphicsItem* parent, qreal angle)
    : GraphicsWidget(parent)
    , GraphicsWidgetAnchor(this)
{
    textItem_ = new QGraphicsTextItem(this);
    setText(text);
    setAngle(angle);
}

LabelItem::LabelItem(const QString& text, const TextStyleOptions& options, QGraphicsItem* parent, qreal angle)
    : GraphicsWidget(parent)
    , GraphicsWidgetAnchor(this)
{
    textItem_ = new QGraphicsTextItem(this);
    style_ = options;
    setText(text, options);
    setAngle(angle);
}

LabelItem::~LabelItem() = default;

void LabelItem::setAttr(const QString& attr, const QVariant& value)
{
    if (attr == QStringLiteral("color")) {
        style_.color = value.value<QColor>();
    } else if (attr == QStringLiteral("justify")) {
        style_.justify = justifyToString(value.toString());
    } else if (attr == QStringLiteral("family")) {
        style_.family = value.toString();
    } else if (attr == QStringLiteral("size")) {
        style_.size = value.toString();
    } else if (attr == QStringLiteral("bold")) {
        style_.bold = value.toBool();
    } else if (attr == QStringLiteral("italic")) {
        style_.italic = value.toBool();
    }
    applyStyledHtml(text_);
}

void LabelItem::setText(const QString& text)
{
    setText(text, style_);
}

void LabelItem::setText(const QString& text, const TextStyleOptions& options)
{
    text_ = text;
    style_ = options;
    style_.justify = justifyToString(style_.justify);
    applyStyledHtml(text_);
    updateMin();
    layoutText();
    updateGeometry();
}

void LabelItem::setAngle(qreal angle)
{
    angle_ = angle;
    textItem_->resetTransform();
    textItem_->setRotation(angle);
    updateMin();
    layoutText();
}

qreal LabelItem::angle() const noexcept
{
    return angle_;
}

QString LabelItem::justify() const
{
    return style_.justify;
}

QRectF LabelItem::itemRect() const
{
    return textItem_->mapRectToParent(textItem_->boundingRect());
}

QSizeF LabelItem::sizeHint(Qt::SizeHint hint, const QSizeF& constraint) const
{
    Q_UNUSED(constraint);
    const auto found = sizeHints_.find(hint);
    if (found == sizeHints_.end()) {
        return QSizeF(0.0, 0.0);
    }
    return found->second;
}

void LabelItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    GraphicsWidget::resizeEvent(event);
    layoutText();
}

void LabelItem::applyStyledHtml(const QString& text)
{
    QColor color = style_.color.has_value() ? mkColor(*style_.color) : mkColor(QStringLiteral("d"));
    QStringList styleParts;
    styleParts.push_back(QStringLiteral("color: %1").arg(color.name(QColor::HexArgb)));
    if (style_.family.has_value()) {
        styleParts.push_back(QStringLiteral("font-family: %1").arg(*style_.family));
    }
    if (style_.size.has_value()) {
        styleParts.push_back(QStringLiteral("font-size: %1").arg(*style_.size));
    }
    if (style_.bold.has_value()) {
        styleParts.push_back(QStringLiteral("font-weight: %1").arg(*style_.bold ? QStringLiteral("bold") : QStringLiteral("normal")));
    }
    if (style_.italic.has_value()) {
        styleParts.push_back(QStringLiteral("font-style: %1").arg(*style_.italic ? QStringLiteral("italic") : QStringLiteral("normal")));
    }
    const QString html = QStringLiteral("<span style='%1'>%2</span>").arg(styleParts.join(QStringLiteral("; ")), text);
    textItem_->setHtml(html);
}

void LabelItem::updateMin()
{
    const QRectF bounds = itemRect();
    setMinimumWidth(bounds.width());
    setMinimumHeight(bounds.height());
    sizeHints_[Qt::MinimumSize] = QSizeF(bounds.width(), bounds.height());
    sizeHints_[Qt::PreferredSize] = QSizeF(bounds.width(), bounds.height());
    sizeHints_[Qt::MaximumSize] = QSizeF(-1.0, -1.0);
    sizeHints_[Qt::MinimumDescent] = QSizeF(0.0, 0.0);
    updateGeometry();
}

void LabelItem::layoutText()
{
    textItem_->setPos(0.0, 0.0);
    QRectF bounds = itemRect();
    const QPointF left = mapFromItem(textItem_, QPointF(0.0, 0.0)) - mapFromItem(textItem_, QPointF(1.0, 0.0));
    const QRectF rect = this->rect();

    if (style_.justify == QStringLiteral("left")) {
        if (!qFuzzyIsNull(left.x())) {
            bounds.moveLeft(rect.left());
        }
        if (left.y() < 0.0) {
            bounds.moveTop(rect.top());
        } else if (left.y() > 0.0) {
            bounds.moveBottom(rect.bottom());
        }
    } else if (style_.justify == QStringLiteral("center")) {
        bounds.moveCenter(rect.center());
    } else if (style_.justify == QStringLiteral("right")) {
        if (!qFuzzyIsNull(left.x())) {
            bounds.moveRight(rect.right());
        }
        if (left.y() < 0.0) {
            bounds.moveBottom(rect.bottom());
        } else if (left.y() > 0.0) {
            bounds.moveTop(rect.top());
        }
    }

    textItem_->setPos(bounds.topLeft() - itemRect().topLeft());
    updateMin();
}

} // namespace pyqtgraph::graphicsItems
