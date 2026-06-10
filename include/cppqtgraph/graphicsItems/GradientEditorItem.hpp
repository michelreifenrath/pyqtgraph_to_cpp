#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GradientEditorItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"

#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <cppqtgraph/colormap.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

#include <cstddef>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

class QGraphicsRectItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class TickSliderItem;

class Tick final : public GraphicsWidget, public cppqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    Q_OBJECT

public:
    Tick(const QPointF& pos,
         const QColor& color,
         bool movable,
         qreal scale,
         const QPen& pen,
         bool removeAllowed,
         TickSliderItem* sliderItem);

    [[nodiscard]] QColor color() const noexcept { return color_; }
    void setColor(const QColor& color);

    [[nodiscard]] bool movable() const noexcept { return movable_; }
    [[nodiscard]] bool removeAllowed() const noexcept { return removeAllowed_; }

    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event) override;
    void mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event) override;
    void mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event) override;

signals:
    void sigMoving(Tick* tick, const QPointF& pos);
    void sigMoved(Tick* tick);
    void sigClicked(Tick* tick, cppqtgraph::GraphicsScene::MouseClickEvent* event);

private:
    TickSliderItem* sliderItem_;
    QColor color_;
    QPen pen_;
    QPen hoverPen_;
    QPen currentPen_;
    QPainterPath path_;
    qreal scale_ = 10.0;
    bool movable_ = true;
    bool moving_ = false;
    bool removeAllowed_ = true;
    QPointF cursorOffset_;
    QPointF startPosition_;
};

class TickSliderItem : public GraphicsWidget, public cppqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    Q_OBJECT

public:
    explicit TickSliderItem(const QString& orientation = QStringLiteral("bottom"),
                            bool allowAdd = true,
                            bool allowRemove = true,
                            QGraphicsItem* parent = nullptr);

    [[nodiscard]] qreal length() const noexcept { return length_; }
    [[nodiscard]] qreal tickSize() const noexcept { return tickSize_; }
    [[nodiscard]] bool allowAdd() const noexcept { return allowAdd_; }
    [[nodiscard]] bool allowRemove() const noexcept { return allowRemove_; }

    void setAllowAdd(bool allowAdd) noexcept { allowAdd_ = allowAdd; }
    void setAllowRemove(bool allowRemove) noexcept { allowRemove_ = allowRemove; }

    virtual Tick* addTick(double fraction,
                          const QColor& color = QColor(),
                          bool movable = true,
                          bool finish = true);
    void removeTick(Tick* tick, bool finish = true);

    [[nodiscard]] std::vector<std::pair<Tick*, double>> listTicks() const;
    [[nodiscard]] double tickValue(Tick* tick) const;
    [[nodiscard]] Tick* tickAt(std::size_t index) const;
    [[nodiscard]] std::size_t tickCount() const noexcept { return ticks_.size(); }

    void setTickValue(Tick* tick, double value);
    virtual void setLength(qreal newLen);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event) override;
    void mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event) override;

signals:
    void sigTicksChanged(TickSliderItem* item);
    void sigTicksChangeFinished(TickSliderItem* item);

protected:
    void tickMoved(Tick* tick, const QPointF& pos);
    void tickMoveFinished(Tick* tick);
    virtual void tickClicked(Tick* tick, cppqtgraph::GraphicsScene::MouseClickEvent* event);

    [[nodiscard]] Tick* getTick(std::variant<Tick*, int> tick) const;

    qreal length_ = 100.0;
    qreal tickSize_ = 15.0;
    qreal maxDim_ = 20.0;
    QString orientation_{QStringLiteral("bottom")};
    bool allowAdd_ = true;
    bool allowRemove_ = true;
    QPen tickPen_{Qt::white};

private:
    void connectTick(Tick* tick);
    void setTickFraction(Tick* tick, double fraction) noexcept;
    void schedulePendingRemovalFlush();
    void flushPendingRemovedTicks();

    std::vector<std::pair<std::unique_ptr<Tick>, double>> ticks_;
    std::vector<std::unique_ptr<Tick>> pendingRemovedTicks_;
    bool pendingRemovalFlushScheduled_ = false;
};

struct GradientEditorState final {
    QString mode{QStringLiteral("rgb")};
    std::vector<std::pair<double, QColor>> ticks;
    bool ticksVisible = true;
};

class GradientEditorItem final : public TickSliderItem {
    Q_OBJECT

public:
    explicit GradientEditorItem(const QString& orientation = QStringLiteral("bottom"),
                                bool allowAdd = true,
                                bool allowRemove = true,
                                QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString colorMode() const noexcept { return colorMode_; }
    void setColorMode(const QString& mode);

    [[nodiscard]] QColor getColor(double fraction, bool toQColor = true) const;
    [[nodiscard]] cppqtgraph::ColorMap colorMap() const;
    [[nodiscard]] QLinearGradient getGradient() const;

    [[nodiscard]] GradientEditorState saveState() const;
    void restoreState(const GradientEditorState& state);

    void showTicks(bool show = true);

    Tick* addTick(double fraction,
                  const QColor& color = QColor(),
                  bool movable = true,
                  bool finish = true) override;

    void setLength(qreal newLen) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void sigGradientChanged(GradientEditorItem* item);
    void sigGradientChangeFinished(GradientEditorItem* item);

protected:
    void tickClicked(Tick* tick, cppqtgraph::GraphicsScene::MouseClickEvent* event) override;

private:
    void updateGradient();
    void initializeDefaultTicks();

    qreal rectSize_ = 15.0;
    QString colorMode_{QStringLiteral("rgb")};
    QGraphicsRectItem* backgroundRect_ = nullptr;
    QGraphicsRectItem* gradRect_ = nullptr;
    bool allowAddBackup_ = true;
};

} // namespace cppqtgraph::graphicsItems
