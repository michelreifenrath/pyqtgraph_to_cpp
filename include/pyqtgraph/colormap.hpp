#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/colormap.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QString>
#include <QtGui/QColor>

#include <cstddef>
#include <vector>

namespace pyqtgraph {

class ColorMap final {
public:
    ColorMap(std::vector<double> positions, std::vector<QColor> colors, QString name = {});

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<double>& positions() const noexcept;
    [[nodiscard]] const std::vector<QColor>& colors() const noexcept;
    [[nodiscard]] const QString& name() const noexcept;

private:
    std::vector<double> positions_;
    std::vector<QColor> colors_;
    QString name_;
};

} // namespace pyqtgraph
