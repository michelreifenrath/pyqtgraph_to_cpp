// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ColorBarItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/ColorBarItem.hpp"

#include "../../../include/pyqtgraph/graphicsItems/AxisItem.hpp"
#include "../../../include/pyqtgraph/graphicsItems/ImageItem.hpp"

#include <QtCore/QObject>
#include <QtGui/QImage>
#include <QtGui/QPixmap>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace pyqtgraph::graphicsItems {
namespace {

std::pair<double, double> defaultValues(std::optional<std::pair<double, double>> values)
{
    return values.value_or(std::make_pair(0.0, 1.0));
}

QImage imageFromLookupBytes(const std::vector<std::uint8_t>& bytes, std::size_t rows, std::size_t channels, bool horizontal)
{
    if (bytes.empty() || rows == 0 || channels < 3) {
        return {};
    }

    QImage image(horizontal ? static_cast<int>(rows) : 1, horizontal ? 1 : static_cast<int>(rows), QImage::Format_RGBA8888);
    for (std::size_t row = 0; row < rows; ++row) {
        const std::size_t offset = row * channels;
        const QColor color(bytes[offset], bytes[offset + 1], bytes[offset + 2], channels >= 4 ? bytes[offset + 3] : 255);
        if (horizontal) {
            image.setPixelColor(static_cast<int>(row), 0, color);
        } else {
            image.setPixelColor(0, static_cast<int>(rows - row - 1), color);
        }
    }
    return image;
}

} // namespace

ColorBarItem::ColorBarItem(std::optional<std::pair<double, double>> values,
                           double width,
                           std::optional<ColorMap> colorMap,
                           bool interactive,
                           std::optional<std::pair<double, double>> limits,
                           double rounding,
                           Orientation orientation,
                           QGraphicsItem* parent,
                           Qt::WindowFlags flags)
    : PlotItem(parent, flags)
    , values_(defaultValues(values))
    , previousValues_(values_)
    , colorMap_(std::move(colorMap))
    , rounding_(rounding)
    , horizontal_(orientation == Orientation::Horizontal)
    , activelyAdjustedValues_(values.has_value())
    , interactive_(interactive)
{
    if (rounding_ <= 0.0) {
        throw std::invalid_argument("ColorBarItem rounding must be positive");
    }

    if (limits.has_value()) {
        lowLimit_ = rounding_ * std::floor(limits->first / rounding_);
        highLimit_ = rounding_ * std::ceil(limits->second / rounding_);
    }

    if (horizontal_) {
        setXRange(0.0, 256.0, 0.0);
        setYRange(0.0, 1.0, 0.0);
    } else {
        setXRange(0.0, 1.0, 0.0);
        setYRange(0.0, 256.0, 0.0);
    }

    for (const QString& axisName : {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("top"), QStringLiteral("bottom")}) {
        showAxis(axisName, true);
        if (auto* axis = getAxis(axisName)) {
            axis->setShowValues((horizontal_ && axisName == QStringLiteral("bottom")) || (!horizontal_ && axisName == QStringLiteral("right")));
        }
    }
    if (auto* valueAxis = getAxis(horizontal_ ? QStringLiteral("bottom") : QStringLiteral("right"))) {
        valueAxis->setRange(values_.first, values_.second);
    }

    bar_ = new QGraphicsPixmapItem(this);
    bar_->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

    if (interactive_) {
        const auto regionOrientation = horizontal_ ? LinearRegionItem::Orientation::Vertical : LinearRegionItem::Orientation::Horizontal;
        region_ = new LinearRegionItem(std::make_pair(63.0, 191.0), regionOrientation, true, std::nullopt, this);
        region_->setSwapMode(LinearRegionItem::SwapMode::Block);
        region_->setZValue(1000.0);
        QObject::connect(region_, &LinearRegionItem::sigRegionChanged, this, [this](LinearRegionItem*) { regionChanging(); });
        QObject::connect(region_, &LinearRegionItem::sigRegionChangeFinished, this, [this](LinearRegionItem*) { regionChanged(); });
        regionChangedEnable_ = true;
        region_->setRegion(std::make_pair(63.0, 191.0));
    }

    if (colorMap_.has_value()) {
        updateItems(true);
    } else {
        updateItems(false);
    }
    Q_UNUSED(width);
    Q_UNUSED(activelyAdjustedValues_);
}

