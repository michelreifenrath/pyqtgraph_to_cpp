#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/colormap.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QColor>
#include <QLinearGradient>
#include <QPointF>
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

    struct Stops final {
        OutputMode mode{OutputMode::Byte};
        std::vector<double> positions;
        std::size_t channels{4};
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
    [[nodiscard]] static ColorMap fromEqualSpacing(
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

    [[nodiscard]] Stops getStops(OutputMode mode = OutputMode::Byte) const;
    [[nodiscard]] LookupTable getColors(OutputMode mode = OutputMode::Byte) const;
    [[nodiscard]] QColor getByIndex(std::size_t index) const;
    [[nodiscard]] std::array<std::uint8_t, 4> mapToByte(double value) const;
    [[nodiscard]] std::array<double, 4> mapToFloat(double value) const;
    [[nodiscard]] QColor mapToQColor(double value) const;
    [[nodiscard]] QLinearGradient getGradient(
        std::optional<QPointF> p1 = std::nullopt,
        std::optional<QPointF> p2 = std::nullopt) const;

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

[[nodiscard]] std::vector<QString> listMaps(const QString& source = {});
[[nodiscard]] std::optional<ColorMap> get(const QString& name, const QString& source = {}, bool skipCache = false);

} // namespace pyqtgraph
