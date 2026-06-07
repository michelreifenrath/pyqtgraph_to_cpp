// Source note: translated/adapted from PyQtGraph pyqtgraph/units.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pyqtgraph::units {

namespace detail {
inline std::vector<std::string> makePrefixes()
{
    return {"y", "z", "a", "f", "p", "n", "μ", "m", " ", "k", "M", "G", "T", "P", "E", "Z", "Y"};
}

inline std::vector<std::string> makeUnits()
{
    return {"m", "s", "g", "W", "J", "V", "A", "F", "T", "Hz", "Ohm", "Ω", "S", "N", "C", "px", "b", "B", "Pa"};
}
} // namespace detail

[[nodiscard]] inline const std::vector<std::string>& SI_PREFIXES()
{
    static const std::vector<std::string> prefixes = detail::makePrefixes();
    return prefixes;
}

[[nodiscard]] inline const std::vector<std::string>& UNITS()
{
    static const std::vector<std::string> units = detail::makeUnits();
    return units;
}

[[nodiscard]] inline std::map<std::string, double>& mutableAllUnits()
{
    static std::map<std::string, double> units = [] {
        std::map<std::string, double> result;
        const auto prefixes = detail::makePrefixes();
        const auto unitNames = detail::makeUnits();
        for (std::size_t index = 0; index < prefixes.size(); ++index) {
            const double scale = std::pow(1000.0, static_cast<int>(index) - 8);
            const std::string prefix = prefixes[index] == " " ? std::string{} : prefixes[index];
            for (const auto& unit : unitNames) {
                result[prefix + unit] = scale;
            }
        }
        for (const auto& unit : unitNames) {
            result[std::string{"µ"} + unit] = 1.0e-6;
            result[std::string{"u"} + unit] = 1.0e-6;
            result[std::string{"c"} + unit] = 0.01;
            result[std::string{"d"} + unit] = 0.1;
            result[std::string{"da"} + unit] = 10.0;
            result[std::string{"h"} + unit] = 100.0;
        }
        return result;
    }();
    return units;
}

[[nodiscard]] inline const std::map<std::string, double>& allUnits()
{
    return mutableAllUnits();
}

inline void addUnit(std::string_view prefix, double value)
{
    auto& target = mutableAllUnits();
    for (const auto& unit : UNITS()) {
        target[std::string{prefix} + unit] = value;
    }
}

// Upstream PyQtGraph 0.14.0 declares these helpers but leaves their bodies as
// pass.  The native C++ equivalent reports no parsed/formatted unit expression
// rather than inventing unit algebra outside P2.07 scope.
[[nodiscard]] inline std::optional<std::string> evalUnits(std::string_view)
{
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string> formatUnits(std::string_view)
{
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string> simplify(std::string_view)
{
    return std::nullopt;
}

} // namespace pyqtgraph::units
