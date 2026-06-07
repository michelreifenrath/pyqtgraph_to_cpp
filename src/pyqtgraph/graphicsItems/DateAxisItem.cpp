// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/DateAxisItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/DateAxisItem.hpp"

#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QLineF>
#include <QtCore/QLocale>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QTime>
#include <QtCore/QTimeZone>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtGui/QPen>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace pyqtgraph::graphicsItems {
namespace {

constexpr double kMsSpacing = 1.0 / 1000.0;
constexpr double kSecondSpacing = 1.0;
constexpr double kMinuteSpacing = 60.0;
constexpr double kHourSpacing = 3600.0;
constexpr double kDaySpacing = 24.0 * kHourSpacing;
constexpr double kWeekSpacing = 7.0 * kDaySpacing;
constexpr double kMonthSpacing = 30.0 * kDaySpacing;
constexpr double kYearSpacing = 365.0 * kDaySpacing;
constexpr double kSecPerYear = 365.25 * 24.0 * 3600.0;
constexpr double kMinRegularTimestamp = -62135596800.0;
constexpr double kMaxRegularTimestamp = 253370764800.0;
constexpr double kDefaultTickLength = -5.0;
constexpr double kTickTextOffset = 2.0;
constexpr int kTextPadding = 10;

bool isVertical(AxisItem::Orientation orientation)
{
    return orientation == AxisItem::Orientation::Left || orientation == AxisItem::Orientation::Right;
}

bool regularTimestamp(double value)
{
    return value >= kMinRegularTimestamp && value <= kMaxRegularTimestamp && std::isfinite(value);
}

QTimeZone utcZone()
{
    return QTimeZone::utc();
}

QDateTime utcDateTimeFromSeconds(double seconds)
{
    if (!regularTimestamp(seconds)) {
        return {};
    }
    const double msecs = std::floor(seconds * 1000.0 + 1.0e-7);
    if (msecs < static_cast<double>(std::numeric_limits<qint64>::min())
        || msecs > static_cast<double>(std::numeric_limits<qint64>::max())) {
        return {};
    }
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(msecs), utcZone());
}

QDateTime localDateTimeFromSeconds(double seconds)
{
    if (!regularTimestamp(seconds)) {
        return {};
    }
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(std::llround(seconds)));
}

int calculateUtcOffset(double timestamp)
{
    const QDateTime local = localDateTimeFromSeconds(timestamp);
    if (!local.isValid()) {
        return 0;
    }
    return -local.offsetFromUtc();
}

int preferredOffset(double timestamp, std::optional<int> utcOffset)
{
    return utcOffset.value_or(calculateUtcOffset(timestamp));
}

double adjustToPreferredOffset(double timestamp, std::optional<int> utcOffset)
{
    return timestamp - static_cast<double>(preferredOffset(timestamp, utcOffset));
}

double offsetToLocalHour(double timestamp)
{
    const QDateTime local = localDateTimeFromSeconds(timestamp);
    if (!local.isValid()) {
        return 0.0;
    }
    QTime rounded = local.time();
    rounded.setHMS(rounded.hour(), 0, 0, 0);
    return static_cast<double>(-rounded.secsTo(local.time()));
}

double applyOffsetToUtc(double timestamp, std::optional<int> utcOffset)
{
    if (utcOffset.has_value()) {
        return static_cast<double>(*utcOffset);
    }
    return static_cast<double>(preferredOffset(timestamp, utcOffset));
}

QString formatPythonG(double value)
{
    std::array<char, 96> buffer{};
    const int written = std::snprintf(buffer.data(), buffer.size(), "%.6g", value);
    if (written <= 0) {
        return QStringLiteral("0");
    }
    return QString::fromLatin1(buffer.data(), std::min<int>(written, static_cast<int>(buffer.size() - 1)));
}

QRectF localWidgetRect(const DateAxisItem& axis)
{
    const QSizeF size = axis.geometry().size();
    if (size.width() > 0.0 && size.height() > 0.0) {
        return QRectF(QPointF(0.0, 0.0), size);
    }
    return axis.AxisItem::boundingRect();
}

} // namespace

enum class ZoomKind {
    YearMonth,
    MonthDay,
    DayHour,
    HourMinute,
    Hms,
    Ms,
};