ColorBarItem::~ColorBarItem() = default;

void ColorBarItem::setImageItem(ImageItem* image)
{
    setImageItems({image});
}

void ColorBarItem::setImageItems(std::initializer_list<ImageItem*> images)
{
    setImageItems(std::vector<ImageItem*>(images));
}

void ColorBarItem::setImageItems(const std::vector<ImageItem*>& images)
{
    imageItems_.clear();
    imageItems_.reserve(images.size());
    for (auto* image : images) {
        if (image != nullptr) {
            imageItems_.push_back(image);
        }
    }
    updateItems(true);
}

std::vector<ImageItem*> ColorBarItem::imageItems() const
{
    std::vector<ImageItem*> items;
    items.reserve(imageItems_.size());
    for (const auto& image : imageItems_) {
        if (image != nullptr) {
            items.push_back(image.data());
        }
    }
    return items;
}

void ColorBarItem::setColorMap(const ColorMap& colorMap)
{
    colorMap_ = colorMap;
    updateItems(true);
}

void ColorBarItem::setColorMap(const QString& name)
{
    const auto found = pyqtgraph::get(name);
    if (!found.has_value()) {
        throw std::invalid_argument("ColorBarItem unknown ColorMap name");
    }
    setColorMap(*found);
}

const std::optional<ColorMap>& ColorBarItem::colorMap() const noexcept
{
    return colorMap_;
}

void ColorBarItem::setLevels(std::optional<std::pair<double, double>> values,
                             std::optional<double> low,
                             std::optional<double> high,
                             bool updateItemsFlag)
{
    if (values.has_value()) {
        low = values->first;
        high = values->second;
    }

    double lowNew = low.value_or(values_.first);
    double highNew = high.value_or(values_.second);
    if (lowNew > highNew) {
        lowNew = highNew = (lowNew + highNew) / 2.0;
    }
    if (lowLimit_.has_value() && lowNew < *lowLimit_) {
        lowNew = *lowLimit_;
    }
    if (highLimit_.has_value() && highNew > *highLimit_) {
        highNew = *highLimit_;
    }

    values_ = {lowNew, highNew};
    previousValues_ = values_;
    if (updateItemsFlag) {
        updateItems(false);
    } else if (auto* valueAxis = getAxis(horizontal_ ? QStringLiteral("bottom") : QStringLiteral("right"))) {
        valueAxis->setRange(values_.first, values_.second);
    }
}

std::pair<double, double> ColorBarItem::levels() const noexcept
{
    return values_;
}

LinearRegionItem* ColorBarItem::interactionRegion() noexcept
{
    return region_;
}

const LinearRegionItem* ColorBarItem::interactionRegion() const noexcept
{
    return region_;
}

bool ColorBarItem::regionChangedEnabled() const noexcept
{
    return regionChangedEnable_;
}

void ColorBarItem::setRegionChangedEnabled(bool enabled) noexcept
{
    regionChangedEnable_ = enabled;
}

bool ColorBarItem::isHorizontal() const noexcept
{
    return horizontal_;
}

std::optional<std::pair<double, double>> ColorBarItem::limits() const noexcept
{
    if (!lowLimit_.has_value() && !highLimit_.has_value()) {
        return std::nullopt;
    }
    return std::make_pair(lowLimit_.value_or(-std::numeric_limits<double>::infinity()),
                          highLimit_.value_or(std::numeric_limits<double>::infinity()));
}

double ColorBarItem::rounding() const noexcept
{
    return rounding_;
}

