#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/IsocurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "cppqtgraph/core/ArrayView.hpp"
#include "cppqtgraph/functions.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class IsocurveItem : public GraphicsObject {
public:
    enum class AxisOrder {
        ColMajor,
        RowMajor,
    };

    explicit IsocurveItem(QGraphicsItem* parent = nullptr);
    IsocurveItem(core::ArrayView<const double, 2> data,
        double level = 0.0,
        const QPen& pen = cppqtgraph::mkPen('w'),
        AxisOrder axisOrder = AxisOrder::ColMajor,
        QGraphicsItem* parent = nullptr);
    ~IsocurveItem() override = default;

    IsocurveItem(const IsocurveItem&) = delete;
    IsocurveItem& operator=(const IsocurveItem&) = delete;
    IsocurveItem(IsocurveItem&&) = delete;
    IsocurveItem& operator=(IsocurveItem&&) = delete;

    void clear();
    void setData(core::ArrayView<const double, 2> data);
    void setData(core::ArrayView<const double, 2> data, double level);
    void updateLines(core::ArrayView<const double, 2> data, double level);
    void setLevel(double level);
    [[nodiscard]] double level() const noexcept;

    void setPen(const QPen& pen);
    void setPen(const QColor& color);
    void setPen(const QString& color);
    void setPen(const char* color);
    void setPen(char color);
    void setPen(int colorIndex);
    [[nodiscard]] QPen pen() const;

    void setBrush(const QBrush& brush);
    void setBrush(const QColor& color);
    [[nodiscard]] QBrush brush() const;

    void setAxisOrder(AxisOrder axisOrder);
    [[nodiscard]] AxisOrder axisOrder() const noexcept;

    [[nodiscard]] bool hasData() const noexcept;
    [[nodiscard]] std::size_t rows() const noexcept;
    [[nodiscard]] std::size_t cols() const noexcept;

    // Public geometry accessors are provided for C++ oracle/unit tests. They
    // expose IsocurveItem.generatePath() output from PyQtGraph, not a general
    // reusable fn.isocurve API. Unsupported Python varargs/kwargs are mapped to
    // typed C++ QPen/QBrush overloads.
    [[nodiscard]] const QPainterPath& path() const;
    [[nodiscard]] const std::vector<std::vector<QPointF>>& isocurves() const;
    [[nodiscard]] const std::vector<std::vector<QPointF>>& pathLines() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    [[nodiscard]] static std::vector<double> copy2d(core::ArrayView<const double, 2> view);
    [[nodiscard]] static std::size_t offset(std::size_t row, std::size_t column, std::size_t columns) noexcept;
    [[nodiscard]] double valueAt(std::size_t row, std::size_t column) const noexcept;
    [[nodiscard]] double logicalValueAt(std::size_t row, std::size_t column) const noexcept;
    [[nodiscard]] std::size_t logicalRows() const noexcept;
    [[nodiscard]] std::size_t logicalCols() const noexcept;
    void invalidate();
    void ensurePath() const;
    [[nodiscard]] std::vector<std::vector<QPointF>> generateIsocurves() const;

    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    std::vector<double> data_;
    double level_ = 0.0;
    QPen pen_ = cppqtgraph::mkPen('w');
    QBrush brush_;
    AxisOrder axisOrder_ = AxisOrder::ColMajor;
    mutable bool pathValid_ = false;
    mutable QPainterPath path_;
    mutable std::vector<std::vector<QPointF>> pathLines_;
};

} // namespace cppqtgraph::graphicsItems