struct TickSpec {
    enum class Stepper {
        Millisecond,
        Second,
        Month,
        Year,
    };

    double spacing = 0.0;
    Stepper stepper = Stepper::Second;
    double stepSize = 1.0;
    QString format;
    std::vector<int> autoSkip;

    [[nodiscard]] int skipFactor(double minimumSpacing) const
    {
        if (autoSkip.empty() || minimumSpacing < spacing) {
            return 1;
        }
        std::vector<int> factors = autoSkip;
        while (true) {
            for (const int factor : factors) {
                if (spacing * static_cast<double>(factor) > minimumSpacing) {
                    return factor;
                }
            }
            for (int& factor : factors) {
                factor *= 10;
            }
        }
    }

    [[nodiscard]] double step(double value, int skipFactor, bool first) const
    {
        if (!regularTimestamp(value)) {
            return std::numeric_limits<double>::infinity();
        }

        const double n = static_cast<double>(skipFactor);
        const double step = stepSize;
        if (stepper == Stepper::Millisecond) {
            if (first) {
                const double unit = n * step * 1000.0;
                return (std::floor((value * 1000.0) / unit) + 1.0) * unit / 1000.0;
            }
            return value + n * step;
        }
        if (stepper == Stepper::Second) {
            if (first) {
                const double unit = n * step;
                return (std::floor(value / unit) + 1.0) * unit;
            }
            return value + n * step;
        }

        const QDateTime dateTime = utcDateTimeFromSeconds(value);
        if (!dateTime.isValid()) {
            return std::numeric_limits<double>::infinity();
        }
        const QDate date = dateTime.date();
        if (stepper == Stepper::Month) {
            const int monthStep = static_cast<int>(std::llround(static_cast<double>(skipFactor) * stepSize));
            const int base0Month = date.month() + monthStep - 1;
            const int year = date.year() + base0Month / 12;
            const int month = base0Month % 12 + 1;
            if (year > 9999) {
                return std::numeric_limits<double>::infinity();
            }
            return static_cast<double>(QDateTime(QDate(year, month, 1), QTime(0, 0), utcZone()).toSecsSinceEpoch());
        }

        const int yearStep = static_cast<int>(std::llround(static_cast<double>(skipFactor) * stepSize));
        const int nextYear = (date.year() / yearStep + 1) * yearStep;
        if (nextYear > 9999) {
            return std::numeric_limits<double>::infinity();
        }
        return static_cast<double>(QDateTime(QDate(nextYear, 1, 1), QTime(0, 0), utcZone()).toSecsSinceEpoch());
    }

    [[nodiscard]] std::pair<std::vector<double>, int> makeTicks(double minimum, double maximum, double minimumSpacing) const
    {
        std::vector<double> ticks;
        const int skip = skipFactor(minimumSpacing);
        double value = step(minimum, skip, true);
        while (value <= maximum && std::isfinite(value)) {
            ticks.push_back(value);
            value = step(value, skip, false);
        }
        return {std::move(ticks), skip};
    }
};

struct ZoomDefinition {
    ZoomKind kind = ZoomKind::YearMonth;
    QString name;
    QString exampleText;
    std::vector<TickSpec> specs;
};

