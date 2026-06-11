// Source note: translated/adapted from PyQtGraph examples/CLIexample.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/core/ArrayView.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <random>
#include <vector>

namespace cppqtgraph::examples {

namespace {

constexpr std::size_t kPlotPointCount = 1000;
constexpr std::size_t kImageSize = 500;
constexpr std::uint32_t kDeterministicSeed = 0x434C49U;

std::vector<double> makeDeterministicPlotData()
{
    std::mt19937 generator(kDeterministicSeed);
    std::normal_distribution<double> distribution(0.0, 1.0);

    std::vector<double> y;
    y.reserve(kPlotPointCount);
    for (std::size_t index = 0; index < kPlotPointCount; ++index) {
        y.push_back(distribution(generator));
    }
    return y;
}

std::vector<float> makeDeterministicImageData()
{
    std::mt19937 generator(kDeterministicSeed);
    std::normal_distribution<double> distribution(0.0, 1.0);

    for (std::size_t index = 0; index < kPlotPointCount; ++index) {
        (void)distribution(generator);
    }

    std::vector<float> image;
    image.reserve(kImageSize * kImageSize);
    for (std::size_t index = 0; index < kImageSize * kImageSize; ++index) {
        image.push_back(static_cast<float>(distribution(generator)));
    }
    return image;
}

} // namespace

struct CLIexampleState {
    std::vector<double> plotY;
    std::vector<float> image;
};

struct CLIexample {
    std::unique_ptr<widgets::PlotWidget> plotWidget;
    std::unique_ptr<imageview::ImageView> imageWidget;
    graphicsItems::PlotCurveItem* plotCurve = nullptr;
    imageview::ImageView* imageView = nullptr;
    std::shared_ptr<CLIexampleState> state;
};

std::size_t cliExamplePlotPointCount() noexcept
{
    return kPlotPointCount;
}

std::size_t cliExampleImageWidth() noexcept
{
    return kImageSize;
}

std::size_t cliExampleImageHeight() noexcept
{
    return kImageSize;
}

CLIexample createCLIexample()
{
    auto state = std::make_shared<CLIexampleState>();
    state->plotY = makeDeterministicPlotData();
    state->image = makeDeterministicImageData();

    auto plotWidget = std::make_unique<widgets::PlotWidget>();
    plotWidget->setWindowTitle(QStringLiteral("Simplest possible plotting example"));
    plotWidget->resize(800, 600);

    auto* plotCurve = new graphicsItems::PlotCurveItem(plotWidget->getPlotItem());
    plotCurve->setData(state->plotY);

    auto imageWidget = std::make_unique<imageview::ImageView>();
    imageWidget->setWindowTitle(QStringLiteral("Simplest possible image example"));
    imageWidget->setAxisOrder(graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageWidget->setImage(
        core::ArrayView<const float, 2>(state->image.data(), {kImageSize, kImageSize}),
        true,
        true);

    auto* imageView = imageWidget.get();
    return {.plotWidget = std::move(plotWidget),
            .imageWidget = std::move(imageWidget),
            .plotCurve = plotCurve,
            .imageView = imageView,
            .state = std::move(state)};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_CLIEXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    auto example = cppqtgraph::examples::createCLIexample();
    example.plotWidget->show();
    example.imageWidget->show();
    return QApplication::exec();
}
#endif
