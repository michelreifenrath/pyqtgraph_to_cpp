#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PColorMeshItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "cppqtgraph/colormap.hpp"
#include "cppqtgraph/core/ArrayView.hpp"

#include <QtCore/QPair>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPolygonF>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

class QGraphicsItem;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

struct PColorMeshCell {
    std::size_t row = 0;
    std::size_t column = 0;
    double value = 0.0;
    std::size_t colorIndex = 0;
    QColor color;
    QPolygonF polygon;
};

class PColorMeshItem : public GraphicsObject {
    Q_OBJECT

public:
    explicit PColorMeshItem(QGraphicsItem* parent = nullptr);
    explicit PColorMeshItem(core::ArrayView<const double, 2> z, QGraphicsItem* parent = nullptr);
    PColorMeshItem(core::ArrayView<const double, 2> x,
        core::ArrayView<const double, 2> y,
        core::ArrayView<const double, 2> z,
        QGraphicsItem* parent = nullptr);
    ~PColorMeshItem() override = default;

    PColorMeshItem(const PColorMeshItem&) = delete;
    PColorMeshItem& operator=(const PColorMeshItem&) = delete;
    PColorMeshItem(PColorMeshItem&&) = delete;
    PColorMeshItem& operator=(PColorMeshItem&&) = delete;

    void clear();
    void setData(core::ArrayView<const double, 2> z, bool autoLevels = false);
    void setData(core::ArrayView<const double, 2> x,
        core::ArrayView<const double, 2> y,
        core::ArrayView<const double, 2> z,
        bool autoLevels = false);

    void setLevels(std::pair<double, double> levels, bool update = true);
    [[nodiscard]] std::optional<std::pair<double, double>> getLevels() const noexcept;
    void setLookupTable(std::span<const QColor> lut, bool update = true);
    void setLookupTable(std::vector<QColor> lut, bool update = true);
    [[nodiscard]] const std::vector<QColor>& lookupTable() const noexcept;
    void setColorMap(const cppqtgraph::ColorMap& colorMap);
    void enableAutoLevels() noexcept;
    void disableAutoLevels() noexcept;
    [[nodiscard]] bool autoLevelsEnabled() const noexcept;

    void setEdgePen(std::optional<QPen> pen);
    [[nodiscard]] std::optional<QPen> edgePen() const;
    void setAntialiasing(bool enabled) noexcept;
    [[nodiscard]] bool antialiasing() const noexcept;

    [[nodiscard]] bool hasData() const noexcept;
    [[nodiscard]] std::size_t zRows() const noexcept;
    [[nodiscard]] std::size_t zCols() const noexcept;
    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;
    [[nodiscard]] qreal width() const noexcept;
    [[nodiscard]] qreal height() const noexcept;
    [[nodiscard]] qreal pixelPadding() const noexcept;
    [[nodiscard]] QRectF boundingRect() const override;
    [[nodiscard]] std::vector<PColorMeshCell> renderedCells() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void sigLevelsChanged(QPair<double, double> levels);

private:
    [[nodiscard]] static std::vector<QColor> defaultLookupTable();
    [[nodiscard]] static std::vector<double> copy2d(core::ArrayView<const double, 2> view);
    [[nodiscard]] static std::size_t offset(std::size_t row, std::size_t column, std::size_t columns) noexcept;
    [[nodiscard]] double xAt(std::size_t row, std::size_t column) const noexcept;
    [[nodiscard]] double yAt(std::size_t row, std::size_t column) const noexcept;
    [[nodiscard]] double zAt(std::size_t row, std::size_t column) const noexcept;
    [[nodiscard]] std::size_t colorIndex(double value) const;
    void updateAutoLevels(bool autoLevels);
    void refreshBounds();
    void ensureLookupTable() const;

    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    std::vector<double> x_;
    std::vector<double> y_;
    std::vector<double> z_;
    QRectF dataBounds_;
    std::optional<std::pair<double, double>> levels_;
    bool defaultAutoLevels_ = true;
    mutable std::vector<QColor> lookupTable_;
    std::optional<QPen> edgePen_;
    bool antialiasing_ = false;
};

