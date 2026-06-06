// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py and
// pyqtgraph/functions_qimage.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/functions_qimage.hpp"

#include "../../include/pyqtgraph/functions.hpp"

#include <QColor>
#include <QImage>
#include <QVector>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
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

template <typename T, std::size_t Rank>
[[nodiscard]] bool hasTryMakeData(const core::ArrayView<const T, Rank>& imageData)
{
    if (imageData.data() == nullptr) {
        return false;
    }
    const std::size_t height = imageData.shape()[0];
    const std::size_t width = imageData.shape()[1];
    if (height == 0 || width == 0) {
        return false;
    }
    return height <= static_cast<std::size_t>(std::numeric_limits<int>::max())
        && width <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

[[nodiscard]] QImage allocateQImage(int width, int height, QImage::Format format)
{
    QImage image(width, height, format);
    if (image.isNull()) {
        throw std::invalid_argument("tryMakeQImage could not allocate output image");
    }
    return image;
}

[[nodiscard]] bool hasLevelsOrLut(const TryMakeQImageOptions& options)
{
    return options.levels.has_value() || options.lut.has_value();
}

[[nodiscard]] bool isValidLut(const LookupTableView& lut)
{
    if (lut.data == nullptr || lut.rows == 0) {
        return false;
    }
    if (lut.channels != 1 && lut.channels != 3 && lut.channels != 4) {
        return false;
    }
    if (lut.channelStride <= 0) {
        return false;
    }
    const std::ptrdiff_t defaultRowStride = static_cast<std::ptrdiff_t>(lut.channels) * lut.channelStride;
    const std::ptrdiff_t rowStride = lut.rowStride == 0 ? defaultRowStride : lut.rowStride;
    return rowStride > 0;
}

[[nodiscard]] std::ptrdiff_t lutRowStride(const LookupTableView& lut)
{
    return lut.rowStride == 0 ? static_cast<std::ptrdiff_t>(lut.channels) * lut.channelStride : lut.rowStride;
}

[[nodiscard]] std::uint8_t lutValue(const LookupTableView& lut, std::size_t row, std::size_t channel)
{
    const std::size_t sourceChannel = lut.channels == 1 ? 0 : channel;
    return lut.data[static_cast<std::ptrdiff_t>(row) * lutRowStride(lut)
                    + static_cast<std::ptrdiff_t>(sourceChannel) * lut.channelStride];
}

[[nodiscard]] QRgb lutColor(const LookupTableView& lut, std::size_t row)
{
    if (lut.channels == 1) {
        const std::uint8_t gray = lutValue(lut, row, 0);
        return qRgb(gray, gray, gray);
    }
    if (lut.channels == 3) {
        return qRgb(lutValue(lut, row, 0), lutValue(lut, row, 1), lutValue(lut, row, 2));
    }
    return qRgba(lutValue(lut, row, 0), lutValue(lut, row, 1), lutValue(lut, row, 2), lutValue(lut, row, 3));
}

[[nodiscard]] ImageLevels defaultLevels(const std::optional<ImageLevels>& levels, double maximum)
{
    return levels.value_or(ImageLevels{0.0, maximum});
}

[[nodiscard]] double levelRange(const ImageLevels& levels)
{
    const double range = levels.maximum - levels.minimum;
    return range == 0.0 ? 1.0 : range;
}

[[nodiscard]] QVector<QRgb> grayLevelsColorTable(const ImageLevels& levels)
{
    QVector<QRgb> table;
    table.reserve(256);
    const double scale = 255.0 / levelRange(levels);
    for (std::size_t value = 0; value < 256; ++value) {
        const std::uint8_t gray = detail::rescaleDataToUint8(static_cast<double>(value), scale, levels.minimum);
        table.append(qRgb(gray, gray, gray));
    }
    return table;
}

[[nodiscard]] QVector<QRgb> combinedLutColorTable(const ImageLevels& levels, const LookupTableView& lut)
{
    QVector<QRgb> table;
    table.reserve(256);
    const double scale = static_cast<double>(lut.rows) / levelRange(levels);
    for (std::size_t value = 0; value < 256; ++value) {
        const std::size_t row = detail::clippedLookupIndex(static_cast<double>(value), scale, levels.minimum, lut.rows);
        table.append(lutColor(lut, row));
    }
    return table;
}

[[nodiscard]] QImage makeIndexed8FromUint8(core::ArrayView<const std::uint8_t, 2> imageData, const QVector<QRgb>& table)
{
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Indexed8);
    image.setColorTable(table);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            row[x] = imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x));
        }
    }
    return image;
}

