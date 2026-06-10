#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/BoxplotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
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

inline constexpr double DefaultBoxWidth = 0.8;
inline constexpr double DefaultBoxSymbolSize = 10.0;

struct BoxplotSampleStats {
    double loc = 0.0;
    double p25 = 0.0;
    double median = 0.0;
    double p75 = 0.0;
    double lowerWhisker = 0.0;
    double upperWhisker = 0.0;
    std::vector<double> outliers;
};

struct BoxplotItemOptions {
    std::optional<std::vector<double>> loc;
    std::vector<std::vector<double>> data;
    bool locAsX = true;
    std::optional<double> width;
    QPen pen = [] {
        QPen pen(QColor(255, 255, 0), 1.0);
        pen.setCosmetic(true);
        return pen;
    }();
    QBrush brush = QBrush(Qt::NoBrush);
    QPen medianPen = [] {
        QPen pen(QColor(255, 0, 0), 1.0);
        pen.setCosmetic(true);
        return pen;
    }();
    bool outlier = true;
    double symbolSize = DefaultBoxSymbolSize;
    QPen symbolPen = QPen(Qt::NoPen);
    QBrush symbolBrush = QBrush(Qt::NoBrush);
};

class BoxplotItem : public GraphicsObject {
public:
    using WhiskerFunc = std::function<std::pair<double, double>(std::span<const double>)>;

    explicit BoxplotItem(QGraphicsItem* parent = nullptr);
    explicit BoxplotItem(const BoxplotItemOptions& options, QGraphicsItem* parent = nullptr);
    ~BoxplotItem() override = default;

    BoxplotItem(const BoxplotItem&) = delete;
    BoxplotItem& operator=(const BoxplotItem&) = delete;
    BoxplotItem(BoxplotItem&&) = delete;
    BoxplotItem& operator=(BoxplotItem&&) = delete;

    void setData(const BoxplotItemOptions& options);
    void setData(std::vector<std::vector<double>> data);
    void clear();
    void setWhiskerFunc(WhiskerFunc func);

    [[nodiscard]] const BoxplotItemOptions& options() const noexcept;
    [[nodiscard]] const std::vector<BoxplotSampleStats>& statistics() const noexcept;
    [[nodiscard]] std::pair<qreal, qreal> dataBounds(int axis) const;
    [[nodiscard]] qreal pixelPadding() const noexcept;
    [[nodiscard]] double width() const noexcept;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    [[nodiscard]] static std::pair<double, double> defaultWhiskers(std::span<const double> values);
    [[nodiscard]] static double percentile(std::span<const double> values, double percent);
    void recompute();
    [[nodiscard]] std::vector<double> effectiveLoc() const;
    [[nodiscard]] bool boxesHidden() const noexcept;

    BoxplotItemOptions options_;
    WhiskerFunc whiskerFunc_ = defaultWhiskers;
    double effectiveWidth_ = DefaultBoxWidth;
    std::vector<BoxplotSampleStats> stats_;
    QRectF dataBounds_;
};

namespace detail_boxplotitem {

inline void validateFiniteValues(std::span<const double> values)
{
    if (values.empty()) {
        throw std::invalid_argument("BoxplotItem datasets must not be empty");
    }
    if (!std::all_of(values.begin(), values.end(), [](double value) { return std::isfinite(value); })) {
        throw std::invalid_argument("BoxplotItem datasets must contain finite non-NaN values");
    }
}

inline qreal cosmeticPenWidth(const QPen& pen)
{
    if (pen.style() == Qt::NoPen || !pen.isCosmetic()) {
        return 0.0;
    }
    return pen.widthF() > 0.0 ? pen.widthF() : 1.0;
}

inline QPainterPath circleSymbol()
{
    QPainterPath path;
    path.addEllipse(QPointF(0.0, 0.0), 0.5, 0.5);
    return path;
}

} // namespace detail_boxplotitem

inline BoxplotItem::BoxplotItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    recompute();
}

inline BoxplotItem::BoxplotItem(const BoxplotItemOptions& options, QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    setData(options);
}

inline void BoxplotItem::setData(const BoxplotItemOptions& options)
{
    prepareGeometryChange();
    options_ = options;
    recompute();
    update();
}

inline void BoxplotItem::setData(std::vector<std::vector<double>> data)
{
    BoxplotItemOptions options = options_;
    options.data = std::move(data);
    setData(options);
}

inline void BoxplotItem::clear()
{
    BoxplotItemOptions options = options_;
    options.data.clear();
    options.loc.reset();
    setData(options);
}