namespace detail_pcolormeshitem {

inline qreal cosmeticPenWidth(const QPen& pen)
{
    if (pen.style() == Qt::NoPen) {
        return 0.0;
    }
    return pen.widthF() > 0.0 ? pen.widthF() : 1.0;
}

inline bool finite(double value) noexcept
{
    return std::isfinite(value);
}

} // namespace detail_pcolormeshitem

inline PColorMeshItem::PColorMeshItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , lookupTable_(defaultLookupTable())
{
}

inline PColorMeshItem::PColorMeshItem(core::ArrayView<const double, 2> z, QGraphicsItem* parent)
    : PColorMeshItem(parent)
{
    setData(z);
}

inline PColorMeshItem::PColorMeshItem(
    core::ArrayView<const double, 2> x, core::ArrayView<const double, 2> y, core::ArrayView<const double, 2> z, QGraphicsItem* parent)
    : PColorMeshItem(parent)
{
    setData(x, y, z);
}

inline void PColorMeshItem::clear()
{
    prepareGeometryChange();
    rows_ = 0;
    cols_ = 0;
    x_.clear();
    y_.clear();
    z_.clear();
    dataBounds_ = QRectF();
    update();
}

inline void PColorMeshItem::setData(core::ArrayView<const double, 2> z, bool autoLevels)
{
    prepareGeometryChange();
    rows_ = z.shape()[0];
    cols_ = z.shape()[1];
    z_ = copy2d(z);
    x_.assign((rows_ + 1) * (cols_ + 1), 0.0);
    y_.assign((rows_ + 1) * (cols_ + 1), 0.0);
    for (std::size_t row = 0; row <= rows_; ++row) {
        for (std::size_t col = 0; col <= cols_; ++col) {
            x_[offset(row, col, cols_ + 1)] = static_cast<double>(row);
            y_[offset(row, col, cols_ + 1)] = static_cast<double>(col);
        }
    }
    refreshBounds();
    updateAutoLevels(autoLevels);
    update();
}

inline void PColorMeshItem::setData(
    core::ArrayView<const double, 2> x, core::ArrayView<const double, 2> y, core::ArrayView<const double, 2> z, bool autoLevels)
{
    const std::size_t zRows = z.shape()[0];
    const std::size_t zCols = z.shape()[1];
    if (x.shape()[0] != zRows + 1 || x.shape()[1] != zCols + 1) {
        throw std::invalid_argument("The dimension of x should be one greater than the one of z");
    }
    if (y.shape()[0] != zRows + 1 || y.shape()[1] != zCols + 1) {
        throw std::invalid_argument("The dimension of y should be one greater than the one of z");
    }

    prepareGeometryChange();
    rows_ = zRows;
    cols_ = zCols;
    x_ = copy2d(x);
    y_ = copy2d(y);
    z_ = copy2d(z);
    refreshBounds();
    updateAutoLevels(autoLevels);
    update();
}

inline void PColorMeshItem::setLevels(std::pair<double, double> levels, bool updateItem)
{
    if (!std::isfinite(levels.first) || !std::isfinite(levels.second)) {
        throw std::invalid_argument("PColorMeshItem levels must be finite");
    }
    levels_ = levels;
    emit sigLevelsChanged(QPair<double, double>(levels.first, levels.second));
    if (updateItem) {
        update();
    }
}

inline std::optional<std::pair<double, double>> PColorMeshItem::getLevels() const noexcept
{
    return levels_;
}

inline void PColorMeshItem::setLookupTable(std::span<const QColor> lut, bool updateItem)
{
    if (lut.empty()) {
        throw std::invalid_argument("PColorMeshItem lookup table must contain at least one color");
    }
    lookupTable_.assign(lut.begin(), lut.end());
    if (updateItem) {
        update();
    }
}

inline void PColorMeshItem::setLookupTable(std::vector<QColor> lut, bool updateItem)
{
    setLookupTable(std::span<const QColor>(lut.data(), lut.size()), updateItem);
}

