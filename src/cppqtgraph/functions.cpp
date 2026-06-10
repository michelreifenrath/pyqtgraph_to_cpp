// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py and
// pyqtgraph/graphicsItems/ScatterPlotItem.py symbol definitions
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/functions.hpp"

#if CPPQTGRAPH_HAS_QT_COLOR_HEADERS
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
#if __has_include(<QList>)
#include <QList>
#else
#include <QtCore/QList>
#endif
#if __has_include(<QPointF>)
#include <QPointF>
#else
#include <QtCore/QPointF>
#endif
#if __has_include(<QPolygonF>)
#include <QPolygonF>
#else
#include <QtGui/QPolygonF>
#endif
#if __has_include(<QRectF>)
#include <QRectF>
#else
#include <QtCore/QRectF>
#endif
#if __has_include(<QTransform>)
#include <QTransform>
#else
#include <QtGui/QTransform>
#endif
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace cppqtgraph {
namespace {

#if CPPQTGRAPH_HAS_QT_COLOR_HEADERS

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

void applyWideLineCap(QPen& pen, double width)
{
    if (width > 4.0) {
        pen.setCapStyle(Qt::RoundCap);
    }
}

[[nodiscard]] QPen makePenFromColor(const QColor& color, double width, Qt::PenStyle style, bool cosmetic)
{
    QPen pen;
    pen.setColor(color);
    pen.setWidthF(width);
    pen.setStyle(style);
    pen.setCosmetic(cosmetic);
    applyWideLineCap(pen, width);
    return pen;
}

[[nodiscard]] QBrush makeBrushFromColor(const QColor& color, Qt::BrushStyle style)
{
    QBrush brush;
    brush.setColor(color);
    brush.setStyle(style);
    return brush;
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

[[nodiscard]] QPainterPath polygonSymbol(std::initializer_list<QPointF> points)
{
    QPainterPath path;
    if (points.size() == 0) {
        return path;
    }

    auto point = points.begin();
    path.moveTo(*point++);
    for (; point != points.end(); ++point) {
        path.lineTo(*point);
    }
    path.closeSubpath();
    return path;
}

[[nodiscard]] QPainterPath ellipseSymbol()
{
    QPainterPath path;
    path.addEllipse(QRectF(-0.5, -0.5, 1.0, 1.0));
    return path;
}

[[nodiscard]] QPainterPath rectSymbol()
{
    QPainterPath path;
    path.addRect(QRectF(-0.5, -0.5, 1.0, 1.0));
    return path;
}

[[nodiscard]] QPainterPath crosshairSymbol()
{
    QPainterPath path;
    path.addEllipse(QRectF(-0.5, -0.5, 1.0, 1.0));
    path.moveTo(-1.0, 0.0);
    path.lineTo(1.0, 0.0);
    path.moveTo(0.0, -1.0);
    path.lineTo(0.0, 1.0);
    return path;
}

[[nodiscard]] QPainterPath plusSymbol()
{
    return polygonSymbol({QPointF(-0.5, -0.1),
                          QPointF(-0.5, 0.1),
                          QPointF(-0.1, 0.1),
                          QPointF(-0.1, 0.5),
                          QPointF(0.1, 0.5),
                          QPointF(0.1, 0.1),
                          QPointF(0.5, 0.1),
                          QPointF(0.5, -0.1),
                          QPointF(0.1, -0.1),
                          QPointF(0.1, -0.5),
                          QPointF(-0.1, -0.5),
                          QPointF(-0.1, -0.1)});
}

[[nodiscard]] const std::array<QString, 19>& orderedSymbolNames()
{
    static const std::array<QString, 19> names = {QStringLiteral("o"),
                                                  QStringLiteral("s"),
                                                  QStringLiteral("t"),
                                                  QStringLiteral("t1"),
                                                  QStringLiteral("t2"),
                                                  QStringLiteral("t3"),
                                                  QStringLiteral("d"),
                                                  QStringLiteral("+"),
                                                  QStringLiteral("x"),
                                                  QStringLiteral("p"),
                                                  QStringLiteral("h"),
                                                  QStringLiteral("star"),
                                                  QStringLiteral("|"),
                                                  QStringLiteral("_"),
                                                  QStringLiteral("arrow_up"),
                                                  QStringLiteral("arrow_right"),
                                                  QStringLiteral("arrow_down"),
                                                  QStringLiteral("arrow_left"),
                                                  QStringLiteral("crosshair")};
    return names;
}

[[nodiscard]] int symbolOrderIndex(const QString& symbol)
{
    const auto& names = orderedSymbolNames();
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (names[index] == symbol) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

[[nodiscard]] SymbolPathMap makeSymbolPaths()
{
    SymbolPathMap symbols;
    symbols.emplace(QStringLiteral("o"), ellipseSymbol());
    symbols.emplace(QStringLiteral("s"), rectSymbol());
    symbols.emplace(QStringLiteral("t"), polygonSymbol({QPointF(-0.5, -0.5), QPointF(0.0, 0.5), QPointF(0.5, -0.5)}));
    symbols.emplace(QStringLiteral("t1"), polygonSymbol({QPointF(-0.5, 0.5), QPointF(0.0, -0.5), QPointF(0.5, 0.5)}));
    symbols.emplace(QStringLiteral("t2"), polygonSymbol({QPointF(-0.5, -0.5), QPointF(-0.5, 0.5), QPointF(0.5, 0.0)}));
    symbols.emplace(QStringLiteral("t3"), polygonSymbol({QPointF(0.5, 0.5), QPointF(0.5, -0.5), QPointF(-0.5, 0.0)}));
    symbols.emplace(QStringLiteral("d"), polygonSymbol({QPointF(0.0, -0.5),
                                                         QPointF(-0.4, 0.0),
                                                         QPointF(0.0, 0.5),
                                                         QPointF(0.4, 0.0)}));

    const QPainterPath plus = plusSymbol();
    symbols.emplace(QStringLiteral("+"), plus);
    symbols.emplace(QStringLiteral("x"), QTransform().rotate(45.0).map(plus));
    symbols.emplace(QStringLiteral("p"), polygonSymbol({QPointF(0.0, -0.5),
                                                         QPointF(-0.4755, -0.1545),
                                                         QPointF(-0.2939, 0.4045),
                                                         QPointF(0.2939, 0.4045),
                                                         QPointF(0.4755, -0.1545)}));
    symbols.emplace(QStringLiteral("h"), polygonSymbol({QPointF(0.433, 0.25),
                                                         QPointF(0.0, 0.5),
                                                         QPointF(-0.433, 0.25),
                                                         QPointF(-0.433, -0.25),
                                                         QPointF(0.0, -0.5),
                                                         QPointF(0.433, -0.25)}));
    symbols.emplace(QStringLiteral("star"), polygonSymbol({QPointF(0.0, -0.5),
                                                            QPointF(-0.1123, -0.1545),
                                                            QPointF(-0.4755, -0.1545),
                                                            QPointF(-0.1816, 0.059),
                                                            QPointF(-0.2939, 0.4045),
                                                            QPointF(0.0, 0.1910),
                                                            QPointF(0.2939, 0.4045),
                                                            QPointF(0.1816, 0.059),
                                                            QPointF(0.4755, -0.1545),
                                                            QPointF(0.1123, -0.1545)}));

    const QPainterPath verticalBar = polygonSymbol({QPointF(-0.1, 0.5),
                                                    QPointF(0.1, 0.5),
                                                    QPointF(0.1, -0.5),
                                                    QPointF(-0.1, -0.5)});
    const QPainterPath arrowUp = polygonSymbol({QPointF(-0.125, 0.125),
                                                QPointF(0.0, 0.0),
                                                QPointF(0.125, 0.125),
                                                QPointF(0.05, 0.125),
                                                QPointF(0.05, 0.5),
                                                QPointF(-0.05, 0.5),
                                                QPointF(-0.05, 0.125)});
    QTransform rotate90;
    rotate90.rotate(90.0);
    const QPainterPath arrowRight = rotate90.map(arrowUp);
    const QPainterPath arrowDown = rotate90.map(arrowRight);
    const QPainterPath arrowLeft = rotate90.map(arrowDown);

    symbols.emplace(QStringLiteral("|"), verticalBar);
    symbols.emplace(QStringLiteral("_"), rotate90.map(verticalBar));
    symbols.emplace(QStringLiteral("arrow_up"), arrowUp);
    symbols.emplace(QStringLiteral("arrow_right"), arrowRight);
    symbols.emplace(QStringLiteral("arrow_down"), arrowDown);
    symbols.emplace(QStringLiteral("arrow_left"), arrowLeft);
    symbols.emplace(QStringLiteral("crosshair"), crosshairSymbol());
    return symbols;
}

#endif // CPPQTGRAPH_HAS_QT_COLOR_HEADERS

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

#if CPPQTGRAPH_HAS_QT_COLOR_HEADERS

bool SymbolPathOrder::operator()(const QString& lhs, const QString& rhs) const
{
    const int lhsIndex = symbolOrderIndex(lhs);
    const int rhsIndex = symbolOrderIndex(rhs);
    if (lhsIndex >= 0 && rhsIndex >= 0) {
        return lhsIndex < rhsIndex;
    }
    if (lhsIndex >= 0) {
        return true;
    }
    if (rhsIndex >= 0) {
        return false;
    }
    return QString::compare(lhs, rhs, Qt::CaseSensitive) < 0;
}

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

QColor hsvColor(double hue, double sat, double val, double alpha)
{
    return QColor::fromHsvF(hue, sat, val, alpha);
}

std::array<int, 4> colorTuple(const QColor& color)
{
    return {color.red(), color.green(), color.blue(), color.alpha()};
}

QString colorStr(const QColor& color)
{
    const auto channels = colorTuple(color);
    return QString::asprintf("%02x%02x%02x%02x", channels[0], channels[1], channels[2], channels[3]);
}

std::array<double, 4> glColor(const QColor& color)
{
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

std::array<double, 4> glColor(const QString& color)
{
    return glColor(mkColor(color));
}

std::array<double, 4> glColor(const char* color)
{
    return glColor(mkColor(color));
}

std::array<double, 4> glColor(std::string_view color)
{
    return glColor(mkColor(color));
}

std::array<double, 4> glColor(char color)
{
    return glColor(mkColor(color));
}

std::array<double, 4> glColor(signed char color)
{
    return glColor(mkColor(color));
}

std::array<double, 4> glColor(unsigned char color)
{
    return glColor(mkColor(color));
}

std::array<double, 4> glColor(int index)
{
    return glColor(mkColor(index));
}

std::array<double, 4> glColor(double gray)
{
    return glColor(mkColor(gray));
}

std::array<double, 4> glColor(double red, double green, double blue)
{
    return glColor(mkColor(red, green, blue));
}

std::array<double, 4> glColor(double red, double green, double blue, double alpha)
{
    return glColor(mkColor(red, green, blue, alpha));
}

std::array<double, 4> glColor(std::initializer_list<double> values)
{
    return glColor(mkColor(values));
}

Color::Color(const QColor& color)
    : QColor(mkColor(color))
{
}

Color::Color(const QString& color)
    : QColor(mkColor(color))
{
}

Color::Color(const char* color)
    : QColor(mkColor(color))
{
}

Color::Color(std::string_view color)
    : QColor(mkColor(color))
{
}

Color::Color(char color)
    : QColor(mkColor(color))
{
}

Color::Color(signed char color)
    : QColor(mkColor(color))
{
}

Color::Color(unsigned char color)
    : QColor(mkColor(color))
{
}

Color::Color(int index)
    : QColor(mkColor(index))
{
}

Color::Color(double gray)
    : QColor(mkColor(gray))
{
}

Color::Color(double red, double green, double blue)
    : QColor(mkColor(red, green, blue))
{
}

Color::Color(double red, double green, double blue, double alpha)
    : QColor(mkColor(red, green, blue, alpha))
{
}

Color::Color(std::initializer_list<double> values)
    : QColor(mkColor(values))
{
}

std::array<double, 4> Color::glColor() const
{
    return cppqtgraph::glColor(*this);
}

QPen mkPen()
{
    return mkPen(mkColor("l"), 1.0, Qt::SolidLine, true);
}

QPen mkPen(std::nullptr_t, double width, Qt::PenStyle style, bool cosmetic)
{
    (void)style;
    QPen pen;
    pen.setColor(mkColor("l"));
    pen.setWidthF(width);
    pen.setStyle(Qt::NoPen);
    pen.setCosmetic(cosmetic);
    applyWideLineCap(pen, width);
    return pen;
}

QPen mkPen(const QPen& pen)
{
    return QPen(pen);
}

QPen mkPen(const QColor& color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(const QString& color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(const char* color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(std::string_view color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(char color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(signed char color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(unsigned char color, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(color), width, style, cosmetic);
}

QPen mkPen(int index, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(index), width, style, cosmetic);
}

QPen mkPen(double gray, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(gray), width, style, cosmetic);
}

QPen mkPen(double red, double green, double blue)
{
    return makePenFromColor(mkColor(red, green, blue), 1.0, Qt::SolidLine, true);
}

QPen mkPen(double red, double green, double blue, double alpha, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(red, green, blue, alpha), width, style, cosmetic);
}

QPen mkPen(std::initializer_list<double> values, double width, Qt::PenStyle style, bool cosmetic)
{
    return makePenFromColor(mkColor(values), width, style, cosmetic);
}

QPen mkPen(const PenOptions& options)
{
    const QColor color = options.hsv.has_value()
        ? hsvColor((*options.hsv)[0], (*options.hsv)[1], (*options.hsv)[2], (*options.hsv)[3])
        : (options.color.has_value() ? mkColor(*options.color) : mkColor("l"));

    QPen pen;
    pen.setColor(color);
    pen.setWidthF(options.width);
    pen.setCosmetic(options.cosmetic);
    if (options.style.has_value()) {
        pen.setStyle(*options.style);
    }
    if (options.hasDash) {
        QList<qreal> dashPattern;
        dashPattern.reserve(static_cast<qsizetype>(options.dash.size()));
        for (const double dash : options.dash) {
            dashPattern.append(static_cast<qreal>(dash));
        }
        pen.setDashPattern(dashPattern);
    }
    applyWideLineCap(pen, options.width);
    return pen;
}

QBrush mkBrush(std::nullptr_t)
{
    return QBrush(Qt::NoBrush);
}

QBrush mkBrush(const QBrush& brush)
{
    return QBrush(brush);
}

QBrush mkBrush(const QColor& color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(const QString& color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(const char* color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(std::string_view color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(char color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(signed char color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(unsigned char color, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(color), style);
}

QBrush mkBrush(int index, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(index), style);
}

QBrush mkBrush(double gray, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(gray), style);
}

QBrush mkBrush(double red, double green, double blue)
{
    return makeBrushFromColor(mkColor(red, green, blue), Qt::SolidPattern);
}

QBrush mkBrush(double red, double green, double blue, double alpha, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(red, green, blue, alpha), style);
}

QBrush mkBrush(std::initializer_list<double> values, Qt::BrushStyle style)
{
    return makeBrushFromColor(mkColor(values), style);
}

const SymbolPathMap& symbolPaths()
{
    static const SymbolPathMap symbols = makeSymbolPaths();
    return symbols;
}

QPainterPath symbolPath(const QString& symbol)
{
    const auto& symbols = symbolPaths();
    const auto found = symbols.find(symbol);
    if (found == symbols.end()) {
        throw std::invalid_argument("Unknown scatter symbol \"" + symbol.toStdString() + "\"");
    }
    return found->second;
}

QPainterPath symbolPath(const char* symbol)
{
    if (symbol == nullptr) {
        throw std::invalid_argument("Unknown null scatter symbol");
    }
    return symbolPath(QString::fromUtf8(symbol));
}

QPainterPath symbolPath(std::string_view symbol)
{
    return symbolPath(QString::fromUtf8(symbol.data(), static_cast<qsizetype>(symbol.size())));
}

#endif // CPPQTGRAPH_HAS_QT_COLOR_HEADERS

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

std::uint8_t rescaleDataToUInt8(double value, double scale, double offset, ImageLevelRange clip)
{
    const double lower = std::max(0.0, std::trunc(clip.minimum));
    const double upper = std::min(255.0, std::trunc(clip.maximum));
    const double scaled = (value - static_cast<double>(offset)) * static_cast<double>(scale);
    const double clipped = std::clamp(scaled, lower, upper);
    return static_cast<std::uint8_t>(clipped);
}

std::size_t rescaleDataIndex(double value, double scale, double offset, std::size_t maximumIndex)
{
    const double scaled = (value - static_cast<double>(offset)) * static_cast<double>(scale);
    const double clipped = std::clamp(scaled, 0.0, static_cast<double>(maximumIndex));
    return static_cast<std::size_t>(clipped);
}

std::array<std::uint8_t, 4> applyLookupTable(std::int64_t index, const ImageLookupTable& lut)
{
    if (lut.data == nullptr || lut.rows == 0) {
        throw std::invalid_argument("lookup table must contain at least one row");
    }
    if (lut.channels != 1 && lut.channels != 3 && lut.channels != 4) {
        throw std::invalid_argument("lookup table must have 1, 3, or 4 channels");
    }

    const std::int64_t maximumIndex = static_cast<std::int64_t>(lut.rows - 1);
    const std::int64_t clippedIndex = std::clamp(index, std::int64_t{0}, maximumIndex);
    const std::uint8_t* row = lut.data + static_cast<std::ptrdiff_t>(clippedIndex) * lut.rowStride;

    if (lut.channels == 1) {
        const std::uint8_t gray = row[0];
        return {gray, gray, gray, 255};
    }

    const std::uint8_t red = row[0];
    const std::uint8_t green = row[lut.channelStride];
    const std::uint8_t blue = row[2 * lut.channelStride];
    const std::uint8_t alpha = lut.channels == 4 ? row[3 * lut.channelStride] : 255;
    return {red, green, blue, alpha};
}

} // namespace cppqtgraph