const ZoomDefinition& zoomDefinition(ZoomKind kind)
{
    static const ZoomDefinition yearMonth{
        ZoomKind::YearMonth,
        QStringLiteral("year_month"),
        QStringLiteral("YYYY"),
        {{kYearSpacing, TickSpec::Stepper::Year, 1, QStringLiteral("%Y"), {1, 5, 10, 25}},
         {kMonthSpacing, TickSpec::Stepper::Month, 1, QStringLiteral("%b"), {}}}};
    static const ZoomDefinition monthDay{
        ZoomKind::MonthDay,
        QStringLiteral("month_day"),
        QStringLiteral("MMM"),
        {{kMonthSpacing, TickSpec::Stepper::Month, 1, QStringLiteral("%b"), {}},
         {kDaySpacing, TickSpec::Stepper::Second, kDaySpacing, QStringLiteral("%d"), {1, 5}}}};
    static const ZoomDefinition dayHour{
        ZoomKind::DayHour,
        QStringLiteral("day_hour"),
        QStringLiteral("MMM 00"),
        {{kDaySpacing, TickSpec::Stepper::Second, kDaySpacing, QStringLiteral("%a %d"), {}},
         {kHourSpacing, TickSpec::Stepper::Second, kHourSpacing, QStringLiteral("%H:%M"), {1, 6}}}};
    static const ZoomDefinition hourMinute{
        ZoomKind::HourMinute,
        QStringLiteral("hour_minute"),
        QStringLiteral("MMM 00"),
        {{kDaySpacing, TickSpec::Stepper::Second, kDaySpacing, QStringLiteral("%a %d"), {}},
         {kMinuteSpacing, TickSpec::Stepper::Second, kMinuteSpacing, QStringLiteral("%H:%M"), {1, 5, 15}}}};
    static const ZoomDefinition hms{
        ZoomKind::Hms,
        QStringLiteral("hms"),
        QStringLiteral("99:99:99"),
        {{kSecondSpacing, TickSpec::Stepper::Second, kSecondSpacing, QStringLiteral("%H:%M:%S"), {1, 5, 15, 30}}}};
    static const ZoomDefinition ms{
        ZoomKind::Ms,
        QStringLiteral("ms"),
        QStringLiteral("99:99:99"),
        {{kMinuteSpacing, TickSpec::Stepper::Second, kMinuteSpacing, QStringLiteral("%H:%M:%S"), {}},
         {kMsSpacing, TickSpec::Stepper::Millisecond, kMsSpacing, QStringLiteral("%S.%f"), {1, 5, 10, 25}}}};

    switch (kind) {
    case ZoomKind::YearMonth:
        return yearMonth;
    case ZoomKind::MonthDay:
        return monthDay;
    case ZoomKind::DayHour:
        return dayHour;
    case ZoomKind::HourMinute:
        return hourMinute;
    case ZoomKind::Hms:
        return hms;
    case ZoomKind::Ms:
        return ms;
    }
    return yearMonth;
}

std::vector<std::pair<double, ZoomKind>> zoomLevels()
{
    return {{std::numeric_limits<double>::infinity(), ZoomKind::YearMonth},
            {5.0 * kDaySpacing, ZoomKind::MonthDay},
            {6.0 * kHourSpacing, ZoomKind::DayHour},
            {15.0 * kMinuteSpacing, ZoomKind::HourMinute},
            {30.0, ZoomKind::Hms},
            {1.0, ZoomKind::Ms}};
}

struct DateAxisItem::Private {
    std::optional<int> utcOffset;
    mutable ZoomKind zoomKind = ZoomKind::YearMonth;
    mutable double minSpacing = 0.0;
    mutable QFontMetricsF fontMetrics = QFontMetricsF(QFont{});
};

DateAxisItem::DateAxisItem()
    : DateAxisItem(QStringLiteral("bottom"), std::nullopt, nullptr, Qt::WindowFlags{})
{
}

DateAxisItem::DateAxisItem(QGraphicsItem* parent, Qt::WindowFlags flags)
    : DateAxisItem(QStringLiteral("bottom"), std::nullopt, parent, flags)
{
}

DateAxisItem::DateAxisItem(
    Orientation orientation,
    std::optional<int> utcOffset,
    QGraphicsItem* parent,
    Qt::WindowFlags flags)
    : AxisItem(orientation, parent, flags)
    , dDate_(std::make_unique<Private>())
{
    dDate_->utcOffset = utcOffset;
    enableAutoSIPrefix(false);
}

DateAxisItem::DateAxisItem(
    const QString& orientation,
    std::optional<int> utcOffset,
    QGraphicsItem* parent,
    Qt::WindowFlags flags)
    : AxisItem(orientation, parent, flags)
    , dDate_(std::make_unique<Private>())
{
    dDate_->utcOffset = utcOffset;
    enableAutoSIPrefix(false);
}

DateAxisItem::~DateAxisItem() = default;

void DateAxisItem::setUtcOffset(std::optional<int> utcOffset)
{
    dDate_->utcOffset = utcOffset;
    update();
}

std::optional<int> DateAxisItem::utcOffset() const
{
    return dDate_->utcOffset;
}

