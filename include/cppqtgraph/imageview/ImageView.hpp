#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "ImageViewTemplate_generic.hpp"
#include "cppqtgraph/core/ArrayView.hpp"
#include "cppqtgraph/functions.hpp"
#include "cppqtgraph/graphicsItems/ImageItem.hpp"

#include <QtWidgets/QWidget>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace cppqtgraph::graphicsItems {
class HistogramLUTItem;
class ViewBox;
} // namespace cppqtgraph::graphicsItems

namespace cppqtgraph::widgets {
class GraphicsView;
} // namespace cppqtgraph::widgets

namespace cppqtgraph::imageview {

class ImageView : public QWidget {
    Q_OBJECT

public:
    explicit ImageView(QWidget* parent = nullptr, bool levelMode = false);
    ~ImageView() override;

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&&) = delete;
    ImageView& operator=(ImageView&&) = delete;

    void setImage(core::ArrayView<const std::uint8_t, 2> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const std::uint8_t, 3> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const std::uint16_t, 2> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const std::uint16_t, 3> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const float, 2> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const float, 3> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const float, 4> image,
                  core::ArrayView<const double, 1> xvals = {},
                  bool autoLevels = false,
                  bool autoRange = true);
    void setImage(core::ArrayView<const double, 4> image,
                  core::ArrayView<const double, 1> xvals = {},
                  bool autoLevels = false,
                  bool autoRange = true);
    void clearImage();

    void setCurrentIndex(int index);
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] std::span<const double> xValues() const noexcept;

    void setLevels(std::optional<ImageLevelRange> levels);
    void autoLevels();
    void setLookupTable(ImageLookupTable lut);
    void clearLookupTable();

    void setAxisOrder(graphicsItems::ImageItem::AxisOrder axisOrder);
    [[nodiscard]] graphicsItems::ImageItem::AxisOrder axisOrder() const noexcept;

    [[nodiscard]] widgets::GraphicsView* getView() noexcept;
    [[nodiscard]] const widgets::GraphicsView* getView() const noexcept;
    [[nodiscard]] graphicsItems::ViewBox* getViewBox() noexcept;
    [[nodiscard]] const graphicsItems::ViewBox* getViewBox() const noexcept;
    [[nodiscard]] graphicsItems::ImageItem* getImageItem() noexcept;
    [[nodiscard]] const graphicsItems::ImageItem* getImageItem() const noexcept;
    [[nodiscard]] graphicsItems::HistogramLUTItem* getHistogram() noexcept;
    [[nodiscard]] const graphicsItems::HistogramLUTItem* getHistogram() const noexcept;

    [[nodiscard]] bool hasImage() const noexcept;

private:
    enum class DataKind {
        None,
        UInt8Rank2,
        UInt8Rank3,
        UInt16Rank2,
        UInt16Rank3,
        FloatRank2,
        FloatRank3,
        FloatRank4TimeRgb,
        DoubleRank4TimeRgb,
    };

    template <typename T, std::size_t Rank>
    void setImageImpl(core::ArrayView<const T, Rank> image,
                      DataKind kind,
                      std::vector<T>& destination,
                      bool autoLevels,
                      bool autoRange);

    template <typename T>
    void setImageTimeRgbImpl(core::ArrayView<const T, 4> image,
                             core::ArrayView<const double, 1> xvals,
                             DataKind kind,
                             std::vector<T>& destination,
                             bool autoLevels,
                             bool autoRange);

    void updateDisplayedFrame(bool autoLevels, bool autoRange);
    void updateDisplayedRgbFrame(bool autoLevels, bool autoRange);
    void applyAutoLevels();
    [[nodiscard]] std::optional<ImageLevelRange> computeAutoLevels() const;

    Ui_Form ui_;
    bool levelMode_ = false;
    DataKind dataKind_ = DataKind::None;
    int currentIndex_ = 0;
    std::array<std::size_t, 4> shape_{};
    std::vector<std::uint8_t> uint8Data_;
    std::vector<std::uint16_t> uint16Data_;
    std::vector<float> floatData_;
    std::vector<double> doubleData_;
    std::vector<double> xvals_;
    std::vector<std::uint8_t> rgbDisplayBuffer_;
    graphicsItems::ImageItem::AxisOrder axisOrder_ = graphicsItems::ImageItem::AxisOrder::ColMajor;

    widgets::GraphicsView* graphicsView_ = nullptr;
    graphicsItems::ViewBox* viewBox_ = nullptr;
    graphicsItems::ImageItem* imageItem_ = nullptr;
    graphicsItems::HistogramLUTItem* histogram_ = nullptr;
};

} // namespace cppqtgraph::imageview
