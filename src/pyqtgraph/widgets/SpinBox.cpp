// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/SpinBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/SpinBox.hpp"

#include "../../../include/pyqtgraph/SignalProxy.hpp"

#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSizePolicy>

#include <algorithm>
#include <cmath>
#include <limits>

namespace pyqtgraph::widgets {

namespace {

bool valuesEqual(double a, double b)
{
    if (std::isnan(a) && std::isnan(b)) {
        return true;
    }
    return a == b;
}

std::pair<double, QString> siScale(double value, double power = 1.0)
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

int siPrefixExponent(const QString& prefix)
{
    if (prefix.isEmpty()) {
        return 0;
    }
    if (prefix == QStringLiteral("u")) {
        return -2;
    }

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

    for (std::size_t index = 0; index < prefixes.size(); ++index) {
        if (prefixes[index] == prefix) {
            return static_cast<int>(index) - 8;
        }
    }
    return 0;
}

double siApply(double value, const QString& siPrefix, double unitPower = 1.0)
{
    const int exponent = siPrefixExponent(siPrefix) * 3;
    const double scaledExponent = static_cast<double>(exponent) * unitPower;
    if (scaledExponent > 0.0) {
        return value * std::pow(10.0, scaledExponent);
    }
    if (scaledExponent < 0.0) {
        return value / std::pow(10.0, -scaledExponent);
    }
    return value;
}

QRegularExpression floatRegexForLocale(const QLocale& locale)
{
    const QString decimal = QRegularExpression::escape(QString(locale.decimalPoint()));
    const QString pattern = QStringLiteral(R"((?<number>[+-]?((((\d+()") + decimal
        + QStringLiteral(R"(\d*)?)|(\d*)") + decimal
        + QStringLiteral(R"(\d+))([eE][+-]?\d+)?)|((?i:nan)|(inf))))\s*((?<siPrefix>[yzafpnµmkMGTPEZYu]?)(?<suffix>\w.*))?$)");
    return QRegularExpression(pattern);
}

struct ParsedInput {
    QString number;
    QString siPrefix;
    QString suffix;
};

std::optional<ParsedInput> siParse(const QString& input, const QRegularExpression& regex, const QString& suffix)
{
    QString working = input.trimmed();
    if (!suffix.isEmpty()) {
        if (!working.endsWith(suffix)) {
            return std::nullopt;
        }
        working = working.left(working.size() - suffix.size()) + QStringLiteral("X");
    } else {
        working += QStringLiteral("X");
    }

    const QRegularExpressionMatch match = regex.match(working);
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    ParsedInput parsed;
    parsed.number = match.captured(QStringLiteral("number"));
    parsed.siPrefix = match.captured(QStringLiteral("siPrefix"));
    if (suffix.isNull()) {
        parsed.suffix = match.captured(QStringLiteral("suffix"));
    } else {
        parsed.suffix = suffix;
    }
    return parsed;
}

double parseNumber(const QString& text)
{
    QString normalized = text;
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    if (normalized.compare(QStringLiteral("nan"), Qt::CaseInsensitive) == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (normalized.compare(QStringLiteral("inf"), Qt::CaseInsensitive) == 0) {
        return std::numeric_limits<double>::infinity();
    }
    if (normalized.compare(QStringLiteral("-inf"), Qt::CaseInsensitive) == 0) {
        return -std::numeric_limits<double>::infinity();
    }

    bool ok = false;
    const double value = normalized.toDouble(&ok);
    if (!ok) {
        throw std::invalid_argument("invalid number");
    }
    return value;
}

QString applyFormat(const QString& formatTemplate, const QString& prefix, const QString& prefixGap,
    const QString& scaledValueString, double value, const QString& scaledValue, int decimals,
    const QString& suffixGap, const QString& siPrefix, const QString& suffix)
{
    QString result = formatTemplate;

    result.replace(QStringLiteral("{prefix}"), prefix);
    result.replace(QStringLiteral("{prefixGap}"), prefixGap);
    result.replace(QStringLiteral("{scaledValueString}"), scaledValueString);
    result.replace(QStringLiteral("{value}"), QString::number(value, 'g', 12));
    result.replace(QStringLiteral("{scaledValue}"), scaledValue);
    result.replace(QStringLiteral("{decimals}"), QString::number(decimals));
    result.replace(QStringLiteral("{suffixGap}"), suffixGap);
    result.replace(QStringLiteral("{siPrefix}"), siPrefix);
    result.replace(QStringLiteral("{suffix}"), suffix);
    return result;
}

} // namespace

ErrorBox::ErrorBox(QWidget* parent)
    : QWidget(parent)
{
    parent->installEventFilter(this);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    resizeToParent();
    setVisible(false);
}

bool ErrorBox::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parent() && event->type() == QEvent::Resize) {
        resizeToParent();
    }
    return false;
}

void ErrorBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPen pen(QColor(Qt::red));
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawRect(rect().adjusted(1, 1, -2, -2));
}

void ErrorBox::resizeToParent()
{
    if (parentWidget() != nullptr) {
        setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
    }
}

SpinBox::SpinBox(QWidget* parent, double value)
    : QAbstractSpinBox(parent)
{
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setCorrectionMode(QAbstractSpinBox::CorrectToPreviousValue);
    setKeyboardTracking(false);

    errorBox_ = new ErrorBox(lineEdit());
    val_ = value;
    updateText();

    changeProxy_ = new pyqtgraph::SignalProxy(opts_.delay, 0.0, false, this);
    connect(this, &SpinBox::sigValueChanging, changeProxy_, [this](SpinBox*, double) {
        changeProxy_->signalReceived();
    });
    connect(changeProxy_, &pyqtgraph::SignalProxy::sigDelayed, this, &SpinBox::delayedChange);
    connect(this, &SpinBox::editingFinished, this, &SpinBox::editingFinishedEvent);

    updateHeight();
}

double SpinBox::value() const
{
    if (opts_.integerMode) {
        return static_cast<double>(static_cast<int>(val_));
    }
    return val_;
}

double SpinBox::setValue(std::optional<double> value, bool update, bool delaySignal)
{
    double next = value.value_or(this->value());

    bool bounded = true;
    if (!std::isnan(next)) {
        if (opts_.minBound.has_value() && opts_.maxBound.has_value() && opts_.wrapping) {
            bounded = false;
            if (std::isinf(next)) {
                next = val_;
            } else {
                const double lower = *opts_.minBound;
                const double upper = *opts_.maxBound;
                next = std::fmod(next - lower, upper - lower);
                if (next < 0.0) {
                    next += (upper - lower);
                }
                next += lower;
            }
        } else {
            if (opts_.minBound.has_value() && next < *opts_.minBound) {
                bounded = false;
                next = *opts_.minBound;
            }
            if (opts_.maxBound.has_value() && next > *opts_.maxBound) {
                bounded = false;
                next = *opts_.maxBound;
            }
        }
    }

    if (opts_.integerMode) {
        next = static_cast<double>(static_cast<int>(next));
    }

    const double previous = val_;
    val_ = next;
    const bool changed = !valuesEqual(next, previous);

    if (update && (changed || !bounded)) {
        updateText();
    }

    if (changed) {
        emit sigValueChanging(this, val_);
        if (!delaySignal) {
            emitChanged();
        }
    }

    return val_;
}

void SpinBox::setOpts(const SpinBoxOptions& opts)
{
    applyOpts(opts);
}

void SpinBox::applyOpts(const SpinBoxOptions& opts)
{
    if (opts.minBound.has_value()) {
        opts_.minBound = opts.minBound;
    }
    if (opts.maxBound.has_value()) {
        opts_.maxBound = opts.maxBound;
    }
    if (opts.wrapping.has_value()) {
        opts_.wrapping = *opts.wrapping;
    }
    if (opts.step.has_value()) {
        opts_.step = *opts.step;
    }
    if (opts.minStep.has_value()) {
        opts_.minStep = *opts.minStep;
    }
    if (opts.dec.has_value()) {
        opts_.dec = *opts.dec;
    }
    if (opts.integerMode.has_value()) {
        opts_.integerMode = *opts.integerMode;
    }
    if (opts.finite.has_value()) {
        opts_.finite = *opts.finite;
    }
    if (opts.prefix.has_value()) {
        opts_.prefix = *opts.prefix;
    }
    if (opts.suffix.has_value()) {
        opts_.suffix = *opts.suffix;
    }
    if (opts.suffixPower.has_value()) {
        opts_.suffixPower = *opts.suffixPower;
    }
    if (opts.siPrefix.has_value()) {
        opts_.siPrefix = *opts.siPrefix;
    }
    if (opts.scaleAtZero.has_value()) {
        opts_.scaleAtZero = opts.scaleAtZero;
    }
    if (opts.delay.has_value()) {
        opts_.delay = *opts.delay;
        if (changeProxy_ != nullptr) {
            changeProxy_->setDelay(opts_.delay);
        }
    }
    if (opts.delayUntilEditFinished.has_value()) {
        opts_.delayUntilEditFinished = *opts.delayUntilEditFinished;
    }
    if (opts.decimals.has_value()) {
        opts_.decimals = *opts.decimals;
    }
    if (opts.format.has_value()) {
        opts_.format = *opts.format;
    }
    if (opts.compactHeight.has_value()) {
        opts_.compactHeight = *opts.compactHeight;
    }

    if (opts_.integerMode) {
        opts_.step = std::round(opts_.step);
        if (opts_.step < 1.0) {
            opts_.step = 1.0;
        }
        if (opts_.minStep < 1.0) {
            opts_.minStep = 1.0;
        }
        opts_.minStep = std::round(opts_.minStep);
    }

    if (opts_.dec && opts.minStep.has_value()) {
        opts_.minStep = *opts.minStep;
    } else if (opts_.dec) {
        opts_.minStep = opts_.step;
    }

    if (opts.value.has_value()) {
        setValue(*opts.value);
    } else if (opts.minBound.has_value() || opts.maxBound.has_value()) {
        setValue();
    } else {
        updateText();
    }

    updateHeight();
}

