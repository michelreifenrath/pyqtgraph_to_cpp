// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/functions.hpp"

#include <QByteArray>
#include <QLatin1Char>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>

namespace pyqtgraph {
namespace {

[[nodiscard]] int finiteChannelToInt(double value)
{
    return std::isfinite(value) ? static_cast<int>(value) : 0;
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

} // namespace

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

    const int span = hues * values;
    const int ind = ((index % span) + span) % span;
    const int indh = ind % hues;
    const int indv = ind / hues;
    const int value = values <= 1 ? maxValue : minValue + indv * ((maxValue - minValue) / (values - 1));
    const int hue = minHue + (indh * (maxHue - minHue)) / hues;
    return QColor::fromHsv(hue, sat, value, alpha);
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
    return mkColor(static_cast<char>(color));
}

QColor mkColor(unsigned char color)
{
    return mkColor(static_cast<char>(color));
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

} // namespace pyqtgraph
