// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/functions_qimage.hpp"

#include <QColor>
#include <QImage>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pyqtgraph {
namespace {

void validateCommon(const std::uint8_t* data, std::size_t firstExtent, std::size_t secondExtent)
{
    if (data == nullptr) {
        throw std::invalid_argument("makeQImage image data must not be null");
    }
    if (firstExtent == 0 || secondExtent == 0) {
        throw std::invalid_argument("makeQImage image dimensions must be non-zero");
    }
    if (firstExtent > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || secondExtent > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("makeQImage image dimensions exceed QImage limits");
    }
}

[[nodiscard]] bool usesAlpha(std::size_t channels, const MakeQImageOptions& options)
{
    if (options.alpha.has_value()) {
        return *options.alpha;
    }
    return channels == 4;
}

[[nodiscard]] QImage::Format formatForChannels(std::size_t channels, const MakeQImageOptions& options)
{
    switch (channels) {
    case 3:
        return usesAlpha(channels, options) ? QImage::Format_ARGB32 : QImage::Format_RGB32;
    case 4:
        return usesAlpha(channels, options) ? QImage::Format_ARGB32 : QImage::Format_RGB32;
    default:
        throw std::invalid_argument("makeQImage expects 3 or 4 channels for rank-3 image data");
    }
}

void setPixel(QImage& image, int x, int y, std::uint8_t gray)
{
    image.setPixelColor(x, y, QColor(gray, gray, gray, 255));
}

void setPixelFromBgra(QImage& image,
                      int x,
                      int y,
                      std::uint8_t blue,
                      std::uint8_t green,
                      std::uint8_t red,
                      std::uint8_t alpha)
{
    image.setPixelColor(x, y, QColor(red, green, blue, alpha));
}

[[nodiscard]] QImage wrapQImageData(const std::uint8_t* data,
                                    int width,
                                    int height,
                                    std::ptrdiff_t bytesPerLine,
                                    QImage::Format format)
{
    if (bytesPerLine <= 0 || bytesPerLine > std::numeric_limits<qsizetype>::max()) {
        throw std::invalid_argument("makeQImage copy=false requires a positive QImage-compatible row stride");
    }

    QImage image(reinterpret_cast<const uchar*>(data), width, height, static_cast<qsizetype>(bytesPerLine), format);
    if (image.isNull()) {
        throw std::invalid_argument("makeQImage could not wrap input image data");
    }
    return image;
}

[[nodiscard]] QImage makeSharedQImage(core::ArrayView<const std::uint8_t, 2> imageData,
                                      int width,
                                      int height,
                                      bool transpose)
{
    const auto& strides = imageData.strides();
    const std::ptrdiff_t rowStride = transpose ? strides[1] : strides[0];
    const std::ptrdiff_t pixelStride = transpose ? strides[0] : strides[1];

    if (pixelStride != 1 || rowStride != static_cast<std::ptrdiff_t>(width)) {
        throw std::invalid_argument("makeQImage copy=false requires QImage-compatible contiguous grayscale input");
    }

    return wrapQImageData(imageData.data(), width, height, rowStride, QImage::Format_Grayscale8);
}

[[nodiscard]] QImage makeSharedQImage(core::ArrayView<const std::uint8_t, 3> imageData,
                                      int width,
                                      int height,
                                      bool transpose,
                                      QImage::Format format)
{
    if constexpr (std::endian::native != std::endian::little) {
        throw std::invalid_argument("makeQImage copy=false color input requires little-endian BGRA storage");
    }

    const auto& shape = imageData.shape();
    if (shape[2] != 4) {
        throw std::invalid_argument("makeQImage copy=false requires four-channel BGRA input");
    }

    const auto& strides = imageData.strides();
    const std::ptrdiff_t rowStride = transpose ? strides[1] : strides[0];
    const std::ptrdiff_t pixelStride = transpose ? strides[0] : strides[1];
    const std::ptrdiff_t channelStride = strides[2];
    const std::ptrdiff_t minimumRowStride = static_cast<std::ptrdiff_t>(width) * 4;

    if (channelStride != 1 || pixelStride != 4 || rowStride != minimumRowStride) {
        throw std::invalid_argument("makeQImage copy=false requires QImage-compatible contiguous BGRA input");
    }

    return wrapQImageData(imageData.data(), width, height, rowStride, format);
}

} // namespace

QImage makeQImage(core::ArrayView<const std::uint8_t, 2> imageData, const MakeQImageOptions& options)
{
    const auto& shape = imageData.shape();
    validateCommon(imageData.data(), shape[0], shape[1]);

    const int width = static_cast<int>(options.transpose ? shape[0] : shape[1]);
    const int height = static_cast<int>(options.transpose ? shape[1] : shape[0]);

    if (!options.copy) {
        return makeSharedQImage(imageData, width, height, options.transpose);
    }

    QImage image(width, height, QImage::Format_Grayscale8);
    if (image.isNull()) {
        throw std::invalid_argument("makeQImage could not allocate output image");
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::uint8_t value = options.transpose
                ? imageData(static_cast<std::size_t>(x), static_cast<std::size_t>(y))
                : imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x));
            setPixel(image, x, y, value);
        }
    }

    return image;
}

QImage makeQImage(core::ArrayView<const std::uint8_t, 3> imageData, const MakeQImageOptions& options)
{
    const auto& shape = imageData.shape();
    validateCommon(imageData.data(), shape[0], shape[1]);
    const std::size_t channels = shape[2];
    const QImage::Format format = formatForChannels(channels, options);
    const bool includeAlpha = usesAlpha(channels, options);

    const int width = static_cast<int>(options.transpose ? shape[0] : shape[1]);
    const int height = static_cast<int>(options.transpose ? shape[1] : shape[0]);

    if (!options.copy) {
        return makeSharedQImage(imageData, width, height, options.transpose, format);
    }

    QImage image(width, height, format);
    if (image.isNull()) {
        throw std::invalid_argument("makeQImage could not allocate output image");
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t first = options.transpose ? static_cast<std::size_t>(x) : static_cast<std::size_t>(y);
            const std::size_t second = options.transpose ? static_cast<std::size_t>(y) : static_cast<std::size_t>(x);
            const std::uint8_t alpha = includeAlpha && channels == 4 ? imageData(first, second, 3) : 255;
            setPixelFromBgra(image,
                             x,
                             y,
                             imageData(first, second, 0),
                             imageData(first, second, 1),
                             imageData(first, second, 2),
                             alpha);
        }
    }

    return image;
}

} // namespace pyqtgraph
