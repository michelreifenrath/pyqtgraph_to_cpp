// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/str.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/StrParameterItem.hpp"

#include <QtWidgets/QLineEdit>

namespace cppqtgraph::parametertree {

StrParameterItem::StrParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param, depth, new QLineEdit())
{
    configureEditor(editor_);
}

void StrParameterItem::bindEditor(QWidget* /*editor*/)
{
}

void StrParameterItem::configureEditor(QWidget* editor)
{
    if (auto* lineEdit = qobject_cast<QLineEdit*>(editor)) {
        lineEdit->setStyleSheet(QStringLiteral("border: 0px"));
    }
}

} // namespace cppqtgraph::parametertree