inline const std::vector<QColor>& PColorMeshItem::lookupTable() const noexcept
{
    return lookupTable_;
}

inline void PColorMeshItem::setColorMap(const cppqtgraph::ColorMap& colorMap)
{
    const auto table = colorMap.getLookupTable(0.0, 1.0, 256, true, cppqtgraph::ColorMap::OutputMode::QColor);
    setLookupTable(table.colors);
}

inline void PColorMeshItem::enableAutoLevels() noexcept
{
    defaultAutoLevels_ = true;
}

inline void PColorMeshItem::disableAutoLevels() noexcept
{
    defaultAutoLevels_ = false;
}

inline bool PColorMeshItem::autoLevelsEnabled() const noexcept
{
    return defaultAutoLevels_;
}

inline void PColorMeshItem::setEdgePen(std::optional<QPen> pen)
{
    if (pen.has_value()) {
        pen->setCosmetic(true);
    }
    const qreal oldPadding = pixelPadding();
    const qreal newPadding = (pen.has_value() && pen->style() != Qt::NoPen) ? detail_pcolormeshitem::cosmeticPenWidth(*pen) * 0.5 : 0.0;
    if (oldPadding != newPadding) {
        prepareGeometryChange();
    }
    edgePen_ = std::move(pen);
    update();
}

inline std::optional<QPen> PColorMeshItem::edgePen() const
{
    return edgePen_;
}

inline void PColorMeshItem::setAntialiasing(bool enabled) noexcept
{
    antialiasing_ = enabled;
    update();
}

inline bool PColorMeshItem::antialiasing() const noexcept
{
    return antialiasing_;
}

inline bool PColorMeshItem::hasData() const noexcept
{
    return !z_.empty();
}

inline std::size_t PColorMeshItem::zRows() const noexcept
{
    return rows_;
}

inline std::size_t PColorMeshItem::zCols() const noexcept
{
    return cols_;
}

inline std::pair<qreal, qreal> PColorMeshItem::dataBounds(int axis) const
{
    if (!hasData()) {
        return {0.0, 0.0};
    }
    if (axis == 0) {
        return {dataBounds_.left(), dataBounds_.right()};
    }
    return {dataBounds_.top(), dataBounds_.bottom()};
}

inline qreal PColorMeshItem::width() const noexcept
{
    return hasData() ? dataBounds_.width() : 0.0;
}

inline qreal PColorMeshItem::height() const noexcept
{
    return hasData() ? dataBounds_.height() : 0.0;
}

inline qreal PColorMeshItem::pixelPadding() const noexcept
{
    if (!edgePen_.has_value() || edgePen_->style() == Qt::NoPen) {
        return 0.0;
    }
    return detail_pcolormeshitem::cosmeticPenWidth(*edgePen_) * 0.5;
}

inline QRectF PColorMeshItem::boundingRect() const
{
    if (!hasData()) {
        return QRectF();
    }
    return dataBounds_.adjusted(-pixelPadding(), -pixelPadding(), pixelPadding(), pixelPadding());
}

inline std::vector<PColorMeshCell> PColorMeshItem::renderedCells() const
{
    std::vector<PColorMeshCell> cells;
    if (!hasData() || !levels_.has_value()) {
        return cells;
    }
    ensureLookupTable();
    cells.reserve(rows_ * cols_);
    for (std::size_t row = 0; row < rows_; ++row) {
        for (std::size_t col = 0; col < cols_; ++col) {
            const double value = zAt(row, col);
            if (std::isnan(value)) {
                continue;
            }
            PColorMeshCell cell;
            cell.row = row;
            cell.column = col;
            cell.value = value;
            cell.colorIndex = colorIndex(value);
            cell.color = lookupTable_[cell.colorIndex];
            cell.polygon << QPointF(xAt(row, col), yAt(row, col)) << QPointF(xAt(row + 1, col), yAt(row + 1, col))
                         << QPointF(xAt(row + 1, col + 1), yAt(row + 1, col + 1)) << QPointF(xAt(row, col + 1), yAt(row, col + 1));
            cells.push_back(std::move(cell));
        }
    }
    return cells;
}

