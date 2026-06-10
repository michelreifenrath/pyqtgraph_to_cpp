// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/AxisItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/AxisItem.hpp"

#include <QtCore/QPointF>
#include <QtCore/QSizeF>
#include <QtCore/QtGlobal>
#include <QtCore/QVariant>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cppqtgraph::graphicsItems {
namespace {

constexpr double kDefaultTextWidth = 30.0;
constexpr double kDefaultTextHeight = 18.0;
constexpr double kDefaultTickLength = -5.0;
constexpr double kDefaultTickDensity = 1.0;
constexpr double kReferenceAxisSize = 300.0;
constexpr double kLog2Of10 = 3.32192809488736;

AxisItem::Orientation parseOrientation(const QString& orientation)
{
    const QString lower = orientation.toLower();
    if (lower == QStringLiteral("left")) {
        return AxisItem::Orientation::Left;
    }
    if (lower == QStringLiteral("right")) {
        return AxisItem::Orientation::Right;
    }
    if (lower == QStringLiteral("top")) {
        return AxisItem::Orientation::Top;
    }
    if (lower == QStringLiteral("bottom")) {
        return AxisItem::Orientation::Bottom;
    }
    throw std::invalid_argument("AxisItem orientation must be left, right, top, or bottom");
}

QString orientationToString(AxisItem::Orientation orientation)
{
    switch (orientation) {
    case AxisItem::Orientation::Left:
        return QStringLiteral("left");
    case AxisItem::Orientation::Right:
        return QStringLiteral("right");
    case AxisItem::Orientation::Top:
        return QStringLiteral("top");
    case AxisItem::Orientation::Bottom:
        return QStringLiteral("bottom");
    }
    return QStringLiteral("bottom");
}

bool isVertical(AxisItem::Orientation orientation)
{
    return orientation == AxisItem::Orientation::Left || orientation == AxisItem::Orientation::Right;
}

QString formatGeneral(double value)
{
    std::array<char, 64> buffer{};
    const int written = std::snprintf(buffer.data(), buffer.size(), "%g", value);
    if (written <= 0) {
        return QStringLiteral("0");
    }
    return QString::fromLatin1(buffer.data(), std::min<int>(written, static_cast<int>(buffer.size() - 1)));
}

QString formatFixed(double value, int places)
{
    std::array<char, 96> buffer{};
    const int clampedPlaces = std::clamp(places, 0, 12);
    const int written = std::snprintf(buffer.data(), buffer.size(), "%.*f", clampedPlaces, value);
    if (written <= 0) {
        return QStringLiteral("0");
    }
    return QString::fromLatin1(buffer.data(), std::min<int>(written, static_cast<int>(buffer.size() - 1)));
}

QString formatOneSignificantDigit(double value)
{
    std::array<char, 96> buffer{};
    const int written = std::snprintf(buffer.data(), buffer.size(), "%.1g", value);
    if (written <= 0) {
        return QStringLiteral("0");
    }
    return QString::fromLatin1(buffer.data(), std::min<int>(written, static_cast<int>(buffer.size() - 1)));
}

QString toSuperscriptExponent(QString exponent)
{
    QString result;
    if (exponent.startsWith(QLatin1Char('-'))) {
        result += QString::fromUtf8("⁻");
        exponent.remove(0, 1);
    } else if (exponent.startsWith(QLatin1Char('+'))) {
        exponent.remove(0, 1);
    }

    while (exponent.size() > 1 && exponent.startsWith(QLatin1Char('0'))) {
        exponent.remove(0, 1);
    }

    for (const QChar digit : exponent) {
        if (digit == QLatin1Char('0')) {
            result += QString::fromUtf8("⁰");
        } else if (digit == QLatin1Char('1')) {
            result += QString::fromUtf8("¹");
        } else if (digit == QLatin1Char('2')) {
            result += QString::fromUtf8("²");
        } else if (digit == QLatin1Char('3')) {
            result += QString::fromUtf8("³");
        } else if (digit == QLatin1Char('4')) {
            result += QString::fromUtf8("⁴");
        } else if (digit == QLatin1Char('5')) {
            result += QString::fromUtf8("⁵");
        } else if (digit == QLatin1Char('6')) {
            result += QString::fromUtf8("⁶");
        } else if (digit == QLatin1Char('7')) {
            result += QString::fromUtf8("⁷");
        } else if (digit == QLatin1Char('8')) {
            result += QString::fromUtf8("⁸");
        } else if (digit == QLatin1Char('9')) {
            result += QString::fromUtf8("⁹");
        }
    }
    return result;
}

std::pair<double, QString> siScale(double value, double power)
{
    if (!std::isfinite(value)) {
        return {1.0, QString{}};
    }

    int magnitude = 0;
    if (std::abs(value) >= 1.0e-25) {
        const double denominator = std::log(1000.0) * power;
        double log1000 = std::log(std::abs(value)) / denominator;
        log1000 = power > 0.0 ? std::floor(log1000) : std::ceil(log1000);
        magnitude = static_cast<int>(std::clamp(log1000, -9.0, 9.0));
    }

    QString prefix;
    if (magnitude == 0) {
        prefix = QString{};
    } else if (magnitude < -8 || magnitude > 8) {
        prefix = QStringLiteral("e%1").arg(magnitude * 3);
    } else {
        static const std::array<QString, 17> prefixes = {
            QStringLiteral("y"),
            QStringLiteral("z"),
            QStringLiteral("a"),
            QStringLiteral("f"),
            QStringLiteral("p"),
            QStringLiteral("n"),
            QString::fromUtf8("µ"),
            QStringLiteral("m"),
            QString{},
            QStringLiteral("k"),
            QStringLiteral("M"),
            QStringLiteral("G"),
            QStringLiteral("T"),
            QStringLiteral("P"),
            QStringLiteral("E"),
            QStringLiteral("Z"),
            QStringLiteral("Y"),
        };
        prefix = prefixes.at(static_cast<std::size_t>(magnitude + 8));
    }

    return {std::pow(10.0, -3.0 * static_cast<double>(magnitude) * power), prefix};
}

bool almostMatchesExisting(const std::vector<double>& values, double candidate, double tolerance)
{
    return std::any_of(values.begin(), values.end(), [candidate, tolerance](double existing) {
        return std::abs(existing - candidate) <= tolerance;
    });
}

QRectF localWidgetRect(const QGraphicsWidget& widget)
{
    QRectF rect = widget.QGraphicsWidget::boundingRect();
    if (rect.width() <= 0.0 || rect.height() <= 0.0) {
        const QSizeF size = widget.geometry().size();
        rect = QRectF(QPointF(0.0, 0.0), size);
    }
    return rect;
}

double deviceAxisLength(const QPainter& painter, const QRectF& bounds, bool vertical)
{
    const QPointF start = vertical ? QPointF(bounds.center().x(), bounds.top())
                                   : QPointF(bounds.left(), bounds.center().y());
    const QPointF stop = vertical ? QPointF(bounds.center().x(), bounds.bottom())
                                  : QPointF(bounds.right(), bounds.center().y());
    const QTransform transform = painter.deviceTransform();
    const QPointF mappedStart = transform.map(start);
    const QPointF mappedStop = transform.map(stop);
    const double length = std::hypot(mappedStop.x() - mappedStart.x(), mappedStop.y() - mappedStart.y());
    if (std::isfinite(length) && length > 0.0) {
        return length;
    }
    return vertical ? bounds.height() : bounds.width();
}

} // namespace

