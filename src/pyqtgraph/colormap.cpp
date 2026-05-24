// Source note: translated/adapted from PyQtGraph pyqtgraph/colormap.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/colormap.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pyqtgraph {
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
        return {
            byteFromUnit(stops.front()[0]),
            byteFromUnit(stops.front()[1]),
            byteFromUnit(stops.front()[2]),
            byteFromUnit(stops.front()[3]),
        };
    }
    if (value >= positions.back()) {
        return {
            byteFromUnit(stops.back()[0]),
            byteFromUnit(stops.back()[1]),
            byteFromUnit(stops.back()[2]),
            byteFromUnit(stops.back()[3]),
        };
    }

    const std::size_t right = upperStopIndex(positions, value);
    const std::size_t left = right - 1;
    const double leftPosition = positions[left];
    const double rightPosition = positions[right];
    if (rightPosition == leftPosition) {
        return {
            byteFromUnit(stops[right][0]),
            byteFromUnit(stops[right][1]),
            byteFromUnit(stops[right][2]),
            byteFromUnit(stops[right][3]),
        };
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
                table.colors.push_back(QColor::fromRgbF(
                    clampUnit(color[0]),
                    clampUnit(color[1]),
                    clampUnit(color[2]),
                    clampUnit(color[3])));
            }
        }
    }

    return table;
}

} // namespace pyqtgraph
