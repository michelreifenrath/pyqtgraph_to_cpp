// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/RawImageWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/RawImageWidget.hpp"

#include "../../../include/pyqtgraph/functions_qimage.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace pyqtgraph::widgets {
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
[[nodiscard]] core::ArrayView<const T, 2> displayView(core::ArrayView<const T, 2> image, RawImageWidget::AxisOrder axisOrder)
{
    if (axisOrder == RawImageWidget::AxisOrder::RowMajor) {
        return image;
    }
    const auto& shape = image.shape();
    const auto& strides = image.strides();
    return core::ArrayView<const T, 2>(image.data(), {shape[1], shape[0]}, {strides[1], strides[0]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 3> displayView(core::ArrayView<const T, 3> image, RawImageWidget::AxisOrder axisOrder)
{
    if (axisOrder == RawImageWidget::AxisOrder::RowMajor) {
        return image;
    }
    const auto& shape = image.shape();
    const auto& strides = image.strides();
    return core::ArrayView<const T, 3>(image.data(), {shape[1], shape[0], shape[2]}, {strides[1], strides[0], strides[2]});
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

[[nodiscard]] TryMakeQImageOptions makeTryOptions(const std::optional<ImageLevelRange>& levels,
                                                  const std::optional<ImageLookupTable>& lut)
{
    TryMakeQImageOptions options;
    options.levels = levels;
    options.lut = lut;
    return options;
}

} // namespace

RawImageWidget::RawImageWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(32, 32);
}

RawImageWidget::~RawImageWidget() = default;

void RawImageWidget::setImage(core::ArrayView<const std::uint8_t, 2> image)
{
    setImageImpl(image, DataKind::UInt8Rank2, uint8Data_);
}

void RawImageWidget::setImage(core::ArrayView<const std::uint8_t, 3> image)
{
    setImageImpl(image, DataKind::UInt8Rank3, uint8Data_);
}

void RawImageWidget::setImage(core::ArrayView<const std::uint16_t, 2> image)
{
    setImageImpl(image, DataKind::UInt16Rank2, uint16Data_);
}

void RawImageWidget::setImage(core::ArrayView<const std::uint16_t, 3> image)
{
    setImageImpl(image, DataKind::UInt16Rank3, uint16Data_);
}

void RawImageWidget::setImage(core::ArrayView<const float, 2> image)
{
    setImageImpl(image, DataKind::FloatRank2, floatData_);
}

template <typename T, std::size_t Rank>
void RawImageWidget::setImageImpl(core::ArrayView<const T, Rank> image, DataKind kind, std::vector<T>& destination)
{
    if (image.empty()) {
        clearImage();
        return;
    }

    copyImage(image, destination);
    dataKind_ = kind;
    shape_[0] = image.shape()[0];
    shape_[1] = image.shape()[1];
    if constexpr (Rank == 3) {
        shape_[2] = image.shape()[2];
    } else {
        shape_[2] = 1;
    }

    uint16Data_.clear();
    floatData_.clear();
    if constexpr (std::is_same_v<T, std::uint16_t>) {
        uint8Data_.clear();
        floatData_.clear();
    } else if constexpr (std::is_same_v<T, float>) {
        uint8Data_.clear();
        uint16Data_.clear();
    } else {
        uint16Data_.clear();
        floatData_.clear();
    }

    invalidateCache();
    update();
}

void RawImageWidget::clearImage()
{
    dataKind_ = DataKind::None;
    shape_ = {};
    uint8Data_.clear();
    uint16Data_.clear();
    floatData_.clear();
    invalidateCache();
    update();
}

void RawImageWidget::setAxisOrder(AxisOrder axisOrder)
{
    if (axisOrder_ == axisOrder) {
        return;
    }
    axisOrder_ = axisOrder;
    invalidateCache();
    update();
}

RawImageWidget::AxisOrder RawImageWidget::axisOrder() const noexcept
{
    return axisOrder_;
}

void RawImageWidget::setLevels(std::optional<ImageLevelRange> levels)
{
    levels_ = levels;
    invalidateCache();
    update();
}

std::optional<ImageLevelRange> RawImageWidget::levels() const noexcept
{
    return levels_;
}

void RawImageWidget::setLookupTable(ImageLookupTable lut)
{
    if (lut.data == nullptr || lut.rows == 0 || lut.channels == 0) {
        clearLookupTable();
        return;
    }

    const std::size_t bytes = lut.rows * static_cast<std::size_t>(lut.rowStride);
    lookupTableData_.assign(lut.data, lut.data + bytes);
    lookupTableRows_ = lut.rows;
    lookupTableChannels_ = lut.channels;
    invalidateCache();
    update();
}

void RawImageWidget::clearLookupTable()
{
    lookupTableData_.clear();
    lookupTableRows_ = 0;
    lookupTableChannels_ = 0;
    invalidateCache();
    update();
}

std::optional<ImageLookupTable> RawImageWidget::lookupTable() const noexcept
{
    return lookupTableView();
}

void RawImageWidget::setScaled(bool scaled)
{
    scaled_ = scaled;
    update();
}

bool RawImageWidget::scaled() const noexcept
{
    return scaled_;
}

bool RawImageWidget::hasImage() const noexcept
{
    return dataKind_ != DataKind::None;
}

std::size_t RawImageWidget::width() const noexcept
{
    if (!hasImage()) {
        return 0;
    }
    return axisOrder_ == AxisOrder::ColMajor ? shape_[0] : shape_[1];
}

std::size_t RawImageWidget::height() const noexcept
{
    if (!hasImage()) {
        return 0;
    }
    return axisOrder_ == AxisOrder::ColMajor ? shape_[1] : shape_[0];
}

const QImage& RawImageWidget::cachedImage() const
{
    ensureCachedImage();
    return cachedImage_;
}

void RawImageWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!hasImage()) {
        return;
    }

    ensureCachedImage();
    if (cachedImage_.isNull()) {
        return;
    }

    QPainter painter(this);
    if (scaled_) {
        painter.drawImage(rect(), cachedImage_);
    } else {
        painter.drawImage(QPoint{0, 0}, cachedImage_);
    }
}

