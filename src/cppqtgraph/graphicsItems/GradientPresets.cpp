// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GradientPresets.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/GradientPresets.hpp"

namespace cppqtgraph::graphicsItems {
namespace {

GradientPreset makeRgbPreset(std::initializer_list<std::pair<qreal, QColor>> ticks)
{
    return GradientPreset{std::vector<std::pair<qreal, QColor>>(ticks), QStringLiteral("rgb")};
}

const std::map<QString, GradientPreset>& defaultGradients()
{
    static const std::map<QString, GradientPreset> presets = {
        {QStringLiteral("thermal"),
         makeRgbPreset({{0.0, QColor(0, 0, 0, 255)},
                        {0.3333, QColor(185, 0, 0, 255)},
                        {0.6666, QColor(255, 220, 0, 255)},
                        {1.0, QColor(255, 255, 255, 255)}})},
        {QStringLiteral("grey"),
         makeRgbPreset({{0.0, QColor(0, 0, 0, 255)}, {1.0, QColor(255, 255, 255, 255)}})},
        {QStringLiteral("viridis"),
         makeRgbPreset({{0.0, QColor(68, 1, 84, 255)},
                        {0.25, QColor(58, 82, 139, 255)},
                        {0.5, QColor(32, 144, 140, 255)},
                        {0.75, QColor(94, 201, 97, 255)},
                        {1.0, QColor(253, 231, 36, 255)}})},
    };
    return presets;
}

} // namespace

const std::map<QString, GradientPreset>& gradients()
{
    return defaultGradients();
}

} // namespace cppqtgraph::graphicsItems
