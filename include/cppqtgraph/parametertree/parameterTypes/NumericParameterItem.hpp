#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/numeric.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/ParameterItem.hpp>

namespace cppqtgraph::widgets {
class SpinBox;
}

namespace cppqtgraph::parametertree {

class NumericParameterItem final : public WidgetParameterItem {
public:
    NumericParameterItem(Parameter* param, int depth);

    void optsChanged(Parameter* param, const QVariantMap& opts) override;

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
    void updateDisplayLabel(const QVariant& value = QVariant()) override;

private:
    void applySpinBoxOpts(const QVariantMap& opts);
    void applyChangedSpinBoxOpts(const QVariantMap& changedOpts);

    widgets::SpinBox* spinBox_ = nullptr;
};

} // namespace cppqtgraph::parametertree
