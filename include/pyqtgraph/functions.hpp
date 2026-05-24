#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#ifndef PYQTGRAPH_CPP_ENABLE_QT_COLOR
#define PYQTGRAPH_CPP_ENABLE_QT_COLOR 0
#endif

#if PYQTGRAPH_CPP_ENABLE_QT_COLOR
#if __has_include(<QColor>) && __has_include(<QString>)
#include <QColor>
#include <QString>
#define PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS 1
#elif __has_include(<QtGui/QColor>) && __has_include(<QtCore/QString>)
#include <QtCore/QString>
#include <QtGui/QColor>
#define PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS 1
#else
#error "PYQTGRAPH_CPP_ENABLE_QT_COLOR requires Qt QColor and QString headers"
#endif
#else
#define PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS 0
#endif

#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#if __has_include(<span>)
#include <span>
#endif
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#if !defined(__cpp_lib_span)
namespace std {
template <typename T>
class span {
public:
    using element_type = T;
    using pointer = T*;
    using iterator = pointer;

    constexpr span() noexcept = default;
    constexpr span(pointer data, std::size_t size) noexcept
        : data_(data)
        , size_(size)
    {
    }

    [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
    [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }
    [[nodiscard]] constexpr pointer data() const noexcept { return data_; }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

private:
    pointer data_ = nullptr;
    std::size_t size_ = 0;
};
} // namespace std
#endif

namespace pyqtgraph {

namespace detail {

template <typename T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename T>
inline constexpr bool is_character_integral_v =
    std::is_same_v<remove_cvref_t<T>, char> || std::is_same_v<remove_cvref_t<T>, signed char> ||
    std::is_same_v<remove_cvref_t<T>, unsigned char> || std::is_same_v<remove_cvref_t<T>, wchar_t> ||
#if defined(__cpp_char8_t)
    std::is_same_v<remove_cvref_t<T>, char8_t> ||
#endif
    std::is_same_v<remove_cvref_t<T>, char16_t> || std::is_same_v<remove_cvref_t<T>, char32_t>;

inline constexpr int defaultIntColorSpan = 9;

template <typename T>
[[nodiscard]] constexpr int reduceDefaultIntColorIndex(T index)
{
    using Value = remove_cvref_t<T>;
    if constexpr (std::is_same_v<Value, bool>) {
        return index ? 1 : 0;
    } else {
        constexpr Value span = static_cast<Value>(defaultIntColorSpan);
        return static_cast<int>(index % span);
    }
}

} // namespace detail

#if PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

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
QColor mkColor(char color);
QColor mkColor(signed char color);
QColor mkColor(unsigned char color);
QColor mkColor(int index);

template <typename T>
    requires(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, int> &&
             !detail::is_character_integral_v<T>)
[[nodiscard]] QColor mkColor(T index)
{
    return intColor(detail::reduceDefaultIntColorIndex(index));
}

QColor mkColor(double gray);
QColor mkColor(double red, double green, double blue);
QColor mkColor(double red, double green, double blue, double alpha);
QColor mkColor(std::initializer_list<double> values);

#endif // PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

[[nodiscard]] float nanmin(std::span<const float> values);
[[nodiscard]] double nanmin(std::span<const double> values);
[[nodiscard]] long double nanmin(std::span<const long double> values);
[[nodiscard]] float nanmax(std::span<const float> values);
[[nodiscard]] double nanmax(std::span<const double> values);
[[nodiscard]] long double nanmax(std::span<const long double> values);

template <typename T,
          std::enable_if_t<std::is_floating_point_v<detail::remove_cvref_t<T>>, int> = 0>
[[nodiscard]] detail::remove_cvref_t<T> nanmin(std::initializer_list<T> values)
{
    using Value = detail::remove_cvref_t<T>;
    return nanmin(std::span<const Value>(values.begin(), values.size()));
}

template <typename T,
          std::enable_if_t<std::is_floating_point_v<detail::remove_cvref_t<T>>, int> = 0>
[[nodiscard]] detail::remove_cvref_t<T> nanmax(std::initializer_list<T> values)
{
    using Value = detail::remove_cvref_t<T>;
    return nanmax(std::span<const Value>(values.begin(), values.size()));
}

#if PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

namespace detail {

template <typename T>
[[nodiscard]] int finiteChannelToInt(T value)
{
    const double numeric = static_cast<double>(value);
    if (!std::isfinite(numeric)) {
        return 0;
    }
    if (numeric <= static_cast<double>(std::numeric_limits<int>::min())) {
        return std::numeric_limits<int>::min();
    }
    if (numeric >= static_cast<double>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(numeric);
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

#endif // PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

} // namespace pyqtgraph
