// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/slider.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/SliderParameter.hpp"
#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>

#include <algorithm>
#include <cmath>
#include <limits>

namespace cppqtgraph::parametertree {

namespace {

double roundToPrecision(double value, int precision)
{
    if (precision < 0) {
        return value;
    }
    const double factor = std::pow(10.0, precision);
    return std::round(value * factor) / factor;
}

std::vector<double> buildSpanFromLimits(const QVariantList& limits, double step, int precision)
{
    std::vector<double> span;
    if (limits.size() < 2) {
        return span;
    }

    const double start = limits.at(0).toDouble();
    const double stop = limits.at(1).toDouble();
    if (step == 0.0) {
        return span;
    }

    for (double value = start; value < stop + step; value += step) {
        span.push_back(roundToPrecision(value, precision));
    }
    return span;
}

QString formatPythonStyleValue(const QString& format, const QString& value)
{
    if (format.size() >= 6 && format.startsWith(QStringLiteral("{0:>")) && format.endsWith(QLatin1Char('}'))) {
        bool ok = false;
        const int width = format.mid(4, format.size() - 5).toInt(&ok);
        if (ok && width > 0) {
            return QStringLiteral("%1").arg(value, width);
        }
    }
    if (format.contains(QStringLiteral("{0}"))) {
        return QString(format).replace(QStringLiteral("{0}"), value);
    }
    return format.arg(value);
}

std::vector<double> spanFromVariant(const QVariant& spanValue, int precision)
{
    std::vector<double> span;
    if (spanValue.canConvert<QVariantList>()) {
        for (const QVariant& entry : spanValue.toList()) {
            span.push_back(roundToPrecision(entry.toDouble(), precision));
        }
        return span;
    }
    if (spanValue.canConvert<QVariantMap>()) {
        const QVariantMap map = spanValue.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.value().canConvert<QVariantList>()) {
                for (const QVariant& entry : it.value().toList()) {
                    span.push_back(roundToPrecision(entry.toDouble(), precision));
                }
                return span;
            }
        }
    }
    return span;
}

} // namespace

std::vector<double> linspaceMinusPiToPi(int count)
{
    std::vector<double> span;
    if (count <= 0) {
        return span;
    }
    if (count == 1) {
        span.push_back(-M_PI);
        return span;
    }
    const double step = (2.0 * M_PI) / static_cast<double>(count - 1);
    for (int i = 0; i < count; ++i) {
        span.push_back(-M_PI + (step * static_cast<double>(i)));
    }
    return span;
}

std::vector<double> arangeSquared(int count)
{
    std::vector<double> span;
    span.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        span.push_back(static_cast<double>(i * i));
    }
    return span;
}

SliderParameterItem::SliderParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param, depth, new QWidget())
{
    auto* layout = new QHBoxLayout(editor_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    sliderLabel_ = new QLabel(editor_);
    sliderLabel_->setAlignment(Qt::AlignLeft);
    slider_ = new QSlider(Qt::Horizontal, editor_);
    layout->addWidget(sliderLabel_);
    layout->addWidget(slider_);

    bindEditor(editor_);
    rebuildSpan(param->options());
    writeEditorValue(param->value());
    updateDisplayLabel(param->value());
}

void SliderParameterItem::bindEditor(QWidget* /*editor*/)
{
    if (slider_ == nullptr) {
        return;
    }
    QObject::connect(slider_, &QSlider::valueChanged, slider_, [this](int) {
        widgetValueChanged();
        updateDisplayLabel();
    });
    QObject::connect(slider_, &QSlider::sliderMoved, slider_, [this](int position) {
        if (position >= 0 && position < static_cast<int>(span_.size())) {
            editorValueChanging(span_.at(static_cast<std::size_t>(position)));
        }
    });
}

QVariant SliderParameterItem::readEditorValue() const
{
    if (slider_ == nullptr || span_.empty()) {
        return QVariant();
    }
    const int index = slider_->value();
    if (index < 0 || index >= static_cast<int>(span_.size())) {
        return QVariant();
    }
    return span_.at(static_cast<std::size_t>(index));
}

void SliderParameterItem::writeEditorValue(const QVariant& val)
{
    if (slider_ == nullptr || span_.empty()) {
        return;
    }
    const QSignalBlocker blocker(slider_);
    slider_->setValue(spanToSliderValue(val.toDouble()));
}

void SliderParameterItem::configureEditor(QWidget* /*editor*/)
{
}

void SliderParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    WidgetParameterItem::optsChanged(param, opts);
    rebuildSpan(param->options());
    if (param->options().contains(QStringLiteral("value"))) {
        writeEditorValue(param->value());
        updateDisplayLabel(param->value());
    }
}

