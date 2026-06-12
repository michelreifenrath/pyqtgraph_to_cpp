#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "ImageViewTemplate_generic.hpp"
#include "cppqtgraph/core/ArrayView.hpp"
#include "cppqtgraph/functions.hpp"
#include "cppqtgraph/graphicsItems/ImageItem.hpp"

#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

class QPushButton;

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cppqtgraph::graphicsItems {
class HistogramLUTItem;
class InfiniteLine;
class PlotCurveItem;
class ROI;
class ViewBox;
} // namespace cppqtgraph::graphicsItems

namespace cppqtgraph::widgets {
class GraphicsView;
class HistogramLUTWidget;
class PlotWidget;
} // namespace cppqtgraph::widgets

namespace cppqtgraph::imageview {

class ImageView : public QWidget {
    Q_OBJECT

public:
    explicit ImageView(QWidget* parent = nullptr,
                       const QString& levelMode = QStringLiteral("mono"),
                       bool discreteTimeLine = false);
    ~ImageView() override;

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&&) = delete;
    ImageView& operator=(ImageView&&) = delete;

    void setImage(core::ArrayView<const std::uint8_t, 2> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const std::uint8_t, 3> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const std::uint16_t, 2> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const std::uint16_t, 3> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const float, 2> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const float, 3> image, bool autoLevels = false, bool autoRange = true);
    void setImage(core::ArrayView<const float, 4> image,
                  core::ArrayView<const double, 1> xvals = {},
                  bool autoLevels = false,
                  bool autoRange = true);
    void setImage(core::ArrayView<const double, 4> image,
                  core::ArrayView<const double, 1> xvals = {},
                  bool autoLevels = false,
                  bool autoRange = true);
    void clearImage();

    void setCurrentIndex(int index);
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] std::span<const double> xValues() const noexcept;

    void play(std::optional<double> rate = std::nullopt);

    void setLevels(std::optional<ImageLevelRange> levels);
    void autoLevels();
    void setLookupTable(ImageLookupTable lut);
    void clearLookupTable();

    void setAxisOrder(graphicsItems::ImageItem::AxisOrder axisOrder);
    [[nodiscard]] graphicsItems::ImageItem::AxisOrder axisOrder() const noexcept;

    [[nodiscard]] widgets::GraphicsView* getView() noexcept;
    [[nodiscard]] const widgets::GraphicsView* getView() const noexcept;
    [[nodiscard]] graphicsItems::ViewBox* getViewBox() noexcept;
    [[nodiscard]] const graphicsItems::ViewBox* getViewBox() const noexcept;
    [[nodiscard]] graphicsItems::ImageItem* getImageItem() noexcept;
    [[nodiscard]] const graphicsItems::ImageItem* getImageItem() const noexcept;
    [[nodiscard]] graphicsItems::HistogramLUTItem* getHistogram() noexcept;
    [[nodiscard]] const graphicsItems::HistogramLUTItem* getHistogram() const noexcept;
    [[nodiscard]] widgets::HistogramLUTWidget* getHistogramWidget() noexcept;
    [[nodiscard]] const widgets::HistogramLUTWidget* getHistogramWidget() const noexcept;

    void setHistogramLabel(const QString& text = QString{});

    [[nodiscard]] bool hasImage() const noexcept;

    [[nodiscard]] widgets::PlotWidget* getRoiPlot() noexcept;
    [[nodiscard]] const widgets::PlotWidget* getRoiPlot() const noexcept;
    [[nodiscard]] graphicsItems::InfiniteLine* timeLine() noexcept;
    [[nodiscard]] const graphicsItems::InfiniteLine* timeLine() const noexcept;
    [[nodiscard]] bool discreteTimeLine() const noexcept;

    [[nodiscard]] QPushButton* roiButton() noexcept;
    [[nodiscard]] const QPushButton* roiButton() const noexcept;
    [[nodiscard]] graphicsItems::ROI* roi() noexcept;
    [[nodiscard]] const graphicsItems::ROI* roi() const noexcept;
    [[nodiscard]] std::size_t roiCurveCount() const noexcept;
    [[nodiscard]] graphicsItems::PlotCurveItem* roiCurve(std::size_t index) noexcept;
    [[nodiscard]] const graphicsItems::PlotCurveItem* roiCurve(std::size_t index) const noexcept;