struct AxisItem::Private {
    explicit Private(AxisItem& owner, Orientation axisOrientation)
        : label(new QGraphicsTextItem(&owner))
        , orientation(axisOrientation)
    {
        if (isVertical(orientation)) {
            label->setRotation(-90.0);
            hideOverlappingLabels = false;
        }
        label->setVisible(false);
    }

    QGraphicsTextItem* label = nullptr;
    Orientation orientation = Orientation::Bottom;
    std::pair<double, double> range = {0.0, 1.0};
    double scale = 1.0;
    bool logMode = false;
    double tickDensity = kDefaultTickDensity;
    std::optional<std::vector<std::vector<ExplicitTick>>> tickLevels;
    std::optional<std::vector<TickSpacing>> tickSpacing;

    QString labelText;
    QString labelUnits;
    QString explicitLabelUnitPrefix;
    QString labelUnitPrefix;
    double unitPower = 1.0;
    std::optional<std::vector<std::pair<double, double>>> siPrefixEnableRanges;
    bool autoSIPrefix = true;
    double autoSIPrefixScale = 1.0;

    QPen axisPen = QPen(Qt::white);
    QPen textPen = QPen(Qt::white);
    std::optional<QPen> explicitTickPen;
    std::optional<QFont> tickFont;

    std::array<double, 2> tickTextOffset = {5.0, 2.0};
    double tickTextWidth = kDefaultTextWidth;
    double tickTextHeight = kDefaultTextHeight;
    bool autoExpandTextSpace = true;
    bool autoReduceTextSpace = true;
    bool hideOverlappingLabels = true;
    bool showValues = true;
    double tickLength = kDefaultTickLength;
    int maxTickLevel = 2;
    int maxTextLevel = 2;
    std::pair<bool, bool> stopAxisAtTick = {false, false};
    std::optional<double> fixedWidth;
    std::optional<double> fixedHeight;
    double textWidth = kDefaultTextWidth;
    double textHeight = kDefaultTextHeight;
};

AxisItem::AxisItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : AxisItem(Orientation::Bottom, parent, flags)
{
}

AxisItem::AxisItem(Orientation orientation, QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
    , d_(std::make_unique<Private>(*this, orientation))
{
    setRange(0.0, 1.0);
    setLabel();
    setPen(QPen(Qt::white));
    setTextPen(QPen(Qt::white));
    setTickPen(std::nullopt);
}

AxisItem::AxisItem(const QString& orientation, QGraphicsItem* parent, Qt::WindowFlags flags)
    : AxisItem(parseOrientation(orientation), parent, flags)
{
}

AxisItem::~AxisItem() = default;

AxisItem::Orientation AxisItem::orientation() const noexcept
{
    return d_->orientation;
}

QString AxisItem::orientationName() const
{
    return orientationToString(d_->orientation);
}

void AxisItem::show()
{
    QGraphicsWidget::show();
    updateSize();
}

void AxisItem::hide()
{
    QGraphicsWidget::hide();
    updateSize();
}

void AxisItem::setRange(double minimum, double maximum)
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
        throw std::invalid_argument("AxisItem::setRange requires finite values");
    }
    d_->range = {minimum, maximum};
    if (d_->autoSIPrefix) {
        updateAutoSIPrefix();
    } else {
        update();
    }
}

