// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/IsocurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "pyqtgraph/graphicsItems/IsocurveItem.hpp"

#include <QtCore/QString>
#include <QtGui/QPainter>

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>

namespace pyqtgraph::graphicsItems {
namespace {

struct GridKey {
    int row = 0;
    int column = 0;
    int parity = 0;

    [[nodiscard]] bool operator<(const GridKey& other) const noexcept
    {
        if (row != other.row) {
            return row < other.row;
        }
        if (column != other.column) {
            return column < other.column;
        }
        return parity < other.parity;
    }

    [[nodiscard]] bool operator==(const GridKey& other) const noexcept
    {
        return row == other.row && column == other.column && parity == other.parity;
    }

    [[nodiscard]] bool operator!=(const GridKey& other) const noexcept
    {
        return !(*this == other);
    }
};

struct Endpoint {
    QPointF point;
    GridKey key;
};

using Chain = std::vector<Endpoint>;

constexpr std::array<std::array<int, 4>, 16> sideTable{{
    {{-1, -1, -1, -1}},
    {{0, 1, -1, -1}},
    {{1, 2, -1, -1}},
    {{0, 2, -1, -1}},
    {{0, 3, -1, -1}},
    {{1, 3, -1, -1}},
    {{0, 1, 2, 3}},
    {{2, 3, -1, -1}},
    {{2, 3, -1, -1}},
    {{0, 1, 2, 3}},
    {{1, 3, -1, -1}},
    {{0, 3, -1, -1}},
    {{0, 2, -1, -1}},
    {{1, 2, -1, -1}},
    {{0, 1, -1, -1}},
    {{-1, -1, -1, -1}},
}};

constexpr std::array<std::array<std::array<int, 2>, 2>, 4> edgeKey{{
    {{{{0, 1}}, {{0, 0}}}},
    {{{{0, 0}}, {{1, 0}}}},
    {{{{1, 0}}, {{1, 1}}}},
    {{{{1, 1}}, {{0, 1}}}},
}};

[[nodiscard]] std::vector<Endpoint> reversedTailPlusChain(const Chain& tail, const Chain& head)
{
    std::vector<Endpoint> out;
    if (tail.size() > 1) {
        for (auto it = tail.rbegin(); it != tail.rend() - 1; ++it) {
            out.push_back(*it);
        }
    }
    out.insert(out.end(), head.begin(), head.end());
    return out;
}

} // namespace

IsocurveItem::IsocurveItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
}

IsocurveItem::IsocurveItem(core::ArrayView<const double, 2> data, double level, const QPen& pen, AxisOrder axisOrder, QGraphicsItem* parent)
    : IsocurveItem(parent)
{
    level_ = level;
    pen_ = pen;
    axisOrder_ = axisOrder;
    setData(data, level);
}

void IsocurveItem::clear()
{
    prepareGeometryChange();
    rows_ = 0;
    cols_ = 0;
    data_.clear();
    invalidate();
    update();
}

void IsocurveItem::setData(core::ArrayView<const double, 2> data)
{
    setData(data, level_);
}

void IsocurveItem::setData(core::ArrayView<const double, 2> data, double level)
{
    prepareGeometryChange();
    level_ = level;
    rows_ = data.shape()[0];
    cols_ = data.shape()[1];
    data_ = copy2d(data);
    invalidate();
    update();
}

void IsocurveItem::updateLines(core::ArrayView<const double, 2> data, double level)
{
    setData(data, level);
}

void IsocurveItem::setLevel(double level)
{
    prepareGeometryChange();
    level_ = level;
    invalidate();
    update();
}

double IsocurveItem::level() const noexcept
{
    return level_;
}

void IsocurveItem::setPen(const QPen& pen)
{
    pen_ = pyqtgraph::mkPen(pen);
    update();
}

void IsocurveItem::setPen(const QColor& color)
{
    setPen(pyqtgraph::mkPen(color));
}

void IsocurveItem::setPen(const QString& color)
{
    setPen(pyqtgraph::mkPen(color));
}

void IsocurveItem::setPen(const char* color)
{
    setPen(pyqtgraph::mkPen(color));
}

void IsocurveItem::setPen(char color)
{
    setPen(pyqtgraph::mkPen(color));
}

void IsocurveItem::setPen(int colorIndex)
{
    setPen(pyqtgraph::mkPen(colorIndex));
}

