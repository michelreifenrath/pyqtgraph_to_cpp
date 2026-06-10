#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ErrorBarItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
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
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace cppqtgraph::graphicsItems {

struct ErrorBarItemOptions {
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> height;
    std::vector<double> width;
    std::vector<double> top;
    std::vector<double> bottom;
    std::vector<double> left;
    std::vector<double> right;
    std::optional<double> beam;
    QPen pen = [] {
        QPen defaultPen(QColor(255, 255, 255), 1.0);
        defaultPen.setCosmetic(true);
        return defaultPen;
    }();
};

class ErrorBarItem : public GraphicsObject {
public:
    explicit ErrorBarItem(QGraphicsItem* parent = nullptr);
    explicit ErrorBarItem(const ErrorBarItemOptions& options, QGraphicsItem* parent = nullptr);
    ~ErrorBarItem() override = default;

    ErrorBarItem(const ErrorBarItem&) = delete;
    ErrorBarItem& operator=(const ErrorBarItem&) = delete;
    ErrorBarItem(ErrorBarItem&&) = delete;
    ErrorBarItem& operator=(ErrorBarItem&&) = delete;

    void setData(const ErrorBarItemOptions& options);
    void setData(std::span<const double> x, std::span<const double> y, std::span<const double> top,
        std::span<const double> bottom, double beam);
    void setOpts(const ErrorBarItemOptions& options);
    void clear();

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;
    [[nodiscard]] QPainterPath path() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void rebuildPath();

    ErrorBarItemOptions options_;
    QPainterPath path_;
};

namespace detail_errorbaritem {

inline QPen defaultErrorPen()
{
    QPen pen(QColor(255, 255, 255), 1.0);
    pen.setCosmetic(true);
    return pen;
}

inline std::vector<double> copySpan(std::span<const double> values)
{
    return std::vector<double>(values.begin(), values.end());
}

inline std::size_t broadcastSize(const ErrorBarItemOptions& options)
{
    std::size_t size = std::max(options.x.size(), options.y.size());
    for (const std::vector<double>* values : {&options.height, &options.width, &options.top, &options.bottom,
             &options.left, &options.right}) {
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
        throw std::invalid_argument(std::string("ErrorBarItem option ") + name + " must be scalar or match point count");
    }
    return values[index];
}

inline bool finitePoint(qreal x, qreal y)
{
    return std::isfinite(static_cast<double>(x)) && std::isfinite(static_cast<double>(y));
}

inline void addLineIfFinite(QPainterPath& path, QPointF start, QPointF stop)
{
    if (!finitePoint(start.x(), start.y()) || !finitePoint(stop.x(), stop.y())) {
        return;
    }
    path.moveTo(start);
    path.lineTo(stop);
}

inline qreal strokePadding(const QPen& pen)
{
    if (pen.style() == Qt::NoPen) {
        return 0.0;
    }
    const qreal width = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
    return width * 0.5;
}

} // namespace detail_errorbaritem

inline ErrorBarItem::ErrorBarItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    options_.pen = detail_errorbaritem::defaultErrorPen();
    setVisible(false);
    rebuildPath();
}

inline ErrorBarItem::ErrorBarItem(const ErrorBarItemOptions& options, QGraphicsItem* parent)
    : ErrorBarItem(parent)
{
    setData(options);
}

inline void ErrorBarItem::setData(const ErrorBarItemOptions& options)
{
    prepareGeometryChange();
    options_ = options;
    setVisible(!options_.x.empty() && !options_.y.empty());
    rebuildPath();
    update();
}

inline void ErrorBarItem::setData(std::span<const double> x, std::span<const double> y, std::span<const double> top,
    std::span<const double> bottom, double beam)
{
    ErrorBarItemOptions options = options_;
    options.x = detail_errorbaritem::copySpan(x);
    options.y = detail_errorbaritem::copySpan(y);
    options.height.clear();
    options.width.clear();
    options.top = detail_errorbaritem::copySpan(top);
    options.bottom = detail_errorbaritem::copySpan(bottom);
    options.left.clear();
    options.right.clear();
    options.beam = beam;
    setData(options);
}

inline void ErrorBarItem::setOpts(const ErrorBarItemOptions& options) { setData(options); }

