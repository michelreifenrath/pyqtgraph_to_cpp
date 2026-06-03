#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#ifndef PYQTGRAPH_CPP_ENABLE_QT_COLOR
#define PYQTGRAPH_CPP_ENABLE_QT_COLOR 0
#endif

#if PYQTGRAPH_CPP_ENABLE_QT_COLOR
#if __has_include(<QBrush>) && __has_include(<QColor>) && __has_include(<QPainterPath>) && __has_include(<QPen>) && __has_include(<QString>)
#include <QBrush>
#include <QColor>
#include <QPainterPath>
#include <QPen>
#include <QString>
#define PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS 1
#elif __has_include(<QtGui/QBrush>) && __has_include(<QtGui/QColor>) && __has_include(<QtGui/QPainterPath>) && __has_include(<QtGui/QPen>) && __has_include(<QtCore/QString>)
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#define PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS 1
#else
#error "PYQTGRAPH_CPP_ENABLE_QT_COLOR requires Qt QBrush, QColor, QPainterPath, QPen, and QString headers"
#endif
#else
#define PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS 0
#endif

#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#if __has_include(<span>)
#include <span>
#endif
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

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
inline constexpr bool is_numeric_channel_integral_v =
    std::is_integral_v<remove_cvref_t<T>> && !std::is_same_v<remove_cvref_t<T>, bool>;

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

QColor hsvColor(double hue, double sat = 1.0, double val = 1.0, double alpha = 1.0);
[[nodiscard]] std::array<int, 4> colorTuple(const QColor& color);
[[nodiscard]] QString colorStr(const QColor& color);
[[nodiscard]] std::array<double, 4> glColor(const QColor& color);
[[nodiscard]] std::array<double, 4> glColor(const QString& color);
[[nodiscard]] std::array<double, 4> glColor(const char* color);
[[nodiscard]] std::array<double, 4> glColor(std::string_view color);
[[nodiscard]] std::array<double, 4> glColor(char color);
[[nodiscard]] std::array<double, 4> glColor(signed char color);
[[nodiscard]] std::array<double, 4> glColor(unsigned char color);
[[nodiscard]] std::array<double, 4> glColor(int index);

class Color : public QColor {
public:
    Color() = default;
    explicit Color(const QColor& color);
    explicit Color(const QString& color);
    explicit Color(const char* color);
    explicit Color(std::string_view color);
    explicit Color(char color);
    explicit Color(signed char color);
    explicit Color(unsigned char color);
    explicit Color(int index);
    explicit Color(double gray);
    Color(double red, double green, double blue);
    Color(double red, double green, double blue, double alpha);
    explicit Color(std::initializer_list<double> values);
    template <typename T, std::size_t N>
    explicit Color(const std::array<T, N>& values);
    template <typename... Values>
    explicit Color(const std::tuple<Values...>& values);

    [[nodiscard]] std::array<double, 4> glColor() const;
};

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

template <typename T>
    requires(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, int> &&
             !detail::is_character_integral_v<T>)
[[nodiscard]] std::array<double, 4> glColor(T index)
{
    return glColor(mkColor(index));
}

[[nodiscard]] std::array<double, 4> glColor(double gray);
[[nodiscard]] std::array<double, 4> glColor(double red, double green, double blue);
[[nodiscard]] std::array<double, 4> glColor(double red, double green, double blue, double alpha);
[[nodiscard]] std::array<double, 4> glColor(std::initializer_list<double> values);

template <typename T, std::size_t N>
[[nodiscard]] std::array<double, 4> glColor(const std::array<T, N>& values)
{
    return glColor(mkColor(values));
}

template <typename... Values>
[[nodiscard]] std::array<double, 4> glColor(const std::tuple<Values...>& values)
{
    return glColor(mkColor(values));
}

template <typename T, std::size_t N>
Color::Color(const std::array<T, N>& values)
    : QColor(mkColor(values))
{
}

template <typename... Values>
Color::Color(const std::tuple<Values...>& values)
    : QColor(mkColor(values))
{
}

