#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/bool.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/ParameterItem.hpp>

namespace cppqtgraph::parametertree {

class BoolParameterItem final : public WidgetParameterItem {
public:
    BoolParameterItem(Parameter* param, int depth);

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
};

} // namespace cppqtgraph::parametertree
