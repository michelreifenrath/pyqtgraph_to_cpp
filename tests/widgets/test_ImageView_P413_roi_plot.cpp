#include <cppqtgraph/graphicsItems/InfiniteLine.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/graphicsItems/ROI.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P413_FIXTURE
#define CPPQTGRAPH_P413_FIXTURE "oracle/fixtures/P413/imageview_roi_plot_oracle.json"
#endif

namespace {

constexpr double kTolerance = 1.0e-5;
constexpr std::size_t kFrames = 4;
constexpr std::size_t kHeight = 4;
constexpr std::size_t kWidth = 4;
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

bool nearlyEqual(double lhs, double rhs, double tolerance = kTolerance)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool readJsonFixture(const QString& path, QJsonObject& object)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "failed to open fixture: " << path.toStdString() << '\n';
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        std::cerr << "fixture is not a JSON object: " << path.toStdString() << '\n';
        return false;
    }
    object = document.object();
    return true;
}

std::vector<float> makeFixtureData()
{
    std::vector<float> data(kFrames * kHeight * kWidth * kChannels);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        for (std::size_t channel = 0; channel < kChannels; ++channel) {
            const float value = 100.0F + 10.0F * static_cast<float>(frame) + 5.0F * static_cast<float>(channel);
            for (std::size_t pixel = 0; pixel < kHeight * kWidth; ++pixel) {
                data[((frame * kHeight * kWidth + pixel) * kChannels) + channel] = value;
            }
        }
    }
    return data;
}

std::vector<double> makeFixtureXVals()
{
    return {0.0, 1.5, 3.0, 5.0};
}

std::unique_ptr<cppqtgraph::imageview::ImageView> makeRoiImageView()
{
    const std::vector<float> data = makeFixtureData();
    const std::vector<double> xvals = makeFixtureXVals();
    auto imageView = std::make_unique<cppqtgraph::imageview::ImageView>(nullptr, QStringLiteral("rgba"), false);
    imageView->setImage(cppqtgraph::core::ArrayView<const float, 4>(
                            data.data(), {kFrames, kHeight, kWidth, kChannels}),
                        cppqtgraph::core::ArrayView<const double, 1>(xvals.data(), {xvals.size()}),
                        false,
                        false);
    return imageView;
}

bool compareCurvesToFixture(const cppqtgraph::imageview::ImageView& imageView, const QJsonArray& expectedCurves)
{
    CHECK(imageView.roiCurveCount() == static_cast<std::size_t>(expectedCurves.size()));
    for (int curveIndex = 0; curveIndex < expectedCurves.size(); ++curveIndex) {
        const QJsonObject expectedCurve = expectedCurves.at(curveIndex).toObject();
        const auto* curve = imageView.roiCurve(static_cast<std::size_t>(curveIndex));
        CHECK(curve != nullptr);
        const auto actualX = curve->xData();
        const auto actualY = curve->yData();
        const QJsonArray expectedX = expectedCurve.value(QStringLiteral("x")).toArray();
        const QJsonArray expectedY = expectedCurve.value(QStringLiteral("y")).toArray();
        CHECK(static_cast<int>(actualX.size()) == expectedX.size());
        CHECK(static_cast<int>(actualY.size()) == expectedY.size());
        for (int index = 0; index < expectedX.size(); ++index) {
            CHECK(nearlyEqual(actualX[static_cast<std::size_t>(index)], expectedX.at(index).toDouble()));
            CHECK(nearlyEqual(actualY[static_cast<std::size_t>(index)], expectedY.at(index).toDouble()));
        }
    }
    return true;
}

bool testStartupRoiCheckedAndVisible()
{
    auto imageView = makeRoiImageView();
    imageView->resize(800, 800);
    imageView->show();
    QTest::qWait(0);

    auto* roiButton = imageView->roiButton();
    auto* roi = imageView->roi();
    CHECK(roiButton != nullptr);
    CHECK(roi != nullptr);
    roiButton->setChecked(true);
    imageView->roiClicked();
    QTest::qWait(0);

    CHECK(roiButton->isChecked());
    CHECK(roi->isVisible());
    CHECK(imageView->getRoiPlot() != nullptr);
    CHECK(!imageView->getRoiPlot()->isHidden());
    CHECK(imageView->roiCurveCount() == kChannels);
    return true;
}

bool testRoiToggleHidesCurvesKeepsTimeline()
{
    auto imageView = makeRoiImageView();
    imageView->resize(800, 800);
    imageView->show();
    QTest::qWait(0);

    auto* roiButton = imageView->roiButton();
    CHECK(roiButton != nullptr);
    roiButton->setChecked(true);
    imageView->roiClicked();
    QTest::qWait(0);
    CHECK(imageView->roi()->isVisible());
    CHECK(imageView->roiCurveCount() == kChannels);
    CHECK(imageView->roiCurve(0)->isVisible());

    roiButton->setChecked(false);
    imageView->roiClicked();
    QTest::qWait(0);
    CHECK(!imageView->roi()->isVisible());
    CHECK(!imageView->roiCurve(0)->isVisible());
    CHECK(imageView->timeLine() != nullptr);
    CHECK(imageView->timeLine()->isVisible());
    CHECK(imageView->getRoiPlot() != nullptr);
    CHECK(!imageView->getRoiPlot()->isHidden());
    return true;
}

bool testRoiCurvesMatchOracle()
{
    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P413_FIXTURE), fixture));
    const QJsonObject startup = fixture.value(QStringLiteral("startup")).toObject();
    const QJsonArray expectedCurves = startup.value(QStringLiteral("curves")).toArray();

    auto imageView = makeRoiImageView();
    imageView->resize(800, 800);
    imageView->show();
    QTest::qWait(0);
    imageView->roiButton()->setChecked(true);
    imageView->roiClicked();
    QTest::qWait(0);

    return compareCurvesToFixture(*imageView, expectedCurves);
}

bool testRoiResizeUpdatesCurves()
{
    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P413_FIXTURE), fixture));
    const QJsonObject resized = fixture.value(QStringLiteral("resized_roi")).toObject();
    const QJsonArray expectedCurves = resized.value(QStringLiteral("curves")).toArray();

    auto imageView = makeRoiImageView();
    imageView->resize(800, 800);
    imageView->show();
    QTest::qWait(0);
    imageView->roiButton()->setChecked(true);
    imageView->roiClicked();
    QTest::qWait(0);

    auto* roi = imageView->roi();
    CHECK(roi != nullptr);
    roi->setSize(QPointF(2.0, 2.0));
    QTest::qWait(0);

    return compareCurvesToFixture(*imageView, expectedCurves);
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard guard(argc, argv);

    if (!testStartupRoiCheckedAndVisible() || !testRoiToggleHidesCurvesKeepsTimeline() || !testRoiCurvesMatchOracle()
        || !testRoiResizeUpdatesCurves()) {
        return 1;
    }

    return 0;
}