template <typename T>
[[nodiscard]] QImage makeGray8Rescaled(core::ArrayView<const T, 2> imageData, const ImageLevels& levels)
{
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Grayscale8);
    const double scale = 255.0 / levelRange(levels);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            row[x] = detail::rescaleDataToUint8(static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x))),
                                                scale,
                                                levels.minimum);
        }
    }
    return image;
}

template <typename T>
[[nodiscard]] QImage makeIndexed8Rescaled(core::ArrayView<const T, 2> imageData,
                                          const ImageLevels& levels,
                                          const LookupTableView& lut)
{
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Indexed8);
    QVector<QRgb> table;
    table.reserve(static_cast<qsizetype>(lut.rows));
    for (std::size_t row = 0; row < lut.rows; ++row) {
        table.append(lutColor(lut, row));
    }
    image.setColorTable(table);

    const double scale = static_cast<double>(lut.rows) / levelRange(levels);
    for (int y = 0; y < height; ++y) {
        uchar* out = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const std::size_t row = detail::clippedLookupIndex(
                static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x))),
                scale,
                levels.minimum,
                lut.rows);
            out[x] = static_cast<uchar>(row);
        }
    }
    return image;
}

[[nodiscard]] QImage allocateLutImage(int width, int height, const LookupTableView& lut)
{
    if (lut.channels == 1) {
        return allocateQImage(width, height, QImage::Format_Grayscale8);
    }
    if (lut.channels == 3) {
        return allocateQImage(width, height, QImage::Format_RGBX8888);
    }
    return allocateQImage(width, height, QImage::Format_RGBA8888);
}

template <typename T>
[[nodiscard]] QImage makeDirectLutImage(core::ArrayView<const T, 2> imageData,
                                        const ImageLevels& levels,
                                        const LookupTableView& lut)
{
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateLutImage(width, height, lut);
    const double scale = static_cast<double>(lut.rows) / levelRange(levels);

    for (int y = 0; y < height; ++y) {
        uchar* out = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const std::size_t lutRow = detail::clippedLookupIndex(
                static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x))),
                scale,
                levels.minimum,
                lut.rows);
            if (lut.channels == 1) {
                out[x] = lutValue(lut, lutRow, 0);
            } else {
                const std::size_t base = static_cast<std::size_t>(x) * 4;
                out[base + 0] = lutValue(lut, lutRow, 0);
                out[base + 1] = lutValue(lut, lutRow, 1);
                out[base + 2] = lutValue(lut, lutRow, 2);
                out[base + 3] = lut.channels == 4 ? lutValue(lut, lutRow, 3) : 255;
            }
        }
    }
    return image;
}