QPen mkPen();
QPen mkPen(std::nullptr_t, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(const QPen& pen);
QPen mkPen(const QColor& color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(const QString& color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(const char* color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(std::string_view color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(char color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(signed char color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(unsigned char color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(int index, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);

template <typename T>
    requires(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, int> &&
             !detail::is_character_integral_v<T>)
[[nodiscard]] QPen mkPen(T index, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true)
{
    return mkPen(mkColor(index), width, style, cosmetic);
}

QPen mkPen(double gray, double width = 1.0, Qt::PenStyle style = Qt::SolidLine, bool cosmetic = true);
QPen mkPen(double red, double green, double blue);
QPen mkPen(double red,
           double green,
           double blue,
           double alpha,
           double width = 1.0,
           Qt::PenStyle style = Qt::SolidLine,
           bool cosmetic = true);

template <typename Red, typename Green, typename Blue>
    requires(detail::is_numeric_channel_integral_v<Red> && detail::is_numeric_channel_integral_v<Green> &&
             detail::is_numeric_channel_integral_v<Blue>)
[[nodiscard]] QPen mkPen(Red red, Green green, Blue blue)
{
    return mkPen(static_cast<double>(red), static_cast<double>(green), static_cast<double>(blue));
}

template <typename Red, typename Green, typename Blue, typename Alpha>
    requires(detail::is_numeric_channel_integral_v<Red> && detail::is_numeric_channel_integral_v<Green> &&
             detail::is_numeric_channel_integral_v<Blue> && detail::is_numeric_channel_integral_v<Alpha>)
[[nodiscard]] QPen mkPen(Red red,
                         Green green,
                         Blue blue,
                         Alpha alpha,
                         double width = 1.0,
                         Qt::PenStyle style = Qt::SolidLine,
                         bool cosmetic = true)
{
    return mkPen(static_cast<double>(red),
                 static_cast<double>(green),
                 static_cast<double>(blue),
                 static_cast<double>(alpha),
                 width,
                 style,
                 cosmetic);
}

QPen mkPen(std::initializer_list<double> values,
           double width = 1.0,
           Qt::PenStyle style = Qt::SolidLine,
           bool cosmetic = true);

// C++ equivalent of mkPen(dict/kwargs): color or hsv override, width/style,
// dash pattern, and cosmetic flag.
struct PenOptions {
    std::optional<QColor> color;
    std::optional<std::array<double, 4>> hsv;
    double width = 1.0;
    std::optional<Qt::PenStyle> style;
    std::vector<double> dash;
    bool hasDash = false;
    bool cosmetic = true;
};

QPen mkPen(const PenOptions& options);

template <typename T, std::size_t N>
[[nodiscard]] QPen mkPen(const std::array<T, N>& values,
                         double width = 1.0,
                         Qt::PenStyle style = Qt::SolidLine,
                         bool cosmetic = true)
{
    return mkPen(mkColor(values), width, style, cosmetic);
}

template <typename... Values>
[[nodiscard]] QPen mkPen(const std::tuple<Values...>& values,
                         double width = 1.0,
                         Qt::PenStyle style = Qt::SolidLine,
                         bool cosmetic = true)
{
    return mkPen(mkColor(values), width, style, cosmetic);
}

QBrush mkBrush(std::nullptr_t);
QBrush mkBrush(const QBrush& brush);
QBrush mkBrush(const QColor& color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(const QString& color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(const char* color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(std::string_view color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(char color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(signed char color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(unsigned char color, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(int index, Qt::BrushStyle style = Qt::SolidPattern);

template <typename T>
    requires(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, int> &&
             !detail::is_character_integral_v<T>)
[[nodiscard]] QBrush mkBrush(T index, Qt::BrushStyle style = Qt::SolidPattern)
{
    return mkBrush(mkColor(index), style);
}

QBrush mkBrush(double gray, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(double red, double green, double blue);
QBrush mkBrush(double red, double green, double blue, double alpha, Qt::BrushStyle style = Qt::SolidPattern);
QBrush mkBrush(std::initializer_list<double> values, Qt::BrushStyle style = Qt::SolidPattern);

using SymbolPathMap = std::map<QString, QPainterPath>;

[[nodiscard]] const SymbolPathMap& symbolPaths();
[[nodiscard]] QPainterPath symbolPath(const QString& symbol);
[[nodiscard]] QPainterPath symbolPath(const char* symbol);
[[nodiscard]] QPainterPath symbolPath(std::string_view symbol);

template <typename T, std::size_t N>
[[nodiscard]] QBrush mkBrush(const std::array<T, N>& values, Qt::BrushStyle style = Qt::SolidPattern)
{
    return mkBrush(mkColor(values), style);
}

template <typename... Values>
[[nodiscard]] QBrush mkBrush(const std::tuple<Values...>& values, Qt::BrushStyle style = Qt::SolidPattern)
{
    return mkBrush(mkColor(values), style);
}

#endif // PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

} // namespace pyqtgraph
