// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/imageview/ImageView.hpp"

#include "../../../include/cppqtgraph/graphicsItems/AxisItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/HistogramLUTItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/InfiniteLine.hpp"
#include "../../../include/cppqtgraph/graphicsItems/LinearRegionItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ROI.hpp"
#include "../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../include/cppqtgraph/widgets/GraphicsView.hpp"
#include "../../../include/cppqtgraph/widgets/HistogramLUTWidget.hpp"
#include "../../../include/cppqtgraph/widgets/PlotWidget.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRectF>
#include <QtCore/QSignalBlocker>
#include <QtGui/QAction>
#include <QtGui/QColor>
#include <QtGui/QKeyEvent>
#include <QtGui/QPen>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace cppqtgraph::imageview {
namespace {

template <typename T, std::size_t Rank>
void copyImage(core::ArrayView<const T, Rank> image, std::vector<T>& destination)
{
    destination.resize(image.size());
    const auto& shape = image.shape();
    if constexpr (Rank == 2) {
        for (std::size_t axis0 = 0; axis0 < shape[0]; ++axis0) {
            for (std::size_t axis1 = 0; axis1 < shape[1]; ++axis1) {
                destination[axis0 * shape[1] + axis1] = image(axis0, axis1);
            }
        }
    } else if constexpr (Rank == 3) {
        for (std::size_t axis0 = 0; axis0 < shape[0]; ++axis0) {
            for (std::size_t axis1 = 0; axis1 < shape[1]; ++axis1) {
                for (std::size_t channel = 0; channel < shape[2]; ++channel) {
                    destination[(axis0 * shape[1] + axis1) * shape[2] + channel] = image(axis0, axis1, channel);
                }
            }
        }
    } else if constexpr (Rank == 4) {
        for (std::size_t frame = 0; frame < shape[0]; ++frame) {
            for (std::size_t axis1 = 0; axis1 < shape[1]; ++axis1) {
                for (std::size_t axis2 = 0; axis2 < shape[2]; ++axis2) {
                    for (std::size_t channel = 0; channel < shape[3]; ++channel) {
                        const std::size_t index = ((frame * shape[1] + axis1) * shape[2] + axis2) * shape[3] + channel;
                        destination[index] = image(frame, axis1, axis2, channel);
                    }
                }
            }
        }
    }
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> contiguousRank2(const std::vector<T>& storage, const std::array<std::size_t, 4>& shape)
{
    return core::ArrayView<const T, 2>(storage.data(), {shape[0], shape[1]});
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 3> contiguousRank3(const std::vector<T>& storage, const std::array<std::size_t, 4>& shape)
{
    return core::ArrayView<const T, 3>(storage.data(), {shape[0], shape[1], shape[2]});
}

[[nodiscard]] bool isColorImageShape(const std::array<std::size_t, 4>& shape) noexcept
{
    return shape[3] == 1 && (shape[2] == 3 || shape[2] == 4);
}

[[nodiscard]] bool isFrameStackShape(const std::array<std::size_t, 4>& shape) noexcept
{
    return shape[3] == 1 && !isColorImageShape(shape);
}

template <typename T>
[[nodiscard]] core::ArrayView<const T, 2> frameView(const std::vector<T>& storage,
                                                     const std::array<std::size_t, 4>& shape,
                                                     int frameIndex)
{
    const std::size_t frame = static_cast<std::size_t>(frameIndex);
    const std::size_t framePixels = shape[1] * shape[2];
    const T* frameData = storage.data() + frame * framePixels;
    return core::ArrayView<const T, 2>(frameData, {shape[1], shape[2]});
}

template <typename T>
void extractRgbFrame(const std::vector<T>& storage,
                     const std::array<std::size_t, 4>& shape,
                     int frameIndex,
                     std::vector<std::uint8_t>& rgbBuffer)
{
    const std::size_t frame = static_cast<std::size_t>(frameIndex);
    const std::size_t height = shape[1];
    const std::size_t width = shape[2];
    const std::size_t channels = shape[3];
    rgbBuffer.resize(height * width * channels);
    for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
        for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
            for (std::size_t channel = 0; channel < channels; ++channel) {
                const std::size_t sourceIndex = ((frame * height + axis1) * width + axis2) * channels + channel;
                const auto rounded = std::lround(static_cast<double>(storage[sourceIndex]));
                rgbBuffer[(axis1 * width + axis2) * channels + channel] =
                    static_cast<std::uint8_t>(std::clamp(rounded, 0L, 255L));
            }
        }
    }
}

void assignDefaultXValues(std::vector<double>& xvals, std::size_t frameCount)
{
    xvals.resize(frameCount);
    for (std::size_t index = 0; index < frameCount; ++index) {
        xvals[index] = static_cast<double>(index);
    }
}

template <typename T>
[[nodiscard]] std::optional<ImageLevelRange> autoLevelsFor(const std::vector<T>& storage)
{
    if (storage.empty()) {
        return std::nullopt;
    }

    if constexpr (std::is_same_v<T, std::uint8_t>) {
        const auto minValue = *std::min_element(storage.begin(), storage.end());
        const auto maxValue = *std::max_element(storage.begin(), storage.end());
        return ImageLevelRange{static_cast<double>(minValue), static_cast<double>(maxValue)};
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        const auto minValue = *std::min_element(storage.begin(), storage.end());
        const auto maxValue = *std::max_element(storage.begin(), storage.end());
        return ImageLevelRange{static_cast<double>(minValue), static_cast<double>(maxValue)};
    } else {
        const double minValue = static_cast<double>(nanmin(std::span<const T>(storage.data(), storage.size())));
        const double maxValue = static_cast<double>(nanmax(std::span<const T>(storage.data(), storage.size())));
        if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
            return std::nullopt;
        }
        return ImageLevelRange{minValue, maxValue};
    }
}

} // namespace

