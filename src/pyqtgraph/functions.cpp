// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/functions.hpp"

#if PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS
#if __has_include(<QByteArray>)
#include <QByteArray>
#else
#include <QtCore/QByteArray>
#endif
#if __has_include(<QLatin1Char>)
#include <QLatin1Char>
#else
#include <QtCore/QLatin1Char>
#endif
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace pyqtgraph {
namespace {

#if PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

[[nodiscard]] int finiteChannelToInt(double value)
{
    if (!std::isfinite(value)) {
        return 0;
    }
    if (value <= static_cast<double>(std::numeric_limits<int>::min())) {
        return std::numeric_limits<int>::min();
    }
    if (value >= static_cast<double>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(value);
}

[[nodiscard]] std::int64_t floorDiv(std::int64_t numerator, std::int64_t denominator)
{
    const std::int64_t quotient = numerator / denominator;
    const std::int64_t remainder = numerator % denominator;
    return remainder < 0 ? quotient - 1 : quotient;
}

[[nodiscard]] int clampToInt(std::int64_t value)
{
    return static_cast<int>(std::clamp(value,
                                       static_cast<std::int64_t>(std::numeric_limits<int>::min()),
                                       static_cast<std::int64_t>(std::numeric_limits<int>::max())));
}

[[nodiscard]] QColor colorFromChannels(double red, double green, double blue, double alpha)
{
    return QColor(finiteChannelToInt(red), finiteChannelToInt(green), finiteChannelToInt(blue), finiteChannelToInt(alpha));
}

[[nodiscard]] QColor shortNamedColor(QChar name)
{
    switch (name.toLatin1()) {
    case 'b':
        return QColor(0, 0, 255, 255);
    case 'g':
        return QColor(0, 255, 0, 255);
    case 'r':
        return QColor(255, 0, 0, 255);
    case 'c':
        return QColor(0, 255, 255, 255);
    case 'm':
        return QColor(255, 0, 255, 255);
    case 'y':
        return QColor(255, 255, 0, 255);
    case 'k':
        return QColor(0, 0, 0, 255);
    case 'w':
        return QColor(255, 255, 255, 255);
    case 'd':
        return QColor(150, 150, 150, 255);
    case 'l':
        return QColor(200, 200, 200, 255);
    case 's':
        return QColor(100, 100, 150, 255);
    default:
        throw std::invalid_argument("No color named \"" + std::string(1, name.toLatin1()) + "\"");
    }
}

[[nodiscard]] bool isHexDigitString(const QString& text)
{
    return std::all_of(text.begin(), text.end(), [](QChar ch) {
        const unsigned char latin = static_cast<unsigned char>(ch.toLatin1());
        return std::isxdigit(latin) != 0;
    });
}

[[nodiscard]] QColor hexColor(const QString& text)
{
    QString hex = text.mid(1);
    if (hex.size() != 3 && hex.size() != 4 && hex.size() != 6 && hex.size() != 8) {
        throw std::invalid_argument("Unable to convert " + text.toStdString() + " to QColor");
    }
    if (!isHexDigitString(hex)) {
        throw std::invalid_argument("Unable to convert " + text.toStdString() + " to QColor");
    }

    if (hex.size() == 3 || hex.size() == 4) {
        QString expanded;
        expanded.reserve(hex.size() * 2);
        for (QChar ch : hex) {
            expanded.append(ch);
            expanded.append(ch);
        }
        hex = expanded;
    }

    const QByteArray bytes = QByteArray::fromHex(hex.toLatin1());
    if (bytes.size() == 3) {
        return QColor(static_cast<unsigned char>(bytes[0]),
                      static_cast<unsigned char>(bytes[1]),
                      static_cast<unsigned char>(bytes[2]),
                      255);
    }
    return QColor(static_cast<unsigned char>(bytes[0]),
                  static_cast<unsigned char>(bytes[1]),
                  static_cast<unsigned char>(bytes[2]),
                  static_cast<unsigned char>(bytes[3]));
}

#endif // PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

template <typename T, typename Compare>
[[nodiscard]] T nanExtrema(std::span<const T> values, Compare compare)
{
    if (values.empty()) {
        throw std::invalid_argument("nanmin/nanmax requires at least one value");
    }

    bool hasValue = false;
    T result{};
    for (const T value : values) {
        if (std::isnan(value)) {
            continue;
        }
        if (!hasValue) {
            result = value;
            hasValue = true;
            continue;
        }
        if (compare(value, result)) {
            result = value;
        }
    }

    return hasValue ? result : std::numeric_limits<T>::quiet_NaN();
}

} // namespace

#if PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

QColor intColor(int index,
                int hues,
                int values,
                int maxValue,
                int minValue,
                int maxHue,
                int minHue,
                int sat,
                int alpha)
{
    if (hues <= 0 || values <= 0) {
        throw std::invalid_argument("intColor requires positive hues and values");
    }

    const std::int64_t wideHues = hues;
    const std::int64_t wideValues = values;
    const std::int64_t span = wideHues * wideValues;
    const std::int64_t ind = ((static_cast<std::int64_t>(index) % span) + span) % span;
    const std::int64_t indh = ind % wideHues;
    const std::int64_t indv = ind / wideHues;
    const std::int64_t value = values <= 1 ? maxValue
                                           : static_cast<std::int64_t>(minValue) +
                                                 indv * floorDiv(static_cast<std::int64_t>(maxValue) - minValue,
                                                                 wideValues - 1);
    const std::int64_t hue = static_cast<std::int64_t>(minHue) +
                             floorDiv(indh * (static_cast<std::int64_t>(maxHue) - minHue), wideHues);
    return QColor::fromHsv(clampToInt(hue), sat, clampToInt(value), alpha);
}

QColor mkColor(const QString& color)
{
    if (color.size() == 1) {
        return shortNamedColor(color.front());
    }

    if (color.startsWith(QLatin1Char('#')) && color.size() < 10) {
        return hexColor(color);
    }

    const QColor qcolor(color);
    if (qcolor.isValid()) {
        return qcolor;
    }

    throw std::invalid_argument("Unable to convert " + color.toStdString() + " to QColor");
}

QColor mkColor(const char* color)
{
    if (color == nullptr) {
        throw std::invalid_argument("Unable to convert null string to QColor");
    }
    return mkColor(QString::fromUtf8(color));
}

QColor mkColor(char color)
{
    QString text;
    text.append(QLatin1Char(color));
    return mkColor(text);
}

QColor mkColor(signed char color)
{
    return intColor(static_cast<int>(color));
}

QColor mkColor(unsigned char color)
{
    return intColor(static_cast<int>(color));
}

QColor mkColor(std::string_view color)
{
    return mkColor(QString::fromUtf8(color.data(), static_cast<qsizetype>(color.size())));
}

QColor mkColor(const QColor& color)
{
    return QColor(color);
}

QColor mkColor(int index)
{
    return intColor(index);
}

QColor mkColor(double gray)
{
    const int channel = finiteChannelToInt(gray * 255.0);
    return QColor(channel, channel, channel, 255);
}

QColor mkColor(double red, double green, double blue)
{
    return colorFromChannels(red, green, blue, 255.0);
}

QColor mkColor(double red, double green, double blue, double alpha)
{
    return colorFromChannels(red, green, blue, alpha);
}

QColor mkColor(std::initializer_list<double> values)
{
    if (values.size() == 2) {
        auto it = values.begin();
        const int index = finiteChannelToInt(*it++);
        const int hues = finiteChannelToInt(*it);
        return intColor(index, hues);
    }
    if (values.size() == 3) {
        auto it = values.begin();
        const double red = *it++;
        const double green = *it++;
        const double blue = *it;
        return mkColor(red, green, blue);
    }
    if (values.size() == 4) {
        auto it = values.begin();
        const double red = *it++;
        const double green = *it++;
        const double blue = *it++;
        const double alpha = *it;
        return mkColor(red, green, blue, alpha);
    }

    throw std::invalid_argument("mkColor sequence input must contain 2, 3, or 4 values");
}

#endif // PYQTGRAPH_CPP_HAS_QT_COLOR_HEADERS

float nanmin(std::span<const float> values)
{
    return nanExtrema(values, [](float lhs, float rhs) { return lhs < rhs; });
}

double nanmin(std::span<const double> values)
{
    return nanExtrema(values, [](double lhs, double rhs) { return lhs < rhs; });
}

long double nanmin(std::span<const long double> values)
{
    return nanExtrema(values, [](long double lhs, long double rhs) { return lhs < rhs; });
}

float nanmax(std::span<const float> values)
{
    return nanExtrema(values, [](float lhs, float rhs) { return lhs > rhs; });
}

double nanmax(std::span<const double> values)
{
    return nanExtrema(values, [](double lhs, double rhs) { return lhs > rhs; });
}

long double nanmax(std::span<const long double> values)
{
    return nanExtrema(values, [](long double lhs, long double rhs) { return lhs > rhs; });
}

} // namespace pyqtgraph
