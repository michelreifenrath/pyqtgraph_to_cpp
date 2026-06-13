// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/checklist.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/RadioParameterItem.hpp"
#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QRadioButton>

namespace cppqtgraph::parametertree {

RadioParameterItem::RadioParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param,
                          depth,
                          new QRadioButton(),
                          WidgetParameterItemOptions{.hideWhenDeselected = false})
{
    bindEditor(editor_);
    writeEditorValue(param->value());
}

void RadioParameterItem::bindEditor(QWidget* editor)
{
    auto* radio = qobject_cast<QRadioButton*>(editor);
    if (radio == nullptr) {
        return;
    }
    QObject::connect(radio, &QRadioButton::toggled, radio, [this](bool checked) {
        if (checked) {
            widgetValueChanged();
        }
    });
}

QVariant RadioParameterItem::readEditorValue() const
{
    if (auto* radio = qobject_cast<const QRadioButton*>(editor_)) {
        return radio->isChecked();
    }
    return QVariant();
}

void RadioParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* radio = qobject_cast<QRadioButton*>(editor_)) {
        const QSignalBlocker blocker(radio);
        radio->setChecked(val.toBool());
    }
}

void RadioParameterItem::configureEditor(QWidget* /*editor*/)
{
}

RadioParameter::RadioParameter(QVariantMap opts, QObject* parent)
    : Parameter(std::move(opts), parent)
{
}

ParameterItem* RadioParameter::makeTreeItem(int depth)
{
    return new RadioParameterItem(this, depth);
}

} // namespace cppqtgraph::parametertree
