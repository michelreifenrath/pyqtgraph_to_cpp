// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/HistogramLUTItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/HistogramLUTItem.hpp"

#include "../../../include/cppqtgraph/graphicsItems/AxisItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/GradientEditorItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ImageItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <QtCore/QObject>
#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsGridLayout>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace cppqtgraph::graphicsItems {
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

QString axisOrientationFor(const QString& gradientPosition)
{
    if (gradientPosition == QStringLiteral("left")) {
        return QStringLiteral("right");
    }
    if (gradientPosition == QStringLiteral("right")) {
        return QStringLiteral("left");
    }
    if (gradientPosition == QStringLiteral("top")) {
        return QStringLiteral("bottom");
    }
    return QStringLiteral("top");
}

void styleRegion(LinearRegionItem* region, const QColor& penColor, const QColor& brushColor, qreal spanMin, qreal spanMax)
{
    const QPen pen(penColor);
    if (region->line(0) != nullptr) {
        region->line(0)->setPen(pen);
    }
    if (region->line(1) != nullptr) {
        region->line(1)->setPen(pen);
    }
    region->setBrush(QBrush(brushColor));
    region->setSpan(spanMin, spanMax);
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

    viewBox_ = new ViewBox(this);
    if (orientation_ == Orientation::Vertical) {
        viewBox_->setMaximumWidth(152);
        viewBox_->setMinimumWidth(45);
        viewBox_->setMouseEnabled(false, true);
    } else {
        viewBox_->setMaximumHeight(152);
        viewBox_->setMinimumHeight(45);
        viewBox_->setMouseEnabled(true, false);
    }

    gradient_ = new GradientEditorItem(gradientPosition_, true, true, this);
    loadGreyGradientPreset();

    const auto regionOrientation = regionOrientationFor(orientation_);
    regions_[0] = new LinearRegionItem(std::make_pair(0.0, 1.0), regionOrientation, true, std::nullopt, viewBox_);
    styleRegion(regions_[0], QColor(200, 200, 200), QColor(100, 100, 200, 100), 0.0, 1.0);
    regions_[1] = new LinearRegionItem(std::make_pair(0.0, 1.0), regionOrientation, true, std::nullopt, viewBox_);
    styleRegion(regions_[1], QColor(255, 50, 50), QColor(255, 50, 50, 50), 0.0, 1.0 / 3.0);
    regions_[2] = new LinearRegionItem(std::make_pair(0.0, 1.0), regionOrientation, true, std::nullopt, viewBox_);
    styleRegion(regions_[2], QColor(50, 255, 50), QColor(50, 255, 50, 50), 1.0 / 3.0, 2.0 / 3.0);
    regions_[3] = new LinearRegionItem(std::make_pair(0.0, 1.0), regionOrientation, true, std::nullopt, viewBox_);
    styleRegion(regions_[3], QColor(50, 50, 255), QColor(50, 50, 255, 80), 2.0 / 3.0, 1.0);
    regions_[4] = new LinearRegionItem(std::make_pair(0.0, 1.0), regionOrientation, true, std::nullopt, viewBox_);
    styleRegion(regions_[4], QColor(255, 255, 255), QColor(255, 255, 255, 50), 2.0 / 3.0, 1.0);
    region_ = regions_[0];

    for (LinearRegionItem* region : regions_) {
        region->setZValue(1000.0);
        region->setSwapMode(LinearRegionItem::SwapMode::Block);
        QObject::connect(region, &LinearRegionItem::sigRegionChanged, this, [this](LinearRegionItem*) { regionChanging(); });
        QObject::connect(region, &LinearRegionItem::sigRegionChangeFinished, this, [this](LinearRegionItem*) { regionChanged(); });
    }

    axis_ = new AxisItem(axisOrientationFor(gradientPosition_), this);
    axis_->setTickLength(-10.0);
    axis_->linkToView(viewBox_);

    const std::array<int, 3> avg = (gradientPosition_ == QStringLiteral("right") || gradientPosition_ == QStringLiteral("bottom"))
                                       ? std::array<int, 3>{0, 1, 2}
                                       : std::array<int, 3>{2, 1, 0};
    if (orientation_ == Orientation::Vertical) {
        layout->addItem(axis_, 0, avg[0]);
        layout->addItem(viewBox_, 0, avg[1]);
        layout->addItem(gradient_, 0, avg[2]);
    } else {
        layout->addItem(axis_, avg[0], 0);
        layout->addItem(viewBox_, avg[1], 0);
        layout->addItem(gradient_, avg[2], 0);
    }

    gradient_->setFlag(QGraphicsItem::ItemStacksBehindParent);
    viewBox_->setFlag(QGraphicsItem::ItemStacksBehindParent);

    QObject::connect(gradient_, &GradientEditorItem::sigGradientChanged, this, [this](GradientEditorItem*) { gradientChanged(); });
    QObject::connect(viewBox_, &ViewBox::sigRangeChanged, this, [this](ViewBox*, ViewBox::Range2D, std::array<bool, 2>) { update(); });

    plots_[0] = new PlotCurveItem(viewBox_);
    plots_[0]->setPen(QPen(QColor(200, 200, 200, 100)));
    plots_[1] = new PlotCurveItem(viewBox_);
    plots_[1]->setPen(QPen(QColor(255, 0, 0, 100)));
    plots_[2] = new PlotCurveItem(viewBox_);
    plots_[2]->setPen(QPen(QColor(0, 255, 0, 100)));
    plots_[3] = new PlotCurveItem(viewBox_);
    plots_[3]->setPen(QPen(QColor(0, 0, 255, 100)));
    plots_[4] = new PlotCurveItem(viewBox_);
    plots_[4]->setPen(QPen(QColor(200, 200, 200, 100)));

    for (PlotCurveItem* plot : plots_) {
        if (orientation_ == Orientation::Vertical) {
            plot->setRotation(90.0);
        }
        viewBox_->addItem(plot);
    }

    viewBox_->enableAutoRange(ViewBox::XYAxes, true);
    showRegions();

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
        if (imageItem_->hasImage()) {
            regionChanged();
            imageChanged(true, false);
        }
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

    const auto oldLevels = mode == QStringLiteral("mono") ? std::vector<std::pair<double, double>>{getLevels()}
                                                          : getChannelLevels();
    levelMode_ = mode;
    showRegions();

    if (mode == QStringLiteral("mono")) {
        double minimum = 0.0;
        double maximum = 1.0;
        if (!oldLevels.empty()) {
            minimum = oldLevels.front().first;
            maximum = oldLevels.front().second;
            for (const auto& level : oldLevels) {
                minimum = std::min(minimum, level.first);
                maximum = std::max(maximum, level.second);
            }
        }
        setLevels(minimum, maximum);
    } else {
        std::vector<std::pair<double, double>> levels = oldLevels;
        if (levels.size() == 1) {
            levels.assign(4, levels.front());
        }
        setChannelLevels(levels);
    }

    setImageLookupTable();
    if (imageItem_ != nullptr) {
        applyImageLevels();
    }
    fillHistogramPlots(false);
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

std::vector<std::pair<double, double>> HistogramLUTItem::getChannelLevels() const
{
    std::size_t channelCount = 3;
    if (imageItem_ != nullptr && imageItem_->channels() > 0) {
        channelCount = imageItem_->channels();
    }
    std::vector<std::pair<double, double>> levels;
    levels.reserve(channelCount);
    for (std::size_t index = 1; index <= channelCount; ++index) {
        const auto region = regions_[index]->getRegion();
        levels.emplace_back(region.first, region.second);
    }
    return levels;
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

void HistogramLUTItem::setChannelLevels(const std::vector<std::pair<double, double>>& levels)
{
    if (levelMode_ != QStringLiteral("rgba")) {
        throw std::invalid_argument("HistogramLUTItem channel levels require rgba mode");
    }
    for (std::size_t index = 0; index < levels.size() && index + 1 < regions_.size(); ++index) {
        regions_[index + 1]->setRegion(levels[index]);
    }
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
    const auto found = cppqtgraph::get(name);
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

AxisItem* HistogramLUTItem::axis() noexcept
{
    return axis_;
}

const AxisItem* HistogramLUTItem::axis() const noexcept
{
    return axis_;
}

GradientEditorItem* HistogramLUTItem::gradient() noexcept
{
    return gradient_;
}

const GradientEditorItem* HistogramLUTItem::gradient() const noexcept
{
    return gradient_;
}

ViewBox* HistogramLUTItem::viewBox() noexcept
{
    return viewBox_;
}

const ViewBox* HistogramLUTItem::viewBox() const noexcept
{
    return viewBox_;
}

LinearRegionItem* HistogramLUTItem::channelRegion(std::size_t channelIndex) noexcept
{
    if (channelIndex + 1 >= regions_.size()) {
        return nullptr;
    }
    return regions_[channelIndex + 1];
}

const LinearRegionItem* HistogramLUTItem::channelRegion(std::size_t channelIndex) const noexcept
{
    if (channelIndex + 1 >= regions_.size()) {
        return nullptr;
    }
    return regions_[channelIndex + 1];
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
    fillHistogramPlots(autoLevel);
    if (autoLevel) {
        applyImageLevels();
    } else if (levelMode_ == QStringLiteral("mono") && !imageItem_->getLevels().has_value()) {
        const auto levels = getLevels();
        region_->setRegion(levels);
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
    if (imageItem_ == nullptr) {
        return;
    }
    if (levelMode_ == QStringLiteral("mono")) {
        const auto levels = getLevels();
        imageItem_->setLevels(ImageLevelRange{levels.first, levels.second});
        return;
    }

    const auto channelLevels = getChannelLevels();
    std::vector<ImageLevelRange> levels;
    levels.reserve(channelLevels.size());
    for (const auto& level : channelLevels) {
        levels.push_back(ImageLevelRange{level.first, level.second});
    }
    imageItem_->setChannelLevels(levels);
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

void HistogramLUTItem::showRegions()
{
    for (LinearRegionItem* region : regions_) {
        if (region != nullptr) {
            region->setVisible(false);
        }
    }

    if (levelMode_ == QStringLiteral("rgba")) {
        std::size_t channelCount = 3;
        if (imageItem_ != nullptr && imageItem_->channels() > 0) {
            channelCount = imageItem_->channels();
        }
        const qreal channelWidth = 1.0 / static_cast<qreal>(channelCount);
        for (std::size_t index = 1; index <= channelCount; ++index) {
            regions_[index]->setVisible(true);
            regions_[index]->setSpan(static_cast<qreal>(index - 1) * channelWidth, static_cast<qreal>(index) * channelWidth);
        }
        gradient_->hide();
    } else if (levelMode_ == QStringLiteral("mono")) {
        regions_[0]->setVisible(true);
        gradient_->show();
    }
}

void HistogramLUTItem::fillHistogramPlots(bool autoLevel)
{
    if (!fillHistogram_ || imageItem_ == nullptr) {
        return;
    }

    if (levelMode_ == QStringLiteral("mono")) {
        for (std::size_t index = 1; index < plots_.size(); ++index) {
            plots_[index]->setVisible(false);
        }
        plots_[0]->setVisible(true);
        const auto histogram = imageItem_->getHistogram();
        if (histogram.first.empty()) {
            return;
        }
        plots_[0]->setData(histogram.first, histogram.second);
        if (!autoLevel) {
            if (const auto levels = imageItem_->getLevels(); levels.has_value()) {
                region_->setRegion(std::make_pair(levels->minimum, levels->maximum));
            }
        }
        return;
    }

    plots_[0]->setVisible(false);
    std::size_t channelCount = imageItem_->channels();
    if (channelCount == 0) {
        channelCount = 3;
    }
    showRegions();
    for (std::size_t channel = 0; channel < 4; ++channel) {
        if (channel < channelCount) {
            const auto histogram = imageItem_->getHistogram(static_cast<int>(channel));
            if (histogram.first.empty()) {
                plots_[channel + 1]->setVisible(false);
                continue;
            }
            plots_[channel + 1]->setVisible(true);
            plots_[channel + 1]->setData(histogram.first, histogram.second);
            if (autoLevel && !histogram.first.empty()) {
                const double minimum = histogram.first.front();
                const double maximum = histogram.first.back();
                regions_[channel + 1]->setRegion(std::make_pair(minimum, maximum));
            } else if (const auto channelLevels = imageItem_->getChannelLevels();
                       channelLevels.has_value() && channel < channelLevels->size()) {
                const auto& level = (*channelLevels)[channel];
                regions_[channel + 1]->setRegion(std::make_pair(level.minimum, level.maximum));
            }
        } else {
            plots_[channel + 1]->setVisible(false);
        }
    }
}

void HistogramLUTItem::loadGreyGradientPreset()
{
    while (gradient_->tickCount() > 0) {
        gradient_->removeTick(gradient_->tickAt(0), false);
    }
    gradient_->addTick(0.0, QColor(0, 0, 0), true, false);
    gradient_->addTick(1.0, QColor(255, 255, 255), true, true);
}

} // namespace cppqtgraph::graphicsItems