void SpinBox::setMinimum(std::optional<double> minimum, bool update)
{
    opts_.minBound = minimum;
    if (update) {
        setValue();
    }
}

void SpinBox::setMaximum(std::optional<double> maximum, bool update)
{
    opts_.maxBound = maximum;
    if (update) {
        setValue();
    }
}

void SpinBox::setRange(std::optional<double> minimum, std::optional<double> maximum)
{
    opts_.minBound = minimum;
    opts_.maxBound = maximum;
    setValue();
    updateHeight();
}

bool SpinBox::wrapping() const
{
    return opts_.wrapping;
}

void SpinBox::setWrapping(bool enable)
{
    opts_.wrapping = enable;
}

void SpinBox::setPrefix(const QString& prefix)
{
    SpinBoxOptions opts;
    opts.prefix = prefix;
    applyOpts(opts);
}

void SpinBox::setSuffix(const QString& suffix)
{
    SpinBoxOptions opts;
    opts.suffix = suffix;
    applyOpts(opts);
}

void SpinBox::setSingleStep(double step)
{
    SpinBoxOptions opts;
    opts.step = step;
    applyOpts(opts);
}

void SpinBox::setDecimals(int decimals)
{
    SpinBoxOptions opts;
    opts.decimals = decimals;
    applyOpts(opts);
}

QString SpinBox::formatText() const
{
    const int decimals = opts_.decimals;
    const QString suffix = opts_.suffix;
    const QString prefix = opts_.prefix;

    double displayValue = value();
    QString siPrefixString;

    if (opts_.siPrefix) {
        double scale = 1.0;
        if (val_ == 0.0) {
            if (opts_.scaleAtZero.has_value()) {
                std::tie(scale, siPrefixString) = siScale(*opts_.scaleAtZero, opts_.suffixPower);
            } else {
                std::tie(scale, siPrefixString) = siScale(stepByValue(1), opts_.suffixPower);
            }
        } else {
            std::tie(scale, siPrefixString) = siScale(displayValue, opts_.suffixPower);
        }
        displayValue *= scale;
    }

    QString scaledValueString;
    QString scaledValue;
    if (opts_.integerMode) {
        const int roundedValue = static_cast<int>(std::lround(displayValue));
        scaledValue = QString::number(roundedValue);
        scaledValueString = scaledValue;
    } else {
        scaledValue = QString::number(displayValue, 'g', 12);
        scaledValueString = locale().toString(displayValue, 'g', decimals);
        scaledValueString.remove(locale().groupSeparator());
    }

    const QString prefixGap = prefix.isEmpty() ? QString{} : QStringLiteral(" ");
    const QString suffixGap = (suffix.isEmpty() && siPrefixString.isEmpty()) ? QString{} : QStringLiteral(" ");

    return applyFormat(opts_.format, prefix, prefixGap, scaledValueString, value(), scaledValue, decimals, suffixGap,
        siPrefixString, suffix);
}

QString SpinBox::editorText() const
{
    return lineEdit()->text();
}

void SpinBox::setEditorText(const QString& text)
{
    lineEdit()->setText(text);
}