std::pair<double, double> AxisItem::range() const noexcept
{
    return d_->range;
}

void AxisItem::setLabel(
    const QString& text,
    const QString& units,
    const QString& unitPrefix,
    std::optional<std::vector<std::pair<double, double>>> siPrefixEnableRanges,
    double unitPower)
{
    d_->labelText = text;
    d_->labelUnits = units;
    d_->explicitLabelUnitPrefix = unitPrefix;
    d_->labelUnitPrefix = unitPrefix;
    d_->unitPower = unitPower;
    setSIPrefixEnableRanges(std::move(siPrefixEnableRanges));
    showLabel(!text.isEmpty() || !units.isEmpty());
    updateAutoSIPrefix();
}

void AxisItem::showLabel(bool show)
{
    d_->label->setVisible(show);
    updateAutoSIPrefix();
}

bool AxisItem::labelVisible() const
{
    return d_->label->isVisible();
}

QString AxisItem::labelText() const
{
    return d_->labelText;
}

QString AxisItem::labelUnits() const
{
    return d_->labelUnits;
}

QString AxisItem::labelString() const
{
    QString units;
    if (d_->labelUnits.isEmpty()) {
        if (!d_->autoSIPrefix || qFuzzyCompare(d_->autoSIPrefixScale, 1.0)) {
            units = QString{};
        } else {
            units = QStringLiteral("(x%1)").arg(1.0 / d_->autoSIPrefixScale, 0, 'g', 6);
        }
    } else {
        units = QStringLiteral("(%1%2)").arg(d_->labelUnitPrefix, d_->labelUnits);
    }

    return QStringLiteral("<span style='color: %1'>%2 %3</span>")
        .arg(d_->textPen.color().name(), d_->labelText, units);
}

void AxisItem::updateSize()
{
    if (isVertical(d_->orientation)) {
        setWidth(d_->fixedWidth);
    } else {
        setHeight(d_->fixedHeight);
    }
}

void AxisItem::setSIPrefixEnableRanges(std::optional<std::vector<std::pair<double, double>>> ranges)
{
    d_->siPrefixEnableRanges = std::move(ranges);
    updateAutoSIPrefix();
}

std::vector<std::pair<double, double>> AxisItem::siPrefixEnableRanges() const
{
    if (d_->siPrefixEnableRanges.has_value()) {
        return *d_->siPrefixEnableRanges;
    }
    if (d_->labelUnits.isEmpty()) {
        return {{0.0, 1.0}, {1.0e9, std::numeric_limits<double>::infinity()}};
    }
    return {{0.0, std::numeric_limits<double>::infinity()}};
}

void AxisItem::enableAutoSIPrefix(bool enable)
{
    d_->autoSIPrefix = enable;
    updateAutoSIPrefix();
}

void AxisItem::updateAutoSIPrefix()
{
    double prefixScale = 1.0;
    QString prefix = d_->autoSIPrefix ? QString{} : d_->explicitLabelUnitPrefix;
    if (d_->autoSIPrefix && d_->label->isVisible()) {
        const double first = d_->logMode ? std::pow(10.0, d_->range.first) : d_->range.first;
        const double second = d_->logMode ? std::pow(10.0, d_->range.second) : d_->range.second;
        const double scalingValue = std::max(std::abs(first), std::abs(second)) * d_->scale;
        const auto ranges = siPrefixEnableRanges();
        const bool enabled = std::any_of(ranges.begin(), ranges.end(), [scalingValue](const auto& range) {
            return range.first <= scalingValue && scalingValue <= range.second;
        });
        if (enabled) {
            std::tie(prefixScale, prefix) = siScale(scalingValue, d_->unitPower);
        }
    }
    d_->autoSIPrefixScale = prefixScale;
    d_->labelUnitPrefix = prefix;
    d_->label->setHtml(labelString());
    updateSize();
    updateLabelPosition();
    update();
}

double AxisItem::autoSIPrefixScale() const noexcept
{
    return d_->autoSIPrefixScale;
}

QString AxisItem::labelUnitPrefix() const
{
    return d_->labelUnitPrefix;
}

void AxisItem::setPen(const QPen& pen)
{
    d_->axisPen = pen;
    update();
}

QPen AxisItem::pen() const
{
    return d_->axisPen;
}

void AxisItem::setTextPen(const QPen& pen)
{
    d_->textPen = pen;
    d_->label->setDefaultTextColor(pen.color());
    d_->label->setHtml(labelString());
    updateLabelPosition();
    update();
}

QPen AxisItem::textPen() const
{
    return d_->textPen;
}