ImageView::ImageView(QWidget* parent, const QString& levelMode, bool discreteTimeLine)
    : QWidget(parent)
    , levelMode_(levelMode)
    , discreteTimeLine_(discreteTimeLine)
{
    ui_.setupUi(this);

    auto* graphicsLayout = new QVBoxLayout(ui_.graphicsContainer);
    graphicsLayout->setContentsMargins(0, 0, 0, 0);
    graphicsLayout->setSpacing(0);

    graphicsView_ = new widgets::GraphicsView(ui_.graphicsContainer);
    graphicsLayout->addWidget(graphicsView_);

    viewBox_ = new graphicsItems::ViewBox();
    imageItem_ = new graphicsItems::ImageItem();
    viewBox_->addItem(imageItem_);
    graphicsView_->setCentralItem(viewBox_);
    viewBox_->setAspectLocked(true);
    viewBox_->invertY(true);

    auto* histogramLayout = new QVBoxLayout(ui_.histogramContainer);
    histogramLayout->setContentsMargins(0, 0, 0, 0);
    histogramLayout->setSpacing(0);
    histogramWidget_ = new widgets::HistogramLUTWidget(ui_.histogramContainer, imageItem_, true, levelMode_);
    histogramLayout->addWidget(histogramWidget_);
    histogram_ = histogramWidget_->item();
    ui_.histogramContainer->setMinimumWidth(135);
    ui_.histogramContainer->show();

    auto* roiPlotLayout = new QVBoxLayout(ui_.roiPlotContainer);
    roiPlotLayout->setContentsMargins(0, 0, 0, 0);
    roiPlotLayout->setSpacing(0);
    roiPlot_ = new widgets::PlotWidget(ui_.roiPlotContainer);
    roiPlotLayout->addWidget(roiPlot_);
    roiPlot_->hideAxis(QStringLiteral("left"));
    roiPlot_->setVisible(false);

    timeLine_ = new graphicsItems::InfiniteLine(0.0, 90.0, true);
    timeLine_->setPen(QPen(QColor(255, 255, 0, 200)));
    timeLine_->setZValue(1.0);
    timeLine_->setAcceptedMouseButtons(Qt::NoButton);
    roiPlot_->addItem(timeLine_);
    timeLine_->hide();

    connect(timeLine_,
            &graphicsItems::InfiniteLine::sigPositionChanged,
            this,
            &ImageView::timeLineChanged);

    roi_ = new graphicsItems::ROI(QPointF(0.0, 0.0), QPointF(10.0, 10.0));
    roi_->addScaleHandle(QPointF(1.0, 1.0), QPointF(0.0, 0.0));
    roi_->addRotateHandle(QPointF(0.0, 0.0), QPointF(0.5, 0.5));
    roi_->setZValue(20.0);
    viewBox_->addItem(roi_);
    roi_->hide();
    connect(roi_, &graphicsItems::ROI::sigRegionChanged, this, &ImageView::roiChanged);

    normRoi_ = new graphicsItems::ROI(QPointF(0.0, 0.0), QPointF(10.0, 10.0));
    normRoi_->addScaleHandle(QPointF(1.0, 1.0), QPointF(0.0, 0.0));
    normRoi_->addRotateHandle(QPointF(0.0, 0.0), QPointF(0.5, 0.5));
    normRoi_->setPen(QPen(QColor(255, 255, 0)));
    normRoi_->setZValue(20.0);
    viewBox_->addItem(normRoi_);
    normRoi_->hide();
    connect(normRoi_, &graphicsItems::ROI::sigRegionChanged, this, &ImageView::updateNorm);

    normRgn_ = new graphicsItems::LinearRegionItem(std::make_pair(0.0, 1.0));
    normRgn_->setZValue(0.0);
    roiPlot_->addItem(normRgn_);
    normRgn_->hide();
    connect(normRgn_, &graphicsItems::LinearRegionItem::sigRegionChanged, this, &ImageView::updateNorm);

    if (ui_.roiBtn != nullptr) {
        connect(ui_.roiBtn, &QPushButton::clicked, this, &ImageView::roiClicked);
    }
    if (ui_.menuBtn != nullptr) {
        connect(ui_.menuBtn, &QPushButton::clicked, this, &ImageView::menuClicked);
    }
    if (ui_.normGroup != nullptr) {
        ui_.normGroup->hide();
    }
    if (ui_.normDivideRadio != nullptr) {
        connect(ui_.normDivideRadio, &QRadioButton::clicked, this, &ImageView::normRadioChanged);
    }
    if (ui_.normSubtractRadio != nullptr) {
        connect(ui_.normSubtractRadio, &QRadioButton::clicked, this, &ImageView::normRadioChanged);
    }
    if (ui_.normOffRadio != nullptr) {
        connect(ui_.normOffRadio, &QRadioButton::clicked, this, &ImageView::normRadioChanged);
    }
    if (ui_.normROICheck != nullptr) {
        connect(ui_.normROICheck, &QCheckBox::clicked, this, &ImageView::updateNorm);
    }
    if (ui_.normFrameCheck != nullptr) {
        connect(ui_.normFrameCheck, &QCheckBox::clicked, this, &ImageView::updateNorm);
    }
    if (ui_.normTimeRangeCheck != nullptr) {
        connect(ui_.normTimeRangeCheck, &QCheckBox::clicked, this, &ImageView::updateNorm);
    }

    if (ui_.splitter != nullptr) {
        ui_.splitter->handle(1)->setEnabled(false);
        ui_.splitter->setStyleSheet(QStringLiteral("QSplitter::handle{background-color: grey}"));
        ui_.splitter->setHandleWidth(2);
    }

    setFocusPolicy(Qt::StrongFocus);
    playTimer_.setParent(this);
    connect(&playTimer_, &QTimer::timeout, this, &ImageView::playbackTimeout);

    applyRoiPlotVisibility();
}

ImageView::~ImageView() = default;

