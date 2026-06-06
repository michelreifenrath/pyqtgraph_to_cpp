#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/InfiniteLine.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QPen>

#include <optional>
#include <utility>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class InfiniteLine : public GraphicsObject, public pyqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    Q_OBJECT

public:
    using Bounds = std::pair<std::optional<qreal>, std::optional<qreal>>;

    explicit InfiniteLine(qreal pos = 0.0,
                          qreal angle = 90.0,
                          bool movable = false,
                          std::optional<Bounds> bounds = std::nullopt,
                          QGraphicsItem* parent = nullptr);
    InfiniteLine(qreal pos, qreal angle, bool movable, std::pair<qreal, qreal> bounds, QGraphicsItem* parent = nullptr);
    explicit InfiniteLine(const QPointF& pos,
                          qreal angle = 90.0,
                          bool movable = false,
                          std::optional<Bounds> bounds = std::nullopt,
                          QGraphicsItem* parent = nullptr);
    InfiniteLine(const QPointF& pos, qreal angle, bool movable, std::pair<qreal, qreal> bounds, QGraphicsItem* parent = nullptr);
    ~InfiniteLine() override;

    void setMovable(bool movable);
    [[nodiscard]] bool movable() const noexcept;

    void setBounds(std::optional<Bounds> bounds);
    void setBounds(std::pair<qreal, qreal> bounds);
    [[nodiscard]] Bounds bounds() const;

    void setAngle(qreal angle);
    [[nodiscard]] qreal angle() const noexcept;

    void setPos(qreal value);
    void setPos(qreal x, qreal y);
    void setPos(const QPointF& pos);
    void setValue(qreal value);

    [[nodiscard]] qreal getXPos() const noexcept;
    [[nodiscard]] qreal getYPos() const noexcept;
    [[nodiscard]] QPointF getPos() const noexcept;
    [[nodiscard]] qreal value() const noexcept;

    void setSpan(qreal min, qreal max);
    [[nodiscard]] std::pair<qreal, qreal> span() const noexcept;

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;
    void setHoverPen(const QPen& pen);
    [[nodiscard]] QPen hoverPen() const;

    void setMouseHover(bool hover);
    [[nodiscard]] bool mouseHovering() const noexcept;

    void setName(const QString& name);
    [[nodiscard]] QString name() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event) override;
    void mouseClickEvent(pyqtgraph::GraphicsScene::MouseClickEvent* event) override;
    void mouseDragEvent(pyqtgraph::GraphicsScene::MouseDragEvent* event) override;

signals:
    void sigDragged(pyqtgraph::graphicsItems::InfiniteLine* line);
    void sigPositionChangeFinished(pyqtgraph::graphicsItems::InfiniteLine* line);
    void sigPositionChanged(pyqtgraph::graphicsItems::InfiniteLine* line);
    void sigClicked(pyqtgraph::graphicsItems::InfiniteLine* line, pyqtgraph::GraphicsScene::MouseClickEvent* event);

private:
    [[nodiscard]] QPointF clampPosition(QPointF pos) const;
    void invalidateBounds();

    qreal angle_ = 0.0;
    bool movable_ = false;
    bool moving_ = false;
    bool mouseHovering_ = false;
    QPointF position_;
    QPointF cursorOffset_;
    QPointF startPosition_;
    Bounds bounds_{std::nullopt, std::nullopt};
    std::pair<qreal, qreal> span_{0.0, 1.0};
    QPen pen_;
    QPen hoverPen_;
    QPen currentPen_;
    QString name_;
};

} // namespace pyqtgraph::graphicsItems
