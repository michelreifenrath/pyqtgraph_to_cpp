#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/HistogramLUTItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"
#include "LinearRegionItem.hpp"
#include "pyqtgraph/colormap.hpp"
#include "pyqtgraph/functions.hpp"

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace pyqtgraph::graphicsItems {

class ImageItem;

class HistogramLUTItem : public GraphicsWidget {
    Q_OBJECT

public:
    enum class Orientation { Vertical, Horizontal };

    explicit HistogramLUTItem(ImageItem* image = nullptr,
                              bool fillHistogram = true,
                              QString levelMode = QStringLiteral("mono"),
                              QString gradientPosition = QStringLiteral("right"),
                              Orientation orientation = Orientation::Vertical,
                              QGraphicsItem* parent = nullptr,
                              Qt::WindowFlags flags = Qt::WindowFlags{});
    ~HistogramLUTItem() override;

    HistogramLUTItem(const HistogramLUTItem&) = delete;
    HistogramLUTItem& operator=(const HistogramLUTItem&) = delete;
    HistogramLUTItem(HistogramLUTItem&&) = delete;
    HistogramLUTItem& operator=(HistogramLUTItem&&) = delete;

    void setImageItem(ImageItem* image);
    [[nodiscard]] ImageItem* imageItem() const noexcept;

    [[nodiscard]] QString levelMode() const;
    void setLevelMode(const QString& mode);

    [[nodiscard]] Orientation orientation() const noexcept;
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

    [[nodiscard]] LinearRegionItem* levelRegion() noexcept;
    [[nodiscard]] const LinearRegionItem* levelRegion() const noexcept;

public slots:
    void gradientChanged();
    void regionChanging();
    void regionChanged();
    void imageChanged(bool autoLevel = false, bool autoRange = false);

signals:
    void sigLookupTableChanged(pyqtgraph::graphicsItems::HistogramLUTItem* item);
    void sigLevelsChanged(pyqtgraph::graphicsItems::HistogramLUTItem* item);
    void sigLevelChangeFinished(pyqtgraph::graphicsItems::HistogramLUTItem* item);

private:
    static QString normalizeGradientPosition(Orientation orientation, const QString& gradientPosition);
    void setImageLookupTable();
    void applyImageLevels();
    void rebuildLookupTable(std::size_t rows, bool alpha) const;

    QPointer<ImageItem> imageItem_;
    LinearRegionItem* region_ = nullptr;
    QString levelMode_;
    Orientation orientation_ = Orientation::Vertical;
    QString gradientPosition_;
    ColorMap colorMap_;
    bool lookupTrivial_ = true;
    bool fillHistogram_ = true;
    mutable std::vector<std::uint8_t> lookupTableBytes_;
    mutable std::size_t lookupTableRows_ = 0;
    mutable std::size_t lookupTableChannels_ = 0;
    QMetaObject::Connection imageChangedConnection_;
};

} // namespace pyqtgraph::graphicsItems
