#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ImageItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "../core/ArrayView.hpp"
#include "../functions.hpp"

#include <QtCore/QRectF>
#include <QtGui/QImage>

#include <cstdint>
#include <optional>
#include <vector>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class ImageItem : public GraphicsObject {
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

    void setAxisOrder(AxisOrder axisOrder);
    [[nodiscard]] AxisOrder axisOrder() const noexcept;

    void setLevels(std::optional<pyqtgraph::ImageLevelRange> levels);
    [[nodiscard]] std::optional<pyqtgraph::ImageLevelRange> levels() const noexcept;

    void setLookupTable(std::optional<pyqtgraph::ImageLookupTable> lut);
    [[nodiscard]] std::optional<pyqtgraph::ImageLookupTable> lookupTable() const noexcept;

    void setImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2> image);
    void setImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3> image);
    void setImage(pyqtgraph::core::ArrayView<const std::uint16_t, 2> image);
    void setImage(pyqtgraph::core::ArrayView<const float, 2> image);

    void clear();
    bool render();

    [[nodiscard]] const QImage& qimage() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    enum class DataType {
        None,
        UInt8,
        UInt16,
        Float32,
    };

    enum class Rank {
        None,
        Two,
        Three,
    };

    template <typename T, std::size_t RankValue>
    void setImageImpl(pyqtgraph::core::ArrayView<const T, RankValue> image, DataType dataType, Rank rank);

    void geometryMayChange(int newWidth, int newHeight);
    void markRenderRequired();

    std::vector<std::uint8_t> uint8Storage_;
    std::vector<std::uint16_t> uint16Storage_;
    std::vector<float> floatStorage_;
    std::size_t shape0_ = 0;
    std::size_t shape1_ = 0;
    std::size_t channels_ = 1;
    DataType dataType_ = DataType::None;
    Rank rank_ = Rank::None;
    AxisOrder axisOrder_ = AxisOrder::ColMajor;
    std::optional<pyqtgraph::ImageLevelRange> levels_;
    std::optional<pyqtgraph::ImageLookupTable> lut_;
    QImage qimage_;
    bool renderRequired_ = true;
    bool unrenderable_ = false;
};

} // namespace pyqtgraph::graphicsItems