void AxisItem::setTickPen(std::optional<QPen> pen)
{
    d_->explicitTickPen = std::move(pen);
    update();
}

QPen AxisItem::tickPen() const
{
    return d_->explicitTickPen.value_or(d_->axisPen);
}

void AxisItem::setTickFont(std::optional<QFont> font)
{
    d_->tickFont = std::move(font);
    update();
}

void AxisItem::setShowValues(bool showValues)
{
    d_->showValues = showValues;
    if (isVertical(d_->orientation)) {
        setWidth(d_->fixedWidth);
    } else {
        setHeight(d_->fixedHeight);
    }
    update();
}

void AxisItem::setTickLength(double length)
{
    if (!qFuzzyCompare(d_->tickLength, length)) {
        prepareGeometryChange();
    }
    d_->tickLength = length;
    if (isVertical(d_->orientation)) {
        setWidth(d_->fixedWidth);
    } else {
        setHeight(d_->fixedHeight);
    }
    update();
}

void AxisItem::setMaxTickLevel(int level)
{
    d_->maxTickLevel = std::max(0, level);
    update();
}

void AxisItem::setMaxTextLevel(int level)
{
    d_->maxTextLevel = std::max(0, level);
    update();
}

void AxisItem::setTickDensity(double density)
{
    d_->tickDensity = std::max(0.01, density);
    update();
}

void AxisItem::setStopAxisAtTick(bool stopAtMinimumTick, bool stopAtMaximumTick)
{
    d_->stopAxisAtTick = {stopAtMinimumTick, stopAtMaximumTick};
    update();
}

void AxisItem::setHeight(std::optional<double> height)
{
    d_->fixedHeight = height;
    double newHeight = height.value_or(0.0);
    if (!height.has_value()) {
        if (!isVisible()) {
            newHeight = 0.0;
        } else {
            if (d_->showValues) {
                newHeight = d_->autoExpandTextSpace ? d_->textHeight : d_->tickTextHeight;
                newHeight += d_->tickTextOffset[1];
            }
            newHeight += std::max(0.0, d_->tickLength);
            if (d_->label->isVisible()) {
                newHeight += d_->label->boundingRect().height() * 0.8;
            }
        }
    }
    setMaximumHeight(newHeight);
    setMinimumHeight(newHeight);
}

void AxisItem::setWidth(std::optional<double> width)
{
    d_->fixedWidth = width;
    double newWidth = width.value_or(0.0);
    if (!width.has_value()) {
        if (!isVisible()) {
            newWidth = 0.0;
        } else {
            if (d_->showValues) {
                newWidth = d_->autoExpandTextSpace ? d_->textWidth : d_->tickTextWidth;
                newWidth += d_->tickTextOffset[0];
            }
            newWidth += std::max(0.0, d_->tickLength);
            if (d_->label->isVisible()) {
                newWidth += d_->label->boundingRect().height() * 0.8;
            }
        }
    }
    setMaximumWidth(newWidth);
    setMinimumWidth(newWidth);
}

void AxisItem::setScale(double scale)
{
    if (!qFuzzyCompare(d_->scale, scale)) {
        d_->scale = scale;
        updateAutoSIPrefix();
    }
}

void AxisItem::setLogMode(bool enabled)
{
    d_->logMode = enabled;
    updateAutoSIPrefix();
}

void AxisItem::setLogMode(bool xEnabled, bool yEnabled)
{
    d_->logMode = isVertical(d_->orientation) ? yEnabled : xEnabled;
    updateAutoSIPrefix();
}

void AxisItem::setTicks(const std::vector<std::vector<ExplicitTick>>& ticks)
{
    d_->tickLevels = ticks;
    update();
}

void AxisItem::clearTicks()
{
    d_->tickLevels.reset();
    update();
}

void AxisItem::setTickSpacing(std::optional<double> major, std::optional<double> minor)
{
    if (major.has_value()) {
        d_->tickSpacing = std::vector<TickSpacing>{{*major, 0.0}, {minor.value_or(*major), 0.0}};
    } else {
        d_->tickSpacing.reset();
    }
    d_->tickLevels.reset();
    update();
}

void AxisItem::setTickSpacingLevels(std::optional<std::vector<TickSpacing>> levels)
{
    d_->tickSpacing = std::move(levels);
    d_->tickLevels.reset();
    update();
}

