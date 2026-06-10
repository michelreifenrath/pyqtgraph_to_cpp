#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ScaleBar.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QMetaObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>

class QGraphicsRectItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class TextItem;
class ViewBox;

class ScaleBar : public GraphicsObject {
public:
    ScaleBar(qreal size,
             qreal width = 5.0,
             const QBrush& brush = QBrush(QColor(200, 200, 200)),
             const QPen& pen = QPen(Qt::NoPen),
             const QString& suffix = QStringLiteral("m"),
             const QPointF& offset = QPointF(0.0, 0.0),
             QGraphicsItem* parent = nullptr);
    ~ScaleBar() override;

    ScaleBar(const ScaleBar&) = delete;
    ScaleBar& operator=(const ScaleBar&) = delete;
    ScaleBar(ScaleBar&&) = delete;
    ScaleBar& operator=(ScaleBar&&) = delete;

    [[nodiscard]] qreal size() const noexcept;
    void setSize(qreal size);
    [[nodiscard]] qreal barWidth() const noexcept;
    void setBarWidth(qreal width);
    [[nodiscard]] QBrush brush() const;
    void setBrush(const QBrush& brush);
    [[nodiscard]] QPen pen() const;
    void setPen(const QPen& pen);
    [[nodiscard]] QString suffix() const;
    void setSuffix(const QString& suffix);
    [[nodiscard]] QPointF offset() const noexcept;
    void setOffset(const QPointF& offset);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void changeParent();
    void connectParentView();
    void disconnectParentView();
    void applyAnchor();
    void updateAnchorPosition();
    void updateBar();
    void updateLabelText();

    QGraphicsRectItem* bar_ = nullptr;
    TextItem* text_ = nullptr;
    qreal size_ = 0.0;
    qreal width_ = 5.0;
    QBrush brush_;
    QPen pen_;
    QString suffix_;
    QPointF offset_;
    QPointF itemAnchor_;
    QPointF parentAnchor_;
    bool hasAnchor_ = false;
    QMetaObject::Connection rangeChangedConnection_;
    QMetaObject::Connection resizedConnection_;
};

} // namespace cppqtgraph::graphicsItems
