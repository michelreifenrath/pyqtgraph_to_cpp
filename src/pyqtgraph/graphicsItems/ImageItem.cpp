// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ImageItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/ImageItem.hpp"

#include "../../../include/pyqtgraph/functions_qimage.hpp"

#include <QtCore/QtGlobal>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace pyqtgraph::graphicsItems {
namespace {

template <typename T, std::size_t RankValue>
std::vector<T> copyImageData(core::ArrayView<const T, RankValue> image)
{
    std::vector<T> copied(image.size());
    if constexpr (RankValue == 2) {
        const auto& shape = image.shape();
        for (std::size_t row = 0; row < shape[0]; ++row) {
            for (std::size_t column = 0; column < shape[1]; ++column) {
                copied[row * shape[1] + column] = image(row, column);
            }
        }
    } else {
        const auto& shape = image.shape();
        for (std::size_t row = 0; row < shape[0]; ++row) {
            for (std::size_t column = 0; column < shape[1]; ++column) {
                for (std::size_t channel = 0; channel < shape[2]; ++channel) {
                    copied[(row * shape[1] + column) * shape[2] + channel] = image(row, column, channel);
                }
            }
        }
    }
    return copied;
}

template <typename T>
core::ArrayView<const T, 2> rowMajor2DView(const std::vector<T>& storage, std::size_t rows, std::size_t columns)
{
    return core::ArrayView<const T, 2>(storage.data(), {rows, columns}, {static_cast<std::ptrdiff_t>(columns), 1});
}

template <typename T>
core::ArrayView<const T, 2> colMajorDisplay2DView(const std::vector<T>& storage, std::size_t rows, std::size_t columns)
{
    return core::ArrayView<const T, 2>(storage.data(), {columns, rows}, {1, static_cast<std::ptrdiff_t>(columns)});
}

core::ArrayView<const std::uint8_t, 3> rowMajor3DView(const std::vector<std::uint8_t>& storage,
                                                      std::size_t rows,
                                                      std::size_t columns,
                                                      std::size_t channels)
{
    return core::ArrayView<const std::uint8_t, 3>(storage.data(), {rows, columns, channels},
        {static_cast<std::ptrdiff_t>(columns * channels), static_cast<std::ptrdiff_t>(channels), 1});
}

core::ArrayView<const std::uint8_t, 3> colMajorDisplay3DView(const std::vector<std::uint8_t>& storage,
                                                            std::size_t rows,
                                                            std::size_t columns,
                                                            std::size_t channels)
{
    return core::ArrayView<const std::uint8_t, 3>(storage.data(), {columns, rows, channels},
        {static_cast<std::ptrdiff_t>(channels), static_cast<std::ptrdiff_t>(columns * channels), 1});
}

bool fitsQImageExtent(std::size_t extent)
{
    return extent <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

} // namespace

ImageItem::ImageItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
{
}

ImageItem::~ImageItem() = default;

void ImageItem::setAxisOrder(AxisOrder axisOrder)
{
    if (axisOrder_ == axisOrder) {
        return;
    }
    const int oldWidth = width();
    const int oldHeight = height();
    axisOrder_ = axisOrder;
    if (oldWidth != width() || oldHeight != height()) {
        prepareGeometryChange();
    }
    markRenderRequired();
}

ImageItem::AxisOrder ImageItem::axisOrder() const noexcept
{
    return axisOrder_;
}

void ImageItem::setLevels(std::optional<pyqtgraph::ImageLevelRange> levels)
{
    levels_ = levels;
    markRenderRequired();
}

std::optional<pyqtgraph::ImageLevelRange> ImageItem::levels() const noexcept
{
    return levels_;
}

void ImageItem::setLookupTable(std::optional<pyqtgraph::ImageLookupTable> lut)
{
    lut_ = lut;
    markRenderRequired();
}

std::optional<pyqtgraph::ImageLookupTable> ImageItem::lookupTable() const noexcept
{
    return lut_;
}

void ImageItem::setImage(pyqtgraph::core::ArrayView<const std::uint8_t, 2> image)
{
    setImageImpl(image, DataType::UInt8, Rank::Two);
}

void ImageItem::setImage(pyqtgraph::core::ArrayView<const std::uint8_t, 3> image)
{
    const auto& shape = image.shape();
    if (shape[2] != 1 && shape[2] != 3 && shape[2] != 4) {
        throw std::invalid_argument("ImageItem::setImage expects rank-3 uint8 image data to have 1, 3, or 4 channels");
    }
    setImageImpl(image, DataType::UInt8, Rank::Three);
}

void ImageItem::setImage(pyqtgraph::core::ArrayView<const std::uint16_t, 2> image)
{
    setImageImpl(image, DataType::UInt16, Rank::Two);
}

void ImageItem::setImage(pyqtgraph::core::ArrayView<const float, 2> image)
{
    setImageImpl(image, DataType::Float32, Rank::Two);
}

void ImageItem::clear()
{
    if (dataType_ != DataType::None) {
        prepareGeometryChange();
    }
    uint8Storage_.clear();
    uint16Storage_.clear();
    floatStorage_.clear();
    shape0_ = 0;
    shape1_ = 0;
    channels_ = 1;
    dataType_ = DataType::None;
    rank_ = Rank::None;
    qimage_ = QImage();
    renderRequired_ = true;
    unrenderable_ = false;
    update();
}

bool ImageItem::render()
{
    unrenderable_ = true;
    qimage_ = QImage();
    if (dataType_ == DataType::None || shape0_ == 0 || shape1_ == 0) {
        return false;
    }

    TryMakeQImageOptions options;
    options.levels = levels_;
    if (rank_ == Rank::Two || channels_ == 1) {
        options.lut = lut_;
    }

    std::optional<QImage> rendered;
    if (dataType_ == DataType::UInt8 && rank_ == Rank::Two) {
        const auto view = axisOrder_ == AxisOrder::ColMajor
            ? colMajorDisplay2DView(uint8Storage_, shape0_, shape1_)
            : rowMajor2DView(uint8Storage_, shape0_, shape1_);
        rendered = tryMakeQImage(view, options);
    } else if (dataType_ == DataType::UInt8 && rank_ == Rank::Three && channels_ == 1) {
        const auto view = axisOrder_ == AxisOrder::ColMajor
            ? colMajorDisplay3DView(uint8Storage_, shape0_, shape1_, channels_)
            : rowMajor3DView(uint8Storage_, shape0_, shape1_, channels_);
        const auto shape = view.shape();
        const core::ArrayView<const std::uint8_t, 2> singleChannel(view.data(), {shape[0], shape[1]},
            {view.strides()[0], view.strides()[1]});
        rendered = tryMakeQImage(singleChannel, options);
    } else if (dataType_ == DataType::UInt8 && rank_ == Rank::Three) {
        const auto view = axisOrder_ == AxisOrder::ColMajor
            ? colMajorDisplay3DView(uint8Storage_, shape0_, shape1_, channels_)
            : rowMajor3DView(uint8Storage_, shape0_, shape1_, channels_);
        rendered = tryMakeQImage(view, options);
    } else if (dataType_ == DataType::UInt16) {
        const auto view = axisOrder_ == AxisOrder::ColMajor
            ? colMajorDisplay2DView(uint16Storage_, shape0_, shape1_)
            : rowMajor2DView(uint16Storage_, shape0_, shape1_);
        rendered = tryMakeQImage(view, options);
    } else if (dataType_ == DataType::Float32) {
        const auto view = axisOrder_ == AxisOrder::ColMajor
            ? colMajorDisplay2DView(floatStorage_, shape0_, shape1_)
            : rowMajor2DView(floatStorage_, shape0_, shape1_);
        rendered = tryMakeQImage(view, options);
    }

    if (!rendered.has_value()) {
        renderRequired_ = true;
        return false;
    }

    qimage_ = *rendered;
    renderRequired_ = false;
    unrenderable_ = false;
    return true;
}

const QImage& ImageItem::qimage() const noexcept
{
    return qimage_;
}

int ImageItem::width() const noexcept
{
    if (dataType_ == DataType::None) {
        return 0;
    }
    const std::size_t width = axisOrder_ == AxisOrder::ColMajor ? shape0_ : shape1_;
    return fitsQImageExtent(width) ? static_cast<int>(width) : 0;
}

int ImageItem::height() const noexcept
{
    if (dataType_ == DataType::None) {
        return 0;
    }
    const std::size_t height = axisOrder_ == AxisOrder::ColMajor ? shape1_ : shape0_;
    return fitsQImageExtent(height) ? static_cast<int>(height) : 0;
}

QRectF ImageItem::boundingRect() const
{
    if (dataType_ == DataType::None) {
        return QRectF(0.0, 0.0, 0.0, 0.0);
    }
    return QRectF(0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height()));
}

void ImageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (dataType_ == DataType::None) {
        return;
    }
    if (renderRequired_ && !render()) {
        return;
    }
    if (unrenderable_ || qimage_.isNull()) {
        return;
    }

    painter->drawImage(QRectF(0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height())), qimage_);
}

template <typename T, std::size_t RankValue>
void ImageItem::setImageImpl(pyqtgraph::core::ArrayView<const T, RankValue> image, DataType dataType, Rank rank)
{
    const auto& shape = image.shape();
    if (image.data() == nullptr && image.size() != 0) {
        throw std::invalid_argument("ImageItem::setImage image data must not be null");
    }
    if (shape[0] == 0 || shape[1] == 0 || !fitsQImageExtent(shape[0]) || !fitsQImageExtent(shape[1])) {
        clear();
        return;
    }

    const std::size_t newChannels = RankValue == 3 ? shape[2] : 1;
    if (RankValue == 3 && newChannels == 0) {
        clear();
        return;
    }
    const int newWidth = axisOrder_ == AxisOrder::ColMajor ? static_cast<int>(shape[0]) : static_cast<int>(shape[1]);
    const int newHeight = axisOrder_ == AxisOrder::ColMajor ? static_cast<int>(shape[1]) : static_cast<int>(shape[0]);
    geometryMayChange(newWidth, newHeight);

    uint8Storage_.clear();
    uint16Storage_.clear();
    floatStorage_.clear();
    if constexpr (std::is_same_v<T, std::uint8_t>) {
        uint8Storage_ = copyImageData(image);
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        uint16Storage_ = copyImageData(image);
    } else if constexpr (std::is_same_v<T, float>) {
        floatStorage_ = copyImageData(image);
    }

    shape0_ = shape[0];
    shape1_ = shape[1];
    channels_ = newChannels;
    dataType_ = dataType;
    rank_ = rank;
    markRenderRequired();
}

void ImageItem::geometryMayChange(int newWidth, int newHeight)
{
    if (newWidth != width() || newHeight != height()) {
        prepareGeometryChange();
    }
}

void ImageItem::markRenderRequired()
{
    renderRequired_ = true;
    unrenderable_ = false;
    update();
}

} // namespace pyqtgraph::graphicsItems
