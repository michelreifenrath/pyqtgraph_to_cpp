#include <cppqtgraph/graphicsItems/InfiniteLine.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QPointF>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace {

constexpr double kTolerance = 1.0e-9;
constexpr std::size_t kFrames = 4;
constexpr std::size_t kHeight = 2;
constexpr std::size_t kWidth = 2;
constexpr std::size_t kChannels = 3;

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }
    return true;
}

#define CHECK(expression) \
    do { \
        if (!check((expression), #expression, __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (false)

class ApplicationGuard {
public:
    ApplicationGuard(int& argc, char** argv)
    {
        if (QApplication::instance() == nullptr) {
            application_ = std::make_unique<QApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QApplication> application_;
};

bool nearlyEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= kTolerance;
}

std::vector<float> makeFrameData()
{
    std::vector<float> data(kFrames * kHeight * kWidth * kChannels);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        const float marker = static_cast<float>(frame * 10);
        for (std::size_t pixel = 0; pixel < kHeight * kWidth * kChannels; ++pixel) {
            data[frame * kHeight * kWidth * kChannels + pixel] = marker + static_cast<float>(pixel);
        }
    }
    return data;
}

std::vector<double> makeNonUniformXVals()
{
    return {0.0, 1.5, 3.0, 5.0};
}

std::unique_ptr<cppqtgraph::imageview::ImageView> makeTimelineImageView(bool discreteTimeLine)
{
    const std::vector<float> data = makeFrameData();
    const std::vector<double> xvals = makeNonUniformXVals();
    auto imageView = std::make_unique<cppqtgraph::imageview::ImageView>(
        nullptr, QStringLiteral("rgba"), discreteTimeLine);
    imageView->setImage(cppqtgraph::core::ArrayView<const float, 4>(
                            data.data(), {kFrames, kHeight, kWidth, kChannels}),
                        cppqtgraph::core::ArrayView<const double, 1>(xvals.data(), {xvals.size()}),
                        false,
                        false);
    return imageView;
}

bool testTimelineHostVisible()
{
    const auto imageView = makeTimelineImageView(false);
    imageView->show();
    QTest::qWait(0);
    CHECK(imageView->getRoiPlot() != nullptr);
    CHECK(imageView->timeLine() != nullptr);
    CHECK(imageView->timeLine()->movable());
    CHECK(!imageView->getRoiPlot()->isHidden());
    return true;
}

bool testTimelineMapsToFrameByXVals()
{
    const auto imageView = makeTimelineImageView(false);
    auto* timeLine = imageView->timeLine();
    CHECK(timeLine != nullptr);

    QSignalSpy spy(imageView.get(), &cppqtgraph::imageview::ImageView::sigTimeChanged);

    timeLine->setValue(2.0);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 1);
    CHECK(nearlyEqual(timeLine->value(), 2.0));
    CHECK(spy.size() >= 1);
    const auto last = spy.last();
    CHECK(last.at(0).toInt() == 1);
    CHECK(nearlyEqual(last.at(1).toDouble(), 2.0));

    timeLine->setValue(0.5);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 0);
    CHECK(nearlyEqual(timeLine->value(), 0.5));
    return true;
}

bool testDiscreteTimelineSnapsToXVals()
{
    const auto imageView = makeTimelineImageView(true);
    auto* timeLine = imageView->timeLine();
    CHECK(timeLine != nullptr);

    timeLine->setValue(2.0);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 1);
    CHECK(nearlyEqual(timeLine->value(), 1.5));

    timeLine->setValue(4.2);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 2);
    CHECK(nearlyEqual(timeLine->value(), 3.0));
    return true;
}

bool testSetCurrentIndexClipsAndUpdatesTimeline()
{
    const auto imageView = makeTimelineImageView(false);
    auto* timeLine = imageView->timeLine();
    CHECK(timeLine != nullptr);

    QSignalSpy spy(imageView.get(), &cppqtgraph::imageview::ImageView::sigTimeChanged);

    imageView->setCurrentIndex(-1);
    CHECK(imageView->currentIndex() == 0);
    CHECK(nearlyEqual(timeLine->value(), 0.0));

    imageView->setCurrentIndex(99);
    CHECK(imageView->currentIndex() == static_cast<int>(kFrames - 1));
    CHECK(nearlyEqual(timeLine->value(), 5.0));
    CHECK(spy.size() >= 1);
    CHECK(spy.last().at(0).toInt() == static_cast<int>(kFrames - 1));
    CHECK(nearlyEqual(spy.last().at(1).toDouble(), 5.0));

    imageView->setCurrentIndex(2);
    CHECK(imageView->currentIndex() == 2);
    CHECK(nearlyEqual(timeLine->value(), 3.0));
    CHECK(spy.size() >= 2);
    CHECK(spy.last().at(0).toInt() == 2);
    CHECK(nearlyEqual(spy.last().at(1).toDouble(), 3.0));
    return true;
}

bool testNonMonotonicXValsSelectLastMatchingIndex()
{
    const std::vector<float> data = makeFrameData();
    const std::vector<double> xvals = {0.0, 5.0, 1.5, 3.0};
    auto imageView = std::make_unique<cppqtgraph::imageview::ImageView>(
        nullptr, QStringLiteral("rgba"), false);
    imageView->setImage(cppqtgraph::core::ArrayView<const float, 4>(
                            data.data(), {kFrames, kHeight, kWidth, kChannels}),
                        cppqtgraph::core::ArrayView<const double, 1>(xvals.data(), {xvals.size()}),
                        false,
                        false);

    auto* timeLine = imageView->timeLine();
    CHECK(timeLine != nullptr);

    timeLine->setValue(4.0);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 3);
    CHECK(nearlyEqual(timeLine->value(), 4.0));
    return true;
}

bool testTimelineDragUpdatesIndex()
{
    const auto imageView = makeTimelineImageView(false);
    imageView->resize(800, 800);
    imageView->show();
    QTest::qWait(200);

    auto* roiPlot = imageView->getRoiPlot();
    auto* timeLine = imageView->timeLine();
    CHECK(roiPlot != nullptr);
    CHECK(timeLine != nullptr);

    QSignalSpy spy(imageView.get(), &cppqtgraph::imageview::ImageView::sigTimeChanged);

    auto* viewBox = roiPlot->getPlotItem()->getViewBox();
    const QPointF startScene = timeLine->sceneBoundingRect().center();
    const double viewY = viewBox->mapSceneToView(startScene).y();
    const QPointF endScene = viewBox->mapViewToScene(QPointF(3.5, viewY));

    QWidget* viewport = roiPlot->viewport();
    const QPoint start = roiPlot->mapFromScene(startScene);
    const QPoint end = roiPlot->mapFromScene(endScene);

    QTest::mouseMove(viewport, start);
    QTest::mousePress(viewport, Qt::LeftButton, Qt::NoModifier, start);
    QTest::qWait(600);
    QTest::mouseMove(viewport, end);
    QTest::mouseRelease(viewport, Qt::LeftButton, Qt::NoModifier, end);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 2);
    CHECK(spy.size() >= 1);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard guard(argc, argv);

    if (!testTimelineHostVisible() || !testTimelineMapsToFrameByXVals() || !testDiscreteTimelineSnapsToXVals()
        || !testSetCurrentIndexClipsAndUpdatesTimeline() || !testNonMonotonicXValsSelectLastMatchingIndex()
        || !testTimelineDragUpdatesIndex()) {
        return 1;
    }

    return 0;
}
