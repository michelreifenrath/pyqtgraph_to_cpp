#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"
#include "../ViewBox/ViewBox.hpp"

#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtWidgets/QGraphicsItem>

#include <array>
#include <optional>
#include <vector>

class QGraphicsSceneResizeEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QVariant;
class QWidget;

namespace pyqtgraph::graphicsItems {

class AxisItem;
class LegendItem;

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
    [[nodiscard]] AxisItem* getAxis(const QString& name) const;

    void showAxis(const QString& axis, bool show = true);
    void hideAxis(const QString& axis);
    void setLabel(
        const QString& axis,
        const QString& text = QString{},
        const QString& units = QString{},
        const QString& unitPrefix = QString{});

    void addItem(QGraphicsItem* item, bool ignoreBounds = false, const QString& name = QString{});
    void removeItem(QGraphicsItem* item);
    void clear();
    [[nodiscard]] std::vector<QGraphicsItem*> items() const;
    [[nodiscard]] std::vector<QGraphicsItem*> listDataItems() const;

    [[nodiscard]] LegendItem* addLegend(const QPointF& offset = QPointF(30.0, 30.0));
    [[nodiscard]] LegendItem* legend() const noexcept;

    void setXRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void setYRange(qreal minimum, qreal maximum, qreal padding = 0.02, bool update = true);
    void setRange(const QRectF& rect, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);
    void autoRange(std::optional<qreal> padding = std::nullopt);
    [[nodiscard]] QRectF viewRect() const;
    [[nodiscard]] ViewBox::Range2D viewRange() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    friend class PlotCurveItem;

    enum AxisIndex : std::size_t {
        Top = 0,
        Bottom = 1,
        Left = 2,
        Right = 3,
    };

    void updateCurveTransforms();
    void initializeLayout();
    void initializeAxes();
    void connectAxesToViewBox();
    [[nodiscard]] static std::optional<AxisIndex> axisIndex(const QString& name);
    [[nodiscard]] bool isManagedInternalItem(QGraphicsItem* item) const noexcept;
    [[nodiscard]] bool ignoresBounds(QGraphicsItem* item) const noexcept;
    [[nodiscard]] bool isPlotDataCurve(QGraphicsItem* item) const noexcept;
    void registerDirectDataChildren();
    void registerDataItem(QGraphicsItem* item);
    void unregisterDataItem(QGraphicsItem* item);
    void setDataItemTransform(QGraphicsItem* item, const QTransform& transform);
    void resetDataItemTransform(QGraphicsItem* item);
    void refreshDataItemTransforms(bool applyAutoRange, bool disableAutoRange = false, std::optional<qreal> padding = std::nullopt);

    ViewBox* viewBox_ = nullptr;
    std::array<AxisItem*, 4> axes_{{nullptr, nullptr, nullptr, nullptr}};
    LegendItem* legend_ = nullptr;
    std::vector<QGraphicsItem*> items_;
    std::vector<QGraphicsItem*> dataItems_;
    std::vector<QGraphicsItem*> ignoredBoundsItems_;
    bool routingDirectChild_ = false;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
};

} // namespace pyqtgraph::graphicsItems
