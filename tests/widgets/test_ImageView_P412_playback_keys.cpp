#include <cppqtgraph/imageview/ImageView.hpp>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>

#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t kFrames = 20;
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

std::vector<float> makeFrameData()
{
    std::vector<float> data(kFrames * kHeight * kWidth * kChannels);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        const float marker = static_cast<float>(frame);
        for (std::size_t pixel = 0; pixel < kHeight * kWidth * kChannels; ++pixel) {
            data[frame * kHeight * kWidth * kChannels + pixel] = marker;
        }
    }
    return data;
}

std::unique_ptr<cppqtgraph::imageview::ImageView> makePlaybackImageView()
{
    const std::vector<float> data = makeFrameData();
    std::vector<double> xvals(kFrames);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        xvals[frame] = static_cast<double>(frame);
    }

    auto imageView = std::make_unique<cppqtgraph::imageview::ImageView>(
        nullptr, QStringLiteral("rgba"), false);
    imageView->setImage(cppqtgraph::core::ArrayView<const float, 4>(
                            data.data(), {kFrames, kHeight, kWidth, kChannels}),
                        cppqtgraph::core::ArrayView<const double, 1>(xvals.data(), {xvals.size()}),
                        false,
                        false);
    imageView->resize(400, 400);
    imageView->show();
    imageView->setFocus(Qt::OtherFocusReason);
    QTest::qWait(0);
    return imageView;
}

bool testPlayAdvancesFrames()
{
    const auto imageView = makePlaybackImageView();
    imageView->setCurrentIndex(0);
    QSignalSpy spy(imageView.get(), &cppqtgraph::imageview::ImageView::sigTimeChanged);

    imageView->play(10.0);
    const int startIndex = imageView->currentIndex();
    QTest::qWait(350);
    CHECK(imageView->currentIndex() > startIndex);
    CHECK(spy.size() >= 1);

    imageView->play(0.0);
    const int pausedIndex = imageView->currentIndex();
    QTest::qWait(200);
    CHECK(imageView->currentIndex() == pausedIndex);
    return true;
}

bool testSpaceTogglesPauseResume()
{
    const auto imageView = makePlaybackImageView();
    imageView->setCurrentIndex(0);
    imageView->play(10.0);
    QTest::qWait(150);

    QTest::keyClick(imageView.get(), Qt::Key_Space);
    QTest::qWait(0);
    const int pausedIndex = imageView->currentIndex();
    QTest::qWait(250);
    CHECK(imageView->currentIndex() == pausedIndex);

    QTest::keyClick(imageView.get(), Qt::Key_Space);
    QTest::qWait(250);
    CHECK(imageView->currentIndex() > pausedIndex);
    imageView->play(0.0);
    return true;
}

bool testHomeEndJumpBoundariesAndPause()
{
    const auto imageView = makePlaybackImageView();
    imageView->setCurrentIndex(5);
    imageView->play(10.0);

    QTest::keyClick(imageView.get(), Qt::Key_Home);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 0);
    const int homeIndex = imageView->currentIndex();
    QTest::qWait(200);
    CHECK(imageView->currentIndex() == homeIndex);

    QTest::keyClick(imageView.get(), Qt::Key_End);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == static_cast<int>(kFrames - 1));
    const int endIndex = imageView->currentIndex();
    QTest::qWait(200);
    CHECK(imageView->currentIndex() == endIndex);
    return true;
}

bool testArrowKeysJumpAndSetDirection()
{
    const auto imageView = makePlaybackImageView();
    imageView->setCurrentIndex(5);

    QTest::keyPress(imageView.get(), Qt::Key_Right);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 6);
    QTest::keyRelease(imageView.get(), Qt::Key_Right);
    QTest::qWait(0);

    QTest::keyPress(imageView.get(), Qt::Key_Left);
    QTest::qWait(0);
    CHECK(imageView->currentIndex() == 5);
    QTest::keyRelease(imageView.get(), Qt::Key_Left);
    QTest::qWait(0);
    return true;
}

bool testPageAndVerticalKeysSetPlaybackRates()
{
    const auto imageView = makePlaybackImageView();
    imageView->setCurrentIndex(10);

    QTest::keyPress(imageView.get(), Qt::Key_Down);
    QTest::qWait(100);
    const int afterDown = imageView->currentIndex();
    CHECK(afterDown > 10);
    QTest::keyRelease(imageView.get(), Qt::Key_Down);
    QTest::qWait(0);

    imageView->setCurrentIndex(10);
    QTest::keyPress(imageView.get(), Qt::Key_Up);
    QTest::qWait(100);
    const int afterUp = imageView->currentIndex();
    CHECK(afterUp < 10);
    QTest::keyRelease(imageView.get(), Qt::Key_Up);
    QTest::qWait(0);

    imageView->setCurrentIndex(10);
    QTest::keyPress(imageView.get(), Qt::Key_PageDown);
    QTest::qWait(100);
    const int afterPageDown = imageView->currentIndex();
    CHECK(afterPageDown > 10);
    QTest::keyRelease(imageView.get(), Qt::Key_PageDown);
    QTest::qWait(0);

    imageView->setCurrentIndex(10);
    QTest::keyPress(imageView.get(), Qt::Key_PageUp);
    QTest::qWait(100);
    const int afterPageUp = imageView->currentIndex();
    CHECK(afterPageUp < 10);
    QTest::keyRelease(imageView.get(), Qt::Key_PageUp);
    QTest::qWait(0);
    return true;
}

bool testBoundaryStopMatchesUpstreamSemantics()
{
    const auto imageView = makePlaybackImageView();
    imageView->setCurrentIndex(static_cast<int>(kFrames - 1));
    imageView->play(1000.0);
    QTest::qWait(150);
    CHECK(imageView->currentIndex() == static_cast<int>(kFrames - 1));

    imageView->setCurrentIndex(0);
    imageView->play(-1000.0);
    QTest::qWait(150);
    CHECK(imageView->currentIndex() == 0);
    imageView->play(0.0);
    return true;
}

bool testSetCurrentIndexClipsDuringPlayback()
{
    const auto imageView = makePlaybackImageView();
    imageView->play(10.0);
    QTest::qWait(50);
    imageView->setCurrentIndex(999);
    CHECK(imageView->currentIndex() == static_cast<int>(kFrames - 1));
    imageView->play(0.0);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard guard(argc, argv);

    if (!testPlayAdvancesFrames() || !testSpaceTogglesPauseResume() || !testHomeEndJumpBoundariesAndPause()
        || !testArrowKeysJumpAndSetDirection() || !testPageAndVerticalKeysSetPlaybackRates()
        || !testBoundaryStopMatchesUpstreamSemantics() || !testSetCurrentIndexClipsDuringPlayback()) {
        return 1;
    }

    return 0;
}
