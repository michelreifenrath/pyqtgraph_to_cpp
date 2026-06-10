// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py and
// pyqtgraph/functions_qimage.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/functions_qimage.hpp"

#include <QColor>
#include <QImage>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace cppqtgraph {
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

void validateLookupTable(const ImageLookupTable& lut)
{
    if (lut.data == nullptr || lut.rows == 0) {
        throw std::invalid_argument("tryMakeQImage lookup table must contain at least one row");
    }
    if (lut.channels != 1 && lut.channels != 3 && lut.channels != 4) {
        throw std::invalid_argument("tryMakeQImage lookup table must have 1, 3, or 4 channels");
    }
    if (lut.rowStride <= 0 || lut.channelStride <= 0) {
        throw std::invalid_argument("tryMakeQImage lookup table strides must be positive");
    }
}

[[nodiscard]] double levelDifference(ImageLevelRange levels)
{
    const double difference = levels.maximum - levels.minimum;
    return difference == 0.0 ? 1.0 : difference;
}

[[nodiscard]] std::uint8_t scaledGray(double value, ImageLevelRange levels)
{
    return rescaleDataToUInt8(value, 255.0 / levelDifference(levels), levels.minimum);
}

[[nodiscard]] std::size_t scaledLookupIndex(double value, ImageLevelRange levels, const ImageLookupTable& lut)
{
    return rescaleDataIndex(value, static_cast<double>(lut.rows) / levelDifference(levels), levels.minimum, lut.rows - 1);
}

[[nodiscard]] std::array<std::uint8_t, 4> lutColorFor(double value, ImageLevelRange levels, const ImageLookupTable& lut)
{
    return applyLookupTable(static_cast<std::int64_t>(scaledLookupIndex(value, levels, lut)), lut);
}

[[nodiscard]] QRgb qrbgFromColor(std::array<std::uint8_t, 4> color)
{
    return qRgba(color[0], color[1], color[2], color[3]);
}

[[nodiscard]] std::vector<QRgb> makeIndexedColorTable(const TryMakeQImageOptions& options, std::size_t entries)
{
    std::vector<QRgb> table(entries);
    const ImageLevelRange levels = options.levels.value_or(ImageLevelRange{0.0, 255.0});
    if (options.lut.has_value()) {
        validateLookupTable(*options.lut);
        for (std::size_t index = 0; index < entries; ++index) {
            table[index] = qrbgFromColor(lutColorFor(static_cast<double>(index), levels, *options.lut));
        }
    } else {
        for (std::size_t index = 0; index < entries; ++index) {
            const std::uint8_t gray = scaledGray(static_cast<double>(index), levels);
            table[index] = qRgb(gray, gray, gray);
        }
    }
    return table;
}

void setColorTable(QImage& image, const std::vector<QRgb>& table)
{
    image.setColorCount(static_cast<int>(table.size()));
    for (std::size_t index = 0; index < table.size(); ++index) {
        image.setColor(static_cast<int>(index), table[index]);
    }
}

[[nodiscard]] QImage makeIndexed8FromUInt8(core::ArrayView<const std::uint8_t, 2> imageData,
                                           const TryMakeQImageOptions& options)
{
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Indexed8);
    setColorTable(image, makeIndexedColorTable(options, 256));

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            row[x] = imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x));
        }
    }
    return image;
}

template <typename T>
[[nodiscard]] QImage makeGrayscale8(core::ArrayView<const T, 2> imageData, ImageLevelRange levels)
{
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            row[x] = scaledGray(static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x))), levels);
        }
    }
    return image;
}

template <typename T>
[[nodiscard]] QImage makeIndexed8FromScalar(core::ArrayView<const T, 2> imageData,
                                            ImageLevelRange levels,
                                            const ImageLookupTable& lut)
{
    validateLookupTable(lut);
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_Indexed8);

    TryMakeQImageOptions tableOptions;
    tableOptions.lut = lut;
    setColorTable(image, makeIndexedColorTable(tableOptions, lut.rows));

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const double value = static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x)));
            row[x] = static_cast<uchar>(scaledLookupIndex(value, levels, lut));
        }
    }
    return image;
}