void ImageView::setImage(core::ArrayView<const std::uint8_t, 2> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt8Rank2, uint8Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const std::uint8_t, 3> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt8Rank3, uint8Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const std::uint16_t, 2> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt16Rank2, uint16Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const std::uint16_t, 3> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::UInt16Rank3, uint16Data_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const float, 2> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::FloatRank2, floatData_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const float, 3> image, bool autoLevels, bool autoRange)
{
    setImageImpl(image, DataKind::FloatRank3, floatData_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const float, 4> image,
                         core::ArrayView<const double, 1> xvals,
                         bool autoLevels,
                         bool autoRange)
{
    setImageTimeRgbImpl(image, xvals, DataKind::FloatRank4TimeRgb, floatData_, autoLevels, autoRange);
}

void ImageView::setImage(core::ArrayView<const double, 4> image,
                         core::ArrayView<const double, 1> xvals,
                         bool autoLevels,
                         bool autoRange)
{
    setImageTimeRgbImpl(image, xvals, DataKind::DoubleRank4TimeRgb, doubleData_, autoLevels, autoRange);
}

template <typename T, std::size_t Rank>
void ImageView::setImageImpl(core::ArrayView<const T, Rank> image,
                             DataKind kind,
                             std::vector<T>& destination,
                             bool autoLevels,
                             bool autoRange)
{
    if (image.empty()) {
        clearImage();
        return;
    }

    if constexpr (std::is_same_v<T, std::uint8_t>) {
        uint16Data_.clear();
        floatData_.clear();
        doubleData_.clear();
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        uint8Data_.clear();
        floatData_.clear();
        doubleData_.clear();
    } else {
        uint8Data_.clear();
        uint16Data_.clear();
        doubleData_.clear();
    }
    xvals_.clear();

    copyImage(image, destination);
    dataKind_ = kind;
    shape_[0] = image.shape()[0];
    shape_[1] = image.shape()[1];
    if constexpr (Rank == 3) {
        shape_[2] = image.shape()[2];
        shape_[3] = 1;
        if (isFrameStackShape(shape_)) {
            if (currentIndex_ < 0 || static_cast<std::size_t>(currentIndex_) >= shape_[0]) {
                currentIndex_ = 0;
            }
        } else {
            currentIndex_ = 0;
        }
    } else {
        shape_[2] = 1;
        shape_[3] = 1;
        currentIndex_ = 0;
    }

    invalidateProcessedImage();
    syncTimelineBounds();
    updateDisplayedFrame(autoLevels, autoRange);
    applyRoiPlotVisibility();
}

template <typename T>
void ImageView::setImageTimeRgbImpl(core::ArrayView<const T, 4> image,
                                    core::ArrayView<const double, 1> xvals,
                                    DataKind kind,
                                    std::vector<T>& destination,
                                    bool autoLevels,
                                    bool autoRange)
{
    if (image.empty()) {
        clearImage();
        return;
    }

    const auto& shape = image.shape();
    if (shape[3] != 3 && shape[3] != 4) {
        return;
    }

    uint8Data_.clear();
    uint16Data_.clear();
    if constexpr (std::is_same_v<T, float>) {
        doubleData_.clear();
    } else {
        floatData_.clear();
    }

    copyImage(image, destination);
    dataKind_ = kind;
    shape_[0] = shape[0];
    shape_[1] = shape[1];
    shape_[2] = shape[2];
    shape_[3] = shape[3];
    if (currentIndex_ < 0 || static_cast<std::size_t>(currentIndex_) >= shape_[0]) {
        currentIndex_ = 0;
    }

    if (!xvals.empty()) {
        if (xvals.shape()[0] != shape_[0]) {
            clearImage();
            return;
        }
        xvals_.assign(xvals.data(), xvals.data() + xvals.shape()[0]);
    } else {
        assignDefaultXValues(xvals_, shape_[0]);
    }

    invalidateProcessedImage();
    syncTimelineBounds();
    updateDisplayedFrame(autoLevels, autoRange);
    applyRoiPlotVisibility();
}

void ImageView::clearImage()
{
    play(0.0);
    dataKind_ = DataKind::None;
    shape_ = {};
    currentIndex_ = 0;
    uint8Data_.clear();
    uint16Data_.clear();
    floatData_.clear();
    doubleData_.clear();
    xvals_.clear();
    processedFloatData_.clear();
    processedFloatDirty_ = true;
    rgbDisplayBuffer_.clear();
    imageItem_->clearImage();
    applyRoiPlotVisibility();
}

void ImageView::setCurrentIndex(int index)
{
    if (!hasImage() || !hasTimeAxis()) {
        return;
    }

    const int frameCountValue = frameCount();
    if (frameCountValue <= 0) {
        return;
    }

    const int clipped = std::clamp(index, 0, frameCountValue - 1);
    currentIndex_ = clipped;
    updateDisplayedFrame(false, true);

    if (timeLine_ == nullptr) {
        return;
    }

    ignoreTimeLine_ = true;
    if (!xvals_.empty()) {
        timeLine_->setValue(xvals_[static_cast<std::size_t>(clipped)]);
    } else {
        timeLine_->setValue(static_cast<double>(clipped));
    }
    ignoreTimeLine_ = false;
}

int ImageView::currentIndex() const noexcept
{
    return currentIndex_;
}

std::span<const double> ImageView::xValues() const noexcept
{
    return xvals_;
}

void ImageView::play(std::optional<double> rate)
{
    double resolvedRate = rate.value_or(pausedPlayRate_.value_or(fps_));
    if (resolvedRate == 0.0 && playRate_ != 0.0) {
        pausedPlayRate_ = playRate_;
    }
    playRate_ = resolvedRate;

    if (resolvedRate == 0.0) {
        playTimer_.stop();
        return;
    }

    lastPlayTime_ = playbackClockSeconds();
    if (!playTimer_.isActive()) {
        playTimer_.start(std::abs(static_cast<int>(1000.0 / resolvedRate)));
    }
}

void ImageView::togglePause()
{
    if (playTimer_.isActive()) {
        play(0.0);
    } else if (playRate_ == 0.0) {
        double resumeRate = 0.0;
        if (pausedPlayRate_.has_value()) {
            resumeRate = *pausedPlayRate_;
        } else if (!xvals_.empty() && xvals_.size() > 1) {
            resumeRate = static_cast<double>(frameCount() - 1) / (xvals_.back() - xvals_.front());
        } else {
            resumeRate = fps_;
        }
        play(resumeRate);
    } else {
        play(playRate_);
    }
}

void ImageView::jumpFrames(int frameDelta)
{
    if (!hasTimeAxis()) {
        return;
    }
    setCurrentIndex(currentIndex_ + frameDelta);
}

void ImageView::evalKeyState()
{
    if (keysPressed_.size() == 1) {
        const int key = *keysPressed_.begin();
        if (key == Qt::Key_Right) {
            play(20.0);
            jumpFrames(1);
            lastPlayTime_ = playbackClockSeconds() + 0.2;
        } else if (key == Qt::Key_Left) {
            play(-20.0);
            jumpFrames(-1);
            lastPlayTime_ = playbackClockSeconds() + 0.2;
        } else if (key == Qt::Key_Up) {
            play(-100.0);
        } else if (key == Qt::Key_Down) {
            play(100.0);
        } else if (key == Qt::Key_PageUp) {
            play(-1000.0);
        } else if (key == Qt::Key_PageDown) {
            play(1000.0);
        }
    } else {
        play(0.0);
    }
}

double ImageView::playbackClockSeconds()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool ImageView::isNoRepeatKey(int key) const noexcept
{
    switch (key) {
    case Qt::Key_Right:
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        return true;
    default:
        return false;
    }
}

void ImageView::playbackTimeout()
{
    const double now = playbackClockSeconds();
    const double dt = now - lastPlayTime_;
    if (dt < 0.0) {
        return;
    }

    const int frameStep = static_cast<int>(playRate_ * dt);
    if (frameStep == 0) {
        return;
    }

    lastPlayTime_ += static_cast<double>(frameStep) / playRate_;
    if (currentIndex_ + frameStep > frameCount()) {
        play(0.0);
    }
    jumpFrames(frameStep);
}

void ImageView::keyPressEvent(QKeyEvent* event)
{
    if (!hasTimeAxis()) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Space) {
        togglePause();
        event->accept();
    } else if (event->key() == Qt::Key_Home) {
        setCurrentIndex(0);
        play(0.0);
        event->accept();
    } else if (event->key() == Qt::Key_End) {
        setCurrentIndex(frameCount() - 1);
        play(0.0);
        event->accept();
    } else if (isNoRepeatKey(event->key())) {
        event->accept();
        if (event->isAutoRepeat()) {
            return;
        }
        keysPressed_.insert(event->key());
        evalKeyState();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ImageView::keyReleaseEvent(QKeyEvent* event)
{
    if (!hasTimeAxis()) {
        QWidget::keyReleaseEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Home || event->key() == Qt::Key_End) {
        event->accept();
    } else if (isNoRepeatKey(event->key())) {
        event->accept();
        if (event->isAutoRepeat()) {
            return;
        }
        if (keysPressed_.erase(event->key()) == 0) {
            keysPressed_.clear();
        }
        evalKeyState();
    } else {
        QWidget::keyReleaseEvent(event);
    }
}

void ImageView::setLevels(std::optional<ImageLevelRange> levels)
{
    imageItem_->setLevels(levels);
}

void ImageView::autoLevels()
{
    applyAutoLevels();
}

void ImageView::setLookupTable(ImageLookupTable lut)
{
    imageItem_->setLookupTable(lut);
}

void ImageView::clearLookupTable()
{
    imageItem_->clearLookupTable();
}

void ImageView::setAxisOrder(graphicsItems::ImageItem::AxisOrder axisOrder)
{
    axisOrder_ = axisOrder;
    imageItem_->setAxisOrder(axisOrder);
    updateDisplayedFrame(false, true);
}

graphicsItems::ImageItem::AxisOrder ImageView::axisOrder() const noexcept
{
    return axisOrder_;
}

widgets::GraphicsView* ImageView::getView() noexcept
{
    return graphicsView_;
}

const widgets::GraphicsView* ImageView::getView() const noexcept
{
    return graphicsView_;
}

graphicsItems::ViewBox* ImageView::getViewBox() noexcept
{
    return viewBox_;
}

const graphicsItems::ViewBox* ImageView::getViewBox() const noexcept
{
    return viewBox_;
}

graphicsItems::ImageItem* ImageView::getImageItem() noexcept
{
    return imageItem_;
}

const graphicsItems::ImageItem* ImageView::getImageItem() const noexcept
{
    return imageItem_;
}

graphicsItems::HistogramLUTItem* ImageView::getHistogram() noexcept
{
    return histogram_;
}

const graphicsItems::HistogramLUTItem* ImageView::getHistogram() const noexcept
{
    return histogram_;
}

widgets::HistogramLUTWidget* ImageView::getHistogramWidget() noexcept
{
    return histogramWidget_;
}

const widgets::HistogramLUTWidget* ImageView::getHistogramWidget() const noexcept
{
    return histogramWidget_;
}

void ImageView::setHistogramLabel(const QString& text)
{
    if (histogram_ == nullptr || histogram_->axis() == nullptr) {
        return;
    }
    histogram_->axis()->setLabel(text);
    if (text.isEmpty()) {
        histogram_->axis()->showLabel(false);
    }
    if (histogramWidget_ != nullptr) {
        histogramWidget_->setMinimumWidth(135);
    }
    if (ui_.histogramContainer != nullptr) {
        ui_.histogramContainer->setMinimumWidth(135);
    }
}

bool ImageView::hasImage() const noexcept
{
    return dataKind_ != DataKind::None;
}

widgets::PlotWidget* ImageView::getRoiPlot() noexcept
{
    return roiPlot_;
}

const widgets::PlotWidget* ImageView::getRoiPlot() const noexcept
{
    return roiPlot_;
}

graphicsItems::InfiniteLine* ImageView::timeLine() noexcept
{
    return timeLine_;
}

const graphicsItems::InfiniteLine* ImageView::timeLine() const noexcept
{
    return timeLine_;
}

bool ImageView::discreteTimeLine() const noexcept
{
    return discreteTimeLine_;
}

QPushButton* ImageView::roiButton() noexcept
{
    return ui_.roiBtn;
}

const QPushButton* ImageView::roiButton() const noexcept
{
    return ui_.roiBtn;
}

QPushButton* ImageView::menuButton() noexcept
{
    return ui_.menuBtn;
}

const QPushButton* ImageView::menuButton() const noexcept
{
    return ui_.menuBtn;
}

QWidget* ImageView::normGroup() noexcept
{
    return ui_.normGroup;
}

const QWidget* ImageView::normGroup() const noexcept
{
    return ui_.normGroup;
}

graphicsItems::ROI* ImageView::roi() noexcept
{
    return roi_;
}

const graphicsItems::ROI* ImageView::roi() const noexcept
{
    return roi_;
}

graphicsItems::ROI* ImageView::normRoi() noexcept
{
    return normRoi_;
}

const graphicsItems::ROI* ImageView::normRoi() const noexcept
{
    return normRoi_;
}

std::size_t ImageView::roiCurveCount() const noexcept
{
    return roiCurves_.size();
}

graphicsItems::PlotCurveItem* ImageView::roiCurve(std::size_t index) noexcept
{
    return index < roiCurves_.size() ? roiCurves_[index] : nullptr;
}

const graphicsItems::PlotCurveItem* ImageView::roiCurve(std::size_t index) const noexcept
{
    return index < roiCurves_.size() ? roiCurves_[index] : nullptr;
}

void ImageView::roiClicked()
{
    applyRoiPlotVisibility();
}

bool ImageView::hasTimeAxis() const noexcept
{
    if (!hasImage()) {
        return false;
    }
    const bool timeRgbStack = dataKind_ == DataKind::FloatRank4TimeRgb || dataKind_ == DataKind::DoubleRank4TimeRgb;
    return timeRgbStack || isFrameStackShape(shape_);
}

int ImageView::frameCount() const noexcept
{
    if (!hasTimeAxis()) {
        return 0;
    }
    return static_cast<int>(shape_[0]);
}

std::pair<int, double> ImageView::timeIndexFor(double time) const
{
    if (!hasTimeAxis()) {
        return {0, 0.0};
    }

    if (xvals_.empty()) {
        return {static_cast<int>(time), time};
    }

    if (xvals_.size() < 2) {
        return {0, 0.0};
    }

    int lastIndex = -1;
    for (std::size_t index = 0; index < xvals_.size(); ++index) {
        if (xvals_[index] <= time) {
            lastIndex = static_cast<int>(index);
        }
    }

    if (lastIndex < 0) {
        return {0, time};
    }

    return {lastIndex, time};
}

void ImageView::syncTimelineBounds()
{
    if (!hasTimeAxis() || roiPlot_ == nullptr || timeLine_ == nullptr) {
        if (timeLine_ != nullptr) {
            timeLine_->hide();
        }
        return;
    }

    if (xvals_.empty()) {
        assignDefaultXValues(xvals_, shape_[0]);
    }

    const double minimum = *std::min_element(xvals_.begin(), xvals_.end());
    const double maximum = *std::max_element(xvals_.begin(), xvals_.end());
    roiPlot_->setXRange(minimum, maximum);

    double start = 0.0;
    double stop = 1.0;
    if (xvals_.size() > 1) {
        start = minimum;
        stop = maximum + std::abs(xvals_.back() - xvals_.front()) * 0.02;
    } else if (xvals_.size() == 1) {
        start = xvals_.front() - 0.5;
        stop = xvals_.front() + 0.5;
    }
    timeLine_->setBounds({start, stop});
    timeLine_->setValue(0.0);
    timeLine_->show();
}

void ImageView::applyRoiPlotVisibility()
{
    if (roiPlot_ == nullptr) {
        return;
    }

    const bool roiChecked = ui_.roiBtn != nullptr && ui_.roiBtn->isChecked();
    bool showRoiPlot = roiChecked;

    if (roi_ != nullptr) {
        if (roiChecked) {
            roi_->show();
            roiPlot_->setMouseEnabled(true, true);
            for (auto* curve : roiCurves_) {
                if (curve != nullptr) {
                    curve->show();
                }
            }
            roiPlot_->showAxis(QStringLiteral("left"));
            if (roiChecked) {
                roiChanged();
            }
        } else {
            roi_->hide();
            roiPlot_->setMouseEnabled(false, false);
            for (auto* curve : roiCurves_) {
                if (curve != nullptr) {
                    curve->hide();
                }
            }
            roiPlot_->hideAxis(QStringLiteral("left"));
        }
    }

    if (hasTimeAxis()) {
        showRoiPlot = true;
        if (!xvals_.empty()) {
            const double minimum = *std::min_element(xvals_.begin(), xvals_.end());
            const double maximum = *std::max_element(xvals_.begin(), xvals_.end());
            roiPlot_->setXRange(minimum, maximum);
        }
        if (timeLine_ != nullptr) {
            timeLine_->show();
        }
        if (ui_.splitter != nullptr) {
            if (roiChecked) {
                ui_.splitter->setSizes({static_cast<int>(height() * 0.6), static_cast<int>(height() * 0.4)});
                ui_.splitter->handle(1)->setEnabled(true);
            } else {
                ui_.splitter->setSizes({height() - 35, 35});
                ui_.splitter->handle(1)->setEnabled(false);
            }
        }
    } else if (timeLine_ != nullptr) {
        timeLine_->hide();
    }

    roiPlot_->setVisible(showRoiPlot);
}

void ImageView::roiChanged()
{
    if (!hasImage() || roi_ == nullptr || imageItem_ == nullptr || roiPlot_ == nullptr) {
        return;
    }
    if (ui_.roiBtn != nullptr && !ui_.roiBtn->isChecked()) {
        return;
    }

    if (dataKind_ == DataKind::FloatRank4TimeRgb || dataKind_ == DataKind::DoubleRank4TimeRgb) {
        updateRoiCurvesFromTimeRgb();
    } else if (dataKind_ == DataKind::FloatRank3 && isFrameStackShape(shape_)) {
        updateRoiCurvesFromFrameStack();
    }
}

void ImageView::updateRoiCurvesFromTimeRgb()
{
    const std::size_t frames = shape_[0];
    const std::size_t height = shape_[1];
    const std::size_t width = shape_[2];
    const std::size_t channels = shape_[3];
    if (frames == 0 || height == 0 || width == 0 || channels == 0) {
        return;
    }

    roiCurveXBuffer_.assign(xvals_.begin(), xvals_.end());
    roiCurveYBuffers_.assign(channels, std::vector<double>(frames, 0.0));

    std::vector<float> channelSlice;
    std::vector<double> channelSliceDouble;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::size_t channel = 0; channel < channels; ++channel) {
            double sum = 0.0;
            std::size_t count = 0;

            if (dataKind_ == DataKind::FloatRank4TimeRgb) {
                channelSlice.resize(height * width);
                for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
                    for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
                        const std::size_t index = ((frame * height + axis1) * width + axis2) * channels + channel;
                        channelSlice[axis1 * width + axis2] = floatData_[index];
                    }
                }
                const auto region = roi_->getArrayRegion(
                    core::ArrayView<const float, 2>(channelSlice.data(), {height, width}), *imageItem_);
                for (const double value : region.values) {
                    sum += value;
                    ++count;
                }
            } else {
                channelSliceDouble.resize(height * width);
                for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
                    for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
                        const std::size_t index = ((frame * height + axis1) * width + axis2) * channels + channel;
                        channelSliceDouble[axis1 * width + axis2] = doubleData_[index];
                    }
                }
                const auto region = roi_->getArrayRegion(
                    core::ArrayView<const double, 2>(channelSliceDouble.data(), {height, width}), *imageItem_);
                for (const double value : region.values) {
                    sum += value;
                    ++count;
                }
            }

            if (count > 0) {
                roiCurveYBuffers_[channel][frame] = sum / static_cast<double>(count);
            }
        }
    }

    static const std::array<QColor, 4> channelColors{
        QColor(255, 0, 0),
        QColor(0, 255, 0),
        QColor(0, 0, 255),
        QColor(255, 255, 255),
    };

    while (roiCurves_.size() > channels) {
        auto* curve = roiCurves_.back();
        roiCurves_.pop_back();
        if (curve != nullptr) {
            roiPlot_->removeItem(curve);
            delete curve;
        }
    }
    while (roiCurves_.size() < channels) {
        auto* curve = roiPlot_->plot(std::span<const double>{}, std::span<const double>{});
        roiCurves_.push_back(curve);
    }

    for (std::size_t channel = 0; channel < channels; ++channel) {
        auto* curve = roiCurves_[channel];
        if (curve == nullptr) {
            continue;
        }
        curve->setData(roiCurveXBuffer_, roiCurveYBuffers_[channel]);
        QPen pen(channelColors[std::min(channel, channelColors.size() - 1)]);
        pen.setWidthF(1.0);
        curve->setPen(pen);
        if (ui_.roiBtn != nullptr && ui_.roiBtn->isChecked()) {
            curve->show();
        } else {
            curve->hide();
        }
    }
}

