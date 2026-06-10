// Source note: translated/adapted from PyQtGraph examples/ImageItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <cppqtgraph/core/ArrayView.hpp>
#include <cppqtgraph/graphicsItems/ImageItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsView>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

namespace cppqtgraph::examples {

struct ImageItemExampleState {
    ImageItemExampleState();

    std::vector<std::vector<std::uint8_t>> frames;
    std::size_t nextFrame = 0;
    QElapsedTimer updateClock;
    double elapsedSeconds = 0.0;
};

struct ImageItemExample {
    std::unique_ptr<GraphicsScene::GraphicsScene> scene;
    std::unique_ptr<QGraphicsView> widget;
    graphicsItems::ViewBox* viewBox = nullptr;
    graphicsItems::ImageItem* image = nullptr;
    QGraphicsRectItem* border = nullptr;
    std::unique_ptr<QTimer> timer;
    std::shared_ptr<ImageItemExampleState> state;
};

std::size_t imageItemExampleFrameCount() noexcept;
std::size_t imageItemExampleWidth() noexcept;
std::size_t imageItemExampleHeight() noexcept;
void updateImageItemExampleData(graphicsItems::ImageItem* image, QTimer* timer, const std::shared_ptr<ImageItemExampleState>& state);
ImageItemExample createImageItemExample();

namespace {

constexpr std::size_t kFrameCount = 15;
constexpr std::size_t kImageWidth = 600;
constexpr std::size_t kImageHeight = 600;

std::vector<std::uint8_t> makeFrame(std::mt19937& generator, std::size_t frameIndex)
{
    std::normal_distribution<double> distribution(127.0 + static_cast<double>(frameIndex % 5) * 3.0, 32.0);
    std::vector<std::uint8_t> frame(kImageWidth * kImageHeight);
    for (auto& value : frame) {
        const int sample = static_cast<int>(std::lround(distribution(generator)));
        value = static_cast<std::uint8_t>(std::clamp(sample, 0, 255));
    }
    return frame;
}

void setCurrentFrame(graphicsItems::ImageItem* image, const std::vector<std::uint8_t>& frame)
{
    image->setImage(core::ArrayView<const std::uint8_t, 2>(frame.data(), {kImageWidth, kImageHeight}));
}

} // namespace

std::size_t imageItemExampleFrameCount() noexcept
{
    return kFrameCount;
}

std::size_t imageItemExampleWidth() noexcept
{
    return kImageWidth;
}

std::size_t imageItemExampleHeight() noexcept
{
    return kImageHeight;
}

ImageItemExampleState::ImageItemExampleState()
{
    std::mt19937 generator(0x14'02'47U);
    frames.reserve(kFrameCount);
    for (std::size_t frame = 0; frame < kFrameCount; ++frame) {
        frames.push_back(makeFrame(generator, frame));
    }
    updateClock.start();
}

void updateImageItemExampleData(graphicsItems::ImageItem* image, QTimer* timer, const std::shared_ptr<ImageItemExampleState>& state)
{
    if (image == nullptr || timer == nullptr || state == nullptr || state->frames.empty()) {
        return;
    }

    setCurrentFrame(image, state->frames[state->nextFrame]);
    state->nextFrame = (state->nextFrame + 1U) % state->frames.size();

    timer->start(1);
    const double elapsedNow = static_cast<double>(state->updateClock.restart()) / 1000.0;
    state->elapsedSeconds = state->elapsedSeconds * 0.9 + elapsedNow * 0.1;
}

ImageItemExample createImageItemExample()
{
    auto scene = std::make_unique<GraphicsScene::GraphicsScene>();
    auto widget = std::make_unique<QGraphicsView>();
    widget->setWindowTitle(QStringLiteral("pyqtgraph example: ImageItem"));
    widget->setFrameStyle(QFrame::NoFrame);
    widget->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    widget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    widget->setScene(scene.get());
    widget->setSceneRect(QRectF(0.0, 0.0, static_cast<qreal>(kImageWidth), static_cast<qreal>(kImageHeight)));
    widget->resize(static_cast<int>(kImageWidth), static_cast<int>(kImageHeight));

    auto* view = new graphicsItems::ViewBox();
    scene->addItem(view);
    view->setGeometry(QRectF(0.0, 0.0, static_cast<qreal>(kImageWidth), static_cast<qreal>(kImageHeight)));
    view->setAspectLocked(true);

    auto* image = new graphicsItems::ImageItem();
    view->addItem(image);

    auto* border = new QGraphicsRectItem(QRectF(0.0, 0.0, static_cast<qreal>(kImageWidth), static_cast<qreal>(kImageHeight)));
    border->setPen(QPen(Qt::white));
    border->setBrush(Qt::NoBrush);
    border->setZValue(image->zValue() + 1.0);
    view->addItem(border, true);

    view->setRange(QRectF(0.0, 0.0, static_cast<qreal>(kImageWidth), static_cast<qreal>(kImageHeight)), 0.0);

    auto timer = std::make_unique<QTimer>();
    timer->setSingleShot(true);
    auto state = std::make_shared<ImageItemExampleState>();
    QObject::connect(timer.get(), &QTimer::timeout, timer.get(), [image, timerPtr = timer.get(), state]() {
        updateImageItemExampleData(image, timerPtr, state);
    });
    updateImageItemExampleData(image, timer.get(), state);

    return {.scene = std::move(scene),
            .widget = std::move(widget),
            .viewBox = view,
            .image = image,
            .border = border,
            .timer = std::move(timer),
            .state = std::move(state)};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_IMAGEITEM_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    auto example = cppqtgraph::examples::createImageItemExample();
    example.widget->show();
    return QApplication::exec();
}
#endif