inline void ErrorBarItem::clear()
{
    ErrorBarItemOptions options = options_;
    options.x.clear();
    options.y.clear();
    options.height.clear();
    options.width.clear();
    options.top.clear();
    options.bottom.clear();
    options.left.clear();
    options.right.clear();
    setData(options);
}

inline void ErrorBarItem::setPen(const QPen& pen)
{
    prepareGeometryChange();
    options_.pen = pen;
    update();
}

inline QPen ErrorBarItem::pen() const { return options_.pen; }
inline QPainterPath ErrorBarItem::path() const { return path_; }
inline QRectF ErrorBarItem::boundingRect() const
{
    QRectF bounds = path_.boundingRect();
    if (bounds.isNull()) {
        return bounds;
    }
    const qreal pad = detail_errorbaritem::strokePadding(options_.pen);
    bounds.adjust(-pad, -pad, pad, pad);
    return bounds;
}

inline void ErrorBarItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr || path_.isEmpty()) {
        return;
    }
    painter->setPen(options_.pen);
    painter->drawPath(path_);
}

inline void ErrorBarItem::rebuildPath()
{
    path_ = QPainterPath();
    if (options_.x.empty() || options_.y.empty()) {
        return;
    }
    const std::size_t size = detail_errorbaritem::broadcastSize(options_);
    if (options_.x.size() != 1 && options_.x.size() != size) {
        throw std::invalid_argument("ErrorBarItem x must be scalar or match point count");
    }
    if (options_.y.size() != 1 && options_.y.size() != size) {
        throw std::invalid_argument("ErrorBarItem y must be scalar or match point count");
    }

    const bool drawVertical = !options_.height.empty() || !options_.top.empty() || !options_.bottom.empty();
    const bool drawHorizontal = !options_.width.empty() || !options_.left.empty() || !options_.right.empty();
    const qreal beam = options_.beam.value_or(0.0);

    for (std::size_t index = 0; index < size; ++index) {
        const double x = *detail_errorbaritem::valueAt(options_.x, index, size, "x");
        const double y = *detail_errorbaritem::valueAt(options_.y, index, size, "y");
        if (drawVertical) {
            const auto height = detail_errorbaritem::valueAt(options_.height, index, size, "height");
            const auto top = detail_errorbaritem::valueAt(options_.top, index, size, "top");
            const auto bottom = detail_errorbaritem::valueAt(options_.bottom, index, size, "bottom");
            const double yBottom = height.has_value() ? y - *height / 2.0 : y - bottom.value_or(0.0);
            const double yTop = height.has_value() ? y + *height / 2.0 : y + top.value_or(0.0);
            detail_errorbaritem::addLineIfFinite(path_, QPointF(x, yBottom), QPointF(x, yTop));
            if (beam > 0.0) {
                const double x1 = x - beam / 2.0;
                const double x2 = x + beam / 2.0;
                if (height.has_value() || top.has_value()) {
                    detail_errorbaritem::addLineIfFinite(path_, QPointF(x1, yTop), QPointF(x2, yTop));
                }
                if (height.has_value() || bottom.has_value()) {
                    detail_errorbaritem::addLineIfFinite(path_, QPointF(x1, yBottom), QPointF(x2, yBottom));
                }
            }
        }
        if (drawHorizontal) {
            const auto width = detail_errorbaritem::valueAt(options_.width, index, size, "width");
            const auto left = detail_errorbaritem::valueAt(options_.left, index, size, "left");
            const auto right = detail_errorbaritem::valueAt(options_.right, index, size, "right");
            const double x1 = width.has_value() ? x - *width / 2.0 : x - left.value_or(0.0);
            const double x2 = width.has_value() ? x + *width / 2.0 : x + right.value_or(0.0);
            detail_errorbaritem::addLineIfFinite(path_, QPointF(x1, y), QPointF(x2, y));
            if (beam > 0.0) {
                const double y1 = y - beam / 2.0;
                const double y2 = y + beam / 2.0;
                if (width.has_value() || right.has_value()) {
                    detail_errorbaritem::addLineIfFinite(path_, QPointF(x2, y1), QPointF(x2, y2));
                }
                if (width.has_value() || left.has_value()) {
                    detail_errorbaritem::addLineIfFinite(path_, QPointF(x1, y1), QPointF(x1, y2));
                }
            }
        }
    }
}

} // namespace cppqtgraph::graphicsItems
