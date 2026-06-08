#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/TextItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPen>
#include <QtGui/QTransform>

#include <optional>
#include <utility>

class QGraphicsTextItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class TextItem : public GraphicsObject {
public:
    explicit TextItem(const QString& text = QString{},
                      const QColor& color = QColor(200, 200, 200),
                      const QPointF& anchor = QPointF(0.0, 0.0),
                      QGraphicsItem* parent = nullptr);
    TextItem(const QString& text,
             const QColor& color,
             const QString& html,
             const QPointF& anchor,
             const std::optional<QPen>& border,
             const std::optional<QBrush>& fill,
             qreal angle,
             const std::optional<QPointF>& rotateAxis,
             QGraphicsItem* parent = nullptr);
    ~TextItem() override;

    TextItem(const TextItem&) = delete;
    TextItem& operator=(const TextItem&) = delete;
    TextItem(TextItem&&) = delete;
    TextItem& operator=(TextItem&&) = delete;

    void setText(const QString& text, const std::optional<QColor>& color = std::nullopt);
    void setPlainText(const QString& text);
    [[nodiscard]] QString toPlainText() const;
    void setHtml(const QString& html);
    [[nodiscard]] QString toHtml() const;
    void setTextWidth(qreal width);
    void setFont(const QFont& font);
    void setAngle(qreal angle);
    [[nodiscard]] qreal angle() const noexcept;
    void setAnchor(const QPointF& anchor);
    [[nodiscard]] QPointF anchor() const noexcept;
    void setColor(const QColor& color);
    [[nodiscard]] QColor color() const;
    void setBorder(const QPen& pen);
    [[nodiscard]] QPen border() const;
    void setFill(const QBrush& brush);
    [[nodiscard]] QBrush fill() const;
    void setRotateAxis(const std::optional<QPointF>& rotateAxis);

    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void updateTextPos();
    void updateTransform(bool force = false);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void connectScene();
    void disconnectScene();
    void applyHtml(const QString& html);

    QGraphicsTextItem* textItem_ = nullptr;
    QPointF anchor_{0.0, 0.0};
    std::optional<QPointF> rotateAxis_;
    qreal angle_ = 0.0;
    QColor color_{200, 200, 200};
    QPen border_{Qt::NoPen};
    QBrush fill_{Qt::NoBrush};
    QTransform lastParentTransform_;
    QMetaObject::Connection scenePrepareConnection_;
    QGraphicsScene* connectedScene_ = nullptr;
};

} // namespace pyqtgraph::graphicsItems
