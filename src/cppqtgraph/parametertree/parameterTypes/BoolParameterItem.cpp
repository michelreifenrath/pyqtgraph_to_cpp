// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/bool.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/BoolParameterItem.hpp"
#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QCheckBox>

namespace cppqtgraph::parametertree {

BoolParameterItem::BoolParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param,
                          depth,
                          new QCheckBox(),
                          WidgetParameterItemOptions{.hideWhenDeselected = false})
{
    bindEditor(editor_);
    writeEditorValue(param->value());
}

void BoolParameterItem::bindEditor(QWidget* editor)
{
    auto* checkBox = qobject_cast<QCheckBox*>(editor);
    if (checkBox == nullptr) {
        return;
    }
    QObject::connect(checkBox, &QCheckBox::toggled, checkBox, [this](bool) { widgetValueChanged(); });
}

QVariant BoolParameterItem::readEditorValue() const
{
    if (auto* checkBox = qobject_cast<const QCheckBox*>(editor_)) {
        return checkBox->isChecked();
    }
    return QVariant();
}

void BoolParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* checkBox = qobject_cast<QCheckBox*>(editor_)) {
        const QSignalBlocker blocker(checkBox);
        checkBox->setChecked(val.toBool());
    }
}

void BoolParameterItem::configureEditor(QWidget* /*editor*/)
{
}

} // namespace cppqtgraph::parametertree
