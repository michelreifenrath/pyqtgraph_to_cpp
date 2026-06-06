#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"
#include "../ViewBox/ViewBox.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

class QGraphicsGridLayout;
class QGraphicsSceneResizeEvent;
class QGraphicsTextItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QVariant;
class QWidget;

namespace pyqtgraph::graphicsItems {

class AxisItem;
class LegendItem;
class PlotCurveItem;

class PlotItem : public GraphicsWidget {
public:
    explicit PlotItem(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~PlotItem() override;

    PlotItem(const PlotItem&) = delete;
    PlotItem& operator=(const PlotItem&) = delete;
    PlotItem(PlotItem&&) = delete;
    PlotItem& operator=(PlotItem&&) = delete;

    [[nodiscard]] ViewBox* getViewBox() noexcept;
    [[nodiscard]] const ViewBox* getViewBox() const noexcept;
    [[nodiscard]] AxisItem* getAxis(const QString& name);
    [[nodiscard]] const AxisItem* getAxis(const QString& name) const;

    void addItem(QGraphicsItem* item, bool ignoreBounds = false, const QString& name = QString{});
    void removeItem(QGraphicsItem* item);
    void clear();
    [[nodiscard]] std::vector<QGraphicsItem*> listDataItems() const;

    PlotCurveItem* plot(std::span<const double> y, const QString& name = QString{}, const QPen& pen = QPen());
    PlotCurveItem* plot(
        std::span<const double> x, std::span<const double> y, const QString& name = QString{}, const QPen& pen = QPen());

    LegendItem* addLegend(const QPointF& offset = QPointF(30.0, 30.0));
    [[nodiscard]] LegendItem* legend() noexcept;
    [[nodiscard]] const LegendItem* legend() const noexcept;

    void setLabel(const QString& axis,
                  const QString& text = QString{},
                  const QString& units = QString{},
                  const QString& unitPrefix = QString{});
    void setTitle(const QString& title = QString{});
    void showAxis(const QString& axis, bool show = true);
    void hideAxis(const QString& axis);

    void setXRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void setYRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void setRange(std::optional<ViewBox::AxisRange> xRange,
                  std::optional<ViewBox::AxisRange> yRange,
                  qreal padding = 0.02,
                  bool update = true,
                  bool disableAutoRange = true);
    void autoRange(std::optional<qreal> padding = std::nullopt);
    [[nodiscard]] QRectF viewRect() const;
    [[nodiscard]] ViewBox::Range2D viewRange() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    friend class PlotCurveItem;

    enum class AxisSlot : std::size_t {
        Left = 0,
        Bottom = 1,
        Right = 2,
        Top = 3,
    };

    [[nodiscard]] AxisSlot axisSlot(const QString& name) const;
    [[nodiscard]] AxisItem* axis(AxisSlot slot) noexcept;
    [[nodiscard]] const AxisItem* axis(AxisSlot slot) const noexcept;
    [[nodiscard]] bool isChromeItem(const QGraphicsItem* item) const noexcept;
    [[nodiscard]] QString itemName(QGraphicsItem* item) const;
    void recordItemName(QGraphicsItem* item, const QString& name);
    void updateCurveTransforms();
    void updateAxisRanges();
    void updateTitleGeometry();

    QGraphicsGridLayout* layout_ = nullptr;
    ViewBox* viewBox_ = nullptr;
    std::array<AxisItem*, 4> axes_{{nullptr, nullptr, nullptr, nullptr}};
    QGraphicsTextItem* titleItem_ = nullptr;
    LegendItem* legend_ = nullptr;
    std::vector<QGraphicsItem*> items_;
    std::vector<QGraphicsItem*> dataItems_;
    std::vector<std::pair<QGraphicsItem*, QString>> itemNames_;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
};

} // namespace pyqtgraph::graphicsItems
