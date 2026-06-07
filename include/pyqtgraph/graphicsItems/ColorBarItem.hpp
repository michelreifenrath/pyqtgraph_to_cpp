#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ColorBarItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "LinearRegionItem.hpp"
#include "PlotItem/PlotItem.hpp"
#include "pyqtgraph/colormap.hpp"
#include "pyqtgraph/functions.hpp"

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtWidgets/QGraphicsPixmapItem>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <utility>
#include <vector>

namespace pyqtgraph::graphicsItems {

class ImageItem;

class ColorBarItem : public PlotItem {
    Q_OBJECT

public:
    enum class Orientation { Vertical, Horizontal };

    explicit ColorBarItem(std::optional<std::pair<double, double>> values = std::nullopt,
                          double width = 25.0,
                          std::optional<ColorMap> colorMap = std::nullopt,
                          bool interactive = true,
                          std::optional<std::pair<double, double>> limits = std::nullopt,
                          double rounding = 1.0,
                          Orientation orientation = Orientation::Vertical,
                          QGraphicsItem* parent = nullptr,
                          Qt::WindowFlags flags = Qt::WindowFlags{});
    ~ColorBarItem() override;

    ColorBarItem(const ColorBarItem&) = delete;
    ColorBarItem& operator=(const ColorBarItem&) = delete;
    ColorBarItem(ColorBarItem&&) = delete;
    ColorBarItem& operator=(ColorBarItem&&) = delete;

    void setImageItem(ImageItem* image);
    void setImageItems(std::initializer_list<ImageItem*> images);
    void setImageItems(const std::vector<ImageItem*>& images);
    [[nodiscard]] std::vector<ImageItem*> imageItems() const;

    void setColorMap(const ColorMap& colorMap);
    void setColorMap(const QString& name);
    [[nodiscard]] const std::optional<ColorMap>& colorMap() const noexcept;

    void setLevels(std::optional<std::pair<double, double>> values = std::nullopt,
                   std::optional<double> low = std::nullopt,
                   std::optional<double> high = std::nullopt,
                   bool updateItems = true);
    [[nodiscard]] std::pair<double, double> levels() const noexcept;

    [[nodiscard]] LinearRegionItem* interactionRegion() noexcept;
    [[nodiscard]] const LinearRegionItem* interactionRegion() const noexcept;
    [[nodiscard]] bool regionChangedEnabled() const noexcept;
    void setRegionChangedEnabled(bool enabled) noexcept;

    [[nodiscard]] bool isHorizontal() const noexcept;
    [[nodiscard]] std::optional<std::pair<double, double>> limits() const noexcept;
    [[nodiscard]] double rounding() const noexcept;

public slots:
    void regionChanging();
    void regionChanged();

signals:
    void sigLevelsChanged(pyqtgraph::graphicsItems::ColorBarItem* item);
    void sigLevelsChangeFinished(pyqtgraph::graphicsItems::ColorBarItem* item);

private:
    void updateItems(bool updateColorMap = false);
    void updateBarPixmap();
    void applyColorMapTo(ImageItem& image) const;
    [[nodiscard]] ImageLookupTable imageLookupTable() const;
    void rebuildLookupTable(std::size_t rows = 256, bool alpha = true) const;

    std::vector<QPointer<ImageItem>> imageItems_;
    std::pair<double, double> values_{0.0, 1.0};
    std::pair<double, double> previousValues_{0.0, 1.0};
    std::optional<ColorMap> colorMap_;
    std::optional<double> lowLimit_;
    std::optional<double> highLimit_;
    double rounding_ = 1.0;
    bool horizontal_ = false;
    bool activelyAdjustedValues_ = false;
    bool interactive_ = true;
    bool regionChangedEnable_ = false;
    LinearRegionItem* region_ = nullptr;
    QGraphicsPixmapItem* bar_ = nullptr;
    mutable std::vector<std::uint8_t> lookupTableBytes_;
    mutable std::size_t lookupTableRows_ = 0;
    mutable std::size_t lookupTableChannels_ = 0;
};

} // namespace pyqtgraph::graphicsItems