std::vector<AxisItem::TickSpacing> AxisItem::tickSpacing(double minimum, double maximum, double size) const
{
    if (d_->tickSpacing.has_value()) {
        return *d_->tickSpacing;
    }

    const double difference = std::abs(maximum - minimum);
    if (difference == 0.0 || size <= 0.0 || !std::isfinite(difference)) {
        return {};
    }

    const double minIntervals = std::max(2.25, 2.25 * d_->tickDensity * std::sqrt(size / kReferenceAxisSize));
    const double majorMaxSpacing = difference / minIntervals;
    int exponent2 = 0;
    std::frexp(majorMaxSpacing, &exponent2);
    double p10unit = std::pow(10.0, std::floor((static_cast<double>(exponent2) - 1.0) / kLog2Of10) - 1.0);

    int majorScaleFactor = 10;
    if (100.0 * p10unit <= majorMaxSpacing) {
        majorScaleFactor = 10;
        p10unit *= 10.0;
    } else {
        for (const int candidate : {50, 20, 10}) {
            if (static_cast<double>(candidate) * p10unit <= majorMaxSpacing) {
                majorScaleFactor = candidate;
                break;
            }
        }
    }

    std::vector<TickSpacing> levels{{static_cast<double>(majorScaleFactor) * p10unit, 0.0}};
    if (d_->maxTickLevel >= 1) {
        const double minorMinSpacing = 2.0 * difference / size;
        double minorInterval = levels.front().spacing;
        const std::vector<int> trials = majorScaleFactor == 10 ? std::vector<int>{5, 10} : std::vector<int>{10, 20, 50};
        for (const int candidate : trials) {
            minorInterval = static_cast<double>(candidate) * p10unit;
            if (minorInterval >= minorMinSpacing) {
                break;
            }
        }
        levels.push_back({minorInterval, 0.0});

        if (d_->maxTickLevel >= 2) {
            std::vector<int> extraTrials;
            if (majorScaleFactor == 10) {
                extraTrials = {1, 2, 5, 10};
            } else if (majorScaleFactor == 20) {
                extraTrials = {2, 5, 10, 20};
            } else if (majorScaleFactor == 50) {
                extraTrials = {5, 10, 50};
            }
            double extraInterval = minorInterval;
            for (const int candidate : extraTrials) {
                extraInterval = static_cast<double>(candidate) * p10unit;
                if (extraInterval >= minorMinSpacing || qFuzzyCompare(extraInterval, minorInterval)) {
                    break;
                }
            }
            if (extraInterval < minorInterval) {
                levels.push_back({extraInterval, 0.0});
            }
        }
    }

    return levels;
}

std::vector<AxisItem::TickLevel> AxisItem::tickValues(double minimum, double maximum, double size) const
{
    if (minimum > maximum) {
        std::swap(minimum, maximum);
    }

    const double tickValueScale = d_->tickSpacing.has_value() ? 1.0 : d_->scale;
    const double scaledMinimum = minimum * tickValueScale;
    const double scaledMaximum = maximum * tickValueScale;
    const auto spacings = tickSpacing(scaledMinimum, scaledMaximum, size);
    std::vector<TickLevel> result;
    std::vector<double> allValues;
    for (const TickSpacing& spacing : spacings) {
        if (spacing.spacing <= 0.0) {
            continue;
        }
        const double start = std::ceil((scaledMinimum - spacing.offset) / spacing.spacing) * spacing.spacing
            + spacing.offset;
        const int count = static_cast<int>((scaledMaximum - start) / spacing.spacing) + 1;
        std::vector<double> values;
        for (int index = 0; index < count; ++index) {
            const double value = (static_cast<double>(index) * spacing.spacing + start) / tickValueScale;
            const double tolerance = std::abs(spacing.spacing / tickValueScale * 0.01);
            if (!almostMatchesExisting(allValues, value, tolerance)) {
                values.push_back(value);
                allValues.push_back(value);
            }
        }
        result.push_back({spacing.spacing / tickValueScale, values});
    }

    if (d_->logMode) {
        std::vector<TickLevel> logLevels;
        for (const TickLevel& level : result) {
            if (level.spacing >= 1.0) {
                logLevels.push_back(level);
            }
        }
        const std::size_t maximumReturnedLevels = static_cast<std::size_t>(d_->maxTickLevel) + 1U;
        if (logLevels.size() < 3U && logLevels.size() < maximumReturnedLevels) {
            const int firstExponent = static_cast<int>(std::floor(scaledMinimum));
            const int lastExponent = static_cast<int>(std::ceil(scaledMaximum));
            std::vector<double> majorValues;
            for (const TickLevel& level : logLevels) {
                majorValues.insert(majorValues.end(), level.values.begin(), level.values.end());
            }
            std::vector<double> minorValues;
            for (int exponent = firstExponent; exponent < lastExponent; ++exponent) {
                for (int multiplier = 2; multiplier < 10; ++multiplier) {
                    const double value = static_cast<double>(exponent) + std::log10(static_cast<double>(multiplier));
                    if (value > scaledMinimum && value < scaledMaximum
                        && !almostMatchesExisting(majorValues, value, 1.0e-12)) {
                        minorValues.push_back(value);
                    }
                }
            }
            logLevels.push_back({0.0, std::move(minorValues)});
        }
        return logLevels;
    }

    return result;
}

