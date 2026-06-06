#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/BarGraphItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pyqtgraph::graphicsItems {

struct BarGraphItemOptions {
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> x0;
    std::vector<double> y0;
    std::vector<double> x1;
    std::vector<double> y1;
    std::vector<double> width;
    std::vector<double> height;
    QPen pen = [] {
        QPen defaultPen(QColor(255, 255, 255), 1.0);
        defaultPen.setCosmetic(true);
        return defaultPen;
    }();
    QBrush brush = QBrush(QColor(128, 128, 128));
    std::vector<QPen> pens;
    std::vector<QBrush> brushes;
    QString name;
};

class BarGraphItem : public GraphicsObject {
public:
    explicit BarGraphItem(QGraphicsItem* parent = nullptr);
    BarGraphItem(std::span<const double> x, std::span<const double> height, double width, QGraphicsItem* parent = nullptr);
    explicit BarGraphItem(const BarGraphItemOptions& options, QGraphicsItem* parent = nullptr);
    ~BarGraphItem() override = default;

    BarGraphItem(const BarGraphItem&) = delete;
    BarGraphItem& operator=(const BarGraphItem&) = delete;
    BarGraphItem(BarGraphItem&&) = delete;
    BarGraphItem& operator=(BarGraphItem&&) = delete;

    void setOpts(const BarGraphItemOptions& options);
    void setData(std::span<const double> x, std::span<const double> height, double width);
    void setData(std::span<const double> x, std::span<const double> y, std::span<const double> height, double width);
    void clear();

    void setPen(const QPen& pen);
    void setPens(std::span<const QPen> pens);
    [[nodiscard]] QPen pen() const;
    void setBrush(const QBrush& brush);
    void setBrushes(std::span<const QBrush> brushes);
    [[nodiscard]] QBrush brush() const;

    void setName(const QString& name);
    [[nodiscard]] QString name() const;
    [[nodiscard]] std::pair<std::span<const double>, std::span<const double>> getData() const noexcept;

    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;
    [[nodiscard]] qreal pixelPadding() const noexcept;
    [[nodiscard]] QPainterPath shape() const override;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void prepareData();
    void refreshBounds();
    [[nodiscard]] QPen effectivePen(std::size_t index) const;
    [[nodiscard]] QBrush effectiveBrush(std::size_t index) const;
    void clearMismatchedPerBarStyles();

    BarGraphItemOptions options_;
    std::vector<QRectF> rects_;
    QRectF dataBounds_;
    QRectF bounds_;
};

namespace detail_bargraphitem {

inline QPen defaultBarPen()
{
    QPen pen(QColor(255, 255, 255), 1.0);
    pen.setCosmetic(true);
    return pen;
}

inline QBrush defaultBarBrush()
{
    return QBrush(QColor(128, 128, 128));
}

inline std::vector<double> copySpan(std::span<const double> values)
{
    return std::vector<double>(values.begin(), values.end());
}

inline bool isPresent(const std::vector<double>& values)
{
    return !values.empty();
}

inline std::size_t broadcastSize(const BarGraphItemOptions& options)
{
    std::size_t size = 1;
    for (const std::vector<double>* values : {&options.x, &options.y, &options.x0, &options.y0, &options.x1,
             &options.y1, &options.width, &options.height}) {
        if (!values->empty()) {
            size = std::max(size, values->size());
        }
    }
    return size;
}

inline std::optional<double> valueAt(const std::vector<double>& values, std::size_t index, std::size_t size, const char* name)
{
    if (values.empty()) {
        return std::nullopt;
    }
    if (values.size() == 1) {
        return values.front();
    }
    if (values.size() != size) {
        throw std::invalid_argument(std::string("BarGraphItem option ") + name + " must be scalar or match the bar count");
    }
    return values[index];
}

inline bool finiteRect(qreal left, qreal top, qreal right, qreal bottom)
{
    return std::isfinite(static_cast<double>(left)) && std::isfinite(static_cast<double>(top))
        && std::isfinite(static_cast<double>(right)) && std::isfinite(static_cast<double>(bottom));
}

inline qreal dataPenPad(const QPen& pen)
{
    if (pen.style() == Qt::NoPen || pen.isCosmetic()) {
        return 0.0;
    }
    const qreal width = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
    return width * 0.5;
}

inline qreal cosmeticPenPad(const QPen& pen)
{
    if (pen.style() == Qt::NoPen || !pen.isCosmetic()) {
        return 0.0;
    }
    const qreal width = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
    return width * 0.5;
}

} // namespace detail_bargraphitem

inline BarGraphItem::BarGraphItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    options_.pen = detail_bargraphitem::defaultBarPen();
    options_.brush = detail_bargraphitem::defaultBarBrush();
    prepareData();
}

