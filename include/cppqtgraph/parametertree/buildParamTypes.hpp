#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/examples/_buildParamTypes.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Parameter.hpp>

#include <memory>

namespace cppqtgraph::parametertree {

std::shared_ptr<Parameter> buildExampleParametersGroup();

} // namespace cppqtgraph::parametertree