std::vector<QString> AxisItem::tickStrings(const std::vector<double>& values, double scale, double spacing) const
{
    std::vector<QString> strings;
    strings.reserve(values.size());
    if (d_->logMode) {
        for (double value : values) {
            const QString exponentString = formatOneSignificantDigit(std::pow(10.0, value) * scale);
            const qsizetype exponentMarker = exponentString.indexOf(QLatin1Char('e'));
            if (exponentMarker < 0) {
                strings.push_back(exponentString);
                continue;
            }
            QString mantissa = exponentString.left(exponentMarker);
            const QString exponent = exponentString.mid(exponentMarker + 1);
            if (mantissa == QStringLiteral("1")) {
                mantissa.clear();
            } else {
                mantissa += QString::fromUtf8("·");
            }
            strings.push_back(mantissa + QStringLiteral("10") + toSuperscriptExponent(exponent));
        }
        return strings;
    }

    const double scaledSpacing = spacing * scale;
    const int places = scaledSpacing > 0.0 ? std::max(0, static_cast<int>(std::ceil(-std::log10(scaledSpacing)))) : 0;
    for (double value : values) {
        const double scaledValue = value * scale;
        if (std::abs(scaledValue) < 0.001 || std::abs(scaledValue) >= 10000.0) {
            strings.push_back(formatGeneral(scaledValue));
        } else {
            strings.push_back(formatFixed(scaledValue, places));
        }
    }
    return strings;
}

