// Source note: translated/adapted from PyQtGraph pyqtgraph/colormap.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/colormap.hpp"

#include <QGradient>
#include <QGradientStops>
#include <QString>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph {
namespace {

using RgbaF = std::array<double, 4>;
using RgbaB = std::array<std::uint8_t, 4>;

RgbaF rgbaFromColor(const QColor& color) noexcept
{
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

double clampUnit(double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

std::uint8_t byteFromUnit(double value) noexcept
{
    const double byte = std::clamp(value * 255.0, 0.0, 255.0);
    return static_cast<std::uint8_t>(byte);
}

RgbaB bytesFromFloat(const RgbaF& color) noexcept
{
    return {
        byteFromUnit(color[0]),
        byteFromUnit(color[1]),
        byteFromUnit(color[2]),
        byteFromUnit(color[3]),
    };
}

QColor qcolorFromFloat(const RgbaF& color)
{
    return QColor::fromRgbF(clampUnit(color[0]), clampUnit(color[1]), clampUnit(color[2]), clampUnit(color[3]));
}

std::vector<double> equalPositions(std::size_t count)
{
    if (count == 0) {
        throw std::invalid_argument("ColorMap requires at least one stop");
    }
    if (count == 1) {
        return {0.0};
    }

    std::vector<double> positions;
    positions.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        positions.push_back(static_cast<double>(index) / static_cast<double>(count - 1));
    }
    return positions;
}

double mapPosition(double value, ColorMap::MappingMode mode) noexcept
{
    switch (mode) {
    case ColorMap::MappingMode::Clip:
        return value;
    case ColorMap::MappingMode::Repeat: {
        double wrapped = std::fmod(value, 1.0);
        if (wrapped < 0.0) {
            wrapped += 1.0;
        }
        return wrapped;
    }
    case ColorMap::MappingMode::Mirror:
        return std::abs(value);
    case ColorMap::MappingMode::Diverging:
        return (value / 2.0) + 0.5;
    }

    return value;
}

std::size_t upperStopIndex(const std::vector<double>& positions, double value) noexcept
{
    const auto upper = std::upper_bound(positions.begin(), positions.end(), value);
    return static_cast<std::size_t>(std::distance(positions.begin(), upper));
}

RgbaF interpolateFloat(const std::vector<double>& positions, const std::vector<RgbaF>& stops, double value) noexcept
{
    if (positions.size() == 1 || value <= positions.front()) {
        return stops.front();
    }
    if (value >= positions.back()) {
        return stops.back();
    }

    const std::size_t right = upperStopIndex(positions, value);
    const std::size_t left = right - 1;
    const double leftPosition = positions[left];
    const double rightPosition = positions[right];
    if (rightPosition == leftPosition) {
        return stops[right];
    }

    const double fraction = (value - leftPosition) / (rightPosition - leftPosition);
    RgbaF result{};
    for (std::size_t channel = 0; channel < result.size(); ++channel) {
        result[channel] = stops[left][channel] + ((stops[right][channel] - stops[left][channel]) * fraction);
    }
    return result;
}

RgbaB interpolateByte(const std::vector<double>& positions, const std::vector<RgbaF>& stops, double value) noexcept
{
    if (positions.size() == 1 || value <= positions.front()) {
        return bytesFromFloat(stops.front());
    }
    if (value >= positions.back()) {
        return bytesFromFloat(stops.back());
    }

    const std::size_t right = upperStopIndex(positions, value);
    const std::size_t left = right - 1;
    const double leftPosition = positions[left];
    const double rightPosition = positions[right];
    if (rightPosition == leftPosition) {
        return bytesFromFloat(stops[right]);
    }

    const double fraction = (value - leftPosition) / (rightPosition - leftPosition);
    RgbaB result{};
    for (std::size_t channel = 0; channel < result.size(); ++channel) {
        const double leftByte = static_cast<double>(byteFromUnit(stops[left][channel]));
        const double rightByte = static_cast<double>(byteFromUnit(stops[right][channel]));
        const double interpolated = leftByte + ((rightByte - leftByte) * fraction);
        result[channel] = static_cast<std::uint8_t>(std::clamp(interpolated, 0.0, 255.0));
    }
    return result;
}

QColor hexColor(const char* value)
{
    return QColor(QString::fromLatin1(value));
}

ColorMap makePalette(const QString& name, std::vector<QColor> colors)
{
    return ColorMap::fromEqualSpacing(std::move(colors), name, ColorMap::MappingMode::Clip);
}

std::optional<ColorMap> buildLocalMap(const QString& name)
{
    if (name == QStringLiteral("PAL-relaxed")) {
        return makePalette(
            name,
            {
                hexColor("#f97f10"),
                hexColor("#e5bb00"),
                hexColor("#94ab00"),
                hexColor("#12a12a"),
                hexColor("#007c8c"),
                hexColor("#0e56c2"),
                hexColor("#813be3"),
                hexColor("#c01188"),
                hexColor("#e23512"),
                hexColor("#f97f10"),
            });
    }
    if (name == QStringLiteral("PAL-relaxed_bright")) {
        return makePalette(
            name,
            {
                hexColor("#ff9d47"),
                hexColor("#f7e100"),
                hexColor("#b3cf00"),
                hexColor("#1ec23a"),
                hexColor("#00a0b5"),
                hexColor("#1f78ff"),
                hexColor("#a54dff"),
                hexColor("#e22ca8"),
                hexColor("#ff532b"),
                hexColor("#ff9d47"),
            });
    }
    return std::nullopt;
}

} // namespace

std::size_t ColorMap::LookupTable::rows() const noexcept
{
    switch (mode) {
    case OutputMode::Byte:
        return channels == 0 ? 0 : bytes.size() / channels;
    case OutputMode::Float:
        return channels == 0 ? 0 : floats.size() / channels;
    case OutputMode::QColor:
        return colors.size();
    }

    return 0;
}

std::size_t ColorMap::Stops::rows() const noexcept
{
    return positions.size();
}

ColorMap::ColorMap(std::vector<double> positions, std::vector<QColor> colors, QString name, MappingMode mappingMode)
    : name_(std::move(name))
    , mappingMode_(mappingMode)
{
    if (positions.empty()) {
        throw std::invalid_argument("ColorMap requires at least one stop");
    }
    if (positions.size() != colors.size()) {
        throw std::invalid_argument("ColorMap positions and colors must have the same length");
    }

    std::vector<std::size_t> order(positions.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&positions](std::size_t lhs, std::size_t rhs) {
        return positions[lhs] < positions[rhs];
    });