void SliderParameterItem::updateDisplayLabel(const QVariant& value)
{
    if (slider_ == nullptr || sliderLabel_ == nullptr || displayLabel_ == nullptr) {
        return;
    }

    const int index = slider_->value();
    sliderLabel_->setText(prettyTextValue(index));

    QVariant display = value.isValid() ? value : readEditorValue();
    QString displayText = display.toString();
    if (!suffix_.isEmpty()) {
        displayText += QStringLiteral(" ") + suffix_;
    }
    displayLabel_->setText(displayText);
}

void SliderParameterItem::rebuildSpan(const QVariantMap& opts)
{
    const int precision = opts.contains(QStringLiteral("precision")) && !opts.value(QStringLiteral("precision")).isNull()
        ? opts.value(QStringLiteral("precision")).toInt()
        : 2;
    suffix_ = opts.value(QStringLiteral("suffix")).toString();

    if (opts.contains(QStringLiteral("span")) && opts.value(QStringLiteral("span")).isValid()) {
        span_ = spanFromVariant(opts.value(QStringLiteral("span")), precision);
    } else {
        const double step = opts.value(QStringLiteral("step"), 1.0).toDouble();
        span_ = buildSpanFromLimits(opts.value(QStringLiteral("limits")).toList(), step, precision);
    }

    charSpan_.clear();
    charSpan_.reserve(static_cast<int>(span_.size()));
    for (double entry : span_) {
        charSpan_.append(QString::number(entry, 'g', 12));
    }

    if (slider_ != nullptr) {
        const QSignalBlocker blocker(slider_);
        slider_->setMinimum(0);
        slider_->setMaximum(std::max(0, static_cast<int>(span_.size()) - 1));
    }
}

int SliderParameterItem::spanToSliderValue(double value) const
{
    if (span_.empty()) {
        return 0;
    }

    int bestIndex = 0;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (int i = 0; i < static_cast<int>(span_.size()); ++i) {
        const double distance = std::abs(span_.at(static_cast<std::size_t>(i)) - value);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    return bestIndex;
}

QString SliderParameterItem::prettyTextValue(int sliderIndex) const
{
    if (sliderIndex < 0 || sliderIndex >= charSpan_.size()) {
        return {};
    }

    const QString suffixText = suffix_.isEmpty() ? QString() : QStringLiteral(" ") + suffix_;
    const QVariantMap& opts = param_->options();
    const QString format = opts.value(QStringLiteral("format")).toString();
    if (format.isEmpty()) {
        const int width = std::max(1, static_cast<int>(charSpan_.at(sliderIndex).size()));
        return QStringLiteral("%1").arg(charSpan_.at(sliderIndex), width) + suffixText;
    }
    return formatPythonStyleValue(format, charSpan_.at(sliderIndex)) + suffixText;
}

SliderParameter::SliderParameter(QVariantMap opts, QObject* parent)
    : Parameter([&opts]() {
          opts.insert(QStringLiteral("type"), QStringLiteral("slider"));
          if (!opts.contains(QStringLiteral("limits"))) {
              opts.insert(QStringLiteral("limits"), QVariantList{0, 0});
          }
          return opts;
      }(),
      parent)
{
}

QVariant SliderParameter::setValue(const QVariant& value, bool blockSignal)
{
    return Parameter::setValue(value, blockSignal);
}

void SliderParameter::setOpts(const QVariantMap& opts)
{
    Parameter::setOpts(opts);
}

ParameterItem* SliderParameter::makeTreeItem(int depth)
{
    return new SliderParameterItem(this, depth);
}

} // namespace cppqtgraph::parametertree
