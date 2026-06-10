// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GradientLegend.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/GradientLegend.hpp"

#include "../../../include/cppqtgraph/functions.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

namespace cppqtgraph::graphicsItems {
namespace {

constexpr qreal textPadding = 2.0;

} // namespace

GradientLegend::GradientLegend(const QPointF& size, const QPointF& offset, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , size_(size)
    , offset_(offset)
    , brush_(QColor(255, 255, 255, 100))
    , pen_(QColor(0, 0, 0))
    , textPen_(QColor(0, 0, 0))
{
    labels_.insert(QStringLiteral("max"), 1.0);
    labels_.insert(QStringLiteral("min"), 0.0);
    gradient_.setColorAt(0.0, QColor(0, 0, 0));
    gradient_.setColorAt(1.0, QColor(255, 0, 0));

    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setZValue(100.0);
}

GradientLegend::~GradientLegend() = default;

void GradientLegend::setGradient(const QLinearGradient& gradient)
{
    gradient_ = gradient;
    update();
}

void GradientLegend::setColorMap(const cppqtgraph::ColorMap& colorMap)
{
    setGradient(colorMap.getGradient());
}

void GradientLegend::setIntColorScale(int minVal,
                                      int maxVal,
                                      int values,
                                      int maxValue,
                                      int minValue,
                                      int maxHue,
                                      int minHue,
                                      int sat,
                                      int alpha,
                                      std::optional<std::pair<QString, QString>> labels)
{
    QLinearGradient gradient;
    const int span = maxVal - minVal;
    const int count = span > 0 ? span : 0;
    for (int i = 0; i < count; ++i) {
        const qreal position = count == 0 ? 0.0 : static_cast<qreal>(i) / static_cast<qreal>(count);
        gradient.setColorAt(position,
                            cppqtgraph::intColor(minVal + i, span, values, maxValue, minValue, maxHue, minHue, sat, alpha));
    }
    setGradient(gradient);

    QMap<QString, qreal> nextLabels;
    if (labels.has_value()) {
        nextLabels.insert(labels->first, 0.0);
        nextLabels.insert(labels->second, 1.0);
    } else {
        nextLabels.insert(QString::number(minVal), 0.0);
        nextLabels.insert(QString::number(maxVal), 1.0);
    }
    setLabels(nextLabels);
}

void GradientLegend::setLabels(const QMap<QString, qreal>& labels)
{
    labels_ = labels;
    update();
}

QRectF GradientLegend::boundingRect() const
{
    if (!boundingRect_.has_value()) {
        if (const ViewBox* view = viewBox(); view != nullptr) {
            boundingRect_ = view->viewRect().normalized();
        } else {
            boundingRect_ = QRectF();
        }
    }
    return *boundingRect_;
}

void GradientLegend::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    ViewBox* view = viewBox();
    if (view == nullptr) {
        return;
    }

    painter->save();
    painter->setTransform(view->sceneTransform());

    const QRectF rect = view->rect();
    const qreal xR = rect.right();
    const qreal xL = rect.left();
    const qreal yT = rect.top();
    const qreal yB = rect.bottom();

    qreal labelWidth = 0.0;
    qreal labelHeight = 0.0;
    for (auto it = labels_.constBegin(); it != labels_.constEnd(); ++it) {
        const QRectF bounds = painter->boundingRect(QRectF(0.0, 0.0, 0.0, 0.0),
                                                  Qt::AlignLeft | Qt::AlignVCenter,
                                                  it.key());
        labelWidth = std::max(labelWidth, bounds.width());
        labelHeight = std::max(labelHeight, bounds.height());
    }

    qreal x1 = 0.0;
    qreal x2 = 0.0;
    qreal x3 = 0.0;
    if (offset_.x() < 0.0) {
        x3 = xR + offset_.x();
        x2 = x3 - labelWidth - 2.0 * textPadding;
        x1 = x2 - size_.x();
    } else {
        x1 = xL + offset_.x();
        x2 = x1 + size_.x();
        x3 = x2 + labelWidth + 2.0 * textPadding;
    }

    qreal y1 = 0.0;
    qreal y2 = 0.0;
    if (offset_.y() < 0.0) {
        y2 = yB + offset_.y();
        y1 = y2 - size_.y();
    } else {
        y1 = yT + offset_.y();
        y2 = y1 + size_.y();
    }

    painter->setPen(pen_);
    painter->setBrush(brush_);
    painter->drawRect(QRectF(QPointF(x1 - textPadding, y1 - labelHeight / 2.0 - textPadding),
                             QPointF(x3 + textPadding, y2 + labelHeight / 2.0 + textPadding)));

    QLinearGradient barGradient = gradient_;
    barGradient.setStart(0.0, y2);
    barGradient.setFinalStop(0.0, y1);
    painter->setBrush(barGradient);
    painter->drawRect(QRectF(QPointF(x1, y1), QPointF(x2, y2)));

    painter->setPen(textPen_);
    const qreal tx = x2 + 2.0 * textPadding;
    const qreal lh = labelHeight;
    const qreal lw = labelWidth;
    for (auto it = labels_.constBegin(); it != labels_.constEnd(); ++it) {
        const qreal y = y2 - it.value() * (y2 - y1);
        painter->drawText(QRectF(tx, y - lh / 2.0, lw, lh), Qt::AlignLeft | Qt::AlignVCenter, it.key());
    }

    painter->restore();
}

QVariant GradientLegend::itemChange(GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsObject::itemChange(change, value);
    if (change == QGraphicsItem::ItemScenePositionHasChanged) {
        invalidateBounds();
    }
    return result;
}

ViewBox* GradientLegend::viewBox() const
{
    for (QGraphicsItem* item = parentItem(); item != nullptr; item = item->parentItem()) {
        if (auto* view = dynamic_cast<ViewBox*>(item)) {
            return view;
        }
    }
    return nullptr;
}

void GradientLegend::invalidateBounds()
{
    boundingRect_.reset();
    prepareGeometryChange();
}

} // namespace cppqtgraph::graphicsItems
