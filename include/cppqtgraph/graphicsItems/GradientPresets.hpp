#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GradientPresets.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QString>
#include <QtGui/QColor>

#include <map>
#include <utility>
#include <vector>

namespace cppqtgraph::graphicsItems {

struct GradientPreset {
    std::vector<std::pair<qreal, QColor>> ticks;
    QString mode;
};

[[nodiscard]] const std::map<QString, GradientPreset>& gradients();

} // namespace cppqtgraph::graphicsItems
