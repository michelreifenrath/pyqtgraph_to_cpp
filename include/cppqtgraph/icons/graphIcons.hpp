#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/icons/__init__.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtGui/QPixmap>

namespace cppqtgraph::icons {

[[nodiscard]] QPixmap getGraphPixmap(const QString& name, const QSize& size = QSize(20, 20));

} // namespace cppqtgraph::icons