inline void BoxplotItem::setWhiskerFunc(WhiskerFunc func)
{
    if (!func) {
        throw std::invalid_argument("BoxplotItem whisker function must be callable");
    }
    const std::vector<double> probe{1.0, 2.0, 3.0};
    const auto [lower, upper] = func(std::span<const double>(probe.data(), probe.size()));
    if (!std::isfinite(lower) || !std::isfinite(upper)) {
        throw std::invalid_argument("BoxplotItem whisker function must return finite numeric bounds");
    }
    prepareGeometryChange();
    whiskerFunc_ = std::move(func);
    recompute();
    update();
}

inline const BoxplotItemOptions& BoxplotItem::options() const noexcept
{
    return options_;
}

inline const std::vector<BoxplotSampleStats>& BoxplotItem::statistics() const noexcept
{
    return stats_;
}

inline std::pair<qreal, qreal> BoxplotItem::dataBounds(int axis) const
{
    if (axis == 0) {
        return {dataBounds_.left(), dataBounds_.right()};
    }
    return {dataBounds_.top(), dataBounds_.bottom()};
}

inline qreal BoxplotItem::pixelPadding() const noexcept
{
    const qreal symbolPadding = options_.outlier ? static_cast<qreal>(0.7072 * options_.symbolSize) : 0.0;
    const qreal penPadding = 0.5 * detail_boxplotitem::cosmeticPenWidth(options_.pen);
    return std::max(symbolPadding, penPadding);
}

inline double BoxplotItem::width() const noexcept
{
    return effectiveWidth_;
}

inline QRectF BoxplotItem::boundingRect() const
{
    if (stats_.empty()) {
        return QRectF();
    }
    const qreal padding = pixelPadding();
    return dataBounds_.adjusted(-padding, -padding, padding, padding);
}

inline void BoxplotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (painter == nullptr || stats_.empty()) {
        return;
    }

    for (const BoxplotSampleStats& stat : stats_) {
        if (effectiveWidth_ != 0.0) {
            painter->setPen(options_.pen);
            painter->setBrush(options_.brush);
            if (options_.locAsX) {
                painter->drawLine(QPointF(stat.loc - effectiveWidth_ / 4.0, stat.upperWhisker), QPointF(stat.loc + effectiveWidth_ / 4.0, stat.upperWhisker));
                painter->drawLine(QPointF(stat.loc - effectiveWidth_ / 4.0, stat.lowerWhisker), QPointF(stat.loc + effectiveWidth_ / 4.0, stat.lowerWhisker));
                painter->drawLine(QPointF(stat.loc, stat.upperWhisker), QPointF(stat.loc, stat.p75));
                painter->drawLine(QPointF(stat.loc, stat.lowerWhisker), QPointF(stat.loc, stat.p25));
                painter->drawRect(QRectF(stat.loc - effectiveWidth_ / 2.0, stat.p25, effectiveWidth_, stat.p75 - stat.p25));
                painter->setPen(options_.medianPen);
                painter->drawLine(QPointF(stat.loc - effectiveWidth_ / 2.0, stat.median), QPointF(stat.loc + effectiveWidth_ / 2.0, stat.median));
            } else {
                painter->drawLine(QPointF(stat.upperWhisker, stat.loc - effectiveWidth_ / 4.0), QPointF(stat.upperWhisker, stat.loc + effectiveWidth_ / 4.0));
                painter->drawLine(QPointF(stat.lowerWhisker, stat.loc - effectiveWidth_ / 4.0), QPointF(stat.lowerWhisker, stat.loc + effectiveWidth_ / 4.0));
                painter->drawLine(QPointF(stat.upperWhisker, stat.loc), QPointF(stat.p75, stat.loc));
                painter->drawLine(QPointF(stat.lowerWhisker, stat.loc), QPointF(stat.p25, stat.loc));
                painter->drawRect(QRectF(stat.p25, stat.loc - effectiveWidth_ / 2.0, stat.p75 - stat.p25, effectiveWidth_));
                painter->setPen(options_.medianPen);
                painter->drawLine(QPointF(stat.median, stat.loc - effectiveWidth_ / 2.0), QPointF(stat.median, stat.loc + effectiveWidth_ / 2.0));
            }
        }

        if (!options_.outlier) {
            continue;
        }
        const QPainterPath symbol = detail_boxplotitem::circleSymbol();
        const QTransform transform = painter->transform();
        painter->setPen(options_.symbolPen);
        painter->setBrush(options_.symbolBrush);
        for (const double outlier : stat.outliers) {
            const QPointF point = options_.locAsX ? QPointF(stat.loc, outlier) : QPointF(outlier, stat.loc);
            painter->save();
            painter->resetTransform();
            painter->translate(transform.map(point));
            painter->scale(options_.symbolSize, options_.symbolSize);
            painter->drawPath(symbol);
            painter->restore();
        }
    }
}