std::optional<AxisItem::DrawSpecs> AxisItem::generateDrawSpecs(QPainter& painter) const
{
    if (d_->tickFont.has_value()) {
        painter.setFont(*d_->tickFont);
    }

    const QRectF bounds = localWidgetRect(*this);
    const bool vertical = isVertical(d_->orientation);
    const double localLength = vertical ? bounds.height() : bounds.width();
    if (localLength <= 0.0) {
        return std::nullopt;
    }
    const double spacingLength = deviceAxisLength(painter, bounds, vertical);

    QLineF span;
    double tickStart = 0.0;
    double tickStop = 0.0;
    int tickDirection = 1;
    if (d_->orientation == Orientation::Left) {
        span = QLineF(bounds.right() - 1.0, bounds.top() - 1.0, bounds.right() - 1.0, bounds.bottom() + 1.0);
        tickStart = bounds.right();
        tickStop = bounds.right();
        tickDirection = -1;
    } else if (d_->orientation == Orientation::Right) {
        span = QLineF(bounds.left() + 1.0, bounds.top() - 1.0, bounds.left() + 1.0, bounds.bottom() + 1.0);
        tickStart = bounds.left();
        tickStop = bounds.left();
        tickDirection = 1;
    } else if (d_->orientation == Orientation::Top) {
        span = QLineF(bounds.left() - 1.0, bounds.bottom() - 1.0, bounds.right() + 1.0, bounds.bottom() - 1.0);
        tickStart = bounds.bottom();
        tickStop = bounds.bottom();
        tickDirection = -1;
    } else {
        span = QLineF(bounds.left() - 1.0, bounds.top() + 1.0, bounds.right() + 1.0, bounds.top() + 1.0);
        tickStart = bounds.top();
        tickStop = bounds.top();
        tickDirection = 1;
    }

    std::vector<TickLevel> levels;
    std::vector<std::vector<QString>> explicitStrings;
    if (d_->tickLevels.has_value()) {
        for (const auto& level : *d_->tickLevels) {
            TickLevel tickLevel;
            tickLevel.spacing = 0.0;
            std::vector<QString> strings;
            for (const ExplicitTick& tick : level) {
                tickLevel.values.push_back(tick.value);
                strings.push_back(tick.text);
            }
            levels.push_back(std::move(tickLevel));
            explicitStrings.push_back(std::move(strings));
        }
    } else {
        levels = tickValues(d_->range.first, d_->range.second, spacingLength);
    }

    const double difference = d_->range.second - d_->range.first;
    double coordinateScale = 1.0;
    double offset = 0.0;
    if (difference != 0.0) {
        if (vertical) {
            coordinateScale = -bounds.height() / difference;
            offset = d_->range.first * coordinateScale - bounds.height();
        } else {
            coordinateScale = bounds.width() / difference;
            offset = d_->range.first * coordinateScale;
        }
    }

    const double firstRangeCoordinate = d_->range.first * coordinateScale - offset;
    const double secondRangeCoordinate = d_->range.second * coordinateScale - offset;
    const double coordinateMinimum = std::min(firstRangeCoordinate, secondRangeCoordinate) - 0.5;
    const double coordinateMaximum = std::max(firstRangeCoordinate, secondRangeCoordinate) + 0.5;

    DrawSpecs specs;
    specs.axisPen = d_->axisPen;
    std::vector<std::vector<std::optional<double>>> tickPositions;
    std::vector<double> validPositions;

    for (std::size_t levelIndex = 0; levelIndex < levels.size(); ++levelIndex) {
        const auto& values = levels[levelIndex].values;
        tickPositions.emplace_back();
        const double tickLength = d_->tickLength / ((static_cast<double>(levelIndex) * 0.5) + 1.0);
        QPen levelPen = tickPen();
        if (levelPen.brush().style() == Qt::SolidPattern) {
            QColor color = levelPen.color();
            const int configuredAlpha = color.alpha();
            color.setAlpha(static_cast<int>(std::round(static_cast<double>(configuredAlpha) / (static_cast<double>(levelIndex) + 1.0))));
            levelPen.setColor(color);
        }

        for (double value : values) {
            const double coordinate = value * coordinateScale - offset;
            if (coordinate < coordinateMinimum || coordinate > coordinateMaximum) {
                tickPositions.back().push_back(std::nullopt);
                continue;
            }
            tickPositions.back().push_back(coordinate);
            validPositions.push_back(coordinate);

            QLineF line;
            if (vertical) {
                line = QLineF(tickStart, coordinate, tickStop + tickLength * static_cast<double>(tickDirection), coordinate);
            } else {
                line = QLineF(coordinate, tickStart, coordinate, tickStop + tickLength * static_cast<double>(tickDirection));
            }
            specs.ticks.push_back({levelPen, line});
        }
    }

    if (!validPositions.empty()) {
        const auto [minimumTick, maximumTick] = std::minmax_element(validPositions.begin(), validPositions.end());
        if (d_->stopAxisAtTick.first) {
            if (vertical) {
                span.setP1(QPointF(span.p1().x(), std::max(span.p1().y(), *minimumTick)));
            } else {
                span.setP1(QPointF(std::max(span.p1().x(), *minimumTick), span.p1().y()));
            }
        }
        if (d_->stopAxisAtTick.second) {
            if (vertical) {
                span.setP2(QPointF(span.p2().x(), std::min(span.p2().y(), *maximumTick)));
            } else {
                span.setP2(QPointF(std::min(span.p2().x(), *maximumTick), span.p2().y()));
            }
        }
    }
    specs.axisLine = span;

    if (!d_->showValues) {
        return specs;
    }

    const int textLevels = std::min<int>(static_cast<int>(levels.size()), d_->maxTextLevel + 1);
    double largestTextOrthogonal = 0.0;
    double cumulativePrimaryTextSize = 0.0;
    for (int levelIndex = 0; levelIndex < textLevels; ++levelIndex) {
        std::vector<QString> strings;
        if (!explicitStrings.empty()) {
            strings = explicitStrings[static_cast<std::size_t>(levelIndex)];
        } else {
            strings = tickStrings(
                levels[static_cast<std::size_t>(levelIndex)].values,
                d_->autoSIPrefixScale * d_->scale,
                levels[static_cast<std::size_t>(levelIndex)].spacing);
        }

        std::vector<TextSpec> levelText;
        double levelPrimaryTextSize = 0.0;
        double levelLargestOrthogonal = 0.0;
        for (std::size_t index = 0; index < strings.size(); ++index) {
            if (index >= tickPositions[static_cast<std::size_t>(levelIndex)].size()) {
                continue;
            }
            const std::optional<double> coordinate = tickPositions[static_cast<std::size_t>(levelIndex)][index];
            if (!coordinate.has_value() || strings[index].isEmpty()) {
                continue;
            }

            QRectF textBounds = painter.boundingRect(QRectF(0.0, 0.0, 160.0, 80.0), Qt::AlignCenter, strings[index]);
            textBounds.setHeight(textBounds.height() * 0.8);
            const double textWidth = textBounds.width();
            const double textHeight = textBounds.height();
            const std::size_t axisIndex = vertical ? 0U : 1U;
            const double textOffset = d_->tickTextOffset[axisIndex];
            const double offsetToText = std::max(0.0, d_->tickLength) + textOffset;
            TextSpec textSpec;
            textSpec.text = strings[index];

            if (d_->orientation == Orientation::Left) {
                textSpec.alignment = Qt::AlignRight | Qt::AlignVCenter;
                textSpec.rect = QRectF(tickStop - offsetToText - textWidth, *coordinate - (textHeight / 2.0), textWidth, textHeight);
                levelPrimaryTextSize += textHeight;
                levelLargestOrthogonal = std::max(levelLargestOrthogonal, textWidth);
            } else if (d_->orientation == Orientation::Right) {
                textSpec.alignment = Qt::AlignLeft | Qt::AlignVCenter;
                textSpec.rect = QRectF(tickStop + offsetToText, *coordinate - (textHeight / 2.0), textWidth, textHeight);
                levelPrimaryTextSize += textHeight;
                levelLargestOrthogonal = std::max(levelLargestOrthogonal, textWidth);
            } else if (d_->orientation == Orientation::Top) {
                textSpec.alignment = Qt::AlignHCenter | Qt::AlignBottom;
                textSpec.rect = QRectF(*coordinate - (textWidth / 2.0), tickStop - offsetToText - textHeight, textWidth, textHeight);
                levelPrimaryTextSize += textWidth;
                levelLargestOrthogonal = std::max(levelLargestOrthogonal, textHeight);
            } else {
                textSpec.alignment = Qt::AlignHCenter | Qt::AlignTop;
                textSpec.rect = QRectF(*coordinate - (textWidth / 2.0), tickStop + offsetToText, textWidth, textHeight);
                levelPrimaryTextSize += textWidth;
                levelLargestOrthogonal = std::max(levelLargestOrthogonal, textHeight);
            }

            if ((boundingRect() & textSpec.rect) == textSpec.rect) {
                levelText.push_back(std::move(textSpec));
            }
        }

        if (levelIndex > 0) {
            const std::array<std::pair<int, double>, 4> fillLimits = {{{0, 0.8}, {2, 0.6}, {4, 0.4}, {6, 0.2}}};
            const double fillRatio = (cumulativePrimaryTextSize + levelPrimaryTextSize) / localLength;
            bool crowded = false;
            for (const auto& [minimumTexts, limit] : fillLimits) {
                if (specs.text.size() >= static_cast<std::size_t>(minimumTexts) && fillRatio >= limit) {
                    crowded = true;
                    break;
                }
            }
            if (crowded) {
                break;
            }
        }

        cumulativePrimaryTextSize += levelPrimaryTextSize;
        largestTextOrthogonal = std::max(largestTextOrthogonal, levelLargestOrthogonal);
        specs.text.insert(specs.text.end(), std::make_move_iterator(levelText.begin()), std::make_move_iterator(levelText.end()));
    }

    if (d_->autoExpandTextSpace && largestTextOrthogonal > 0.0) {
        bool textSpaceChanged = false;
        if (vertical) {
            double updatedTextWidth = largestTextOrthogonal;
            if (!d_->autoReduceTextSpace) {
                updatedTextWidth = std::max(d_->textWidth, updatedTextWidth);
            }
            if (updatedTextWidth > d_->textWidth || updatedTextWidth < d_->textWidth - 10.0) {
                d_->textWidth = updatedTextWidth;
                textSpaceChanged = true;
            }
        } else {
            double updatedTextHeight = largestTextOrthogonal;
            if (!d_->autoReduceTextSpace) {
                updatedTextHeight = std::max(d_->textHeight, updatedTextHeight);
            }
            if (updatedTextHeight > d_->textHeight || updatedTextHeight < d_->textHeight - 10.0) {
                d_->textHeight = updatedTextHeight;
                textSpaceChanged = true;
            }
        }

        if (textSpaceChanged) {
            if (vertical) {
                const_cast<AxisItem*>(this)->setWidth(d_->fixedWidth);
            } else {
                const_cast<AxisItem*>(this)->setHeight(d_->fixedHeight);
            }
        }
    }

    return specs;
}