template <typename T>
[[nodiscard]] QImage makeLutApplied(core::ArrayView<const T, 2> imageData, ImageLevelRange levels, const ImageLookupTable& lut)
{
    validateLookupTable(lut);
    const auto& shape = imageData.shape();
    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    const bool grayscale = lut.channels == 1;
    QImage image = allocateQImage(width, height, grayscale ? QImage::Format_Grayscale8 : QImage::Format_RGBA8888);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const double value = static_cast<double>(imageData(static_cast<std::size_t>(y), static_cast<std::size_t>(x)));
            const auto color = lutColorFor(value, levels, lut);
            if (grayscale) {
                row[x] = color[0];
            } else {
                const std::size_t base = static_cast<std::size_t>(x) * 4;
                row[base + 0] = color[0];
                row[base + 1] = color[1];
                row[base + 2] = color[2];
                row[base + 3] = color[3];
            }
        }
    }
    return image;
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

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 2> imageData)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
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

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 2> imageData,
                                    const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }
    if (!options.levels.has_value() && !options.lut.has_value()) {
        return tryMakeQImage(imageData);
    }

    return makeIndexed8FromUInt8(imageData, options);
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 3> imageData)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }

    const auto& shape = imageData.shape();
    const std::size_t channels = shape[2];
    QImage::Format format = QImage::Format_Invalid;
    if (channels == 3) {
        format = QImage::Format_RGB888;
    } else if (channels == 4) {
        format = QImage::Format_RGBA8888;
    } else {
        return std::nullopt;
    }

    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, format);

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const std::size_t rowIndex = static_cast<std::size_t>(y);
            const std::size_t colIndex = static_cast<std::size_t>(x);
            const std::size_t base = static_cast<std::size_t>(x) * channels;
            row[base + 0] = imageData(rowIndex, colIndex, 0);
            row[base + 1] = imageData(rowIndex, colIndex, 1);
            row[base + 2] = imageData(rowIndex, colIndex, 2);
            if (channels == 4) {
                row[base + 3] = imageData(rowIndex, colIndex, 3);
            }
        }
    }

    return image;
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 3> imageData,
                                    const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }
    if (!options.levels.has_value() && !options.lut.has_value()) {
        return tryMakeQImage(imageData);
    }
    if (options.lut.has_value()) {
        return std::nullopt;
    }

    const auto& shape = imageData.shape();
    if (shape[2] != 3) {
        return std::nullopt;
    }

    const int width = static_cast<int>(shape[1]);
    const int height = static_cast<int>(shape[0]);
    QImage image = allocateQImage(width, height, QImage::Format_RGB888);
    const ImageLevelRange levels = *options.levels;

    for (int y = 0; y < height; ++y) {
        uchar* row = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const std::size_t rowIndex = static_cast<std::size_t>(y);
            const std::size_t colIndex = static_cast<std::size_t>(x);
            const std::size_t base = static_cast<std::size_t>(x) * 3;
            row[base + 0] = scaledGray(imageData(rowIndex, colIndex, 0), levels);
            row[base + 1] = scaledGray(imageData(rowIndex, colIndex, 1), levels);
            row[base + 2] = scaledGray(imageData(rowIndex, colIndex, 2), levels);
        }
    }

    return image;
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 2> imageData)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
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

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 2> imageData,
                                     const TryMakeQImageOptions& options)
{
    if (!hasTryMakeData(imageData)) {
        return std::nullopt;
    }
    if (!options.levels.has_value() && !options.lut.has_value()) {
        return tryMakeQImage(imageData);
    }

    const ImageLevelRange levels = options.levels.value_or(ImageLevelRange{0.0, 65535.0});
    if (!options.lut.has_value()) {
        return makeGrayscale8(imageData, levels);
    }
    if (options.lut->rows <= 256) {
        return makeIndexed8FromScalar(imageData, levels, *options.lut);
    }
    return makeLutApplied(imageData, levels, *options.lut);
}

std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 3> imageData)
{
    if (!hasTryMakeData(imageData)) {
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

    if (!options.lut.has_value()) {
        return makeGrayscale8(imageData, *options.levels);
    }
    if (options.lut->rows <= 256) {
        return makeIndexed8FromScalar(imageData, *options.levels, *options.lut);
    }
    return makeLutApplied(imageData, *options.levels, *options.lut);
}

} // namespace cppqtgraph