public slots:
    void roiClicked();

signals:
    void sigTimeChanged(int index, double time);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void timeLineChanged();
    void playbackTimeout();
    void roiChanged();

private:
    enum class DataKind {
        None,
        UInt8Rank2,
        UInt8Rank3,
        UInt16Rank2,
        UInt16Rank3,
        FloatRank2,
        FloatRank3,
        FloatRank4TimeRgb,
        DoubleRank4TimeRgb,
    };

    template <typename T, std::size_t Rank>
    void setImageImpl(core::ArrayView<const T, Rank> image,
                      DataKind kind,
                      std::vector<T>& destination,
                      bool autoLevels,
                      bool autoRange);

    template <typename T>
    void setImageTimeRgbImpl(core::ArrayView<const T, 4> image,
                             core::ArrayView<const double, 1> xvals,
                             DataKind kind,
                             std::vector<T>& destination,
                             bool autoLevels,
                             bool autoRange);

    void updateDisplayedFrame(bool autoLevels, bool autoRange);
    void updateDisplayedRgbFrame(bool autoLevels, bool autoRange);
    void applyAutoLevels();
    [[nodiscard]] std::optional<ImageLevelRange> computeAutoLevels() const;
    [[nodiscard]] std::vector<ImageLevelRange> computeRgbaAutoLevels() const;
    void syncRgbaHistogramLevels(const std::vector<ImageLevelRange>& channelLevels);
    [[nodiscard]] bool hasTimeAxis() const noexcept;
    [[nodiscard]] int frameCount() const noexcept;
    [[nodiscard]] std::pair<int, double> timeIndexFor(double time) const;
    void syncTimelineBounds();
    void applyRoiPlotVisibility();
    void updateRoiCurvesFromTimeRgb();
    void togglePause();
    void jumpFrames(int frameDelta);
    void evalKeyState();
    [[nodiscard]] static double playbackClockSeconds();
    [[nodiscard]] bool isNoRepeatKey(int key) const noexcept;

    Ui_Form ui_;
    QString levelMode_;
    bool discreteTimeLine_ = false;
    bool ignoreTimeLine_ = false;
    DataKind dataKind_ = DataKind::None;
    int currentIndex_ = 0;
    std::array<std::size_t, 4> shape_{};
    std::vector<std::uint8_t> uint8Data_;
    std::vector<std::uint16_t> uint16Data_;
    std::vector<float> floatData_;
    std::vector<double> doubleData_;
    std::vector<double> xvals_;
    std::vector<std::uint8_t> rgbDisplayBuffer_;
    graphicsItems::ImageItem::AxisOrder axisOrder_ = graphicsItems::ImageItem::AxisOrder::ColMajor;

    widgets::GraphicsView* graphicsView_ = nullptr;
    graphicsItems::ViewBox* viewBox_ = nullptr;
    graphicsItems::ImageItem* imageItem_ = nullptr;
    widgets::HistogramLUTWidget* histogramWidget_ = nullptr;
    graphicsItems::HistogramLUTItem* histogram_ = nullptr;
    widgets::PlotWidget* roiPlot_ = nullptr;
    graphicsItems::InfiniteLine* timeLine_ = nullptr;
    graphicsItems::ROI* roi_ = nullptr;
    std::vector<graphicsItems::PlotCurveItem*> roiCurves_;
    std::vector<double> roiCurveXBuffer_;
    std::vector<std::vector<double>> roiCurveYBuffers_;

    QTimer playTimer_;
    double playRate_ = 0.0;
    std::optional<double> pausedPlayRate_;
    double fps_ = 1.0;
    double lastPlayTime_ = 0.0;
    std::unordered_set<int> keysPressed_;
};

} // namespace cppqtgraph::imageview
