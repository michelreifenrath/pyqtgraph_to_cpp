// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/imageview/ImageView.hpp"

#include "../../../include/cppqtgraph/graphicsItems/HistogramLUTItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../include/cppqtgraph/widgets/GraphicsView.hpp"

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

namespace cppqtgraph::imageview {
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
    } else if constexpr (Rank == 4) {
        for (std::size_t frame = 0; frame < shape[0]; ++frame) {
            for (std::size_t axis1 = 0; axis1 < shape[1]; ++axis1) {
                for (std::size_t axis2 = 0; axis2 < shape[2]; ++axis2) {
                    for (std::size_t channel = 0; channel < shape[3]; ++channel) {
                        const std::size_t index = ((frame * shape[1] + axis1) * shape[2] + axis2) * shape[3] + channel;
                        destination[index] = image(frame, axis1, axis2, channel);
                    }
                }
            }
        }
    }
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> contiguousRank2(const std::vector<T>& storage, const std::array<std::size_t, 4>& shape)
{
    return core::ArrayView<const T, 2>(storage.data(), {shape[0], shape[1]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 3> contiguousRank3(const std::vector<T>& storage, const std::array<std::size_t, 4>& shape)
{
    return core::ArrayView<const T, 3>(storage.data(), {shape[0], shape[1], shape[2]});
}

[[nodiscard]] bool isColorImageShape(const std::array<std::size_t, 4>& shape) noexcept
{
    return shape[3] == 1 && (shape[2] == 3 || shape[2] == 4);
}

[[nodiscard]] bool isFrameStackShape(const std::array<std::size_t, 4>& shape) noexcept
{
    return shape[3] == 1 && !isColorImageShape(shape);
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> frameView(const std::vector<T>& storage,
                                                     const std::array<std::size_t, 4>& shape,
                                                     int frameIndex)
{
    const std::size_t frame = static_cast<std::size_t>(frameIndex);
    const std::size_t framePixels = shape[1] * shape[2];
    const T* frameData = storage.data() + frame * framePixels;
    return core::ArrayView<const T, 2>(frameData, {shape[1], shape[2]});
}

template <typename T>
void extractRgbFrame(const std::vector<T>& storage,
                     const std::array<std::size_t, 4>& shape,
                     int frameIndex,
                     std::vector<std::uint8_t>& rgbBuffer)
{
    const std::size_t frame = static_cast<std::size_t>(frameIndex);
    const std::size_t height = shape[1];
    const std::size_t width = shape[2];
    const std::size_t channels = shape[3];
    rgbBuffer.resize(height * width * channels);
    for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
        for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
            for (std::size_t channel = 0; channel < channels; ++channel) {
                const std::size_t sourceIndex = ((frame * height + axis1) * width + axis2) * channels + channel;
                const auto rounded = std::lround(static_cast<double>(storage[sourceIndex]));
                rgbBuffer[(axis1 * width + axis2) * channels + channel] =
                    static_cast<std::uint8_t>(std::clamp(rounded, 0L, 255L));
            }
        }
    }
}

void assignDefaultXValues(std::vector<double>& xvals, std::size_t frameCount)
{
    xvals.resize(frameCount);
    for (std::size_t index = 0; index < frameCount; ++index) {
        xvals[index] = static_cast<double>(index);
    }
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

void ImageView::setImage(core::ArrayView<const float, 4> image,
                         core::ArrayView<const double, 1> xvals,
                         bool autoLevels,
                         bool autoRange)
{
    setImageTimeRgbImpl(image, xvals, DataKind::FloatRank4TimeRgb, floatData_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const double, 4> image,
                         core::ArrayView<const double, 1> xvals,
                         bool autoLevels,
                         bool autoRange)
{
    setImageTimeRgbImpl(image, xvals, DataKind::DoubleRank4TimeRgb, doubleData_, autoLevels, autoRange);
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
        doubleData_.clear();
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        uint8Data_.clear();
        floatData_.clear();
        doubleData_.clear();
    } else {
        uint8Data_.clear();
        uint16Data_.clear();
        doubleData_.clear();
    }
    xvals_.clear();

    copyImage(image, destination);
    dataKind_ = kind;
    shape_[0] = image.shape()[0];
    shape_[1] = image.shape()[1];
    if constexpr (Rank == 3) {
        shape_[2] = image.shape()[2];
        shape_[3] = 1;
        if (isFrameStackShape(shape_)) {
            if (currentIndex_ < 0 || static_cast<std::size_t>(currentIndex_) >= shape_[0]) {
                currentIndex_ = 0;
            }
        } else {
            currentIndex_ = 0;
        }
    } else {
        shape_[2] = 1;
        shape_[3] = 1;
        currentIndex_ = 0;
    }

    updateDisplayedFrame(autoLevels, autoRange);
}

template <typename T>
void ImageView::setImageTimeRgbImpl(core::ArrayView<const T, 4> image,
                                    core::ArrayView<const double, 1> xvals,
                                    DataKind kind,
                                    std::vector<T>& destination,
                                    bool autoLevels,
                                    bool autoRange)
{
    if (image.empty()) {
        clearImage();
        return;
    }

    const auto& shape = image.shape();
    if (shape[3] != 3 && shape[3] != 4) {
        return;
    }

    uint8Data_.clear();
    uint16Data_.clear();
    if constexpr (std::is_same_v<T, float>) {
        doubleData_.clear();
    } else {
        floatData_.clear();
    }

    copyImage(image, destination);
    dataKind_ = kind;
    shape_[0] = shape[0];
    shape_[1] = shape[1];
    shape_[2] = shape[2];
    shape_[3] = shape[3];
    if (currentIndex_ < 0 || static_cast<std::size_t>(currentIndex_) >= shape_[0]) {
        currentIndex_ = 0;
    }

    if (!xvals.empty()) {
        if (xvals.shape()[0] != shape_[0]) {
            clearImage();
            return;
        }
        xvals_.assign(xvals.data(), xvals.data() + xvals.shape()[0]);
    } else {
        assignDefaultXValues(xvals_, shape_[0]);
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
    doubleData_.clear();
    xvals_.clear();
    rgbDisplayBuffer_.clear();
    imageItem_->clearImage();
}

void ImageView::setCurrentIndex(int index)
{
    if (!hasImage()) {
        return;
    }
    const bool timeRgbStack = dataKind_ == DataKind::FloatRank4TimeRgb || dataKind_ == DataKind::DoubleRank4TimeRgb;
    if (!isFrameStackShape(shape_) && !timeRgbStack) {
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

std::span<const double> ImageView::xValues() const noexcept
{
    return xvals_;
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
    case DataKind::FloatRank4TimeRgb:
        updateDisplayedRgbFrame(autoLevels, autoRange);
        return;
    case DataKind::DoubleRank4TimeRgb:
        updateDisplayedRgbFrame(autoLevels, autoRange);
        return;
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

void ImageView::updateDisplayedRgbFrame(bool autoLevels, bool autoRange)
{
    switch (dataKind_) {
    case DataKind::FloatRank4TimeRgb:
        extractRgbFrame(floatData_, shape_, currentIndex_, rgbDisplayBuffer_);
        break;
    case DataKind::DoubleRank4TimeRgb:
        extractRgbFrame(doubleData_, shape_, currentIndex_, rgbDisplayBuffer_);
        break;
    default:
        return;
    }

    const std::array<std::size_t, 3> rgbShape{shape_[1], shape_[2], shape_[3]};
    imageItem_->setImage(core::ArrayView<const std::uint8_t, 3>(rgbDisplayBuffer_.data(), rgbShape));

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
    case DataKind::FloatRank4TimeRgb:
        return autoLevelsFor(floatData_);
    case DataKind::DoubleRank4TimeRgb:
        return autoLevelsFor(doubleData_);
    case DataKind::None:
        return std::nullopt;
    }
    return std::nullopt;
}

} // namespace cppqtgraph::imageview
