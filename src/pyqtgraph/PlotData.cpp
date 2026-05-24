// Source note: translated/adapted from PyQtGraph pyqtgraph/PlotData.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/PlotData.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace pyqtgraph {
namespace {

std::out_of_range missingFieldError(std::string_view field)
{
    return std::out_of_range("PlotData field not found: " + std::string(field));
}

double numpyLikeMin(const PlotData::Values& values)
{
    if (values.empty()) {
        throw std::invalid_argument("PlotData extrema require a non-empty field");
    }

    for (const double value : values) {
        if (std::isnan(value)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    return *std::min_element(values.begin(), values.end());
}

double numpyLikeMax(const PlotData::Values& values)
{
    if (values.empty()) {
        throw std::invalid_argument("PlotData extrema require a non-empty field");
    }

    for (const double value : values) {
        if (std::isnan(value)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    return *std::max_element(values.begin(), values.end());
}

} // namespace

void PlotData::addFields(std::initializer_list<std::string> fields)
{
    for (const auto& field : fields) {
        fields_.try_emplace(field, Values{});
    }
}

bool PlotData::hasField(std::string_view field) const
{
    return fields_.find(std::string(field)) != fields_.end();
}

const PlotData::Values& PlotData::operator[](std::string_view field) const
{
    return valuesFor(field);
}

PlotData::Values& PlotData::operator[](std::string_view field)
{
    return valuesFor(field);
}

void PlotData::set(std::string field, std::span<const double> values)
{
    fields_[std::move(field)] = Values(values.begin(), values.end());
    // Match upstream PlotData.py: __setitem__ replaces data but does not clear
    // minVals/maxVals, so previously computed extrema remain stale and cached.
}

void PlotData::set(std::string field, const Values& values)
{
    set(std::move(field), std::span<const double>(values.data(), values.size()));
}

void PlotData::set(std::string field, std::initializer_list<double> values)
{
    set(std::move(field), std::span<const double>(values.begin(), values.size()));
}

double PlotData::min(std::string_view field) const
{
    const auto key = std::string(field);
    const auto cached = minValues_.find(key);
    if (cached != minValues_.end()) {
        return cached->second;
    }

    const double value = numpyLikeMin(valuesFor(field));
    minValues_[key] = value;
    return value;
}

double PlotData::max(std::string_view field) const
{
    const auto key = std::string(field);
    const auto cached = maxValues_.find(key);
    if (cached != maxValues_.end()) {
        return cached->second;
    }

    const double value = numpyLikeMax(valuesFor(field));
    maxValues_[key] = value;
    return value;
}

const PlotData::Values& PlotData::valuesFor(std::string_view field) const
{
    const auto found = fields_.find(std::string(field));
    if (found == fields_.end()) {
        throw missingFieldError(field);
    }

    return found->second;
}

PlotData::Values& PlotData::valuesFor(std::string_view field)
{
    const auto found = fields_.find(std::string(field));
    if (found == fields_.end()) {
        throw missingFieldError(field);
    }

    return found->second;
}

} // namespace pyqtgraph