inline BarGraphItem::BarGraphItem(std::span<const double> x, std::span<const double> height, double width, QGraphicsItem* parent)
    : BarGraphItem(parent)
{
    setData(x, height, width);
}

inline BarGraphItem::BarGraphItem(const BarGraphItemOptions& options, QGraphicsItem* parent)
    : BarGraphItem(parent)
{
    setOpts(options);
}

inline void BarGraphItem::setOpts(const BarGraphItemOptions& options)
{
    prepareGeometryChange();
    options_ = options;
    prepareData();
    update();
}

inline void BarGraphItem::setData(std::span<const double> x, std::span<const double> height, double width)
{
    BarGraphItemOptions options = options_;
    options.x = detail_bargraphitem::copySpan(x);
    options.x0.clear();
    options.x1.clear();
    options.y.clear();
    options.y0.clear();
    options.y1.clear();
    options.height = detail_bargraphitem::copySpan(height);
    options.width = {width};
    setOpts(options);
}

inline void BarGraphItem::setData(std::span<const double> x, std::span<const double> y, std::span<const double> height, double width)
{
    BarGraphItemOptions options = options_;
    options.x = detail_bargraphitem::copySpan(x);
    options.x0.clear();
    options.x1.clear();
    options.y = detail_bargraphitem::copySpan(y);
    options.y0.clear();
    options.y1.clear();
    options.height = detail_bargraphitem::copySpan(height);
    options.width = {width};
    setOpts(options);
}

inline void BarGraphItem::clear()
{
    BarGraphItemOptions options = options_;
    options.x.clear();
    options.y.clear();
    options.x0.clear();
    options.y0.clear();
    options.x1.clear();
    options.y1.clear();
    options.width.clear();
    options.height.clear();
    setOpts(options);
}

inline void BarGraphItem::setPen(const QPen& pen)
{
    options_.pen = pen;
    options_.pens.clear();
    refreshBounds();
    update();
}

inline void BarGraphItem::setPens(std::span<const QPen> pens)
{
    if (!rects_.empty() && pens.size() != rects_.size()) {
        throw std::invalid_argument("BarGraphItem::setPens requires one pen per bar");
    }
    options_.pens = std::vector<QPen>(pens.begin(), pens.end());
    refreshBounds();
    update();
}

inline QPen BarGraphItem::pen() const { return options_.pen; }

inline void BarGraphItem::setBrush(const QBrush& brush)
{
    options_.brush = brush;
    options_.brushes.clear();
    update();
}

inline void BarGraphItem::setBrushes(std::span<const QBrush> brushes)
{
    if (!rects_.empty() && brushes.size() != rects_.size()) {
        throw std::invalid_argument("BarGraphItem::setBrushes requires one brush per bar");
    }
    options_.brushes = std::vector<QBrush>(brushes.begin(), brushes.end());
    update();
}

inline QBrush BarGraphItem::brush() const { return options_.brush; }
inline void BarGraphItem::setName(const QString& name) { options_.name = name; }
inline QString BarGraphItem::name() const { return options_.name; }

inline std::pair<std::span<const double>, std::span<const double>> BarGraphItem::getData() const noexcept
{
    return {options_.x, options_.height};
}

inline std::pair<qreal, qreal> BarGraphItem::dataBounds(int axis) const
{
    if (axis != 0 && axis != 1) {
        throw std::invalid_argument("BarGraphItem::dataBounds axis must be 0 or 1");
    }
    if (dataBounds_.isNull()) {
        const qreal quietNaN = std::numeric_limits<qreal>::quiet_NaN();
        return {quietNaN, quietNaN};
    }
    qreal pad = detail_bargraphitem::dataPenPad(options_.pens.empty() ? options_.pen : options_.pens.front());
    for (const QPen& pen : options_.pens) {
        pad = std::max(pad, detail_bargraphitem::dataPenPad(pen));
    }
    if (axis == 0) {
        return {dataBounds_.left() - pad, dataBounds_.right() + pad};
    }
    return {dataBounds_.top() - pad, dataBounds_.bottom() + pad};
}

inline qreal BarGraphItem::pixelPadding() const noexcept
{
    qreal pad = detail_bargraphitem::cosmeticPenPad(options_.pens.empty() ? options_.pen : options_.pens.front());
    for (const QPen& pen : options_.pens) {
        pad = std::max(pad, detail_bargraphitem::cosmeticPenPad(pen));
    }
    return pad > 0.0 ? pad : 0.5;
}

inline QPainterPath BarGraphItem::shape() const
{
    QPainterPath path;
    for (const QRectF& rect : rects_) {
        path.addRect(rect);
    }
    return path;
}

inline QRectF BarGraphItem::boundingRect() const { return bounds_; }