QPen IsocurveItem::pen() const
{
    return pen_;
}

void IsocurveItem::setBrush(const QBrush& brush)
{
    brush_ = brush;
    update();
}

void IsocurveItem::setBrush(const QColor& color)
{
    setBrush(pyqtgraph::mkBrush(color));
}

QBrush IsocurveItem::brush() const
{
    return brush_;
}

void IsocurveItem::setAxisOrder(AxisOrder axisOrder)
{
    if (axisOrder_ == axisOrder) {
        return;
    }
    prepareGeometryChange();
    axisOrder_ = axisOrder;
    invalidate();
    update();
}

IsocurveItem::AxisOrder IsocurveItem::axisOrder() const noexcept
{
    return axisOrder_;
}

bool IsocurveItem::hasData() const noexcept
{
    return !data_.empty();
}

std::size_t IsocurveItem::rows() const noexcept
{
    return rows_;
}

std::size_t IsocurveItem::cols() const noexcept
{
    return cols_;
}

const QPainterPath& IsocurveItem::path() const
{
    ensurePath();
    return path_;
}

const std::vector<std::vector<QPointF>>& IsocurveItem::isocurves() const
{
    ensurePath();
    return pathLines_;
}

const std::vector<std::vector<QPointF>>& IsocurveItem::pathLines() const
{
    ensurePath();
    return pathLines_;
}

QRectF IsocurveItem::boundingRect() const
{
    if (!hasData()) {
        return QRectF();
    }
    return path().boundingRect();
}

void IsocurveItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (painter == nullptr || !hasData()) {
        return;
    }
    painter->setPen(pen_);
    painter->setBrush(brush_);
    painter->drawPath(path());
}

std::vector<double> IsocurveItem::copy2d(core::ArrayView<const double, 2> view)
{
    std::vector<double> result;
    result.reserve(view.shape()[0] * view.shape()[1]);
    for (std::size_t row = 0; row < view.shape()[0]; ++row) {
        for (std::size_t column = 0; column < view.shape()[1]; ++column) {
            result.push_back(view(row, column));
        }
    }
    return result;
}

std::size_t IsocurveItem::offset(std::size_t row, std::size_t column, std::size_t columns) noexcept
{
    return (row * columns) + column;
}

double IsocurveItem::valueAt(std::size_t row, std::size_t column) const noexcept
{
    return data_[offset(row, column, cols_)];
}

double IsocurveItem::logicalValueAt(std::size_t row, std::size_t column) const noexcept
{
    if (axisOrder_ == AxisOrder::RowMajor) {
        return valueAt(column, row);
    }
    return valueAt(row, column);
}

std::size_t IsocurveItem::logicalRows() const noexcept
{
    return axisOrder_ == AxisOrder::RowMajor ? cols_ : rows_;
}

std::size_t IsocurveItem::logicalCols() const noexcept
{
    return axisOrder_ == AxisOrder::RowMajor ? rows_ : cols_;
}

void IsocurveItem::invalidate()
{
    pathValid_ = false;
    path_ = QPainterPath();
    pathLines_.clear();
}

void IsocurveItem::ensurePath() const
{
    if (pathValid_) {
        return;
    }
    path_ = QPainterPath();
    pathLines_ = generateIsocurves();
    for (const auto& line : pathLines_) {
        if (line.empty()) {
            continue;
        }
        path_.moveTo(line.front());
        for (std::size_t index = 1; index < line.size(); ++index) {
            path_.lineTo(line[index]);
        }
    }
    pathValid_ = true;
}