void ColorBarItem::regionChanging()
{
    if (!regionChangedEnable_ || region_ == nullptr) {
        return;
    }

    auto [bottom, top] = region_->getRegion();
    bottom = (bottom - 63.0) / 64.0;
    top = (top - 191.0) / 64.0;
    bottom = std::copysign(bottom * bottom, bottom);
    top = std::copysign(top * top, top);

    const double previousSpan = previousValues_.second - previousValues_.first;
    double highNew = previousValues_.second + (previousSpan + 2.0 * rounding_) * top;
    double lowNew = previousValues_.first + (previousSpan + 2.0 * rounding_) * bottom;

    if (highLimit_.has_value() && highNew > *highLimit_) {
        highNew = *highLimit_;
        if (top != 0.0 && bottom != 0.0) {
            lowNew = highNew - previousSpan;
        }
    }
    if (lowLimit_.has_value() && lowNew < *lowLimit_) {
        lowNew = *lowLimit_;
        if (top != 0.0 && bottom != 0.0) {
            highNew = lowNew + previousSpan;
        }
    }
    if (highNew - lowNew < rounding_) {
        if (bottom == 0.0) {
            highNew = lowNew + rounding_;
        } else if (top == 0.0) {
            lowNew = highNew - rounding_;
        } else {
            const double middle = (highNew + lowNew) / 2.0;
            highNew = middle + rounding_ / 2.0;
            lowNew = middle - rounding_ / 2.0;
        }
    }

    lowNew = rounding_ * std::round(lowNew / rounding_);
    highNew = rounding_ * std::round(highNew / rounding_);
    values_ = {lowNew, highNew};
    updateItems(false);
    emit sigLevelsChanged(this);
}

void ColorBarItem::regionChanged()
{
    if (!regionChangedEnable_) {
        return;
    }
    previousValues_ = values_;
    if (region_ != nullptr) {
        regionChangedEnable_ = false;
        region_->setRegion(std::make_pair(63.0, 191.0));
        regionChangedEnable_ = true;
    }
    emit sigLevelsChangeFinished(this);
}

void ColorBarItem::updateItems(bool updateColorMap)
{
    if (auto* valueAxis = getAxis(horizontal_ ? QStringLiteral("bottom") : QStringLiteral("right"))) {
        valueAxis->setRange(values_.first, values_.second);
    }
    if (updateColorMap && colorMap_.has_value()) {
        rebuildLookupTable();
        updateBarPixmap();
    }

    for (auto& imagePointer : imageItems_) {
        if (imagePointer == nullptr) {
            continue;
        }
        imagePointer->setLevels(ImageLevelRange{values_.first, values_.second});
        if (updateColorMap && colorMap_.has_value()) {
            applyColorMapTo(*imagePointer);
        }
    }
}

void ColorBarItem::updateBarPixmap()
{
    if (bar_ == nullptr || lookupTableBytes_.empty()) {
        return;
    }
    const QImage image = imageFromLookupBytes(lookupTableBytes_, lookupTableRows_, lookupTableChannels_, horizontal_);
    if (!image.isNull()) {
        bar_->setPixmap(QPixmap::fromImage(image));
    }
}

void ColorBarItem::applyColorMapTo(ImageItem& image) const
{
    image.setLookupTable(imageLookupTable());
}

ImageLookupTable ColorBarItem::imageLookupTable() const
{
    rebuildLookupTable();
    return ImageLookupTable{lookupTableBytes_.data(), lookupTableRows_, lookupTableChannels_, static_cast<std::ptrdiff_t>(lookupTableChannels_), 1};
}

void ColorBarItem::rebuildLookupTable(std::size_t rows, bool alpha) const
{
    if (!colorMap_.has_value()) {
        return;
    }
    if (lookupTableRows_ == rows && lookupTableChannels_ == (alpha ? 4U : 3U) && !lookupTableBytes_.empty()) {
        return;
    }
    const auto table = colorMap_->getLookupTable(0.0, 1.0, rows, alpha, ColorMap::OutputMode::Byte);
    lookupTableBytes_ = table.bytes;
    lookupTableRows_ = table.rows();
    lookupTableChannels_ = table.channels;
}

} // namespace pyqtgraph::graphicsItems