void ImageView::timeLineChanged()
{
    if (timeLine_ == nullptr || !hasTimeAxis()) {
        return;
    }

    if (!ignoreTimeLine_) {
        play(0.0);
    }

    const auto [index, time] = timeIndexFor(timeLine_->value());
    if (index != currentIndex_) {
        currentIndex_ = index;
        updateDisplayedFrame(false, true);
    }

    if (discreteTimeLine_) {
        const QSignalBlocker blocker(timeLine_);
        if (!xvals_.empty()) {
            timeLine_->setPos(xvals_[static_cast<std::size_t>(index)]);
        } else {
            timeLine_->setPos(static_cast<double>(index));
        }
    }

    emit sigTimeChanged(index, time);
}

void ImageView::updateDisplayedFrame(bool autoLevels, bool autoRange)
{
    if (!hasImage()) {
        return;
    }

    imageItem_->setAxisOrder(axisOrder_);

    switch (dataKind_) {
    case DataKind::UInt8Rank2:
        imageItem_->setImage(contiguousRank2(uint8Data_, shape_));
        break;
    case DataKind::UInt8Rank3:
        if (isColorImageShape(shape_)) {
            imageItem_->setImage(contiguousRank3(uint8Data_, shape_));
        } else {
            imageItem_->setImage(frameView(uint8Data_, shape_, currentIndex_));
        }
        break;
    case DataKind::UInt16Rank2:
        imageItem_->setImage(contiguousRank2(uint16Data_, shape_));
        break;
    case DataKind::UInt16Rank3:
        if (isColorImageShape(shape_)) {
            imageItem_->setImage(contiguousRank3(uint16Data_, shape_));
        } else {
            imageItem_->setImage(frameView(uint16Data_, shape_, currentIndex_));
        }
        break;
    case DataKind::FloatRank2:
        imageItem_->setImage(contiguousRank2(floatData_, shape_));
        break;
    case DataKind::FloatRank3:
        if (isFrameStackShape(shape_)) {
            refreshProcessedImage();
            imageItem_->setImage(frameView(processedFloatData_, shape_, currentIndex_));
        } else {
            imageItem_->setImage(frameView(floatData_, shape_, currentIndex_));
        }
        break;
    case DataKind::FloatRank4TimeRgb:
        updateDisplayedRgbFrame(autoLevels, autoRange);
        return;
    case DataKind::DoubleRank4TimeRgb:
        updateDisplayedRgbFrame(autoLevels, autoRange);
        return;
    case DataKind::None:
        break;
    }

    if (autoLevels) {
        applyAutoLevels();
    }

    if (autoRange && viewBox_ != nullptr && imageItem_->hasImage()) {
        viewBox_->autoRange();
    }

    if (histogram_ != nullptr) {
        histogram_->imageChanged(false, autoRange);
    }
}

