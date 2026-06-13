// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/numeric.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/NumericParameterItem.hpp"
#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"

#include <cppqtgraph/widgets/SpinBox.hpp>

#include <QtCore/QSignalBlocker>
#include <QtCore/QVariant>
#include <QtWidgets/QLabel>

namespace cppqtgraph::parametertree {

namespace {

std::optional<double> boundFromVariant(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return std::nullopt;
    }
    return value.toDouble();
}

void applyLimitsToSpinBoxOptions(const QVariant& limits, widgets::SpinBoxOptions& options)
{
    if (!limits.isValid()) {
        return;
    }
    if (limits.canConvert<QVariantList>()) {
        const QVariantList pair = limits.toList();
        if (pair.size() >= 2) {
            options.minBound = boundFromVariant(pair.at(0));
            options.maxBound = boundFromVariant(pair.at(1));
        }
    }
}

} // namespace

NumericParameterItem::NumericParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param, depth, new widgets::SpinBox())
{
    spinBox_ = qobject_cast<widgets::SpinBox*>(editor_);
    bindEditor(editor_);
    applySpinBoxOpts(param->options());
    writeEditorValue(param->value());
}

void NumericParameterItem::bindEditor(QWidget* editor)
{
    spinBox_ = qobject_cast<widgets::SpinBox*>(editor);
    if (spinBox_ == nullptr) {
        return;
    }
    QObject::connect(spinBox_, &widgets::SpinBox::sigValueChanged, spinBox_, [this](widgets::SpinBox*) {
        widgetValueChanged();
    });
    QObject::connect(spinBox_,
                     &widgets::SpinBox::sigValueChanging,
                     spinBox_,
                     [this](widgets::SpinBox*, double value) { editorValueChanging(value); });
}

QVariant NumericParameterItem::readEditorValue() const
{
    if (spinBox_ == nullptr) {
        return QVariant();
    }
    if (param_ != nullptr && param_->type() == QStringLiteral("int")) {
        return spinBox_->value();
    }
    return spinBox_->value();
}

void NumericParameterItem::writeEditorValue(const QVariant& val)
{
    if (spinBox_ == nullptr) {
        return;
    }
    const QSignalBlocker blocker(spinBox_);
    if (param_ != nullptr && param_->type() == QStringLiteral("int")) {
        spinBox_->setValue(val.toInt());
    } else {
        spinBox_->setValue(val.toDouble());
    }
}

void NumericParameterItem::configureEditor(QWidget* editor)
{
    Q_UNUSED(editor);
}

void NumericParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    WidgetParameterItem::optsChanged(param, opts);
    applySpinBoxOpts(opts);
    updateDisplayLabel();
}

void NumericParameterItem::updateDisplayLabel(const QVariant& value)
{
    if (displayLabel_ == nullptr || spinBox_ == nullptr) {
        return;
    }
    if (value.isValid()) {
        writeEditorValue(value);
    }
    displayLabel_->setText(spinBox_->formatText());
}

void NumericParameterItem::applySpinBoxOpts(const QVariantMap& opts)
{
    if (spinBox_ == nullptr || param_ == nullptr) {
        return;
    }

    widgets::SpinBoxOptions spinOpts;
    const bool isInt = param_->type() == QStringLiteral("int");
    spinOpts.integerMode = isInt;
    if (isInt) {
        spinOpts.minStep = 1.0;
        spinOpts.decimals = 0;
    } else {
        spinOpts.decimals = 3;
        spinOpts.minStep = 0.01;
    }

    const QVariantMap& allOpts = opts.isEmpty() ? param_->options() : opts;
    if (allOpts.contains(QStringLiteral("limits"))) {
        applyLimitsToSpinBoxOptions(allOpts.value(QStringLiteral("limits")), spinOpts);
    }
    if (allOpts.contains(QStringLiteral("step"))) {
        spinOpts.step = allOpts.value(QStringLiteral("step")).toDouble();
    }
    if (allOpts.contains(QStringLiteral("suffix"))) {
        spinOpts.suffix = allOpts.value(QStringLiteral("suffix")).toString();
    } else if (allOpts.contains(QStringLiteral("units"))) {
        spinOpts.suffix = allOpts.value(QStringLiteral("units")).toString();
    }
    if (allOpts.contains(QStringLiteral("siPrefix"))) {
        spinOpts.siPrefix = allOpts.value(QStringLiteral("siPrefix")).toBool();
    }
    if (allOpts.contains(QStringLiteral("finite"))) {
        spinOpts.finite = allOpts.value(QStringLiteral("finite")).toBool();
    }
    if (allOpts.contains(QStringLiteral("dec"))) {
        spinOpts.dec = allOpts.value(QStringLiteral("dec")).toBool();
    }
    if (allOpts.contains(QStringLiteral("decimals"))) {
        spinOpts.decimals = allOpts.value(QStringLiteral("decimals")).toInt();
    }
    if (allOpts.contains(QStringLiteral("minStep"))) {
        spinOpts.minStep = allOpts.value(QStringLiteral("minStep")).toDouble();
    }

    spinBox_->setOpts(spinOpts);
}

} // namespace cppqtgraph::parametertree