void RawImageWidget::invalidateCache()
{
    cacheValid_ = false;
    cachedImage_ = QImage{};
}

void RawImageWidget::ensureCachedImage() const
{
    if (cacheValid_) {
        return;
    }

    const auto lut = lookupTableView();
    const TryMakeQImageOptions options = makeTryOptions(levels_, lut);

    switch (dataKind_) {
    case DataKind::UInt8Rank2: {
        auto view = displayView(contiguousRank2(uint8Data_, shape_), axisOrder_);
        const std::optional<QImage> image = tryMakeQImage(view, options);
        cachedImage_ = image.value_or(QImage{});
        break;
    }
    case DataKind::UInt8Rank3: {
        auto view = displayView(contiguousRank3(uint8Data_, shape_), axisOrder_);
        if (options.levels.has_value() && !options.lut.has_value()) {
            cachedImage_ = tryMakeQImage(view, options).value_or(QImage{});
        } else {
            cachedImage_ = tryMakeQImage(view).value_or(QImage{});
        }
        break;
    }
    case DataKind::UInt16Rank2: {
        auto view = displayView(contiguousRank2(uint16Data_, shape_), axisOrder_);
        const std::optional<QImage> image = tryMakeQImage(view, options);
        cachedImage_ = image.value_or(QImage{});
        break;
    }
    case DataKind::UInt16Rank3: {
        auto view = displayView(contiguousRank3(uint16Data_, shape_), axisOrder_);
        cachedImage_ = tryMakeQImage(view).value_or(QImage{});
        break;
    }
    case DataKind::FloatRank2: {
        auto view = displayView(contiguousRank2(floatData_, shape_), axisOrder_);
        const std::optional<QImage> image = tryMakeQImage(view, options);
        cachedImage_ = image.value_or(QImage{});
        break;
    }
    case DataKind::None:
        cachedImage_ = QImage{};
        break;
    }

    cacheValid_ = true;
}

std::optional<ImageLookupTable> RawImageWidget::lookupTableView() const noexcept
{
    if (lookupTableData_.empty() || lookupTableRows_ == 0 || lookupTableChannels_ == 0) {
        return std::nullopt;
    }
    return ImageLookupTable{
        lookupTableData_.data(),
        lookupTableRows_,
        lookupTableChannels_,
        static_cast<std::ptrdiff_t>(lookupTableChannels_),
        1,
    };
}

} // namespace pyqtgraph::widgets
