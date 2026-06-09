// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/HistogramLUTWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/HistogramLUTWidget.hpp"

#include <QtWidgets/QSizePolicy>

namespace pyqtgraph::widgets {

HistogramLUTWidget::HistogramLUTWidget(QWidget* parent,
                                         graphicsItems::ImageItem* image,
                                         bool fillHistogram,
                                         const QString& levelMode,
                                         const QString& gradientPosition,
                                         graphicsItems::HistogramLUTItem::Orientation orientation)
    : GraphicsView(parent)
    , item_(new graphicsItems::HistogramLUTItem(image, fillHistogram, levelMode, gradientPosition, orientation))
    , orientation_(orientation)
{
    setCentralItem(item_);
    applyOrientationSizing();
}

void HistogramLUTWidget::setImageItem(graphicsItems::ImageItem* image)
{
    item_->setImageItem(image);
}

graphicsItems::ImageItem* HistogramLUTWidget::imageItem() const noexcept
{
    return item_->imageItem();
}

QString HistogramLUTWidget::levelMode() const
{
    return item_->levelMode();
}

void HistogramLUTWidget::setLevelMode(const QString& mode)
{
    item_->setLevelMode(mode);
}

QString HistogramLUTWidget::orientationName() const
{
    return item_->orientationName();
}

QString HistogramLUTWidget::gradientPosition() const
{
    return item_->gradientPosition();
}

std::pair<double, double> HistogramLUTWidget::getLevels() const
{
    return item_->getLevels();
}

void HistogramLUTWidget::setLevels(double minimum, double maximum)
{
    item_->setLevels(minimum, maximum);
}

void HistogramLUTWidget::setLevels(const std::pair<double, double>& levels)
{
    item_->setLevels(levels);
}

void HistogramLUTWidget::setColorMap(const ColorMap& colorMap)
{
    item_->setColorMap(colorMap);
}

void HistogramLUTWidget::setColorMap(const QString& name)
{
    item_->setColorMap(name);
}

const ColorMap& HistogramLUTWidget::colorMap() const noexcept
{
    return item_->colorMap();
}

bool HistogramLUTWidget::isLookupTrivial() const noexcept
{
    return item_->isLookupTrivial();
}

ImageLookupTable HistogramLUTWidget::getLookupTable(std::size_t rows, bool alpha) const
{
    return item_->getLookupTable(rows, alpha);
}

graphicsItems::LinearRegionItem* HistogramLUTWidget::levelRegion() noexcept
{
    return item_->levelRegion();
}

const graphicsItems::LinearRegionItem* HistogramLUTWidget::levelRegion() const noexcept
{
    return item_->levelRegion();
}

QSize HistogramLUTWidget::sizeHint() const
{
    if (orientation_ == graphicsItems::HistogramLUTItem::Orientation::Vertical) {
        return QSize(115, 200);
    }
    return QSize(200, 115);
}

void HistogramLUTWidget::applyOrientationSizing()
{
    if (orientation_ == graphicsItems::HistogramLUTItem::Orientation::Vertical) {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        setMinimumWidth(95);
        setMinimumHeight(0);
    } else {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumHeight(95);
        setMinimumWidth(0);
    }
}

} // namespace pyqtgraph::widgets
