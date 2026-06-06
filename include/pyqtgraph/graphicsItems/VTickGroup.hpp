#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/VTickGroup.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

#include <array>
#include <span>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class ViewBox;

class VTickGroup : public GraphicsObject {
public:
    explicit VTickGroup(QGraphicsItem* parent = nullptr);
    explicit VTickGroup(std::vector<double> xvals,
                        std::array<qreal, 2> yrange = {0.0, 1.0},
                        QPen pen = QPen(QColor(200, 200, 200)),
                        QGraphicsItem* parent = nullptr);
    ~VTickGroup() override;

    VTickGroup(const VTickGroup&) = delete;
    VTickGroup& operator=(const VTickGroup&) = delete;
    VTickGroup(VTickGroup&&) = delete;
    VTickGroup& operator=(VTickGroup&&) = delete;

    [[nodiscard]] QPen pen() const;
    void setPen(const QPen& pen);
    [[nodiscard]] const std::vector<double>& xValues() const noexcept;
    void setXVals(std::span<const double> values);
    void setXVals(std::initializer_list<double> values);
    [[nodiscard]] std::array<qreal, 2> yRange() const noexcept;
    void setYRange(std::array<qreal, 2> values);
    [[nodiscard]] const QPainterPath& path() const noexcept;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    [[nodiscard]] ViewBox* viewBox() const;
    void rebuildTicks();

    QPen pen_;
    std::vector<double> xvals_;
    std::array<qreal, 2> yrange_{{0.0, 1.0}};
    QPainterPath path_;
};

} // namespace pyqtgraph::graphicsItems
