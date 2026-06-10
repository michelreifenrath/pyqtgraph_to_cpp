#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/HistogramLUTWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/graphicsItems/HistogramLUTItem.hpp>
#include <cppqtgraph/widgets/GraphicsView.hpp>

#include <QtCore/QSize>
#include <QtCore/QString>

#include <cstddef>
#include <utility>

namespace cppqtgraph::widgets {

class HistogramLUTWidget final : public GraphicsView {
    Q_OBJECT

public:
    explicit HistogramLUTWidget(QWidget* parent = nullptr,
                                graphicsItems::ImageItem* image = nullptr,
                                bool fillHistogram = true,
                                const QString& levelMode = QStringLiteral("mono"),
                                const QString& gradientPosition = QStringLiteral("right"),
                                graphicsItems::HistogramLUTItem::Orientation orientation =
                                    graphicsItems::HistogramLUTItem::Orientation::Vertical);

    [[nodiscard]] graphicsItems::HistogramLUTItem* item() const noexcept { return item_; }
    [[nodiscard]] graphicsItems::HistogramLUTItem::Orientation orientation() const noexcept
    {
        return orientation_;
    }

    void setImageItem(graphicsItems::ImageItem* image);
    [[nodiscard]] graphicsItems::ImageItem* imageItem() const noexcept;
    [[nodiscard]] QString levelMode() const;
    void setLevelMode(const QString& mode);
    [[nodiscard]] QString orientationName() const;
    [[nodiscard]] QString gradientPosition() const;
    [[nodiscard]] std::pair<double, double> getLevels() const;
    void setLevels(double minimum, double maximum);
    void setLevels(const std::pair<double, double>& levels);
    void setColorMap(const ColorMap& colorMap);
    void setColorMap(const QString& name);
    [[nodiscard]] const ColorMap& colorMap() const noexcept;
    [[nodiscard]] bool isLookupTrivial() const noexcept;
    [[nodiscard]] ImageLookupTable getLookupTable(std::size_t rows = 256, bool alpha = true) const;
    [[nodiscard]] graphicsItems::LinearRegionItem* levelRegion() noexcept;
    [[nodiscard]] const graphicsItems::LinearRegionItem* levelRegion() const noexcept;

    [[nodiscard]] QSize sizeHint() const override;

private:
    void applyOrientationSizing();

    graphicsItems::HistogramLUTItem* item_ = nullptr;
    graphicsItems::HistogramLUTItem::Orientation orientation_ =
        graphicsItems::HistogramLUTItem::Orientation::Vertical;
};

} // namespace cppqtgraph::widgets
