// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/NonUniformImage.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/NonUniformImage.hpp"

#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph::graphicsItems {
namespace {

[[nodiscard]] bool isFinite(double value)
{
    return std::isfinite(value);
}

[[nodiscard]] double levelDifference(ImageLevelRange levels)
{
    const double difference = levels.maximum - levels.minimum;
    return difference == 0.0 ? 1.0 : difference;
}

} // namespace

NonUniformImage::NonUniformImage(core::ArrayView<const double, 1> x,
                                 core::ArrayView<const double, 1> y,
                                 core::ArrayView<const double, 2> z,
                                 QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    validateInput(x, y, z);
    copyData(x, y, z);
    resetDefaultLookupTable();
    update();
}

NonUniformImage::~NonUniformImage() = default;

void NonUniformImage::setLookupTable(ImageLookupTable lut)
{
    copyLookupTable(lut);
    invalidatePicture();
    update();
}

void NonUniformImage::clearLookupTable()
{
    resetDefaultLookupTable();
    invalidatePicture();
    update();
}

std::optional<ImageLookupTable> NonUniformImage::lookupTable() const noexcept
{
    return lookupTableView();
}

void NonUniformImage::setLevels(ImageLevelRange levels)
{
    levels_ = levels;
    invalidatePicture();
    update();
}

void NonUniformImage::clearLevels()
{
    levels_ = std::nullopt;
    invalidatePicture();
    update();
}

ImageLevelRange NonUniformImage::getLevels() const
{
    if (levels_.has_value()) {
        return *levels_;
    }

    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    for (double value : z_) {
        if (!isFinite(value)) {
            continue;
        }
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    if (!isFinite(minimum) || !isFinite(maximum)) {
        throw std::invalid_argument("NonUniformImage levels require at least one finite z value");
    }
    levels_ = ImageLevelRange{minimum, maximum};
    return *levels_;
}

void NonUniformImage::setBorder(std::optional<QPen> border)
{
    border_ = std::move(border);
    invalidatePicture();
    update();
}

const std::optional<QPen>& NonUniformImage::border() const noexcept
{
    return border_;
}

QRectF NonUniformImage::boundingRect() const
{
    if (x_.empty() || y_.empty()) {
        return QRectF();
    }
    return QRectF(x_.front(), y_.front(), x_.back() - x_.front(), y_.back() - y_.front());
}

void NonUniformImage::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (painter == nullptr) {
        return;
    }
    if (!pictureValid_) {
        generatePicture();
    }
    painter->drawPicture(0, 0, picture_);
}

void NonUniformImage::validateInput(core::ArrayView<const double, 1> x,
                                    core::ArrayView<const double, 1> y,
                                    core::ArrayView<const double, 2> z) const
{
    if (x.data() == nullptr || y.data() == nullptr || z.data() == nullptr) {
        throw std::invalid_argument("NonUniformImage x, y, and z data must not be null");
    }
    if (x.size() == 0 || y.size() == 0) {
        throw std::invalid_argument("NonUniformImage x and y must be non-empty 1-d arrays");
    }
    for (std::size_t index = 1; index < x.size(); ++index) {
        if (x[index] < x[index - 1]) {
            throw std::invalid_argument("NonUniformImage x values must be monotonically increasing");
        }
    }
    for (std::size_t index = 1; index < y.size(); ++index) {
        if (y[index] < y[index - 1]) {
            throw std::invalid_argument("NonUniformImage y values must be monotonically increasing");
        }
    }
    if (z.shape()[0] != x.size() || z.shape()[1] != y.size()) {
        throw std::invalid_argument("NonUniformImage z shape must match x and y lengths");
    }
}

void NonUniformImage::copyData(core::ArrayView<const double, 1> x,
                               core::ArrayView<const double, 1> y,
                               core::ArrayView<const double, 2> z)
{
    x_.resize(x.size());
    for (std::size_t index = 0; index < x.size(); ++index) {
        x_[index] = x[index];
    }
    y_.resize(y.size());
    for (std::size_t index = 0; index < y.size(); ++index) {
        y_[index] = y[index];
    }
    z_.resize(z.size());
    for (std::size_t xi = 0; xi < x.size(); ++xi) {
        for (std::size_t yi = 0; yi < y.size(); ++yi) {
            z_[xi * y.size() + yi] = z(xi, yi);
        }
    }
}

void NonUniformImage::resetDefaultLookupTable()
{
    lookupTableRows_ = 256;
    lookupTableChannels_ = 4;
    lookupTableData_.resize(lookupTableRows_ * lookupTableChannels_);
    for (std::size_t row = 0; row < lookupTableRows_; ++row) {
        const auto gray = static_cast<std::uint8_t>(row);
        lookupTableData_[row * lookupTableChannels_ + 0] = gray;
        lookupTableData_[row * lookupTableChannels_ + 1] = gray;
        lookupTableData_[row * lookupTableChannels_ + 2] = gray;
        lookupTableData_[row * lookupTableChannels_ + 3] = 255;
    }
}

void NonUniformImage::copyLookupTable(ImageLookupTable lut)
{
    if (lut.data == nullptr || lut.rows == 0) {
        throw std::invalid_argument("NonUniformImage lookup table must contain at least one row");
    }
    if (lut.channels != 1 && lut.channels != 3 && lut.channels != 4) {
        throw std::invalid_argument("NonUniformImage lookup table must have 1, 3, or 4 channels");
    }
    if (lut.rowStride <= 0 || lut.channelStride <= 0) {
        throw std::invalid_argument("NonUniformImage lookup table strides must be positive");
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
}

std::optional<ImageLookupTable> NonUniformImage::lookupTableView() const noexcept
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

std::vector<double> NonUniformImage::cellEdges(const std::vector<double>& centers) const
{
    std::vector<double> edges(centers.size() + 1);
    edges.front() = centers.front();
    edges.back() = centers.back();
    for (std::size_t index = 1; index < centers.size(); ++index) {
        edges[index] = (centers[index - 1] + centers[index]) / 2.0;
    }
    return edges;
}

void NonUniformImage::invalidatePicture()
{
    pictureValid_ = false;
}

void NonUniformImage::generatePicture() const
{
    const auto lut = lookupTableView();
    if (!lut.has_value()) {
        picture_ = QPicture();
        pictureValid_ = true;
        return;
    }

    const std::vector<double> xEdges = cellEdges(x_);
    const std::vector<double> yEdges = cellEdges(y_);
    const ImageLevelRange levels = getLevels();
    const double scale = static_cast<double>(lut->rows) / levelDifference(levels);

    picture_ = QPicture();
    QPainter painter(&picture_);
    painter.setPen(Qt::NoPen);

    for (std::size_t xi = 0; xi < x_.size(); ++xi) {
        for (std::size_t yi = 0; yi < y_.size(); ++yi) {
            const double value = z_[xi * y_.size() + yi];
            if (std::isnan(value)) {
                continue;
            }
            const std::size_t colorIndex = rescaleDataIndex(value, scale, levels.minimum, lut->rows - 1);
            const auto color = applyLookupTable(static_cast<std::int64_t>(colorIndex), *lut);
            painter.setBrush(QBrush(QColor(color[0], color[1], color[2], color[3])));
            painter.drawRect(QRectF(xEdges[xi], yEdges[yi], xEdges[xi + 1] - xEdges[xi], yEdges[yi + 1] - yEdges[yi]));
        }
    }

    if (border_.has_value()) {
        painter.setPen(*border_);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(boundingRect());
    }

    painter.end();
    pictureValid_ = true;
}

} // namespace cppqtgraph::graphicsItems