void ImageView::updateDisplayedRgbFrame(bool autoLevels, bool autoRange)
{
    switch (dataKind_) {
    case DataKind::FloatRank4TimeRgb:
        extractRgbFrame(floatData_, shape_, currentIndex_, rgbDisplayBuffer_);
        break;
    case DataKind::DoubleRank4TimeRgb:
        extractRgbFrame(doubleData_, shape_, currentIndex_, rgbDisplayBuffer_);
        break;
    default:
        return;
    }

    const std::array<std::size_t, 3> rgbShape{shape_[1], shape_[2], shape_[3]};
    imageItem_->setImage(core::ArrayView<const std::uint8_t, 3>(rgbDisplayBuffer_.data(), rgbShape));

    if (autoLevels) {
        applyAutoLevels();
    }

    if (autoRange && viewBox_ != nullptr && imageItem_->hasImage()) {
        viewBox_->autoRange();
    }

    if (histogram_ != nullptr) {
        histogram_->imageChanged(false, autoRange);
    }
}

void ImageView::applyAutoLevels()
{
    if (levelMode_ == QStringLiteral("rgba")) {
        const std::vector<ImageLevelRange> channelLevels = computeRgbaAutoLevels();
        if (!channelLevels.empty()) {
            syncRgbaHistogramLevels(channelLevels);
            imageItem_->setChannelLevels(channelLevels);
        }
        return;
    }

    const std::optional<ImageLevelRange> levels = computeAutoLevels();
    if (levels.has_value()) {
        imageItem_->setLevels(*levels);
    }
}