template <typename T>
[[nodiscard]] std::optional<QImage> tryMakeMonoLeveled(core::ArrayView<const T, 2> imageData,
                                                       const TryMakeQImageOptions& options,
                                                       double dtypeMaximum)
{
    if (options.lut.has_value() && !isValidLut(*options.lut)) {
        return std::nullopt;
    }

    const ImageLevels levels = defaultLevels(options.levels, dtypeMaximum);
    if (!options.lut.has_value()) {
        return makeGray8Rescaled(imageData, levels);
    }

    const LookupTableView& lut = *options.lut;
    if (lut.rows <= 256) {
        return makeIndexed8Rescaled(imageData, levels, lut);
    }
    return makeDirectLutImage(imageData, levels, lut);
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

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 2> imageData, const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }

    if (options.lut.has_value() && !isValidLut(*options.lut)) {
        return std::nullopt;
    }

    if (options.lut.has_value()) {
        const ImageLevels levels = defaultLevels(options.levels, 255.0);
        return makeIndexed8FromUint8(imageData, combinedLutColorTable(levels, *options.lut));
    }

    if (options.levels.has_value()) {
        return makeIndexed8FromUint8(imageData, grayLevelsColorTable(*options.levels));
    }

    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            row[x] = imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x));
        }
    }

    return image;
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 3> imageData, const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }
    if (options.lut.has_value()) {
        return std::nullopt;
    }

    const auto& shape = imageData.shape();
    const std::size_t channels = shape[2];
    QImage::Format format = QImage::Format_Invalid;
    if (channels == 3) {
        format = QImage::Format_RGB888;
    } else if (channels == 4 && !options.levels.has_value()) {
        format = QImage::Format_RGBA8888;
    } else {
        return std::nullopt;
    }

    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, format);
    const ImageLevels levels = defaultLevels(options.levels, 255.0);
    const double scale = 255.0 / levelRange(levels);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const std::size_t rowIndex = static_cast<std::size_t>(y);
            const std::size_t colIndex = static_cast<std::size_t>(x);
            const std::size_t base = static_cast<std::size_t>(x) * channels;
            for (std::size_t channel = 0; channel < channels; ++channel) {
                const std::uint8_t value = options.levels.has_value()
                    ? detail::rescaleDataToUint8(static_cast<double>(imageData(rowIndex, colIndex, channel)), scale, levels.minimum)
                    : imageData(rowIndex, colIndex, channel);
                row[base + channel] = value;
            }
        }
    }

    return image;
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 2> imageData, const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }

    if (hasLevelsOrLut(options)) {
        return tryMakeMonoLeveled(imageData, options, 65535.0);
    }

    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image(width, height, QImage::Format_Grayscale16);
    if (image.isNull() || image.format() != QImage::Format_Grayscale16) {
        return std::nullopt;
    }

    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<std::uint16_t*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            row[x] = imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x));
        }
    }

    return image;
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 3> imageData, const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData) || hasLevelsOrLut(options)) {
        return std::nullopt;
    }

    const auto& shape = imageData.shape();
    if (shape[2] != 4) {
        return std::nullopt;
    }

    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image(width, height, QImage::Format_RGBA64);
    if (image.isNull() || image.format() != QImage::Format_RGBA64) {
        return std::nullopt;
    }

    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<std::uint16_t*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const std::size_t rowIndex = static_cast<std::size_t>(y);
            const std::size_t colIndex = static_cast<std::size_t>(x);
            const std::size_t base = static_cast<std::size_t>(x) * 4;
            row[base + 0] = imageData(rowIndex, colIndex, 0);
            row[base + 1] = imageData(rowIndex, colIndex, 1);
            row[base + 2] = imageData(rowIndex, colIndex, 2);
            row[base + 3] = imageData(rowIndex, colIndex, 3);
        }
    }

    return image;
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const float, 2> imageData, const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData) || !options.levels.has_value()) {
        return std::nullopt;
    }
    return tryMakeMonoLeveled(imageData, options, 1.0);
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const float, 3> imageData, const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData) || !options.levels.has_value() || options.lut.has_value()) {
        return std::nullopt;
    }

    const auto& shape = imageData.shape();
    if (shape[2] != 3) {
        return std::nullopt;
    }

    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_RGB888);
    const double scale = 255.0 / levelRange(*options.levels);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const std::size_t base = static_cast<std::size_t>(x) * 3;
            for (std::size_t channel = 0; channel < 3; ++channel) {
                row[base + channel] = detail::rescaleDataToUint8(
                    static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x), channel)),
                    scale,
                    options.levels->minimum);
            }
        }
    }
    return image;
}

} // namespace pyqtgraph
