// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/TextItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/TextItem.hpp"

#include "../../../include/cppqtgraph/GraphicsScene/GraphicsScene.hpp"
#include "../../../include/cppqtgraph/functions.hpp"

#include <QtCore/QMetaObject>
#include <QtCore/QVariant>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <cmath>

namespace cppqtgraph::graphicsItems {
namespace {

qreal axisAngleDegrees(const QTransform& parentSceneTransform, const QPointF& rotateAxis)
{
    const QPointF mappedAxis = parentSceneTransform.map(rotateAxis);
    const QPointF mappedOrigin = parentSceneTransform.map(QPointF(0.0, 0.0));
    const QPointF delta = mappedAxis - mappedOrigin;
    return qRadiansToDegrees(std::atan2(delta.y(), delta.x()));
}

} // namespace

TextItem::TextItem(const QString& text, const QColor& color, const QPointF& anchor, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , anchor_(anchor)
    , color_(color)
{
    textItem_ = new QGraphicsTextItem(this);
    setColor(color);
    setPlainText(text);
}

TextItem::TextItem(const QString& text,
                   const QColor& color,
                   const QString& html,
                   const QPointF& anchor,
                   const std::optional<QPen>& border,
                   const std::optional<QBrush>& fill,
                   qreal angle,
                   const std::optional<QPointF>& rotateAxis,
                   QGraphicsItem* parent)
    : GraphicsObject(parent)
    , anchor_(anchor)
    , rotateAxis_(rotateAxis)
    , angle_(angle)
    , color_(color)
{
    textItem_ = new QGraphicsTextItem(this);
    if (border.has_value()) {
        border_ = *border;
    }
    if (fill.has_value()) {
        fill_ = *fill;
    }
    if (html.isEmpty()) {
        setColor(color);
        setPlainText(text);
    } else {
        applyHtml(html);
    }
    setAngle(angle);
}

TextItem::~TextItem()
{
    disconnectScene();
}

void TextItem::setText(const QString& text, const std::optional<QColor>& color)
{
    if (color.has_value()) {
        setColor(*color);
    }
    setPlainText(text);
}

void TextItem::setPlainText(const QString& text)
{
    if (text == toPlainText()) {
        return;
    }
    textItem_->setPlainText(text);
    updateTextPos();
}

QString TextItem::toPlainText() const
{
    return textItem_->toPlainText();
}

void TextItem::setHtml(const QString& html)
{
    applyHtml(html);
}

QString TextItem::toHtml() const
{
    return textItem_->toHtml();
}

void TextItem::setTextWidth(qreal width)
{
    textItem_->setTextWidth(width);
    updateTextPos();
}

void TextItem::setFont(const QFont& font)
{
    textItem_->setFont(font);
    updateTextPos();
}

void TextItem::setAngle(qreal angle)
{
    angle_ = angle;
    updateTransform(true);
}

qreal TextItem::angle() const noexcept
{
    return angle_;
}

void TextItem::setAnchor(const QPointF& anchor)
{
    anchor_ = anchor;
    updateTextPos();
}

QPointF TextItem::anchor() const noexcept
{
    return anchor_;
}

void TextItem::setColor(const QColor& color)
{
    color_ = mkColor(color);
    textItem_->setDefaultTextColor(color_);
}

QColor TextItem::color() const
{
    return color_;
}

void TextItem::setBorder(const QPen& pen)
{
    border_ = pen;
    update();
}

QPen TextItem::border() const
{
    return border_;
}

void TextItem::setFill(const QBrush& brush)
{
    fill_ = brush;
    update();
}

QBrush TextItem::fill() const
{
    return fill_;
}

void TextItem::setRotateAxis(const std::optional<QPointF>& rotateAxis)
{
    rotateAxis_ = rotateAxis;
    updateTransform(true);
}

std::pair<qreal, qreal> TextItem::dataBounds(int axis) const
{
    const qreal anchorValue = axis == 0 ? anchor_.x() : anchor_.y();
    return {anchorValue, anchorValue};
}

QRectF TextItem::boundingRect() const
{
    return textItem_->mapRectToParent(textItem_->boundingRect());
}

void TextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    QGraphicsScene* scene = this->scene();
    if (scene != connectedScene_) {
        disconnectScene();
        connectScene();
    }
    updateTransform();

    if (border_.style() != Qt::NoPen || fill_.style() != Qt::NoBrush) {
        painter->setPen(border_);
        painter->setBrush(fill_);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawPolygon(textItem_->mapToParent(textItem_->boundingRect()));
    }
}

QVariant TextItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemVisibleHasChanged && value.toBool()) {
        updateTransform(true);
    }
    if (change == QGraphicsItem::ItemSceneHasChanged) {
        disconnectScene();
        connectScene();
    }
    return GraphicsObject::itemChange(change, value);
}

void TextItem::updateTextPos()
{
    const QRectF bounds = textItem_->boundingRect();
    const QPointF topLeft = textItem_->mapToParent(bounds.topLeft());
    const QPointF bottomRight = textItem_->mapToParent(bounds.bottomRight());
    const QPointF delta = bottomRight - topLeft;
    const QPointF offset(delta.x() * anchor_.x(), delta.y() * anchor_.y());
    textItem_->setPos(-offset);
}

void TextItem::updateTransform(bool force)
{
    if (!isVisible()) {
        return;
    }

    QGraphicsItem* parent = parentItem();
    QTransform parentSceneTransform;
    if (parent != nullptr) {
        parentSceneTransform = parent->sceneTransform();
    }

    if (!force && parentSceneTransform == lastParentTransform_) {
        return;
    }

    bool invertible = false;
    QTransform transform = parentSceneTransform.inverted(&invertible);
    if (!invertible) {
        transform = QTransform();
    }
    transform.setMatrix(transform.m11(),
                        transform.m12(),
                        transform.m13(),
                        transform.m21(),
                        transform.m22(),
                        transform.m23(),
                        0.0,
                        0.0,
                        transform.m33());

    qreal angle = -angle_;
    if (rotateAxis_.has_value()) {
        angle += axisAngleDegrees(parentSceneTransform, *rotateAxis_);
    }
    transform.rotate(angle);
    setTransform(transform);
    lastParentTransform_ = parentSceneTransform;
    updateTextPos();
}

void TextItem::connectScene()
{
    QGraphicsScene* scene = this->scene();
    if (scene == nullptr) {
        connectedScene_ = nullptr;
        return;
    }

    auto* graphicsScene = qobject_cast<cppqtgraph::GraphicsScene::GraphicsScene*>(scene);
    if (graphicsScene == nullptr) {
        connectedScene_ = scene;
        return;
    }

    scenePrepareConnection_ = QObject::connect(graphicsScene,
                                               &cppqtgraph::GraphicsScene::GraphicsScene::sigPrepareForPaint,
                                               this,
                                               [this]() { updateTransform(); });
    connectedScene_ = scene;
}

void TextItem::disconnectScene()
{
    if (scenePrepareConnection_) {
        QObject::disconnect(scenePrepareConnection_);
        scenePrepareConnection_ = {};
    }
    connectedScene_ = nullptr;
}

void TextItem::applyHtml(const QString& html)
{
    if (toHtml() == html) {
        return;
    }
    textItem_->setHtml(html);
    updateTextPos();
}

} // namespace cppqtgraph::graphicsItems