QRectF AxisItem::boundingRect() const
{
    QRectF rect = localWidgetRect(*this);
    double margin = 0.0;
    if (!d_->hideOverlappingLabels) {
        margin = 15.0;
    }

    const double tickLength = d_->tickLength;
    if (d_->orientation == Orientation::Left) {
        rect = rect.adjusted(0.0, -margin, -std::min(0.0, tickLength), margin);
    } else if (d_->orientation == Orientation::Right) {
        rect = rect.adjusted(std::min(0.0, tickLength), -margin, 0.0, margin);
    } else if (d_->orientation == Orientation::Top) {
        rect = rect.adjusted(-margin, 0.0, margin, -std::min(0.0, tickLength));
    } else {
        rect = rect.adjusted(-margin, std::min(0.0, tickLength), margin, 0.0);
    }
    return rect;
}

QPainterPath AxisItem::shape() const
{
    QPainterPath path;
    path.addRect(localWidgetRect(*this));
    return path;
}

void AxisItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    const std::optional<DrawSpecs> specs = generateDrawSpecs(*painter);
    if (!specs.has_value()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setPen(specs->axisPen);
    painter->drawLine(specs->axisLine);

    for (const auto& [tickPen, tickLine] : specs->ticks) {
        painter->setPen(tickPen);
        painter->drawLine(tickLine);
    }

    if (d_->tickFont.has_value()) {
        painter->setFont(*d_->tickFont);
    }
    painter->setPen(d_->textPen);
    painter->setClipRect(boundingRect().toAlignedRect());
    for (const TextSpec& textSpec : specs->text) {
        painter->drawText(textSpec.rect, static_cast<int>(textSpec.alignment | Qt::TextDontClip), textSpec.text);
    }
}

QVariant AxisItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsWidget::itemChange(change, value);
    if (change == QGraphicsItem::ItemVisibleHasChanged && d_) {
        updateSize();
    }
    return result;
}

void AxisItem::updateLabelPosition()
{
    const QRectF labelBounds = d_->label->boundingRect();
    const QSizeF itemSize = geometry().size();
    constexpr double nudge = 5.0;
    QPointF position;
    if (d_->orientation == Orientation::Left) {
        position.setY(std::floor(itemSize.height() / 2.0 + labelBounds.width() / 2.0));
        position.setX(-nudge);
    } else if (d_->orientation == Orientation::Right) {
        position.setY(std::floor(itemSize.height() / 2.0 + labelBounds.width() / 2.0));
        position.setX(std::floor(itemSize.width() - labelBounds.height() + nudge));
    } else if (d_->orientation == Orientation::Top) {
        position.setY(-nudge);
        position.setX(std::floor(itemSize.width() / 2.0 - labelBounds.width() / 2.0));
    } else {
        position.setX(std::floor(itemSize.width() / 2.0 - labelBounds.width() / 2.0));
        position.setY(std::floor(itemSize.height() - labelBounds.height() + nudge));
    }
    d_->label->setPos(position);
}

void AxisItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    GraphicsWidget::resizeEvent(event);
    updateLabelPosition();
}

} // namespace cppqtgraph::graphicsItems
