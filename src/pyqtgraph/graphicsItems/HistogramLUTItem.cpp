// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/HistogramLUTItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/HistogramLUTItem.hpp"

#include "../../../include/pyqtgraph/graphicsItems/ImageItem.hpp"

#include <QtCore/QObject>
#include <QtWidgets/QGraphicsGridLayout>

#include <stdexcept>
#include <utility>

namespace pyqtgraph::graphicsItems {
namespace {

ColorMap defaultGreyMap()
{
    return ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)}, QStringLiteral("grey"));
}

LinearRegionItem::Orientation regionOrientationFor(HistogramLUTItem::Orientation orientation)
{
    return orientation == HistogramLUTItem::Orientation::Vertical ? LinearRegionItem::Orientation::Horizontal
                                                                 : LinearRegionItem::Orientation::Vertical;
}

} // namespace

HistogramLUTItem::HistogramLUTItem(ImageItem* image,
                                   bool fillHistogram,
                                   QString levelMode,
                                   QString gradientPosition,
                                   Orientation orientation,
                                   QGraphicsItem* parent,
                                   Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
    , levelMode_(std::move(levelMode))
    , orientation_(orientation)
    , gradientPosition_(normalizeGradientPosition(orientation_, gradientPosition))
    , colorMap_(defaultGreyMap())
    , fillHistogram_(fillHistogram)
{
    if (levelMode_ != QStringLiteral("mono") && levelMode_ != QStringLiteral("rgba")) {
        throw std::invalid_argument("HistogramLUTItem levelMode must be 'mono' or 'rgba'");
    }

    auto* layout = new QGraphicsGridLayout;
    layout->setContentsMargins(1.0, 1.0, 1.0, 1.0);
    layout->setSpacing(0.0);
    setLayout(layout);

    region_ = new LinearRegionItem(std::make_pair(0.0, 1.0), regionOrientationFor(orientation_), true, std::nullopt, this);
    region_->setSwapMode(LinearRegionItem::SwapMode::Block);
    region_->setZValue(1000.0);
    QObject::connect(region_, &LinearRegionItem::sigRegionChanged, this, [this](LinearRegionItem*) { regionChanging(); });
    QObject::connect(region_, &LinearRegionItem::sigRegionChangeFinished, this, [this](LinearRegionItem*) { regionChanged(); });

    if (image != nullptr) {
        setImageItem(image);
    }
}

HistogramLUTItem::~HistogramLUTItem() = default;

void HistogramLUTItem::setImageItem(ImageItem* image)
{
    if (imageItem_ == image) {
        return;
    }
    if (imageChangedConnection_) {
        QObject::disconnect(imageChangedConnection_);
        imageChangedConnection_ = QMetaObject::Connection{};
    }

    imageItem_ = image;
    if (imageItem_ != nullptr) {
        imageChangedConnection_ = QObject::connect(imageItem_.data(), &ImageItem::sigImageChanged, this, [this] { imageChanged(false, false); });
        setImageLookupTable();
        regionChanged();
        imageChanged(true, false);
    }
}

ImageItem* HistogramLUTItem::imageItem() const noexcept
{
    return imageItem_.data();
}

QString HistogramLUTItem::levelMode() const
{
    return levelMode_;
}

void HistogramLUTItem::setLevelMode(const QString& mode)
{
    if (mode != QStringLiteral("mono") && mode != QStringLiteral("rgba")) {
        throw std::invalid_argument("HistogramLUTItem levelMode must be 'mono' or 'rgba'");
    }
    if (mode == levelMode_) {
        return;
    }
    levelMode_ = mode;
    setImageLookupTable();
    applyImageLevels();
    update();
}

HistogramLUTItem::Orientation HistogramLUTItem::orientation() const noexcept
{
    return orientation_;
}

QString HistogramLUTItem::orientationName() const
{
    return orientation_ == Orientation::Vertical ? QStringLiteral("vertical") : QStringLiteral("horizontal");
}

QString HistogramLUTItem::gradientPosition() const
{
    return gradientPosition_;
}

std::pair<double, double> HistogramLUTItem::getLevels() const
{
    const auto region = region_->getRegion();
    return {region.first, region.second};
}

void HistogramLUTItem::setLevels(double minimum, double maximum)
{
    setLevels(std::make_pair(minimum, maximum));
}

void HistogramLUTItem::setLevels(const std::pair<double, double>& levels)
{
    if (levelMode_ != QStringLiteral("mono")) {
        throw std::invalid_argument("HistogramLUTItem C++ P4.06 slice supports scalar levels in mono mode");
    }
    region_->setRegion(levels);
}

void HistogramLUTItem::setColorMap(const ColorMap& colorMap)
{
    colorMap_ = colorMap;
    lookupTrivial_ = false;
    gradientChanged();
}

void HistogramLUTItem::setColorMap(const QString& name)
{
    if (name == QStringLiteral("grey") || name == QStringLiteral("gray")) {
        colorMap_ = defaultGreyMap();
        lookupTrivial_ = false;
        gradientChanged();
        return;
    }
    const auto found = pyqtgraph::get(name);
    if (!found.has_value()) {
        throw std::invalid_argument("HistogramLUTItem unknown ColorMap name");
    }
    setColorMap(*found);
}

const ColorMap& HistogramLUTItem::colorMap() const noexcept
{
    return colorMap_;
}

bool HistogramLUTItem::isLookupTrivial() const noexcept
{
    return lookupTrivial_;
}

ImageLookupTable HistogramLUTItem::getLookupTable(std::size_t rows, bool alpha) const
{
    if (levelMode_ != QStringLiteral("mono")) {
        return {};
    }
    rebuildLookupTable(rows, alpha);
    return ImageLookupTable{lookupTableBytes_.data(), lookupTableRows_, lookupTableChannels_, static_cast<std::ptrdiff_t>(lookupTableChannels_), 1};
}

LinearRegionItem* HistogramLUTItem::levelRegion() noexcept
{
    return region_;
}

const LinearRegionItem* HistogramLUTItem::levelRegion() const noexcept
{
    return region_;
}

void HistogramLUTItem::gradientChanged()
{
    if (imageItem_ != nullptr) {
        setImageLookupTable();
    }
    emit sigLookupTableChanged(this);
}

void HistogramLUTItem::regionChanging()
{
    applyImageLevels();
    update();
    emit sigLevelsChanged(this);
}

void HistogramLUTItem::regionChanged()
{
    applyImageLevels();
    emit sigLevelChangeFinished(this);
}

void HistogramLUTItem::imageChanged(bool autoLevel, bool autoRange)
{
    Q_UNUSED(autoRange);
    if (imageItem_ == nullptr) {
        return;
    }
    if (autoLevel && !imageItem_->getLevels().has_value()) {
        applyImageLevels();
    }
}

QString HistogramLUTItem::normalizeGradientPosition(Orientation orientation, const QString& gradientPosition)
{
    if (orientation == Orientation::Vertical) {
        if (gradientPosition == QStringLiteral("left") || gradientPosition == QStringLiteral("right")) {
            return gradientPosition;
        }
        return QStringLiteral("right");
    }
    if (gradientPosition == QStringLiteral("top") || gradientPosition == QStringLiteral("bottom")) {
        return gradientPosition;
    }
    return QStringLiteral("bottom");
}

void HistogramLUTItem::setImageLookupTable()
{
    if (imageItem_ == nullptr) {
        return;
    }
    if (lookupTrivial_ || levelMode_ != QStringLiteral("mono")) {
        imageItem_->clearLookupTable();
        return;
    }
    const auto lut = getLookupTable(256, true);
    imageItem_->setLookupTable(lut);
}

void HistogramLUTItem::applyImageLevels()
{
    if (imageItem_ == nullptr || levelMode_ != QStringLiteral("mono")) {
        return;
    }
    const auto levels = getLevels();
    imageItem_->setLevels(ImageLevelRange{levels.first, levels.second});
}

void HistogramLUTItem::rebuildLookupTable(std::size_t rows, bool alpha) const
{
    if (rows == 0) {
        rows = 1;
    }
    if (lookupTableRows_ == rows && lookupTableChannels_ == (alpha ? 4U : 3U) && !lookupTableBytes_.empty()) {
        return;
    }
    const auto table = colorMap_.getLookupTable(0.0, 1.0, rows, alpha, ColorMap::OutputMode::Byte);
    lookupTableBytes_ = table.bytes;
    lookupTableRows_ = table.rows();
    lookupTableChannels_ = table.channels;
}

} // namespace pyqtgraph::graphicsItems
