#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/TargetItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>

#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class TargetItem : public GraphicsObject, public cppqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    Q_OBJECT

public:
    explicit TargetItem(const QPointF& pos = QPointF(0.0, 0.0),
                        qreal size = 10.0,
                        bool movable = true,
                        QGraphicsItem* parent = nullptr);
    ~TargetItem() override;

    void setMovable(bool movable);
    [[nodiscard]] bool movable() const noexcept;

    void setPos(const QPointF& pos);
    void setPos(qreal x, qreal y);
    [[nodiscard]] QPointF pos() const;

    void setSize(qreal size);
    [[nodiscard]] qreal size() const noexcept;

    void setSymbol(const QString& symbol);
    void setSymbol(const QPainterPath& path);
    [[nodiscard]] QString symbol() const;

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;
    void setHoverPen(const QPen& pen);
    [[nodiscard]] QPen hoverPen() const;

    void setBrush(const QBrush& brush);
    [[nodiscard]] QBrush brush() const;
    void setHoverBrush(const QBrush& brush);
    [[nodiscard]] QBrush hoverBrush() const;

    void setMouseHover(bool hover);
    [[nodiscard]] bool mouseHovering() const noexcept;

    [[nodiscard]] QPainterPath shape() const override;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event) override;
    void mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event) override;
    void mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event) override;

signals:
    void sigPositionChanged(cppqtgraph::graphicsItems::TargetItem* target);
    void sigPositionChangeFinished(cppqtgraph::graphicsItems::TargetItem* target);

private:
    [[nodiscard]] QPainterPath scaledPath() const;

    bool movable_ = true;
    bool moving_ = false;
    bool mouseHovering_ = false;
    QPointF position_;
    QPointF symbolOffset_;
    qreal size_ = 10.0;
    QString symbol_ = QStringLiteral("crosshair");
    QPainterPath unitPath_;
    QPen pen_;
    QPen hoverPen_;
    QPen currentPen_;
    QBrush brush_;
    QBrush hoverBrush_;
    QBrush currentBrush_;
};

} // namespace cppqtgraph::graphicsItems
