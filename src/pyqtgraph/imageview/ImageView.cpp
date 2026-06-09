// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/imageview/ImageView.hpp"

#include "../../../include/pyqtgraph/graphicsItems/HistogramLUTItem.hpp"
#include "../../../include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../include/pyqtgraph/widgets/GraphicsView.hpp"

#include <QtCore/QRectF>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace pyqtgraph::imageview {
namespace {

template <typename T, std::size_t Rank>
void copyImage(core::ArrayView<const T, Rank> image, std::vector<T>& destination)
{
    destination.resize(image.size());
    const auto& shape = image.shape();
    if constexpr (Rank == 2) {
        for (std::size_t axis0 = 0; axis0 < shape[0]; ++axis0) {
            for (std::size_t axis1 = 0; axis1 < shape[1]; ++axis1) {
                destination[axis0 * shape[1] + axis1] = image(axis0, axis1);
            }
        }
    } else if constexpr (Rank == 3) {
        for (std::size_t axis0 = 0; axis0 < shape[0]; ++axis0) {
            for (std::size_t axis1 = 0; axis1 < shape[1]; ++axis1) {
                for (std::size_t channel = 0; channel < shape[2]; ++channel) {
                    destination[(axis0 * shape[1] + axis1) * shape[2] + channel] = image(axis0, axis1, channel);
                }
            }
        }
    }
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> contiguousRank2(const std::vector<T>& storage, const std::array<std::size_t, 3>& shape)
{
    return core::ArrayView<const T, 2>(storage.data(), {shape[0], shape[1]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 3> contiguousRank3(const std::vector<T>& storage, const std::array<std::size_t, 3>& shape)
{
    return core::ArrayView<const T, 3>(storage.data(), {shape[0], shape[1], shape[2]});
}

[[nodiscard]] bool isColorImageShape(const std::array<std::size_t, 3>& shape) noexcept
{
    return shape[2] == 3 || shape[2] == 4;
}

[[nodiscard]] bool isFrameStackShape(const std::array<std::size_t, 3>& shape) noexcept
{
    return !isColorImageShape(shape);
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> frameView(const std::vector<T>& storage,
                                                     const std::array<std::size_t, 3>& shape,
                                                     int frameIndex)
{
    const std::size_t frame = static_cast<std::size_t>(frameIndex);
    const std::size_t framePixels = shape[1] * shape[2];
    const T* frameData = storage.data() + frame * framePixels;
    return core::ArrayView<const T, 2>(frameData, {shape[1], shape[2]});
}

template <typename T>
[[nodiscard]] std::optional<ImageLevelRange> autoLevelsFor(const std::vector<T>& storage)
{
    if (storage.empty()) {
        return std::nullopt;
    }

    if constexpr (std::is_same_v<T, std::uint8_t>) {
        const auto minValue = *std::min_element(storage.begin(), storage.end());
        const auto maxValue = *std::max_element(storage.begin(), storage.end());
        return ImageLevelRange{static_cast<double>(minValue), static_cast<double>(maxValue)};
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        const auto minValue = *std::min_element(storage.begin(), storage.end());
        const auto maxValue = *std::max_element(storage.begin(), storage.end());
        return ImageLevelRange{static_cast<double>(minValue), static_cast<double>(maxValue)};
    } else {
        const double minValue = static_cast<double>(nanmin(std::span<const T>(storage.data(), storage.size())));
        const double maxValue = static_cast<double>(nanmax(std::span<const T>(storage.data(), storage.size())));
        if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
            return std::nullopt;
        }
        return ImageLevelRange{minValue, maxValue};
    }
}

} // namespace

ImageView::ImageView(QWidget* parent, bool levelMode)
    : QWidget(parent)
    , levelMode_(levelMode)
{
    ui_.setupUi(this);

    auto* graphicsLayout = new QVBoxLayout(ui_.graphicsContainer);
    graphicsLayout->setContentsMargins(0, 0, 0, 0);
    graphicsLayout->setSpacing(0);

    graphicsView_ = new widgets::GraphicsView(ui_.graphicsContainer);
    graphicsLayout->addWidget(graphicsView_);

    viewBox_ = new graphicsItems::ViewBox();
    imageItem_ = new graphicsItems::ImageItem();
    viewBox_->addItem(imageItem_);
    graphicsView_->setCentralItem(viewBox_);

    if (levelMode_) {
        histogram_ = new graphicsItems::HistogramLUTItem(imageItem_);
        ui_.histogramContainer->show();
    } else {
        ui_.histogramContainer->hide();
    }
}

ImageView::~ImageView() = default;

void ImageView::setImage(core::ArrayView<const std::uint8_t, 2> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt8Rank2, uint8Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const std::uint8_t, 3> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt8Rank3, uint8Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const std::uint16_t, 2> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt16Rank2, uint16Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const std::uint16_t, 3> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt16Rank3, uint16Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const float, 2> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::FloatRank2, floatData_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const float, 3> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::FloatRank3, floatData_, autoLevels, autoRange);
}

template <typename T, std::size_t Rank>
void ImageView::setImageImpl(core::ArrayView<const T, Rank> image,
                             DataKind kind,
                             std::vector<T>& destination,
                             bool autoLevels,
                             bool autoRange)
{
    if (image.empty()) {
        clearImage();
        return;
    }

    if constexpr (std::is_same_v<T, std::uint8_t>) {
        uint16Data_.clear();
        floatData_.clear();
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        uint8Data_.clear();
        floatData_.clear();
    } else {
        uint8Data_.clear();
        uint16Data_.clear();
    }

    copyImage(image, destination);
    dataKind_ = kind;
    shape_[0] = image.shape()[0];
    shape_[1] = image.shape()[1];
    if constexpr (Rank == 3) {
        shape_[2] = image.shape()[2];
        if (isFrameStackShape(shape_)) {
            if (currentIndex_ < 0 || static_cast<std::size_t>(currentIndex_) >= shape_[0]) {
                currentIndex_ = 0;
            }
        } else {
            currentIndex_ = 0;
        }
    } else {
        shape_[2] = 1;
        currentIndex_ = 0;
    }

    updateDisplayedFrame(autoLevels, autoRange);
}

void ImageView::clearImage()
{
    dataKind_ = DataKind::None;
    shape_ = {};
    currentIndex_ = 0;
    uint8Data_.clear();
    uint16Data_.clear();
    floatData_.clear();
    imageItem_->clearImage();
}

void ImageView::setCurrentIndex(int index)
{
    if (!hasImage() || !isFrameStackShape(shape_)) {
        return;
    }
    if (index < 0 || static_cast<std::size_t>(index) >= shape_[0]) {
        return;
    }
    currentIndex_ = index;
    updateDisplayedFrame(false, true);
}

int ImageView::currentIndex() const noexcept
{
    return currentIndex_;
}

void ImageView::setLevels(std::optional<ImageLevelRange> levels)
{
    imageItem_->setLevels(levels);
}

void ImageView::autoLevels()
{
    applyAutoLevels();
}

void ImageView::setLookupTable(ImageLookupTable lut)
{
    imageItem_->setLookupTable(lut);
}

void ImageView::clearLookupTable()
{
    imageItem_->clearLookupTable();
}

void ImageView::setAxisOrder(graphicsItems::ImageItem::AxisOrder axisOrder)
{
    axisOrder_ = axisOrder;
    imageItem_->setAxisOrder(axisOrder);
    updateDisplayedFrame(false, true);
}

graphicsItems::ImageItem::AxisOrder ImageView::axisOrder() const noexcept
{
    return axisOrder_;
}

widgets::GraphicsView* ImageView::getView() noexcept
{
    return graphicsView_;
}

const widgets::GraphicsView* ImageView::getView() const noexcept
{
    return graphicsView_;
}

graphicsItems::ViewBox* ImageView::getViewBox() noexcept
{
    return viewBox_;
}

const graphicsItems::ViewBox* ImageView::getViewBox() const noexcept
{
    return viewBox_;
}

graphicsItems::ImageItem* ImageView::getImageItem() noexcept
{
    return imageItem_;
}

const graphicsItems::ImageItem* ImageView::getImageItem() const noexcept
{
    return imageItem_;
}

graphicsItems::HistogramLUTItem* ImageView::getHistogram() noexcept
{
    return histogram_;
}

const graphicsItems::HistogramLUTItem* ImageView::getHistogram() const noexcept
{
    return histogram_;
}

bool ImageView::hasImage() const noexcept
{
    return dataKind_ != DataKind::None;
}

void ImageView::updateDisplayedFrame(bool autoLevels, bool autoRange)
{
    if (!hasImage()) {
        return;
    }

    imageItem_->setAxisOrder(axisOrder_);

    switch (dataKind_) {
    case DataKind::UInt8Rank2:
        imageItem_->setImage(contiguousRank2(uint8Data_, shape_));
        break;
    case DataKind::UInt8Rank3:
        if (isColorImageShape(shape_)) {
            imageItem_->setImage(contiguousRank3(uint8Data_, shape_));
        } else {
            imageItem_->setImage(frameView(uint8Data_, shape_, currentIndex_));
        }
        break;
    case DataKind::UInt16Rank2:
        imageItem_->setImage(contiguousRank2(uint16Data_, shape_));
        break;
    case DataKind::UInt16Rank3:
        if (isColorImageShape(shape_)) {
            imageItem_->setImage(contiguousRank3(uint16Data_, shape_));
        } else {
            imageItem_->setImage(frameView(uint16Data_, shape_, currentIndex_));
        }
        break;
    case DataKind::FloatRank2:
        imageItem_->setImage(contiguousRank2(floatData_, shape_));
        break;
    case DataKind::FloatRank3:
        imageItem_->setImage(frameView(floatData_, shape_, currentIndex_));
        break;
    case DataKind::None:
        break;
    }

    if (autoLevels) {
        applyAutoLevels();
    }

    if (autoRange && viewBox_ != nullptr && imageItem_->hasImage()) {
        const QRectF bounds(0.0, 0.0, static_cast<qreal>(imageItem_->width()), static_cast<qreal>(imageItem_->height()));
        viewBox_->setRange(bounds, 0.0, true, true);
    }

    if (histogram_ != nullptr) {
        histogram_->imageChanged(autoLevels, autoRange);
    }
}

void ImageView::applyAutoLevels()
{
    const std::optional<ImageLevelRange> levels = computeAutoLevels();
    if (levels.has_value()) {
        imageItem_->setLevels(*levels);
    }
}

std::optional<ImageLevelRange> ImageView::computeAutoLevels() const
{
    switch (dataKind_) {
    case DataKind::UInt8Rank2:
        return autoLevelsFor(uint8Data_);
    case DataKind::UInt8Rank3:
        return autoLevelsFor(uint8Data_);
    case DataKind::UInt16Rank2:
        return autoLevelsFor(uint16Data_);
    case DataKind::UInt16Rank3:
        return autoLevelsFor(uint16Data_);
    case DataKind::FloatRank2:
        return autoLevelsFor(floatData_);
    case DataKind::FloatRank3:
        return autoLevelsFor(floatData_);
    case DataKind::None:
        return std::nullopt;
    }
    return std::nullopt;
}

} // namespace pyqtgraph::imageview
