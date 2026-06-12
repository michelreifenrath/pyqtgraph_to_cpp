// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ImageItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/ImageItem.hpp"

#include "../../../include/cppqtgraph/functions_qimage.hpp"

#include <QtCore/QtGlobal>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace cppqtgraph::graphicsItems {
namespace {

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> contiguousRank2(const std::vector<T>& storage,
                                                          const std::array<std::size_t, 3>& shape)
{
    return core::ArrayView<const T, 2>(storage.data(), {shape[0], shape[1]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 3> contiguousRank3(const std::vector<T>& storage,
                                                          const std::array<std::size_t, 3>& shape)
{
    return core::ArrayView<const T, 3>(storage.data(), {shape[0], shape[1], shape[2]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> displayView(core::ArrayView<const T, 2> image, ImageItem::AxisOrder axisOrder)
{
    if (axisOrder == ImageItem::AxisOrder::RowMajor) {
        return image;
    }
    const auto& shape = image.shape();
    const auto& strides = image.strides();
    return core::ArrayView<const T, 2>(image.data(), {shape[1], shape[0]}, {strides[1], strides[0]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 3> displayView(core::ArrayView<const T, 3> image, ImageItem::AxisOrder axisOrder)
{
    if (axisOrder == ImageItem::AxisOrder::RowMajor) {
        return image;
    }
    const auto& shape = image.shape();
    const auto& strides = image.strides();
    return core::ArrayView<const T, 3>(image.data(), {shape[1], shape[0], shape[2]}, {strides[1], strides[0], strides[2]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> singleChannelView(core::ArrayView<const T, 3> image)
{
    const auto& shape = image.shape();
    const auto& strides = image.strides();
    return core::ArrayView<const T, 2>(image.data(), {shape[0], shape[1]}, {strides[0], strides[1]});
}

[[nodiscard]] std::array<std::size_t, 2> extentsForShape(const std::array<std::size_t, 3>& shape,
                                                         ImageItem::AxisOrder axisOrder) noexcept
{
    if (shape[0] == 0 || shape[1] == 0) {
        return {0, 0};
    }
    if (axisOrder == ImageItem::AxisOrder::ColMajor) {
        return {shape[0], shape[1]};
    }
    return {shape[1], shape[0]};
}

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

} // namespace

ImageItem::ImageItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
}

ImageItem::~ImageItem() = default;

void ImageItem::setImage(core::ArrayView<const std::uint8_t, 2> image)
{
    setImageImpl(image, DataKind::UInt8Rank2, uint8Data_);
}

void ImageItem::setImage(core::ArrayView<const std::uint8_t, 3> image)
{
    setImageImpl(image, DataKind::UInt8Rank3, uint8Data_);
}

void ImageItem::setImage(core::ArrayView<const std::uint16_t, 2> image)
{
    setImageImpl(image, DataKind::UInt16Rank2, uint16Data_);
}

void ImageItem::setImage(core::ArrayView<const std::uint16_t, 3> image)
{
    setImageImpl(image, DataKind::UInt16Rank3, uint16Data_);
}

void ImageItem::setImage(core::ArrayView<const float, 2> image)
{
    setImageImpl(image, DataKind::FloatRank2, floatData_);
}

template <typename T, std::size_t Rank>
void ImageItem::setImageImpl(core::ArrayView<const T, Rank> image, DataKind kind, std::vector<T>& destination)
{
    const auto oldExtents = extents();
    std::array<std::size_t, 3> newShape{};
    newShape[0] = image.shape()[0];
    newShape[1] = image.shape()[1];
    if constexpr (Rank == 3) {
        newShape[2] = image.shape()[2];
    } else {
        newShape[2] = 1;
    }

    if (oldExtents != extentsForShape(newShape, axisOrder_)) {
        prepareGeometryChange();
    }

    copyImage(image, destination);
    if constexpr (!std::is_same_v<T, std::uint8_t>) {
        uint8Data_.clear();
    }
    if constexpr (!std::is_same_v<T, std::uint16_t>) {
        uint16Data_.clear();
    }
    if constexpr (!std::is_same_v<T, float>) {
        floatData_.clear();
    }
    shape_ = newShape;
    dataKind_ = kind;
    markRenderRequired();
    update();
    emit sigImageChanged();
}

void ImageItem::clearImage()
{
    if (dataKind_ == DataKind::None) {
        return;
    }
    prepareGeometryChange();
    dataKind_ = DataKind::None;
    shape_ = {};
    uint8Data_.clear();
    uint16Data_.clear();
    floatData_.clear();
    qimage_ = QImage();
    markRenderRequired();
    update();
    emit sigImageChanged();
}

void ImageItem::setAxisOrder(AxisOrder axisOrder)
{
    if (axisOrder_ == axisOrder) {
        return;
    }
    const auto oldExtents = extents();
    axisOrder_ = axisOrder;
    if (oldExtents != extents()) {
        prepareGeometryChange();
    }
    markRenderRequired();
    update();
}

ImageItem::AxisOrder ImageItem::axisOrder() const noexcept
{
    return axisOrder_;
}

void ImageItem::setCompositionMode(QPainter::CompositionMode mode)
{
    compositionMode_ = mode;
    hasCompositionMode_ = true;
    update();
}

void ImageItem::clearCompositionMode()
{
    hasCompositionMode_ = false;
    update();
}

void ImageItem::setLevels(std::optional<ImageLevelRange> levels)
{
    levels_ = levels;
    if (levels.has_value()) {
        channelLevels_.reset();
    }
    markRenderRequired();
    update();
}

void ImageItem::setChannelLevels(const std::vector<ImageLevelRange>& levels)
{
    channelLevels_ = levels;
    levels_.reset();
    markRenderRequired();
    update();
}

void ImageItem::clearChannelLevels()
{
    if (!channelLevels_.has_value()) {
        return;
    }
    channelLevels_.reset();
    markRenderRequired();
    update();
}

std::optional<ImageLevelRange> ImageItem::getLevels() const noexcept
{
    return levels_;
}

std::optional<std::vector<ImageLevelRange>> ImageItem::getChannelLevels() const noexcept
{
    return channelLevels_;
}

std::pair<std::vector<double>, std::vector<double>> ImageItem::getHistogram(int channel) const
{
    std::vector<double> values;
    if (!hasImage()) {
        return {};
    }

    const std::size_t step = std::max<std::size_t>(1, std::max(shape_[0], shape_[1]) / 200);
    switch (dataKind_) {
    case DataKind::UInt8Rank2:
        for (std::size_t row = 0; row < shape_[0]; row += step) {
            for (std::size_t col = 0; col < shape_[1]; col += step) {
                values.push_back(static_cast<double>(uint8Data_[row * shape_[1] + col]));
            }
        }
        break;
    case DataKind::UInt8Rank3: {
        const std::size_t channels = shape_[2];
        if (channel < 0) {
            for (std::size_t row = 0; row < shape_[0]; row += step) {
                for (std::size_t col = 0; col < shape_[1]; col += step) {
                    for (std::size_t ch = 0; ch < channels; ++ch) {
                        values.push_back(static_cast<double>(
                            uint8Data_[(row * shape_[1] + col) * channels + ch]));
                    }
                }
            }
        } else if (static_cast<std::size_t>(channel) < channels) {
            for (std::size_t row = 0; row < shape_[0]; row += step) {
                for (std::size_t col = 0; col < shape_[1]; col += step) {
                    values.push_back(static_cast<double>(
                        uint8Data_[(row * shape_[1] + col) * channels + static_cast<std::size_t>(channel)]));
                }
            }
        }
        break;
    }
    case DataKind::UInt16Rank2:
        for (std::size_t row = 0; row < shape_[0]; row += step) {
            for (std::size_t col = 0; col < shape_[1]; col += step) {
                values.push_back(static_cast<double>(uint16Data_[row * shape_[1] + col]));
            }
        }
        break;
    case DataKind::UInt16Rank3: {
        const std::size_t channels = shape_[2];
        if (channel < 0) {
            for (std::size_t row = 0; row < shape_[0]; row += step) {
                for (std::size_t col = 0; col < shape_[1]; col += step) {
                    for (std::size_t ch = 0; ch < channels; ++ch) {
                        values.push_back(static_cast<double>(
                            uint16Data_[(row * shape_[1] + col) * channels + ch]));
                    }
                }
            }
        } else if (static_cast<std::size_t>(channel) < channels) {
            for (std::size_t row = 0; row < shape_[0]; row += step) {
                for (std::size_t col = 0; col < shape_[1]; col += step) {
                    values.push_back(static_cast<double>(
                        uint16Data_[(row * shape_[1] + col) * channels + static_cast<std::size_t>(channel)]));
                }
            }
        }
        break;
    }
    case DataKind::FloatRank2:
        for (std::size_t row = 0; row < shape_[0]; row += step) {
            for (std::size_t col = 0; col < shape_[1]; col += step) {
                values.push_back(static_cast<double>(floatData_[row * shape_[1] + col]));
            }
        }
        break;
    case DataKind::None:
        break;
    }

    if (values.empty()) {
        return {};
    }

    const auto minValue = *std::min_element(values.begin(), values.end());
    const auto maxValue = *std::max_element(values.begin(), values.end());
    constexpr std::size_t kBins = 256;
    const double span = maxValue - minValue;
    const double binWidth = span <= 0.0 ? 1.0 : span / static_cast<double>(kBins);

    std::vector<double> counts(kBins, 0.0);
    std::vector<double> edges(kBins);
    for (std::size_t index = 0; index < kBins; ++index) {
        edges[index] = minValue + static_cast<double>(index) * binWidth;
    }

    for (double value : values) {
        std::size_t bin = 0;
        if (span > 0.0) {
            bin = static_cast<std::size_t>((value - minValue) / span * static_cast<double>(kBins - 1));
            bin = std::min(bin, kBins - 1);
        }
        counts[bin] += 1.0;
    }

    return {edges, counts};
}

void ImageItem::setLookupTable(ImageLookupTable lut)
{
    if (lut.data == nullptr || lut.rows == 0) {
        throw std::invalid_argument("ImageItem lookup table must contain at least one row");
    }
    if (lut.channels != 1 && lut.channels != 3 && lut.channels != 4) {
        throw std::invalid_argument("ImageItem lookup table must have 1, 3, or 4 channels");
    }
    if (lut.rowStride <= 0 || lut.channelStride <= 0) {
        throw std::invalid_argument("ImageItem lookup table strides must be positive");
    }

    lookupTableRows_ = lut.rows;
    lookupTableChannels_ = lut.channels;
    lookupTableData_.resize(lut.rows * lut.channels);
    for (std::size_t row = 0; row < lut.rows; ++row) {
        const auto* sourceRow = lut.data + static_cast<std::ptrdiff_t>(row) * lut.rowStride;
        for (std::size_t channel = 0; channel < lut.channels; ++channel) {
            lookupTableData_[row * lut.channels + channel] = sourceRow[static_cast<std::ptrdiff_t>(channel) * lut.channelStride];
        }
    }

    markRenderRequired();
    update();
}

void ImageItem::clearLookupTable()
{
    if (lookupTableData_.empty()) {
        return;
    }
    lookupTableData_.clear();
    lookupTableRows_ = 0;
    lookupTableChannels_ = 0;
    markRenderRequired();
    update();
}

std::optional<ImageLookupTable> ImageItem::lookupTable() const noexcept
{
    return lookupTableView();
}

bool ImageItem::hasImage() const noexcept
{
    return dataKind_ != DataKind::None;
}

std::size_t ImageItem::width() const noexcept
{
    return extents()[0];
}

std::size_t ImageItem::height() const noexcept
{
    return extents()[1];
}

std::size_t ImageItem::channels() const noexcept
{
    return hasImage() ? shape_[2] : 0;
}

bool ImageItem::renderRequired() const noexcept
{
    return renderRequired_;
}

bool ImageItem::isUnrenderable() const noexcept
{
    return unrenderable_;
}

const QImage& ImageItem::cachedImage() const noexcept
{
    return qimage_;
}

bool ImageItem::render()
{
    unrenderable_ = true;
    qimage_ = QImage();
    if (!hasImage() || shape_[0] == 0 || shape_[1] == 0 || shape_[2] == 0) {
        renderRequired_ = false;
        return false;
    }

    TryMakeQImageOptions scalarOptions;
    scalarOptions.levels = levels_;
    scalarOptions.lut = lookupTableView();

    TryMakeQImageOptions colorOptions;
    if (channelLevels_.has_value()) {
        colorOptions.channelLevels = channelLevels_;
    } else {
        colorOptions.levels = levels_;
    }

    std::optional<QImage> rendered;
    switch (dataKind_) {
    case DataKind::UInt8Rank2:
        rendered = cppqtgraph::tryMakeQImage(displayView(contiguousRank2(uint8Data_, shape_), axisOrder_), scalarOptions);
        break;
    case DataKind::UInt8Rank3: {
        const auto input = contiguousRank3(uint8Data_, shape_);
        if (shape_[2] == 1) {
            rendered = cppqtgraph::tryMakeQImage(displayView(singleChannelView(input), axisOrder_), scalarOptions);
        } else {
            rendered = (levels_.has_value() || channelLevels_.has_value())
                             ? cppqtgraph::tryMakeQImage(displayView(input, axisOrder_), colorOptions)
                             : cppqtgraph::tryMakeQImage(displayView(input, axisOrder_));
        }
        break;
    }
    case DataKind::UInt16Rank2:
        rendered = cppqtgraph::tryMakeQImage(displayView(contiguousRank2(uint16Data_, shape_), axisOrder_), scalarOptions);
        break;
    case DataKind::UInt16Rank3: {
        const auto input = contiguousRank3(uint16Data_, shape_);
        if (shape_[2] == 1) {
            rendered = cppqtgraph::tryMakeQImage(displayView(singleChannelView(input), axisOrder_), scalarOptions);
        } else {
            rendered = cppqtgraph::tryMakeQImage(displayView(input, axisOrder_));
        }
        break;
    }
    case DataKind::FloatRank2:
        rendered = cppqtgraph::tryMakeQImage(displayView(contiguousRank2(floatData_, shape_), axisOrder_), scalarOptions);
        break;
    case DataKind::None:
        break;
    }

    if (!rendered.has_value() || rendered->isNull()) {
        renderRequired_ = false;
        return false;
    }

    qimage_ = *rendered;
    renderRequired_ = false;
    unrenderable_ = false;
    return true;
}

QRectF ImageItem::boundingRect() const
{
    const auto [itemWidth, itemHeight] = extents();
    return QRectF(0.0, 0.0, static_cast<qreal>(itemWidth), static_cast<qreal>(itemHeight));
}

void ImageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (painter == nullptr || !hasImage()) {
        return;
    }
    if (renderRequired_ && !render()) {
        return;
    }
    if (unrenderable_ || qimage_.isNull()) {
        return;
    }
    if (hasCompositionMode_) {
        painter->setCompositionMode(compositionMode_);
    }
    painter->drawImage(QRectF(0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height())), qimage_);
}

std::array<std::size_t, 2> ImageItem::extents() const noexcept
{
    if (!hasImage()) {
        return {0, 0};
    }
    return extentsForShape(shape_, axisOrder_);
}

std::optional<ImageLookupTable> ImageItem::lookupTableView() const noexcept
{
    if (lookupTableData_.empty()) {
        return std::nullopt;
    }
    return ImageLookupTable{lookupTableData_.data(),
                            lookupTableRows_,
                            lookupTableChannels_,
                            static_cast<std::ptrdiff_t>(lookupTableChannels_),
                            1};
}

void ImageItem::markRenderRequired()
{
    qimage_ = QImage();
    renderRequired_ = true;
    unrenderable_ = false;
}

} // namespace cppqtgraph::graphicsItems
