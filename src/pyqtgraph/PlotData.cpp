// Source note: translated/adapted from PyQtGraph pyqtgraph/PlotData.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/PlotData.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace pyqtgraph {
namespace {

std::out_of_range missingFieldError(std::string_view field)
{
    return std::out_of_range("PlotData field not found: " + std::string(field));
}

void validateMaskSize(const PlotData::Values& values, const PlotData::Mask* mask)
{
    if (mask != nullptr && mask->size() != values.size()) {
        throw std::invalid_argument("PlotData mask length must match field length");
    }
}

template <typename Better>
double numpyLikeExtremum(const PlotData::Values& values, const PlotData::Mask* mask, Better better)
{
    if (values.empty()) {
        throw std::invalid_argument("PlotData extrema require a non-empty field");
    }
    validateMaskSize(values, mask);

    bool hasUnmaskedValue = false;
    double result = 0.0;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (mask != nullptr && (*mask)[index]) {
            continue;
        }

        const double value = values[index];
        if (std::isnan(value)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (!hasUnmaskedValue || better(value, result)) {
            result = value;
            hasUnmaskedValue = true;
        }
    }

    if (!hasUnmaskedValue) {
        throw std::invalid_argument("PlotData extrema require at least one unmasked value");
    }
    return result;
}

double numpyLikeMin(const PlotData::Values& values, const PlotData::Mask* mask)
{
    return numpyLikeExtremum(values, mask, [](double value, double current) { return value < current; });
}

double numpyLikeMax(const PlotData::Values& values, const PlotData::Mask* mask)
{
    return numpyLikeExtremum(values, mask, [](double value, double current) { return value > current; });
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
    const auto key = std::move(field);
    fields_[key] = Values(values.begin(), values.end());
    masks_.erase(key);
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

void PlotData::setMasked(std::string field, std::initializer_list<double> values, std::initializer_list<bool> mask)
{
    setMaskedValues(std::move(field), Values(values.begin(), values.end()), Mask(mask.begin(), mask.end()));
}

void PlotData::setMaskedValues(std::string field, Values values, Mask mask)
{
    if (values.size() != mask.size()) {
        throw std::invalid_argument("PlotData mask length must match field length");
    }

    const auto key = std::move(field);
    fields_[key] = std::move(values);
    masks_[key] = std::move(mask);
    // Match upstream PlotData.py: __setitem__ replaces data but does not clear
    // minVals/maxVals, so previously computed extrema remain stale and cached.
}

double PlotData::min(std::string_view field) const
{
    const auto key = std::string(field);
    const auto cached = minValues_.find(key);
    if (cached != minValues_.end()) {
        return cached->second;
    }

    const auto mask = masks_.find(key);
    const double value = numpyLikeMin(valuesFor(field), mask == masks_.end() ? nullptr : &mask->second);
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

    const auto mask = masks_.find(key);
    const double value = numpyLikeMax(valuesFor(field), mask == masks_.end() ? nullptr : &mask->second);
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