void ImageView::syncRgbaHistogramLevels(const std::vector<ImageLevelRange>& channelLevels)
{
    if (histogram_ == nullptr) {
        return;
    }
    std::vector<std::pair<double, double>> levels;
    levels.reserve(channelLevels.size());
    for (const auto& range : channelLevels) {
        levels.emplace_back(range.minimum, range.maximum);
    }
    histogram_->setChannelLevels(levels);
}

std::vector<ImageLevelRange> ImageView::computeRgbaAutoLevels() const
{
    std::vector<ImageLevelRange> levels;
    switch (dataKind_) {
    case DataKind::FloatRank4TimeRgb: {
        const std::size_t channels = shape_[3];
        levels.resize(channels);
        for (std::size_t channel = 0; channel < channels; ++channel) {
            double minimum = std::numeric_limits<double>::infinity();
            double maximum = -std::numeric_limits<double>::infinity();
            for (std::size_t frame = 0; frame < shape_[0]; ++frame) {
                for (std::size_t axis1 = 0; axis1 < shape_[1]; ++axis1) {
                    for (std::size_t axis2 = 0; axis2 < shape_[2]; ++axis2) {
                        const std::size_t index = ((frame * shape_[1] + axis1) * shape_[2] + axis2) * channels + channel;
                        const double value = static_cast<double>(floatData_[index]);
                        minimum = std::min(minimum, value);
                        maximum = std::max(maximum, value);
                    }
                }
            }
            levels[channel] = ImageLevelRange{minimum, maximum};
        }
        break;
    }
    case DataKind::DoubleRank4TimeRgb: {
        const std::size_t channels = shape_[3];
        levels.resize(channels);
        for (std::size_t channel = 0; channel < channels; ++channel) {
            double minimum = std::numeric_limits<double>::infinity();
            double maximum = -std::numeric_limits<double>::infinity();
            for (std::size_t frame = 0; frame < shape_[0]; ++frame) {
                for (std::size_t axis1 = 0; axis1 < shape_[1]; ++axis1) {
                    for (std::size_t axis2 = 0; axis2 < shape_[2]; ++axis2) {
                        const std::size_t index = ((frame * shape_[1] + axis1) * shape_[2] + axis2) * channels + channel;
                        const double value = doubleData_[index];
                        minimum = std::min(minimum, value);
                        maximum = std::max(maximum, value);
                    }
                }
            }
            levels[channel] = ImageLevelRange{minimum, maximum};
        }
        break;
    }
    case DataKind::UInt8Rank3:
    case DataKind::UInt16Rank3: {
        const std::vector<std::uint8_t>* uint8 = dataKind_ == DataKind::UInt8Rank3 ? &uint8Data_ : nullptr;
        const std::vector<std::uint16_t>* uint16 = dataKind_ == DataKind::UInt16Rank3 ? &uint16Data_ : nullptr;
        const std::size_t channels = shape_[2];
        levels.resize(channels);
        for (std::size_t channel = 0; channel < channels; ++channel) {
            double minimum = std::numeric_limits<double>::infinity();
            double maximum = -std::numeric_limits<double>::infinity();
            for (std::size_t row = 0; row < shape_[0]; ++row) {
                for (std::size_t col = 0; col < shape_[1]; ++col) {
                    const std::size_t index = (row * shape_[1] + col) * channels + channel;
                    const double value = uint8 != nullptr ? static_cast<double>((*uint8)[index])
                                                          : static_cast<double>((*uint16)[index]);
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                }
            }
            levels[channel] = ImageLevelRange{minimum, maximum};
        }
        break;
    }
    default:
        break;
    }
    return levels;
}

