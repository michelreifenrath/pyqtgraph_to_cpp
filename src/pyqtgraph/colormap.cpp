// Source note: translated/adapted from PyQtGraph pyqtgraph/colormap.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/colormap.hpp"

#include <stdexcept>
#include <utility>

namespace pyqtgraph {

ColorMap::ColorMap(std::vector<double> positions, std::vector<QColor> colors, QString name)
    : positions_(std::move(positions))
    , colors_(std::move(colors))
    , name_(std::move(name))
{
    if (positions_.empty()) {
        throw std::invalid_argument("ColorMap requires at least one stop");
    }
    if (positions_.size() != colors_.size()) {
        throw std::invalid_argument("ColorMap positions and colors must have the same length");
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

} // namespace pyqtgraph
