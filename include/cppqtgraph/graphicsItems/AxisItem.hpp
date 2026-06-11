#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/AxisItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"

#include <QtCore/QLineF>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QFont>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

class QGraphicsSceneMouseEvent;
class QGraphicsSceneResizeEvent;
class QGraphicsSceneWheelEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QVariant;
class QWidget;

namespace cppqtgraph::graphicsItems {

class ViewBox;

class AxisItem : public GraphicsWidget {
public:
    enum class Orientation {
        Left,
        Right,
        Top,
        Bottom,
    };

    struct TickSpacing {
        double spacing = 0.0;
        double offset = 0.0;
    };

    struct TickLevel {
        double spacing = 0.0;
        std::vector<double> values;
    };

    struct ExplicitTick {
        double value = 0.0;
        QString text;
    };

    struct TextSpec {
        QRectF rect;
        Qt::Alignment alignment = Qt::AlignCenter;
        QString text;
    };

    struct DrawSpecs {
        QPen axisPen;
        QLineF axisLine;
        std::vector<std::pair<QPen, QLineF>> ticks;
        std::vector<TextSpec> text;
    };

    explicit AxisItem(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    explicit AxisItem(
        Orientation orientation,
        QGraphicsItem* parent = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags{});
    explicit AxisItem(
        const QString& orientation,
        QGraphicsItem* parent = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags{});
    ~AxisItem() override;

    AxisItem(const AxisItem&) = delete;
    AxisItem& operator=(const AxisItem&) = delete;
    AxisItem(AxisItem&&) = delete;
    AxisItem& operator=(AxisItem&&) = delete;

    [[nodiscard]] Orientation orientation() const noexcept;
    [[nodiscard]] QString orientationName() const;

    void setRange(double minimum, double maximum);
    [[nodiscard]] std::pair<double, double> range() const noexcept;

    void setLabel(
        const QString& text = QString{},
        const QString& units = QString{},
        const QString& unitPrefix = QString{},
        std::optional<std::vector<std::pair<double, double>>> siPrefixEnableRanges = std::nullopt,
        double unitPower = 1.0);
    void showLabel(bool show = true);
    [[nodiscard]] bool labelVisible() const;
    [[nodiscard]] QString labelText() const;
    [[nodiscard]] QString labelUnits() const;
    [[nodiscard]] QString labelString() const;

    void setSIPrefixEnableRanges(std::optional<std::vector<std::pair<double, double>>> ranges);
    [[nodiscard]] std::vector<std::pair<double, double>> siPrefixEnableRanges() const;
    void enableAutoSIPrefix(bool enable = true);
    void updateAutoSIPrefix();
    [[nodiscard]] double autoSIPrefixScale() const noexcept;
    [[nodiscard]] QString labelUnitPrefix() const;

    void setPen(const QPen& pen = QPen(Qt::white));
    [[nodiscard]] QPen pen() const;
    void setTextPen(const QPen& pen = QPen(Qt::white));
    [[nodiscard]] QPen textPen() const;
    void setTickPen(std::optional<QPen> pen = std::nullopt);
    [[nodiscard]] QPen tickPen() const;
    void setTickFont(std::optional<QFont> font);

    void setShowValues(bool showValues);
    void setTickLength(double length);
    void setMaxTickLevel(int level);
    void setMaxTextLevel(int level);
    void setTickDensity(double density);
    void setStopAxisAtTick(bool stopAtMinimumTick, bool stopAtMaximumTick);
    void setHeight(std::optional<double> height = std::nullopt);
    void setWidth(std::optional<double> width = std::nullopt);
    void setScale(double scale = 1.0);
    void setLogMode(bool enabled);
    void setLogMode(bool xEnabled, bool yEnabled);

    void setTicks(const std::vector<std::vector<ExplicitTick>>& ticks);
    void clearTicks();
    void setTickSpacing(std::optional<double> major = std::nullopt, std::optional<double> minor = std::nullopt);
    void setTickSpacingLevels(std::optional<std::vector<TickSpacing>> levels);
    [[nodiscard]] std::vector<TickSpacing> tickSpacing(double minimum, double maximum, double size) const;
    [[nodiscard]] std::vector<TickLevel> tickValues(double minimum, double maximum, double size) const;
    [[nodiscard]] std::vector<QString> tickStrings(
        const std::vector<double>& values,
        double scale,
        double spacing) const;

    [[nodiscard]] std::optional<DrawSpecs> generateDrawSpecs(QPainter& painter) const;

    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void show();
    void hide();

    void linkToView(ViewBox* view);
    void unlinkFromView();
    [[nodiscard]] ViewBox* linkedView() const noexcept;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
    void wheelEvent(QGraphicsSceneWheelEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    [[nodiscard]] int linkedAxisIndex() const noexcept;
    [[nodiscard]] bool shouldIgnoreLinkedViewEvent(const QPointF& scenePos) const;

    void updateSize();
    void updateLabelPosition();

    struct Private;
    std::unique_ptr<Private> d_;

    QPointer<ViewBox> linkedView_;
    bool dragActive_ = false;
    Qt::MouseButton dragButton_ = Qt::NoButton;
    QPointF dragLastPos_;
};

} // namespace cppqtgraph::graphicsItems