void DateAxisItem::setZoomLevelForDensity(double density) const
{
    if (!std::isfinite(density) || density < 0.0) {
        density = std::numeric_limits<double>::infinity();
    }

    const bool vertical = isVertical(orientation());
    auto sizeOf = [&](const QString& text) {
        const QRectF bounds = dDate_->fontMetrics.boundingRect(text);
        return (vertical ? bounds.height() : bounds.width()) + static_cast<double>(kTextPadding);
    };

    ZoomKind selected = ZoomKind::YearMonth;
    for (const auto& [maximalSpacing, kind] : zoomLevels()) {
        const double labelSize = std::max(1.0, sizeOf(zoomDefinition(kind).exampleText));
        if (maximalSpacing / labelSize < density) {
            break;
        }
        selected = kind;
    }

    dDate_->zoomKind = selected;
    const double labelSize = std::max(1.0, sizeOf(zoomDefinition(selected).exampleText));
    dDate_->minSpacing = density * labelSize;
}

QString DateAxisItem::zoomLevelName() const
{
    return zoomDefinition(dDate_->zoomKind).name;
}

double DateAxisItem::minSpacing() const noexcept
{
    return dDate_->minSpacing;
}

std::vector<AxisItem::TickLevel> DateAxisItem::tickValues(double minimum, double maximum, double size) const
{
    if (minimum > maximum) {
        std::swap(minimum, maximum);
    }
    if (size <= 0.0 || !std::isfinite(size)) {
        return {};
    }

    const double density = (maximum - minimum) / size;
    setZoomLevelForDensity(density);
    const ZoomDefinition& zoom = zoomDefinition(dDate_->zoomKind);

    std::vector<TickLevel> levels;
    std::vector<double> allTicks;
    for (const TickSpec& spec : zoom.specs) {
        double extendedMinimum = minimum;
        double extendedMaximum = maximum;
        if (spec.spacing >= kHourSpacing) {
            extendedMaximum += std::abs(static_cast<double>(preferredOffset(maximum, dDate_->utcOffset)));
            extendedMinimum -= std::abs(static_cast<double>(preferredOffset(minimum, dDate_->utcOffset)));
        }

        auto [ticks, skipFactor] = spec.makeTicks(extendedMinimum, extendedMaximum, dDate_->minSpacing);
        if (!ticks.empty()) {
            std::vector<double> moved;
            moved.reserve(ticks.size());
            for (double tick : ticks) {
                if ((spec.spacing == kHourSpacing && skipFactor > 1) || spec.spacing > kHourSpacing) {
                    moved.push_back(tick + applyOffsetToUtc(tick, dDate_->utcOffset));
                } else if (spec.spacing == kHourSpacing) {
                    const double localOffset = offsetToLocalHour(tick);
                    if (std::abs(localOffset) <= 1.0e-9) {
                        moved.push_back(tick + localOffset);
                    }
                } else {
                    moved.push_back(tick);
                }
            }
            ticks = std::move(moved);
        }

        std::vector<double> uniqueTicks;
        for (double tick : ticks) {
            const double tolerance = std::abs(dDate_->minSpacing * 0.01);
            const bool close = std::any_of(allTicks.begin(), allTicks.end(), [tick, tolerance](double existing) {
                return std::abs(existing - tick) <= tolerance;
            });
            if (!close) {
                uniqueTicks.push_back(tick);
                allTicks.push_back(tick);
            }
        }

        levels.push_back({spec.spacing, std::move(uniqueTicks)});
        if (skipFactor > 1) {
            break;
        }
    }
    return levels;
}

const TickSpec* findTickSpec(const ZoomDefinition& definition, double spacing)
{
    const auto match = std::find_if(definition.specs.begin(), definition.specs.end(), [spacing](const TickSpec& spec) {
        return std::abs(spec.spacing - spacing) <= std::max(1.0e-12, std::abs(spacing) * 1.0e-12);
    });
    if (match == definition.specs.end()) {
        return nullptr;
    }
    return &*match;
}

const TickSpec& tickSpecForSpacing(ZoomKind currentKind, double spacing)
{
    if (const TickSpec* spec = findTickSpec(zoomDefinition(currentKind), spacing)) {
        return *spec;
    }
    if (std::abs(spacing - kDaySpacing) <= 1.0e-9) {
        return zoomDefinition(ZoomKind::DayHour).specs.front();
    }
    for (const auto& [ignored, kind] : zoomLevels()) {
        Q_UNUSED(ignored);
        if (const TickSpec* spec = findTickSpec(zoomDefinition(kind), spacing)) {
            return *spec;
        }
    }
    return zoomDefinition(ZoomKind::YearMonth).specs.front();
}