std::optional<ImageLevelRange> ImageView::computeAutoLevels() const
{
    switch (dataKind_) {
    case DataKind::UInt8Rank2:
        return autoLevelsFor(uint8Data_);
    case DataKind::UInt8Rank3:
        return autoLevelsFor(uint8Data_);
    case DataKind::UInt16Rank2:
        return autoLevelsFor(uint16Data_);
    case DataKind::UInt16Rank3:
        return autoLevelsFor(uint16Data_);
    case DataKind::FloatRank2:
        return autoLevelsFor(floatData_);
    case DataKind::FloatRank3:
        if (isFrameStackShape(shape_) && normalizationEnabled()) {
            refreshProcessedImage();
            return autoLevelsFor(processedFloatData_);
        }
        return autoLevelsFor(floatData_);
    case DataKind::FloatRank4TimeRgb:
        return autoLevelsFor(floatData_);
    case DataKind::DoubleRank4TimeRgb:
        return autoLevelsFor(doubleData_);
    case DataKind::None:
        return std::nullopt;
    }
    return std::nullopt;
}

void ImageView::buildMenu()
{
    if (menu_ != nullptr) {
        return;
    }

    menu_ = new QMenu(this);
    normAction_ = menu_->addAction(tr("Normalization"));
    normAction_->setCheckable(true);
    connect(normAction_, &QAction::toggled, this, &ImageView::normToggled);
    exportAction_ = menu_->addAction(tr("Export"));
    connect(exportAction_, &QAction::triggered, this, &ImageView::exportClicked);
}

QMenu* ImageView::menu()
{
    if (menu_ == nullptr) {
        buildMenu();
    }
    return menu_;
}

const QMenu* ImageView::menu() const
{
    return menu_;
}

void ImageView::menuClicked()
{
    if (menu_ == nullptr) {
        buildMenu();
    }
    menu_->popup(mapToGlobal(ui_.menuBtn != nullptr ? ui_.menuBtn->rect().bottomLeft() : QPoint()));
}

void ImageView::normToggled(bool enabled)
{
    if (ui_.normGroup != nullptr) {
        ui_.normGroup->setVisible(enabled);
    }
    if (normRoi_ != nullptr) {
        normRoi_->setVisible(enabled && ui_.normROICheck != nullptr && ui_.normROICheck->isChecked());
    }
    if (normRgn_ != nullptr) {
        normRgn_->setVisible(enabled && ui_.normTimeRangeCheck != nullptr && ui_.normTimeRangeCheck->isChecked());
    }
}

void ImageView::normRadioChanged()
{
    invalidateProcessedImage();
    updateDisplayedFrame(true, false);
    roiChanged();
}

void ImageView::updateNorm()
{
    if (normRgn_ != nullptr) {
        normRgn_->setVisible(ui_.normTimeRangeCheck != nullptr && ui_.normTimeRangeCheck->isChecked()
                              && ui_.normGroup != nullptr && ui_.normGroup->isVisible());
    }
    if (normRoi_ != nullptr) {
        normRoi_->setVisible(ui_.normROICheck != nullptr && ui_.normROICheck->isChecked()
                             && ui_.normGroup != nullptr && ui_.normGroup->isVisible());
    }

    if (ui_.normOffRadio != nullptr && ui_.normOffRadio->isChecked()) {
        return;
    }

    invalidateProcessedImage();
    updateDisplayedFrame(true, false);
    roiChanged();
}

bool ImageView::normalizationEnabled() const noexcept
{
    return ui_.normOffRadio == nullptr || !ui_.normOffRadio->isChecked();
}

void ImageView::invalidateProcessedImage()
{
    processedFloatDirty_ = true;
}

const std::vector<float>& ImageView::processedFloatData() const
{
    refreshProcessedImage();
    return processedFloatData_;
}

core::ArrayView<const float, 3> ImageView::processedFloatStackView() const
{
    refreshProcessedImage();
    return core::ArrayView<const float, 3>(processedFloatData_.data(), {shape_[0], shape_[1], shape_[2]});
}

void ImageView::refreshProcessedImage() const
{
    if (!processedFloatDirty_ || dataKind_ != DataKind::FloatRank3 || !isFrameStackShape(shape_)) {
        return;
    }

    auto* self = const_cast<ImageView*>(this);
    self->normalizeFloatStack(processedFloatData_);
    processedFloatDirty_ = false;
}

