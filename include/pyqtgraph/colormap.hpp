#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/colormap.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QColor>
#include <QString>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace pyqtgraph {

class ColorMap final {
public:
    enum class MappingMode {
        Clip = 1,
        Repeat = 2,
        Mirror = 3,
        Diverging = 4,
    };

    enum class OutputMode {
        Byte = 1,
        Float = 2,
        QColor = 3,
    };

    struct LookupTable final {
        OutputMode mode{OutputMode::Byte};
        std::size_t channels{0};
        std::vector<std::uint8_t> bytes;
        std::vector<double> floats;
        std::vector<QColor> colors;

        [[nodiscard]] std::size_t rows() const noexcept;
    };

    ColorMap(
        std::vector<double> positions,
        std::vector<QColor> colors,
        QString name = {},
        MappingMode mappingMode = MappingMode::Clip);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<double>& positions() const noexcept;
    [[nodiscard]] const std::vector<QColor>& colors() const noexcept;
    [[nodiscard]] const QString& name() const noexcept;
    [[nodiscard]] MappingMode mappingMode() const noexcept;
    [[nodiscard]] bool usesAlpha() const noexcept;

    void setMappingMode(MappingMode mappingMode) noexcept;

    [[nodiscard]] LookupTable getLookupTable(
        double start = 0.0,
        double stop = 1.0,
        std::size_t nPts = 512,
        std::optional<bool> alpha = std::nullopt,
        OutputMode mode = OutputMode::Byte) const;

private:
    std::vector<double> positions_;
    std::vector<QColor> colors_;
    std::vector<std::array<double, 4>> colorStops_;
    QString name_;
    MappingMode mappingMode_{MappingMode::Clip};
};

} // namespace pyqtgraph
