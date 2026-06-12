#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ImageItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "cppqtgraph/core/ArrayView.hpp"
#include "cppqtgraph/functions.hpp"

#include <QtCore/QRectF>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class QGraphicsItem;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class ImageItem : public GraphicsObject {
    Q_OBJECT

public:
    enum class AxisOrder {
        ColMajor,
        RowMajor,
    };

    explicit ImageItem(QGraphicsItem* parent = nullptr);
    ~ImageItem() override;

    ImageItem(const ImageItem&) = delete;
    ImageItem& operator=(const ImageItem&) = delete;
    ImageItem(ImageItem&&) = delete;
    ImageItem& operator=(ImageItem&&) = delete;

    void setImage(core::ArrayView<const std::uint8_t, 2> image);
    void setImage(core::ArrayView<const std::uint8_t, 3> image);
    void setImage(core::ArrayView<const std::uint16_t, 2> image);
    void setImage(core::ArrayView<const std::uint16_t, 3> image);
    void setImage(core::ArrayView<const float, 2> image);
    void clearImage();

    void setAxisOrder(AxisOrder axisOrder);
    [[nodiscard]] AxisOrder axisOrder() const noexcept;

    void setCompositionMode(QPainter::CompositionMode mode);
    void clearCompositionMode();

    void setLevels(std::optional<ImageLevelRange> levels);
    void setChannelLevels(const std::vector<ImageLevelRange>& levels);
    void clearChannelLevels();
    [[nodiscard]] std::optional<ImageLevelRange> getLevels() const noexcept;
    [[nodiscard]] std::optional<std::vector<ImageLevelRange>> getChannelLevels() const noexcept;
    [[nodiscard]] std::pair<std::vector<double>, std::vector<double>> getHistogram(int channel = -1) const;
    void setLookupTable(ImageLookupTable lut);
    void clearLookupTable();
    [[nodiscard]] std::optional<ImageLookupTable> lookupTable() const noexcept;

    [[nodiscard]] bool hasImage() const noexcept;
    [[nodiscard]] std::size_t width() const noexcept;
    [[nodiscard]] std::size_t height() const noexcept;
    [[nodiscard]] std::size_t channels() const noexcept;
    [[nodiscard]] bool renderRequired() const noexcept;
    [[nodiscard]] bool isUnrenderable() const noexcept;
    [[nodiscard]] const QImage& cachedImage() const noexcept;

    [[nodiscard]] bool render();

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void sigImageChanged();

private:
    enum class DataKind {
        None,
        UInt8Rank2,
        UInt8Rank3,
        UInt16Rank2,
        UInt16Rank3,
        FloatRank2,
    };

    template <typename T, std::size_t Rank>
    void setImageImpl(core::ArrayView<const T, Rank> image, DataKind kind, std::vector<T>& destination);

    [[nodiscard]] std::array<std::size_t, 2> extents() const noexcept;
    [[nodiscard]] std::optional<ImageLookupTable> lookupTableView() const noexcept;
    void markRenderRequired();

    DataKind dataKind_ = DataKind::None;
    AxisOrder axisOrder_ = AxisOrder::ColMajor;
    std::array<std::size_t, 3> shape_{};
    std::vector<std::uint8_t> uint8Data_;
    std::vector<std::uint16_t> uint16Data_;
    std::vector<float> floatData_;
    std::optional<ImageLevelRange> levels_;
    std::optional<std::vector<ImageLevelRange>> channelLevels_;
    std::vector<std::uint8_t> lookupTableData_;
    std::size_t lookupTableRows_ = 0;
    std::size_t lookupTableChannels_ = 0;
    QImage qimage_;
    bool renderRequired_ = true;
    bool unrenderable_ = false;
    bool hasCompositionMode_ = false;
    QPainter::CompositionMode compositionMode_ = QPainter::CompositionMode_SourceOver;
};

} // namespace cppqtgraph::graphicsItems
