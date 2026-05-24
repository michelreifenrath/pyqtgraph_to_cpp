// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/functions_qimage.hpp"

#include <QColor>

#include <cstddef>
#include <cstdint>
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

} // namespace

QImage makeQImage(core::ArrayView<const std::uint8_t, 2> imageData, const MakeQImageOptions& options)
{
    const auto& shape = imageData.shape();
    validateCommon(imageData.data(), shape[0], shape[1]);

    const int width = static_cast<int>(options.transpose ? shape[0] : shape[1]);
    const int height = static_cast<int>(options.transpose ? shape[1] : shape[0]);

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