    positions_.reserve(positions.size());
    colors_.reserve(colors.size());
    colorStops_.reserve(colors.size());

    for (const std::size_t index : order) {
        positions_.push_back(positions[index]);
        colors_.push_back(colors[index]);
        colorStops_.push_back(rgbaFromColor(colors[index]));
    }
}

ColorMap ColorMap::fromEqualSpacing(std::vector<QColor> colors, QString name, MappingMode mappingMode)
{
    std::vector<double> positions = equalPositions(colors.size());
    return ColorMap(std::move(positions), std::move(colors), std::move(name), mappingMode);
}

std::size_t ColorMap::size() const noexcept
{
    return positions_.size();
}

bool ColorMap::empty() const noexcept
{
    return positions_.empty();
}

const std::vector<double>& ColorMap::positions() const noexcept
{
    return positions_;
}

const std::vector<QColor>& ColorMap::colors() const noexcept
{
    return colors_;
}

const QString& ColorMap::name() const noexcept
{
    return name_;
}

ColorMap::MappingMode ColorMap::mappingMode() const noexcept
{
    return mappingMode_;
}

bool ColorMap::usesAlpha() const noexcept
{
    return std::any_of(colorStops_.begin(), colorStops_.end(), [](const RgbaF& color) { return color[3] != 1.0; });
}

void ColorMap::setMappingMode(MappingMode mappingMode) noexcept
{
    mappingMode_ = mappingMode;
}

ColorMap::Stops ColorMap::getStops(OutputMode mode) const
{
    Stops stops;
    stops.mode = mode;
    stops.positions = positions_;
    stops.channels = 4;

    if (mode == OutputMode::Byte) {
        stops.bytes.reserve(colorStops_.size() * stops.channels);
        for (const RgbaF& color : colorStops_) {
            const RgbaB bytes = bytesFromFloat(color);
            stops.bytes.insert(stops.bytes.end(), bytes.begin(), bytes.end());
        }
    } else if (mode == OutputMode::Float) {
        stops.floats.reserve(colorStops_.size() * stops.channels);
        for (const RgbaF& color : colorStops_) {
            stops.floats.insert(stops.floats.end(), color.begin(), color.end());
        }
    } else {
        stops.colors = colors_;
    }

    return stops;
}

ColorMap::LookupTable ColorMap::getColors(OutputMode mode) const
{
    LookupTable table;
    table.mode = mode;
    table.channels = 4;
    if (mode == OutputMode::Byte) {
        table.bytes.reserve(colorStops_.size() * table.channels);
        for (const RgbaF& color : colorStops_) {
            const RgbaB bytes = bytesFromFloat(color);
            table.bytes.insert(table.bytes.end(), bytes.begin(), bytes.end());
        }
    } else if (mode == OutputMode::Float) {
        table.floats.reserve(colorStops_.size() * table.channels);
        for (const RgbaF& color : colorStops_) {
            table.floats.insert(table.floats.end(), color.begin(), color.end());
        }
    } else {
        table.colors = colors_;
    }
    return table;
}