std::optional<double> SpinBox::interpret() const
{
    QString text = lineEdit()->text();
    if (text.startsWith(opts_.prefix)) {
        text = text.mid(opts_.prefix.size());
    }

    const QRegularExpression regex = floatRegexForLocale(locale());
    const std::optional<ParsedInput> parsed = siParse(text, regex, opts_.suffix);
    if (!parsed.has_value()) {
        return std::nullopt;
    }

    try {
        double parsedValue = parseNumber(parsed->number);
        if ((opts_.integerMode || opts_.finite) && (!std::isfinite(parsedValue))) {
            return std::nullopt;
        }

        if (opts_.integerMode) {
            parsedValue = static_cast<double>(static_cast<int>(siApply(parsedValue, parsed->siPrefix, opts_.suffixPower)));
        } else {
            parsedValue = siApply(parsedValue, parsed->siPrefix, opts_.suffixPower);
        }
        return parsedValue;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

QAbstractSpinBox::StepEnabled SpinBox::stepEnabled() const
{
    return StepUpEnabled | StepDownEnabled;
}

void SpinBox::stepBy(int steps)
{
    setValue(stepByValue(steps), true, true);
}

double SpinBox::stepByValue(int steps) const
{
    if (std::isinf(val_) || std::isnan(val_)) {
        return val_;
    }

    const int stepCount = std::abs(steps);
    const double sign = steps >= 0 ? 1.0 : -1.0;
    double next = val_;

    for (int index = 0; index < stepCount; ++index) {
        if (opts_.dec) {
            double step = opts_.step;
            if (next == 0.0) {
                step = opts_.minStep;
            } else {
                const double valueSign = next >= 0.0 ? 1.0 : -1.0;
                const double fudge = std::pow(1.01, sign * valueSign);
                const double exponent = std::floor(std::log10(std::abs(next * fudge)));
                step = opts_.step * std::pow(10.0, exponent);
            }
            step = std::max(step, opts_.minStep);
            next += sign * step;
            if (std::abs(next) < opts_.minStep) {
                next = 0.0;
            }
        } else {
            next += sign * opts_.step;
            if (opts_.minStep > 0.0 && std::abs(next) < opts_.minStep) {
                next = 0.0;
            }
        }
    }

    return next;
}

QValidator::State SpinBox::validate(QString& input, int& pos) const
{
    Q_UNUSED(input);
    QValidator::State state = QValidator::Acceptable;
    auto* mutableThis = const_cast<SpinBox*>(this);
    if (!skipValidate_) {
        const std::optional<double> parsed = interpret();
        if (!parsed.has_value()) {
            state = QValidator::Intermediate;
        } else if (!valueInRange(*parsed)) {
            state = QValidator::Intermediate;
        } else if (!opts_.delayUntilEditFinished) {
            mutableThis->setValue(*parsed, false);
            state = QValidator::Acceptable;
        }
    }

    if (state == QValidator::Intermediate) {
        mutableThis->textValid_ = false;
    } else if (state == QValidator::Acceptable) {
        mutableThis->textValid_ = true;
    }

    if (errorBox_ != nullptr) {
        errorBox_->setVisible(!textValid_);
    }
    mutableThis->update();

    Q_UNUSED(pos);
    return state;
}

void SpinBox::fixup(QString& input) const
{
    Q_UNUSED(input);
    const_cast<SpinBox*>(this)->updateText();
}

void SpinBox::editingFinishedEvent()
{
    if (lineEdit()->text() == lastText_) {
        return;
    }

    const std::optional<double> parsed = interpret();
    if (!parsed.has_value() || valuesEqual(*parsed, val_)) {
        return;
    }

    setValue(*parsed, true, false);
}

void SpinBox::delayedChange()
{
    if (!hasLastValEmitted_ || !valuesEqual(val_, lastValEmitted_)) {
        emitChanged();
    }
}

void SpinBox::emitChanged()
{
    lastValEmitted_ = val_;
    hasLastValEmitted_ = true;
    emit valueChanged(val_);
    emit sigValueChanged(this);
}

void SpinBox::updateText()
{
    skipValidate_ = true;
    const QString text = formatText();
    lineEdit()->setText(text);
    lastText_ = text;
    skipValidate_ = false;
}

bool SpinBox::valueInRange(double value) const
{
    if (!std::isnan(value)) {
        if (opts_.minBound.has_value() && value < *opts_.minBound) {
            return false;
        }
        if (opts_.maxBound.has_value() && value > *opts_.maxBound) {
            return false;
        }
        if (opts_.integerMode && static_cast<double>(static_cast<int>(value)) != value) {
            return false;
        }
    }
    return true;
}

void SpinBox::updateHeight()
{
    if (!opts_.compactHeight) {
        setMaximumHeight(QWIDGETSIZE_MAX);
        return;
    }
    const int height = QFontMetrics(font()).height();
    if (lastFontHeight_ != height) {
        lastFontHeight_ = height;
        setMaximumHeight(height);
    }
}

} // namespace pyqtgraph::widgets