void ImageView::normalizeFloatStack(std::vector<float>& output) const
{
    output = floatData_;
    if (!normalizationEnabled() || dataKind_ != DataKind::FloatRank3 || !isFrameStackShape(shape_)) {
        return;
    }

    const std::size_t frames = shape_[0];
    const std::size_t height = shape_[1];
    const std::size_t width = shape_[2];
    const bool divide = ui_.normDivideRadio != nullptr && ui_.normDivideRadio->isChecked();

    auto applyFactor = [&](std::size_t frame, double factor) {
        if (factor == 0.0) {
            return;
        }
        for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
            for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
                const std::size_t index = (frame * height + axis1) * width + axis2;
                if (divide) {
                    output[index] = static_cast<float>(output[index] / factor);
                } else {
                    output[index] = static_cast<float>(output[index] - factor);
                }
            }
        }
    };

    if (ui_.normTimeRangeCheck != nullptr && ui_.normTimeRangeCheck->isChecked() && normRgn_ != nullptr) {
        const auto region = normRgn_->getRegion();
        const auto [startIndex, startTime] = timeIndexFor(region.first);
        const auto [endIndex, endTime] = timeIndexFor(region.second);
        Q_UNUSED(startTime);
        Q_UNUSED(endTime);
        const std::size_t start = static_cast<std::size_t>(std::clamp(startIndex, 0, static_cast<int>(frames) - 1));
        const std::size_t end = static_cast<std::size_t>(std::clamp(endIndex, 0, static_cast<int>(frames) - 1));
        std::vector<double> mean(height * width, 0.0);
        const std::size_t count = end >= start ? end - start + 1 : 0;
        if (count > 0) {
            for (std::size_t frame = start; frame <= end; ++frame) {
                for (std::size_t pixel = 0; pixel < height * width; ++pixel) {
                    mean[pixel] += static_cast<double>(floatData_[frame * height * width + pixel]);
                }
            }
            for (double& value : mean) {
                value /= static_cast<double>(count);
            }
            for (std::size_t frame = 0; frame < frames; ++frame) {
                for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
                    for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
                        const std::size_t pixel = axis1 * width + axis2;
                        const std::size_t index = frame * height * width + pixel;
                        const double factor = mean[pixel];
                        if (divide) {
                            if (factor != 0.0) {
                                output[index] = static_cast<float>(output[index] / factor);
                            }
                        } else {
                            output[index] = static_cast<float>(output[index] - factor);
                        }
                    }
                }
            }
        }
    }

    if (ui_.normFrameCheck != nullptr && ui_.normFrameCheck->isChecked()) {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            double sum = 0.0;
            for (std::size_t pixel = 0; pixel < height * width; ++pixel) {
                sum += static_cast<double>(floatData_[frame * height * width + pixel]);
            }
            const double mean = sum / static_cast<double>(height * width);
            applyFactor(frame, mean);
        }
    }

    if (ui_.normROICheck != nullptr && ui_.normROICheck->isChecked() && normRoi_ != nullptr && imageItem_ != nullptr) {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            std::vector<float> slice(height * width);
            for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
                for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
                    slice[axis1 * width + axis2] = output[frame * height * width + axis1 * width + axis2];
                }
            }
            const auto region = normRoi_->getArrayRegion(
                core::ArrayView<const float, 2>(slice.data(), {height, width}), *imageItem_);
            double sum = 0.0;
            for (const double value : region.values) {
                sum += value;
            }
            const double mean = region.values.empty() ? 0.0 : sum / static_cast<double>(region.values.size());
            applyFactor(frame, mean);
        }
    }
}

double ImageView::normalizedSamplePixel(int frame, int row, int col) const
{
    if (dataKind_ != DataKind::FloatRank3 || !isFrameStackShape(shape_)) {
        return 0.0;
    }
    refreshProcessedImage();
    const std::size_t frameIndex = static_cast<std::size_t>(std::clamp(frame, 0, frameCount() - 1));
    const std::size_t rowIndex = static_cast<std::size_t>(std::max(row, 0));
    const std::size_t colIndex = static_cast<std::size_t>(std::max(col, 0));
    if (rowIndex >= shape_[1] || colIndex >= shape_[2]) {
        return 0.0;
    }
    const std::size_t index = (frameIndex * shape_[1] + rowIndex) * shape_[2] + colIndex;
    return static_cast<double>(processedFloatData_[index]);
}

bool ImageView::saveCurrentImageItem(const QString& fileName) const
{
    if (imageItem_ == nullptr) {
        return false;
    }
    if (imageItem_->renderRequired()) {
        if (!const_cast<graphicsItems::ImageItem*>(imageItem_)->render()) {
            return false;
        }
    }
    return imageItem_->cachedImage().save(fileName);
}

void ImageView::exportImage(const QString& fileName)
{
    if (!hasImage() || imageItem_ == nullptr) {
        return;
    }

    if (hasTimeAxis() && (dataKind_ == DataKind::FloatRank3 || dataKind_ == DataKind::FloatRank4TimeRgb
                          || dataKind_ == DataKind::DoubleRank4TimeRgb)) {
        const QFileInfo info(fileName);
        const QString base = info.path() + QLatin1Char('/') + info.completeBaseName();
        const QString ext = info.suffix().isEmpty() ? QStringLiteral(".png") : QStringLiteral(".") + info.suffix();
        const int frames = frameCount();
        const int digits = frames > 0 ? static_cast<int>(std::log10(static_cast<double>(frames))) + 1 : 1;
        const int savedIndex = currentIndex_;

        for (int frame = 0; frame < frames; ++frame) {
            setCurrentIndex(frame);
            const QString indexedName = QStringLiteral("%1%2%3").arg(base).arg(frame, digits, 10, QLatin1Char('0')).arg(ext);
            saveCurrentImageItem(indexedName);
        }
        setCurrentIndex(savedIndex);
        return;
    }

    saveCurrentImageItem(fileName);
}

void ImageView::exportClicked()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Image"));
    if (fileName.isEmpty()) {
        return;
    }
    exportImage(fileName);
}

void ImageView::updateRoiCurvesFromFrameStack()
{
    const std::size_t frames = shape_[0];
    const std::size_t height = shape_[1];
    const std::size_t width = shape_[2];
    if (frames == 0 || height == 0 || width == 0) {
        return;
    }

    refreshProcessedImage();
    const std::vector<float>& source = normalizationEnabled() ? processedFloatData_ : floatData_;

    if (xvals_.empty()) {
        roiCurveXBuffer_.assign(frames, 0.0);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            roiCurveXBuffer_[frame] = static_cast<double>(frame);
        }
    } else {
        roiCurveXBuffer_.assign(xvals_.begin(), xvals_.end());
    }

    std::vector<double> curveY(frames, 0.0);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        std::vector<float> slice(height * width);
        for (std::size_t axis1 = 0; axis1 < height; ++axis1) {
            for (std::size_t axis2 = 0; axis2 < width; ++axis2) {
                slice[axis1 * width + axis2] = source[frame * height * width + axis1 * width + axis2];
            }
        }
        const auto region = roi_->getArrayRegion(
            core::ArrayView<const float, 2>(slice.data(), {height, width}), *imageItem_);
        double sum = 0.0;
        for (const double value : region.values) {
            sum += value;
        }
        curveY[frame] = region.values.empty() ? 0.0 : sum / static_cast<double>(region.values.size());
    }

    while (roiCurves_.size() > 1) {
        auto* curve = roiCurves_.back();
        roiCurves_.pop_back();
        if (curve != nullptr) {
            roiPlot_->removeItem(curve);
            delete curve;
        }
    }
    while (roiCurves_.empty()) {
        auto* curve = roiPlot_->plot(std::span<const double>{}, std::span<const double>{});
        roiCurves_.push_back(curve);
    }

    auto* curve = roiCurves_.front();
    if (curve != nullptr) {
        curve->setData(roiCurveXBuffer_, curveY);
        QPen pen(Qt::white);
        pen.setWidthF(1.0);
        curve->setPen(pen);
        if (ui_.roiBtn != nullptr && ui_.roiBtn->isChecked()) {
            curve->show();
        } else {
            curve->hide();
        }
    }
}

} // namespace cppqtgraph::imageview
