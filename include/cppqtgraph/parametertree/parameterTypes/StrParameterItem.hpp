#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/str.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/ParameterItem.hpp>

namespace cppqtgraph::parametertree {

class StrParameterItem final : public WidgetParameterItem {
public:
    StrParameterItem(Parameter* param, int depth);

protected:
    void bindEditor(QWidget* editor) override;
    void configureEditor(QWidget* editor) override;
};

} // namespace cppqtgraph::parametertree