std::vector<QString> DateAxisItem::tickStrings(const std::vector<double>& values, double scale, double spacing) const
{
    Q_UNUSED(scale);
    const TickSpec& spec = tickSpecForSpacing(dDate_->zoomKind, spacing);
    std::vector<QString> strings;
    strings.reserve(values.size());
    const QLocale cLocale = QLocale::c();

    for (double value : values) {
        const double adjusted = adjustToPreferredOffset(value, dDate_->utcOffset);
        const QDateTime dateTime = utcDateTimeFromSeconds(adjusted);
        if (!dateTime.isValid()) {
            const double offset = static_cast<double>(dDate_->utcOffset.value_or(0));
            strings.push_back(formatPythonG(std::floor((value - offset) / kSecPerYear) + 1970.0));
            continue;
        }

        const QDate date = dateTime.date();
        const QTime time = dateTime.time();
        if (spec.format == QStringLiteral("%Y")) {
            strings.push_back(QString::number(date.year()));
        } else if (spec.format == QStringLiteral("%b")) {
            strings.push_back(cLocale.toString(date, QStringLiteral("MMM")));
        } else if (spec.format == QStringLiteral("%d")) {
            strings.push_back(cLocale.toString(date, QStringLiteral("dd")));
        } else if (spec.format == QStringLiteral("%a %d")) {
            strings.push_back(cLocale.toString(date, QStringLiteral("ddd dd")));
        } else if (spec.format == QStringLiteral("%H:%M")) {
            strings.push_back(cLocale.toString(time, QStringLiteral("HH:mm")));
        } else if (spec.format == QStringLiteral("%H:%M:%S")) {
            strings.push_back(cLocale.toString(time, QStringLiteral("HH:mm:ss")));
        } else if (spec.format == QStringLiteral("%S.%f")) {
            strings.push_back(QStringLiteral("%1.%2")
                                  .arg(time.second(), 2, 10, QLatin1Char('0'))
                                  .arg(time.msec(), 3, 10, QLatin1Char('0')));
        } else {
            strings.push_back(cLocale.toString(dateTime, QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        }
    }

    return strings;
}

std::optional<AxisItem::DrawSpecs> DateAxisItem::generateDrawSpecs(QPainter& painter) const
{
    dDate_->fontMetrics = QFontMetricsF(painter.font());

    const QRectF bounds = localWidgetRect(*this);
    const bool vertical = isVertical(orientation());
    const double length = vertical ? bounds.height() : bounds.width();
    if (length <= 0.0) {
        return std::nullopt;
    }

    const auto [minimum, maximum] = range();
    const auto levels = tickValues(minimum, maximum, length);
    const double difference = maximum - minimum;
    if (difference == 0.0) {
        return std::nullopt;
    }

    const double coordinateScale = vertical ? -bounds.height() / difference : bounds.width() / difference;
    const double offset = vertical ? minimum * coordinateScale - bounds.height() : minimum * coordinateScale;

    DrawSpecs specs;
    specs.axisPen = pen();
    if (orientation() == Orientation::Left) {
        specs.axisLine = QLineF(bounds.right() - 1.0, bounds.top() - 1.0, bounds.right() - 1.0, bounds.bottom() + 1.0);
    } else if (orientation() == Orientation::Right) {
        specs.axisLine = QLineF(bounds.left() + 1.0, bounds.top() - 1.0, bounds.left() + 1.0, bounds.bottom() + 1.0);
    } else if (orientation() == Orientation::Top) {
        specs.axisLine = QLineF(bounds.left() - 1.0, bounds.bottom() - 1.0, bounds.right() + 1.0, bounds.bottom() - 1.0);
    } else {
        specs.axisLine = QLineF(bounds.left() - 1.0, bounds.top() + 1.0, bounds.right() + 1.0, bounds.top() + 1.0);
    }

    std::vector<std::vector<std::optional<double>>> tickPositions;
    for (std::size_t levelIndex = 0; levelIndex < levels.size(); ++levelIndex) {
        const auto& level = levels[levelIndex];
        tickPositions.emplace_back();
        QPen levelPen = tickPen();
        if (levelPen.brush().style() == Qt::SolidPattern) {
            QColor color = levelPen.color();
            color.setAlpha(static_cast<int>(std::round(static_cast<double>(color.alpha()) / (static_cast<double>(levelIndex) + 1.0))));
            levelPen.setColor(color);
        }
        const double tickLength = kDefaultTickLength / ((static_cast<double>(levelIndex) * 0.5) + 1.0);
        for (double value : level.values) {
            const double coordinate = value * coordinateScale - offset;
            if (coordinate < -0.5 || coordinate > length + 0.5) {
                tickPositions.back().push_back(std::nullopt);
                continue;
            }
            tickPositions.back().push_back(coordinate);
            if (vertical) {
                const double x = orientation() == Orientation::Left ? bounds.right() : bounds.left();
                const double direction = orientation() == Orientation::Left ? -1.0 : 1.0;
                specs.ticks.push_back({levelPen, QLineF(x, coordinate, x + tickLength * direction, coordinate)});
            } else {
                const double y = orientation() == Orientation::Top ? bounds.bottom() : bounds.top();
                const double direction = orientation() == Orientation::Top ? -1.0 : 1.0;
                specs.ticks.push_back({levelPen, QLineF(coordinate, y, coordinate, y + tickLength * direction)});
            }
        }
    }

    const int textLevels = std::min<int>(static_cast<int>(levels.size()), 2);
    for (int levelIndex = 0; levelIndex < textLevels; ++levelIndex) {
        const auto& level = levels[static_cast<std::size_t>(levelIndex)];
        const auto strings = tickStrings(level.values, 1.0, level.spacing);
        for (std::size_t index = 0; index < strings.size(); ++index) {
            if (index >= tickPositions[static_cast<std::size_t>(levelIndex)].size()) {
                continue;
            }
            const std::optional<double> coordinate = tickPositions[static_cast<std::size_t>(levelIndex)][index];
            if (!coordinate.has_value() || strings[index].isEmpty()) {
                continue;
            }
            QRectF textBounds = painter.boundingRect(QRectF(0.0, 0.0, 180.0, 80.0), Qt::AlignCenter, strings[index]);
            textBounds.setHeight(textBounds.height() * 0.8);

            TextSpec textSpec;
            textSpec.text = strings[index];
            if (orientation() == Orientation::Left) {
                textSpec.alignment = Qt::AlignRight | Qt::AlignVCenter;
                textSpec.rect = QRectF(bounds.right() + kDefaultTickLength - kTickTextOffset - textBounds.width(),
                    *coordinate - textBounds.height() / 2.0,
                    textBounds.width(),
                    textBounds.height());
            } else if (orientation() == Orientation::Right) {
                textSpec.alignment = Qt::AlignLeft | Qt::AlignVCenter;
                textSpec.rect = QRectF(bounds.left() - kDefaultTickLength + kTickTextOffset,
                    *coordinate - textBounds.height() / 2.0,
                    textBounds.width(),
                    textBounds.height());
            } else if (orientation() == Orientation::Top) {
                textSpec.alignment = Qt::AlignHCenter | Qt::AlignBottom;
                textSpec.rect = QRectF(*coordinate - textBounds.width() / 2.0,
                    bounds.bottom() + kDefaultTickLength - kTickTextOffset - textBounds.height(),
                    textBounds.width(),
                    textBounds.height());
            } else {
                textSpec.alignment = Qt::AlignHCenter | Qt::AlignTop;
                textSpec.rect = QRectF(*coordinate - textBounds.width() / 2.0,
                    bounds.top() - kDefaultTickLength + kTickTextOffset,
                    textBounds.width(),
                    textBounds.height());
            }
            specs.text.push_back(std::move(textSpec));
        }
    }

    return specs;
}

void DateAxisItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
    painter->setPen(textPen());
    painter->setClipRect(AxisItem::boundingRect().toAlignedRect());
    for (const TextSpec& text : specs->text) {
        painter->drawText(text.rect, static_cast<int>(text.alignment | Qt::TextDontClip), text.text);
    }
}

} // namespace pyqtgraph::graphicsItems