std::vector<std::vector<QPointF>> IsocurveItem::generateIsocurves() const
{
    if (!hasData() || logicalRows() == 0 || logicalCols() == 0) {
        return {};
    }

    const int paddedRows = static_cast<int>(logicalRows() + 2);
    const int paddedCols = static_cast<int>(logicalCols() + 2);
    auto paddedValue = [&](int row, int column) {
        const std::size_t sourceRow = static_cast<std::size_t>(std::clamp(row - 1, 0, paddedRows - 3));
        const std::size_t sourceColumn = static_cast<std::size_t>(std::clamp(column - 1, 0, paddedCols - 3));
        return logicalValueAt(sourceRow, sourceColumn);
    };

    std::vector<std::array<Endpoint, 2>> segments;
    for (int row = 0; row < paddedRows - 1; ++row) {
        for (int column = 0; column < paddedCols - 1; ++column) {
            int caseIndex = 0;
            for (int ii = 0; ii < 2; ++ii) {
                for (int jj = 0; jj < 2; ++jj) {
                    if (paddedValue(row + ii, column + jj) < level_) {
                        caseIndex += 1 << (ii + (2 * jj));
                    }
                }
            }
            const auto sides = sideTable[static_cast<std::size_t>(caseIndex)];
            for (std::size_t sideIndex = 0; sideIndex < sides.size(); sideIndex += 2) {
                if (sides[sideIndex] < 0) {
                    break;
                }
                std::array<Endpoint, 2> segment;
                for (int endpointIndex = 0; endpointIndex < 2; ++endpointIndex) {
                    const int edge = sides[sideIndex + static_cast<std::size_t>(endpointIndex)];
                    const auto p1 = edgeKey[static_cast<std::size_t>(edge)][0];
                    const auto p2 = edgeKey[static_cast<std::size_t>(edge)][1];
                    const double v1 = paddedValue(row + p1[0], column + p1[1]);
                    const double v2 = paddedValue(row + p2[0], column + p2[1]);
                    const double f = (level_ - v1) / (v2 - v1);
                    const double fi = 1.0 - f;
                    double x = (static_cast<double>(p1[0]) * fi) + (static_cast<double>(p2[0]) * f) + static_cast<double>(row) + 0.5;
                    double y = (static_cast<double>(p1[1]) * fi) + (static_cast<double>(p2[1]) * f) + static_cast<double>(column) + 0.5;
                    x = std::clamp(x - 1.0, 0.0, static_cast<double>(paddedRows - 2));
                    y = std::clamp(y - 1.0, 0.0, static_cast<double>(paddedCols - 2));
                    segment[static_cast<std::size_t>(endpointIndex)] = Endpoint{
                        QPointF(x, y),
                        GridKey{row + (edge == 2 ? 1 : 0), column + (edge == 3 ? 1 : 0), edge % 2},
                    };
                }
                segments.push_back(segment);
            }
        }
    }

    std::map<GridKey, std::vector<Chain>> points;
    for (const auto& segment : segments) {
        const Endpoint& a = segment[0];
        const Endpoint& b = segment[1];
        points[a.key].push_back(Chain{a, b});
        points[b.key].push_back(Chain{b, a});
    }

    std::vector<GridKey> keys;
    keys.reserve(points.size());
    for (const auto& [key, chains] : points) {
        (void)chains;
        keys.push_back(key);
    }

    for (const GridKey& key : keys) {
        auto pointIt = points.find(key);
        if (pointIt == points.end()) {
            continue;
        }
        auto& chains = pointIt->second;
        for (std::size_t chainIndex = 0; chainIndex < chains.size(); ++chainIndex) {
            Chain& chain = chains[chainIndex];
            std::optional<GridKey> previousEnd;
            while (true) {
                if (previousEnd.has_value() && *previousEnd == chain.back().key) {
                    break;
                }
                previousEnd = chain.back().key;
                if (*previousEnd == key) {
                    break;
                }
                const GridKey prior = chain[chain.size() - 2].key;
                auto connectsIt = points.find(*previousEnd);
                if (connectsIt == points.end()) {
                    break;
                }
                const auto connects = connectsIt->second;
                for (const Chain& connection : connects) {
                    if (connection.size() > 1 && connection[1].key != prior) {
                        chain.insert(chain.end(), connection.begin() + 1, connection.end());
                    }
                }
                points.erase(*previousEnd);
            }
            if (chain.front().key == chain.back().key) {
                if (!chains.empty()) {
                    chains.pop_back();
                }
                break;
            }
        }
    }

    std::vector<std::vector<QPointF>> lines;
    for (const auto& [key, chainList] : points) {
        (void)key;
        if (chainList.empty()) {
            continue;
        }
        Chain chain;
        if (chainList.size() == 2) {
            chain = reversedTailPlusChain(chainList[1], chainList[0]);
        } else {
            chain = chainList[0];
        }
        std::vector<QPointF> line;
        line.reserve(chain.size());
        for (const Endpoint& endpoint : chain) {
            line.push_back(endpoint.point);
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

} // namespace pyqtgraph::graphicsItems
