#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QColor>
#include <QString>

#include <array>
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pyqtgraph {

QColor intColor(int index,
                int hues = 9,
                int values = 1,
                int maxValue = 255,
                int minValue = 150,
                int maxHue = 360,
                int minHue = 0,
                int sat = 255,
                int alpha = 255);

QColor mkColor(const QString& color);
QColor mkColor(const char* color);
QColor mkColor(std::string_view color);
QColor mkColor(const QColor& color);
QColor mkColor(int index);

template <typename T>
    requires(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, int>)
[[nodiscard]] QColor mkColor(T index)
{
    return intColor(static_cast<int>(index));
}

QColor mkColor(double gray);
QColor mkColor(double red, double green, double blue);
QColor mkColor(double red, double green, double blue, double alpha);
QColor mkColor(std::initializer_list<double> values);

namespace detail {

template <typename T>
[[nodiscard]] int finiteChannelToInt(T value)
{
    const double numeric = static_cast<double>(value);
    return std::isfinite(numeric) ? static_cast<int>(numeric) : 0;
}

template <typename T, std::size_t N>
[[nodiscard]] QColor mkColorFromArray(const std::array<T, N>& values)
{
    if constexpr (N == 2) {
        return intColor(finiteChannelToInt(values[0]), finiteChannelToInt(values[1]));
    } else if constexpr (N == 3) {
        return mkColor(static_cast<double>(values[0]), static_cast<double>(values[1]), static_cast<double>(values[2]));
    } else if constexpr (N == 4) {
        return mkColor(static_cast<double>(values[0]),
                       static_cast<double>(values[1]),
                       static_cast<double>(values[2]),
                       static_cast<double>(values[3]));
    } else {
        throw std::invalid_argument("mkColor array input must contain 2, 3, or 4 values");
    }
}

template <typename Tuple, std::size_t... Indices>
[[nodiscard]] QColor mkColorFromTupleImpl(const Tuple& values, std::index_sequence<Indices...>)
{
    return mkColorFromArray(std::array<double, sizeof...(Indices)>{static_cast<double>(std::get<Indices>(values))...});
}

} // namespace detail

template <typename T, std::size_t N>
[[nodiscard]] QColor mkColor(const std::array<T, N>& values)
{
    return detail::mkColorFromArray(values);
}

template <typename... Values>
[[nodiscard]] QColor mkColor(const std::tuple<Values...>& values)
{
    return detail::mkColorFromTupleImpl(values, std::index_sequence_for<Values...>{});
}

} // namespace pyqtgraph
