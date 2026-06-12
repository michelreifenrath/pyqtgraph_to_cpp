// Source note: translated/adapted from PyQtGraph examples/ImageView.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/imageview/ImageView.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>

#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

namespace cppqtgraph::examples {
namespace {

constexpr std::size_t kFrames = 100;
constexpr std::size_t kHeight = 200;
constexpr std::size_t kWidth = 200;
constexpr std::size_t kChannels = 3;

std::vector<float> makeExampleImageData()
{
    std::vector<float> data(kFrames * kHeight * kWidth * kChannels);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        const double redBase = 90.0 + (150.0 - 90.0) * static_cast<double>(frame) / static_cast<double>(kFrames - 1);
        const double greenBase = 90.0 + (180.0 - 90.0) * static_cast<double>(frame) / static_cast<double>(kFrames - 1);
        const double blueBase = 180.0 + (90.0 - 180.0) * static_cast<double>(frame) / static_cast<double>(kFrames - 1);
        for (std::size_t y = 0; y < kHeight; ++y) {
            for (std::size_t x = 0; x < kWidth; ++x) {
                const std::size_t base = ((frame * kHeight + y) * kWidth + x) * kChannels;
                data[base + 0] = static_cast<float>(redBase);
                data[base + 1] = static_cast<float>(greenBase);
                data[base + 2] = static_cast<float>(blueBase);
            }
        }
    }
    return data;
}

std::vector<double> makeExampleXValues()
{
    std::vector<double> xvals(kFrames);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        xvals[frame] = 1.0 + (2.0 * static_cast<double>(frame) / static_cast<double>(kFrames - 1));
    }
    return xvals;
}

} // namespace

struct ImageViewExample {
    std::unique_ptr<QMainWindow> window;
    imageview::ImageView* imageView = nullptr;
};

ImageViewExample createImageViewExample()
{
    auto window = std::make_unique<QMainWindow>();
    window->resize(800, 800);
    window->setWindowTitle(QStringLiteral("pyqtgraph example: ImageView"));

    auto* imageView = new imageview::ImageView(window.get(), QStringLiteral("rgba"));
    window->setCentralWidget(imageView);
    imageView->setHistogramLabel(QStringLiteral("Histogram label goes here"));

    const std::vector<float> data = makeExampleImageData();
    const std::vector<double> xvals = makeExampleXValues();
    imageView->setImage(core::ArrayView<const float, 4>(data.data(), {kFrames, kHeight, kWidth, kChannels}),
                        core::ArrayView<const double, 1>(xvals.data(), {xvals.size()}),
                        true);
    imageView->roiButton()->setChecked(true);
    imageView->roiClicked();

    return {.window = std::move(window), .imageView = imageView};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_IMAGEVIEW_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    auto example = cppqtgraph::examples::createImageViewExample();
    example.imageView->play(10.0);
    example.window->show();
    return QApplication::exec();
}
#endif