inline void PColorMeshItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (painter == nullptr || !hasData()) {
        return;
    }
    if (edgePen_.has_value()) {
        painter->setPen(*edgePen_);
        if (antialiasing_) {
            painter->setRenderHint(QPainter::Antialiasing, true);
        }
    } else {
        painter->setPen(Qt::NoPen);
    }
    for (const PColorMeshCell& cell : renderedCells()) {
        painter->setBrush(QBrush(cell.color));
        painter->drawConvexPolygon(cell.polygon);
    }
}

inline std::vector<QColor> PColorMeshItem::defaultLookupTable()
{
    if (const std::optional<cppqtgraph::ColorMap> viridis = cppqtgraph::get(QStringLiteral("viridis")); viridis.has_value()) {
        return viridis->getLookupTable(0.0, 1.0, 256, true, cppqtgraph::ColorMap::OutputMode::QColor).colors;
    }
    return cppqtgraph::ColorMap({0.0, 0.25, 0.5, 0.75, 1.0},
        {QColor(68, 1, 84), QColor(59, 82, 139), QColor(33, 145, 140), QColor(94, 201, 98), QColor(253, 231, 37)},
        QStringLiteral("viridis-fallback"))
        .getLookupTable(0.0, 1.0, 256, true, cppqtgraph::ColorMap::OutputMode::QColor)
        .colors;
}

inline std::vector<double> PColorMeshItem::copy2d(core::ArrayView<const double, 2> view)
{
    std::vector<double> result;
    result.reserve(view.shape()[0] * view.shape()[1]);
    for (std::size_t row = 0; row < view.shape()[0]; ++row) {
        for (std::size_t col = 0; col < view.shape()[1]; ++col) {
            result.push_back(view(row, col));
        }
    }
    return result;
}

inline std::size_t PColorMeshItem::offset(std::size_t row, std::size_t column, std::size_t columns) noexcept
{
    return (row * columns) + column;
}

inline double PColorMeshItem::xAt(std::size_t row, std::size_t column) const noexcept
{
    return x_[offset(row, column, cols_ + 1)];
}

inline double PColorMeshItem::yAt(std::size_t row, std::size_t column) const noexcept
{
    return y_[offset(row, column, cols_ + 1)];
}

inline double PColorMeshItem::zAt(std::size_t row, std::size_t column) const noexcept
{
    return z_[offset(row, column, cols_)];
}

inline std::size_t PColorMeshItem::colorIndex(double value) const
{
    ensureLookupTable();
    if (!levels_.has_value()) {
        return 0;
    }
    const auto [low, high] = *levels_;
    double range = high - low;
    if (range == 0.0) {
        range = 1.0;
    }
    const double scaled = (value - low) * (static_cast<double>(lookupTable_.size() - 1) / range);
    const double clipped = std::clamp(scaled, 0.0, static_cast<double>(lookupTable_.size() - 1));
    return static_cast<std::size_t>(std::trunc(clipped));
}

inline void PColorMeshItem::updateAutoLevels(bool autoLevels)
{
    bool anyFinite = false;
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    for (const double value : z_) {
        if (detail_pcolormeshitem::finite(value)) {
            anyFinite = true;
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
    }
    if (anyFinite && (!levels_.has_value() || autoLevels || defaultAutoLevels_)) {
        setLevels({minimum, maximum}, false);
    }
}

inline void PColorMeshItem::refreshBounds()
{
    if (!hasData()) {
        dataBounds_ = QRectF();
        return;
    }
    const auto [minXIt, maxXIt] = std::minmax_element(x_.begin(), x_.end());
    const auto [minYIt, maxYIt] = std::minmax_element(y_.begin(), y_.end());
    dataBounds_ = QRectF(QPointF(*minXIt, *minYIt), QPointF(*maxXIt, *maxYIt)).normalized();
}

inline void PColorMeshItem::ensureLookupTable() const
{
    if (lookupTable_.empty()) {
        lookupTable_ = defaultLookupTable();
    }
}

} // namespace cppqtgraph::graphicsItems