inline void BarGraphItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }
    for (std::size_t index = 0; index < rects_.size(); ++index) {
        painter->setPen(effectivePen(index));
        painter->setBrush(effectiveBrush(index));
        painter->drawRect(rects_[index]);
    }
}

inline void BarGraphItem::prepareData()
{
    rects_.clear();
    dataBounds_ = QRectF();
    const bool hasAny = detail_bargraphitem::isPresent(options_.x) || detail_bargraphitem::isPresent(options_.x0)
        || detail_bargraphitem::isPresent(options_.x1) || detail_bargraphitem::isPresent(options_.width)
        || detail_bargraphitem::isPresent(options_.height) || detail_bargraphitem::isPresent(options_.y)
        || detail_bargraphitem::isPresent(options_.y0) || detail_bargraphitem::isPresent(options_.y1);
    if (!hasAny) {
        refreshBounds();
        return;
    }

    const std::size_t size = detail_bargraphitem::broadcastSize(options_);
    rects_.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        const auto x = detail_bargraphitem::valueAt(options_.x, index, size, "x");
        auto x0 = detail_bargraphitem::valueAt(options_.x0, index, size, "x0");
        const auto x1Input = detail_bargraphitem::valueAt(options_.x1, index, size, "x1");
        auto width = detail_bargraphitem::valueAt(options_.width, index, size, "width");
        if (!x0.has_value()) {
            if (!width.has_value()) {
                throw std::invalid_argument("BarGraphItem requires either x0 or width");
            }
            if (x1Input.has_value()) {
                x0 = *x1Input - *width;
            } else if (x.has_value()) {
                x0 = *x - *width / 2.0;
            } else {
                throw std::invalid_argument("BarGraphItem requires at least one of x, x0, or x1");
            }
        }
        if (!width.has_value()) {
            if (!x1Input.has_value()) {
                throw std::invalid_argument("BarGraphItem requires either x1 or width");
            }
            width = *x1Input - *x0;
        }

        const auto y = detail_bargraphitem::valueAt(options_.y, index, size, "y");
        auto y0 = detail_bargraphitem::valueAt(options_.y0, index, size, "y0");
        const auto y1Input = detail_bargraphitem::valueAt(options_.y1, index, size, "y1");
        auto height = detail_bargraphitem::valueAt(options_.height, index, size, "height");
        if (!y0.has_value()) {
            if (!height.has_value()) {
                y0 = 0.0;
            } else if (y1Input.has_value()) {
                y0 = *y1Input - *height;
            } else if (y.has_value()) {
                y0 = *y - *height / 2.0;
            } else {
                y0 = 0.0;
            }
        }
        if (!height.has_value()) {
            if (!y1Input.has_value()) {
                throw std::invalid_argument("BarGraphItem requires either y1 or height");
            }
            height = *y1Input - *y0;
        }

        const qreal left = std::min<qreal>(*x0, *x0 + *width);
        const qreal right = std::max<qreal>(*x0, *x0 + *width);
        const qreal top = std::min<qreal>(*y0, *y0 + *height);
        const qreal bottom = std::max<qreal>(*y0, *y0 + *height);
        if (detail_bargraphitem::finiteRect(left, top, right, bottom)) {
            rects_.emplace_back(QPointF(left, top), QPointF(right, bottom));
        }
    }

    clearMismatchedPerBarStyles();
    for (const QRectF& rect : rects_) {
        dataBounds_ = dataBounds_.isNull() ? rect : dataBounds_.united(rect);
    }
    refreshBounds();
}

inline void BarGraphItem::refreshBounds()
{
    QRectF newBounds;
    if (!dataBounds_.isNull()) {
        const auto [xMin, xMax] = dataBounds(0);
        const auto [yMin, yMax] = dataBounds(1);
        newBounds = QRectF(QPointF(xMin, yMin), QPointF(xMax, yMax)).normalized();
        const qreal pad = pixelPadding();
        newBounds.adjust(-pad, -pad, pad, pad);
    }
    if (newBounds != bounds_) {
        prepareGeometryChange();
        bounds_ = newBounds;
    }
}

inline QPen BarGraphItem::effectivePen(std::size_t index) const
{
    return options_.pens.empty() ? options_.pen : options_.pens.at(index);
}

inline QBrush BarGraphItem::effectiveBrush(std::size_t index) const
{
    return options_.brushes.empty() ? options_.brush : options_.brushes.at(index);
}

inline void BarGraphItem::clearMismatchedPerBarStyles()
{
    if (!options_.pens.empty() && options_.pens.size() != rects_.size()) {
        options_.pens.clear();
    }
    if (!options_.brushes.empty() && options_.brushes.size() != rects_.size()) {
        options_.brushes.clear();
    }
}

} // namespace pyqtgraph::graphicsItems
