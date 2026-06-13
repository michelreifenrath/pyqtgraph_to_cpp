#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/checklist.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>

namespace cppqtgraph::parametertree {

class RadioParameterItem final : public WidgetParameterItem {
public:
    RadioParameterItem(Parameter* param, int depth);

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
};

class RadioParameter final : public Parameter {
public:
    explicit RadioParameter(QVariantMap opts, QObject* parent = nullptr);

    ParameterItem* makeTreeItem(int depth) override;
};

} // namespace cppqtgraph::parametertree
