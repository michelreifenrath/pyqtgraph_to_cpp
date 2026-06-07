#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/colors/palette.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QPalette>

namespace pyqtgraph::colors {

[[nodiscard]] QPalette getQDarkStyleDarkQPalette();
[[nodiscard]] QPalette getQDarkStyleLightQPalette();

} // namespace pyqtgraph::colors
