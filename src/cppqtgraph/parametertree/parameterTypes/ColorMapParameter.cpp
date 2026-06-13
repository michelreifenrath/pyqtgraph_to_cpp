// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/colormap.py
// and pyqtgraph/parametertree/parameterTypes/colormaplut.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/ColorMapParameter.hpp"

#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"

#include <cppqtgraph/widgets/ColorMapButton.hpp>
#include <cppqtgraph/widgets/GradientWidget.hpp>

#include <cppqtgraph/graphicsItems/GradientEditorItem.hpp>

#include <QtCore/QSignalBlocker>
#include <QtCore/QSize>

#include <stdexcept>

namespace cppqtgraph::parametertree {
namespace {

struct ColorMapMetaTypeRegistration {
    ColorMapMetaTypeRegistration() { qRegisterMetaType<cppqtgraph::ColorMap>(); }
};

const ColorMapMetaTypeRegistration& colorMapMetaTypeRegistration()
{
    static const ColorMapMetaTypeRegistration registration;
    return registration;
}

QVariant interpretColorMapLutValue(const QVariant& value)
{
    colorMapMetaTypeRegistration();
    if (!value.isValid() || value.isNull()) {
        return value;
    }
    if (value.metaType().id() == QMetaType::QString) {
        if (const auto resolved = cppqtgraph::get(value.toString())) {
            return QVariant::fromValue(*resolved);
        }
        throw std::invalid_argument("Unknown colormap name");
    }
    if (value.metaType().id() == QMetaType::fromType<cppqtgraph::ColorMap>().id()) {
        return value;
    }
    throw std::invalid_argument("Cannot set colormap lut parameter from incompatible value");
}

QVariantMap normalizeColorMapLutOpts(QVariantMap opts)
{
    if (opts.contains(QStringLiteral("value"))) {
        opts.insert(QStringLiteral("value"), interpretColorMapLutValue(opts.value(QStringLiteral("value"))));
    }
    if (opts.contains(QStringLiteral("default"))) {
        opts.insert(QStringLiteral("default"), interpretColorMapLutValue(opts.value(QStringLiteral("default"))));
    }
    return opts;
}

QVariant colorMapVariant(const cppqtgraph::ColorMap& colorMap)
{
    return QVariant::fromValue(colorMap);
}

cppqtgraph::ColorMap colorMapFromVariant(const QVariant& value)
{
    colorMapMetaTypeRegistration();
    if (value.metaType().id() == QMetaType::fromType<cppqtgraph::ColorMap>().id()) {
        return value.value<cppqtgraph::ColorMap>();
    }
    return {};
}

} // namespace

ColorMapParameterItem::ColorMapParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param,
                          depth,
                          new widgets::GradientWidget(nullptr, QStringLiteral("bottom")),
                          WidgetParameterItemOptions{.hideWhenDeselected = false, .asSubItem = true})
{
    bindEditor(editor_);
    writeEditorValue(param->value());
}

void ColorMapParameterItem::bindEditor(QWidget* editor)
{
    auto* gradient = qobject_cast<widgets::GradientWidget*>(editor);
    if (gradient == nullptr) {
        return;
    }
    QObject::connect(gradient,
                     &widgets::GradientWidget::sigGradientChangeFinished,
                     gradient,
                     [this](graphicsItems::GradientEditorItem*) { widgetValueChanged(); });
    QObject::connect(gradient,
                     &widgets::GradientWidget::sigGradientChanged,
                     gradient,
                     [this](graphicsItems::GradientEditorItem*) {
                         editorValueChanging(colorMapVariant(
                             qobject_cast<widgets::GradientWidget*>(editor_)->colorMap()));
                     });
}

QVariant ColorMapParameterItem::readEditorValue() const
{
    if (auto* gradient = qobject_cast<const widgets::GradientWidget*>(editor_)) {
        return colorMapVariant(gradient->colorMap());
    }
    return QVariant();
}

void ColorMapParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* gradient = qobject_cast<widgets::GradientWidget*>(editor_)) {
        const QSignalBlocker blocker(gradient);
        if (val.isValid() && !val.isNull()) {
            gradient->setColorMap(colorMapFromVariant(val));
        }
    }
}

void ColorMapParameterItem::configureEditor(QWidget* editor)
{
    if (auto* gradient = qobject_cast<widgets::GradientWidget*>(editor)) {
        gradient->setMinimumSize(300, 35);
        gradient->setMaximumHeight(35);
        gradient->setLength(300);
    }
}

ColorMapParameter::ColorMapParameter(QVariantMap opts, QObject* parent)
    : Parameter(std::move(opts), parent)
{
}

QVariant ColorMapParameter::interpretValue(const QVariant& value) const
{
    if (!value.isValid() || value.isNull()) {
        return value;
    }
    if (value.metaType().id() == QMetaType::fromType<cppqtgraph::ColorMap>().id()) {
        return value;
    }
    throw std::invalid_argument("Cannot set colormap parameter from incompatible value");
}

QVariant ColorMapParameter::setValue(const QVariant& value, bool blockSignal)
{
    return Parameter::setValue(interpretValue(value), blockSignal);
}

ParameterItem* ColorMapParameter::makeTreeItem(int depth)
{
    return new ColorMapParameterItem(this, depth);
}

ColorMapLutParameterItem::ColorMapLutParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param,
                          depth,
                          new widgets::ColorMapButton(),
                          WidgetParameterItemOptions{.hideWhenDeselected = false})
{
    bindEditor(editor_);
    writeEditorValue(param->value());
}

void ColorMapLutParameterItem::bindEditor(QWidget* editor)
{
    auto* button = qobject_cast<widgets::ColorMapButton*>(editor);
    if (button == nullptr) {
        return;
    }
    QObject::connect(button, &widgets::ColorMapButton::sigColorMapChanged, button, [this](const cppqtgraph::ColorMap&) {
        widgetValueChanged();
    });
}

QVariant ColorMapLutParameterItem::readEditorValue() const
{
    if (auto* button = qobject_cast<const widgets::ColorMapButton*>(editor_)) {
        return colorMapVariant(button->colorMap());
    }
    return QVariant();
}

void ColorMapLutParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* button = qobject_cast<widgets::ColorMapButton*>(editor_)) {
        const QSignalBlocker blocker(button);
        if (!val.isValid() || val.isNull()) {
            return;
        }
        if (val.metaType().id() == QMetaType::QString) {
            button->setColorMap(val.toString());
        } else if (val.metaType().id() == QMetaType::fromType<cppqtgraph::ColorMap>().id()) {
            button->setColorMap(val.value<cppqtgraph::ColorMap>());
        }
    }
}

ColorMapLutParameter::ColorMapLutParameter(QVariantMap opts, QObject* parent)
    : Parameter(normalizeColorMapLutOpts(std::move(opts)), parent)
{
}

QVariant ColorMapLutParameter::interpretValue(const QVariant& value) const
{
    return interpretColorMapLutValue(value);
}

QVariant ColorMapLutParameter::setValue(const QVariant& value, bool blockSignal)
{
    return Parameter::setValue(interpretValue(value), blockSignal);
}

ParameterItem* ColorMapLutParameter::makeTreeItem(int depth)
{
    return new ColorMapLutParameterItem(this, depth);
}

} // namespace cppqtgraph::parametertree