QColor ColorMap::getByIndex(std::size_t index) const
{
    if (index >= colorStops_.size()) {
        throw std::out_of_range("ColorMap stop index is out of range");
    }
    return qcolorFromFloat(colorStops_[index]);
}

std::array<std::uint8_t, 4> ColorMap::mapToByte(double value) const
{
    return interpolateByte(positions_, colorStops_, mapPosition(value, mappingMode_));
}

std::array<double, 4> ColorMap::mapToFloat(double value) const
{
    return interpolateFloat(positions_, colorStops_, mapPosition(value, mappingMode_));
}

QColor ColorMap::mapToQColor(double value) const
{
    return qcolorFromFloat(mapToFloat(value));
}

QLinearGradient ColorMap::getGradient(std::optional<QPointF> p1, std::optional<QPointF> p2) const
{
    const QPointF start = p1.value_or(QPointF(0.0, 0.0));
    const double span = positions_.empty() ? 0.0 : positions_.back() - positions_.front();
    const QPointF end = p2.value_or(QPointF(span, 0.0));

    QLinearGradient gradient(start, end);
    QGradientStops stops;

    if (mappingMode_ == MappingMode::Mirror) {
        for (std::size_t offset = 0; offset < positions_.size(); ++offset) {
            const std::size_t index = positions_.size() - 1 - offset;
            stops.append(QGradientStop((1.0 - positions_[index]) / 2.0, qcolorFromFloat(colorStops_[index])));
        }
        for (std::size_t index = 0; index < positions_.size(); ++index) {
            stops.append(QGradientStop((1.0 + positions_[index]) / 2.0, qcolorFromFloat(colorStops_[index])));
        }
    } else {
        for (std::size_t index = 0; index < positions_.size(); ++index) {
            stops.append(QGradientStop(positions_[index], qcolorFromFloat(colorStops_[index])));
        }
    }

    gradient.setStops(stops);
    if (mappingMode_ == MappingMode::Repeat) {
        gradient.setSpread(QGradient::RepeatSpread);
    }
    return gradient;
}

ColorMap::LookupTable ColorMap::getLookupTable(
    double start,
    double stop,
    std::size_t nPts,
    std::optional<bool> alpha,
    OutputMode mode) const
{
    LookupTable table;
    table.mode = mode;
    const bool includeAlpha = alpha.value_or(usesAlpha());
    table.channels = mode == OutputMode::QColor || includeAlpha ? 4U : 3U;

    if (nPts == 0) {
        return table;
    }

    if (mode == OutputMode::Byte) {
        table.bytes.reserve(nPts * table.channels);
    } else if (mode == OutputMode::Float) {
        table.floats.reserve(nPts * table.channels);
    } else {
        table.colors.reserve(nPts);
    }

    for (std::size_t row = 0; row < nPts; ++row) {
        const double sample = nPts == 1 ? start : start + ((stop - start) * static_cast<double>(row) / static_cast<double>(nPts - 1));
        const double mappedSample = mapPosition(sample, mappingMode_);

        if (mode == OutputMode::Byte) {
            const RgbaB color = interpolateByte(positions_, colorStops_, mappedSample);
            for (std::size_t channel = 0; channel < table.channels; ++channel) {
                table.bytes.push_back(color[channel]);
            }
        } else {
            const RgbaF color = interpolateFloat(positions_, colorStops_, mappedSample);
            if (mode == OutputMode::Float) {
                for (std::size_t channel = 0; channel < table.channels; ++channel) {
                    table.floats.push_back(color[channel]);
                }
            } else {
                table.colors.push_back(qcolorFromFloat(color));
            }
        }
    }

    return table;
}

std::vector<QString> listMaps(const QString& source)
{
    if (!source.isEmpty()) {
        return {};
    }
    return {QStringLiteral("PAL-relaxed"), QStringLiteral("PAL-relaxed_bright")};
}

std::optional<ColorMap> get(const QString& name, const QString& source, bool skipCache)
{
    if (!source.isEmpty()) {
        return std::nullopt;
    }

    static std::optional<ColorMap> relaxed;
    static std::optional<ColorMap> relaxedBright;
    if (name == QStringLiteral("PAL-relaxed")) {
        if (skipCache || !relaxed.has_value()) {
            relaxed = buildLocalMap(name);
        }
        return relaxed;
    }
    if (name == QStringLiteral("PAL-relaxed_bright")) {
        if (skipCache || !relaxedBright.has_value()) {
            relaxedBright = buildLocalMap(name);
        }
        return relaxedBright;
    }

    return std::nullopt;
}

} // namespace cppqtgraph
