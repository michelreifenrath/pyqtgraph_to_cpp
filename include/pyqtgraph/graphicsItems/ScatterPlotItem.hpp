#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ScatterPlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QBrush>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QGraphicsItem>

#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

void drawSymbol(QPainter& painter, const QString& symbol, qreal size, const QPen& pen, const QBrush& brush);
void drawSymbol(QPainter& painter, const QPainterPath& symbol, qreal size, const QPen& pen, const QBrush& brush);
[[nodiscard]] QImage renderSymbol(const QString& symbol, qreal size, const QPen& pen, const QBrush& brush,
    qreal devicePixelRatio = 1.0);
[[nodiscard]] QImage renderSymbol(const QPainterPath& symbol, qreal size, const QPen& pen, const QBrush& brush,
    qreal devicePixelRatio = 1.0);

class SymbolAtlas {
public:
    SymbolAtlas();
    ~SymbolAtlas();

    SymbolAtlas(const SymbolAtlas&) = delete;
    SymbolAtlas& operator=(const SymbolAtlas&) = delete;
    SymbolAtlas(SymbolAtlas&&) noexcept;
    SymbolAtlas& operator=(SymbolAtlas&&) noexcept;

    void clear();
    void setDevicePixelRatio(qreal devicePixelRatio);
    [[nodiscard]] qreal devicePixelRatio() const noexcept;

    [[nodiscard]] QRect sourceRect(const QString& symbol, qreal size, const QPen& pen, const QBrush& brush);
    [[nodiscard]] const QPixmap& pixmap() const;
    [[nodiscard]] qreal maxWidth() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    class Private;
    std::unique_ptr<Private> d_;
};

class SpotItem {
public:
    SpotItem() = default;
    SpotItem(std::size_t index, QPointF position, qreal size, QString symbol, QPen pen, QBrush brush,
        QVariant data = {});

    [[nodiscard]] std::size_t index() const noexcept;
    [[nodiscard]] QPointF pos() const noexcept;
    [[nodiscard]] qreal size() const noexcept;
    [[nodiscard]] QString symbol() const;
    [[nodiscard]] QPen pen() const;
    [[nodiscard]] QBrush brush() const;
    [[nodiscard]] QVariant data() const;

private:
    std::size_t index_ = 0;
    QPointF position_;
    qreal size_ = 0.0;
    QString symbol_;
    QPen pen_;
    QBrush brush_;
    QVariant data_;
};

class ScatterPlotItem : public GraphicsObject {
    Q_OBJECT

public:
    explicit ScatterPlotItem(QGraphicsItem* parent = nullptr);
    explicit ScatterPlotItem(std::span<const double> y, QGraphicsItem* parent = nullptr);
    ScatterPlotItem(std::span<const double> x, std::span<const double> y, QGraphicsItem* parent = nullptr);
    ~ScatterPlotItem() override;

    ScatterPlotItem(const ScatterPlotItem&) = delete;
    ScatterPlotItem& operator=(const ScatterPlotItem&) = delete;
    ScatterPlotItem(ScatterPlotItem&&) = delete;
    ScatterPlotItem& operator=(ScatterPlotItem&&) = delete;

    void setData();
    void setData(std::span<const double> y);
    void setData(std::span<const double> x, std::span<const double> y);
    void addPoints(std::span<const double> x, std::span<const double> y);
    void addPoints(std::span<const QPointF> points);
    void clear();

    [[nodiscard]] bool hasData() const noexcept;
    [[nodiscard]] std::span<const double> xData() const noexcept;
    [[nodiscard]] std::span<const double> yData() const noexcept;
    [[nodiscard]] std::pair<std::span<const double>, std::span<const double>> getData() const noexcept;

    void setPen(const QPen& pen);
    void setPen(std::nullptr_t);
    [[nodiscard]] QPen pen() const;
    void setPens(std::span<const QPen> pens);

    void setBrush(const QBrush& brush);
    void setBrush(std::nullptr_t);
    [[nodiscard]] QBrush brush() const;
    void setBrushes(std::span<const QBrush> brushes);

    void setSymbol(const QString& symbol);
    [[nodiscard]] QString symbol() const;
    void setSymbols(std::span<const QString> symbols);

    void setSize(qreal size);
    [[nodiscard]] qreal size() const noexcept;
    void setSizes(std::span<const qreal> sizes);

    void setPointData(std::span<const QVariant> data);

    void setPxMode(bool enabled);
    [[nodiscard]] bool pxMode() const noexcept;
    void setUseCache(bool enabled);
    [[nodiscard]] bool useCache() const noexcept;
    void setAntialias(bool enabled);
    [[nodiscard]] bool antialias() const noexcept;
    void setCompositionMode(QPainter::CompositionMode mode);
    void clearCompositionMode();

    void setName(const QString& name);
    [[nodiscard]] QString name() const;

    [[nodiscard]] std::vector<SpotItem> points() const;
    [[nodiscard]] std::vector<SpotItem> pointsAt(const QPointF& position) const;
    [[nodiscard]] std::vector<SpotItem> pointsAt(const QRectF& rect) const;

    [[nodiscard]] qreal pixelPadding() const noexcept;
    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void sigPlotChanged(pyqtgraph::graphicsItems::ScatterPlotItem* item);

private:
    struct Spot;

    [[nodiscard]] qreal effectiveSize(const Spot& spot) const noexcept;
    [[nodiscard]] QString effectiveSymbol(const Spot& spot) const;
    [[nodiscard]] QPen effectivePen(const Spot& spot) const;
    [[nodiscard]] QBrush effectiveBrush(const Spot& spot) const;
    [[nodiscard]] SpotItem makeSpotItem(std::size_t index) const;

    void resetPerSpotStyles();
    void markSpotsDirty();
    void updateSpots();
    void refreshBounds();
    [[nodiscard]] bool maskContains(const Spot& spot, const QRectF& rect) const;
    [[nodiscard]] qreal devicePixelRatioFor(QWidget* widget, const QPainter* painter) const;

    std::vector<double> xData_;
    std::vector<double> yData_;
    std::vector<Spot> spots_;
    QRectF bounds_;
    SymbolAtlas fragmentAtlas_;
    qreal maxSpotWidth_ = 0.0;
    qreal maxSpotPxWidth_ = 0.0;
    bool pxMode_ = true;
    bool useCache_ = true;
    bool antialias_ = false;
    bool hasCompositionMode_ = false;
    QPainter::CompositionMode compositionMode_ = QPainter::CompositionMode_SourceOver;
    QString defaultSymbol_ = QStringLiteral("o");
    qreal defaultSize_ = 7.0;
    QPen defaultPen_;
    QBrush defaultBrush_;
    QString name_;
};

} // namespace pyqtgraph::graphicsItems