inline std::pair<double, double> BoxplotItem::defaultWhiskers(std::span<const double> values)
{
    detail_boxplotitem::validateFiniteValues(values);
    const double p75 = percentile(values, 75.0);
    const double p25 = percentile(values, 25.0);
    const double iqr = p75 - p25;
    const double upperTheory = p75 + 1.5 * iqr;
    const double lowerTheory = p25 - 1.5 * iqr;
    double lower = std::numeric_limits<double>::infinity();
    double upper = -std::numeric_limits<double>::infinity();
    for (const double value : values) {
        if (value >= lowerTheory) {
            lower = std::min(lower, value);
        }
        if (value <= upperTheory) {
            upper = std::max(upper, value);
        }
    }
    return {lower, upper};
}

inline double BoxplotItem::percentile(std::span<const double> values, double percent)
{
    detail_boxplotitem::validateFiniteValues(values);
    std::vector<double> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    if (sorted.size() == 1) {
        return sorted.front();
    }
    const double rank = (static_cast<double>(sorted.size() - 1) * percent) / 100.0;
    const auto left = static_cast<std::size_t>(std::floor(rank));
    const auto right = static_cast<std::size_t>(std::ceil(rank));
    if (left == right) {
        return sorted[left];
    }
    const double fraction = rank - static_cast<double>(left);
    return sorted[left] + ((sorted[right] - sorted[left]) * fraction);
}

inline void BoxplotItem::recompute()
{
    stats_.clear();
    dataBounds_ = QRectF();
    effectiveWidth_ = options_.width.value_or(DefaultBoxWidth);
    if (boxesHidden()) {
        effectiveWidth_ = 0.0;
    }
    if (options_.data.empty()) {
        return;
    }

    const std::vector<double> loc = effectiveLoc();
    if (loc.size() != options_.data.size()) {
        throw std::invalid_argument("BoxplotItem loc and data lengths must match");
    }

    double valueMin = std::numeric_limits<double>::infinity();
    double valueMax = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0; index < options_.data.size(); ++index) {
        const std::vector<double>& dataset = options_.data[index];
        detail_boxplotitem::validateFiniteValues(dataset);
        BoxplotSampleStats stat;
        stat.loc = loc[index];
        stat.p25 = percentile(dataset, 25.0);
        stat.median = percentile(dataset, 50.0);
        stat.p75 = percentile(dataset, 75.0);
        const auto [lower, upper] = whiskerFunc_(std::span<const double>(dataset.data(), dataset.size()));
        if (!std::isfinite(lower) || !std::isfinite(upper)) {
            throw std::invalid_argument("BoxplotItem whisker function must return finite numeric bounds");
        }
        stat.lowerWhisker = lower;
        stat.upperWhisker = upper;
        if (options_.outlier) {
            for (const double value : dataset) {
                if (value < lower || value > upper) {
                    stat.outliers.push_back(value);
                }
            }
            const auto [minIt, maxIt] = std::minmax_element(dataset.begin(), dataset.end());
            valueMin = std::min(valueMin, *minIt);
            valueMax = std::max(valueMax, *maxIt);
        } else {
            valueMin = std::min(valueMin, lower);
            valueMax = std::max(valueMax, upper);
        }
        stats_.push_back(std::move(stat));
    }

    const auto [locMinIt, locMaxIt] = std::minmax_element(loc.begin(), loc.end());
    double minLoc = *locMinIt - (effectiveWidth_ / 2.0);
    double maxLoc = *locMaxIt + (effectiveWidth_ / 2.0);
    if (options_.locAsX) {
        dataBounds_ = QRectF(QPointF(minLoc, valueMin), QPointF(maxLoc, valueMax)).normalized();
    } else {
        dataBounds_ = QRectF(QPointF(valueMin, minLoc), QPointF(valueMax, maxLoc)).normalized();
    }
}

inline std::vector<double> BoxplotItem::effectiveLoc() const
{
    if (options_.loc.has_value()) {
        return *options_.loc;
    }
    std::vector<double> loc(options_.data.size());
    for (std::size_t index = 0; index < loc.size(); ++index) {
        loc[index] = static_cast<double>(index);
    }
    return loc;
}

inline bool BoxplotItem::boxesHidden() const noexcept
{
    return options_.pen.style() == Qt::NoPen && options_.brush.style() == Qt::NoBrush && options_.medianPen.style() == Qt::NoPen;
}

} // namespace cppqtgraph::graphicsItems
