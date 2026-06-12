#include <cppqtgraph/graphicsItems/AxisItem.hpp>
#include <cppqtgraph/graphicsItems/GradientEditorItem.hpp>
#include <cppqtgraph/graphicsItems/HistogramLUTItem.hpp>
#include <cppqtgraph/graphicsItems/ImageItem.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QImage>
#include <QtWidgets/QApplication>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P410_FIXTURE
#define CPPQTGRAPH_P410_FIXTURE "oracle/fixtures/P410/imageview_rgba_histogram_oracle.json"
#endif

namespace {

constexpr double kTolerance = 1.0e-6;

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

int displayChannel(double value, double minimum, double maximum)
{
    const double span = maximum - minimum;
    if (span == 0.0) {
        return static_cast<int>(std::lround(std::clamp(value, 0.0, 255.0)));
    }
    const double scaled = (value - minimum) / span;
    return static_cast<int>(std::lround(std::clamp(scaled * 255.0, 0.0, 255.0)));
}

bool loadFixtureVectors(const QJsonObject& fixture,
                        std::vector<float>& data,
                        std::vector<double>& xvals,
                        std::array<std::size_t, 4>& shape)
{
    const QJsonArray shapeArray = fixture.value(QStringLiteral("shape")).toArray();
    CHECK(shapeArray.size() == 4);
    for (int index = 0; index < 4; ++index) {
        shape[static_cast<std::size_t>(index)] = static_cast<std::size_t>(shapeArray.at(index).toInt());
    }

    const QJsonArray dataArray = fixture.value(QStringLiteral("data")).toArray();
    data.clear();
    data.reserve(static_cast<std::size_t>(dataArray.size()));
    for (const QJsonValue& value : dataArray) {
        data.push_back(static_cast<float>(value.toDouble()));
    }
    CHECK(data.size() == shape[0] * shape[1] * shape[2] * shape[3]);

    const QJsonArray xvalsArray = fixture.value(QStringLiteral("xvals")).toArray();
    xvals.clear();
    xvals.reserve(static_cast<std::size_t>(xvalsArray.size()));
    for (const QJsonValue& value : xvalsArray) {
        xvals.push_back(value.toDouble());
    }
    CHECK(xvals.size() == shape[0]);
    return true;
}

bool testImageViewRgbaHistogram()
{
    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P410_FIXTURE), fixture));
    CHECK(fixture.value(QStringLiteral("level_mode")).toString() == QStringLiteral("rgba"));

    std::vector<float> data;
    std::vector<double> xvals;
    std::array<std::size_t, 4> shape{};
    CHECK(loadFixtureVectors(fixture, data, xvals, shape));

    cppqtgraph::imageview::ImageView imageView(nullptr, QStringLiteral("rgba"));
    imageView.setHistogramLabel(fixture.value(QStringLiteral("histogram_label")).toString());
    imageView.setImage(cppqtgraph::core::ArrayView<const float, 4>(data.data(), {shape[0], shape[1], shape[2], shape[3]}),
                       cppqtgraph::core::ArrayView<const double, 1>(xvals.data(), {xvals.size()}),
                       true);
    for (int iteration = 0; iteration < 5; ++iteration) {
        QApplication::processEvents();
    }

    auto* histogram = imageView.getHistogram();
    CHECK(histogram != nullptr);
    CHECK(histogram->levelMode() == QStringLiteral("rgba"));
    CHECK(histogram->axis() != nullptr);
    CHECK(histogram->axis()->labelText() == fixture.value(QStringLiteral("histogram_label")).toString());
    CHECK(histogram->gradient() != nullptr);
    CHECK(histogram->gradient()->isVisible() == !fixture.value(QStringLiteral("gradient_hidden")).toBool());
    CHECK(histogram->viewBox() != nullptr);

    const QJsonArray regions = fixture.value(QStringLiteral("regions")).toArray();
    CHECK(regions.size() == static_cast<int>(shape[3]));
    const auto channelLevels = histogram->getChannelLevels();
    CHECK(channelLevels.size() == shape[3]);
    for (int index = 0; index < regions.size(); ++index) {
        const QJsonObject region = regions.at(index).toObject();
        CHECK(region.value(QStringLiteral("visible")).toBool());
        auto* channelRegion = histogram->channelRegion(static_cast<std::size_t>(index));
        CHECK(channelRegion != nullptr);
        CHECK(channelRegion->isVisible());
        const auto span = channelRegion->span();
        const QJsonArray expectedSpan = region.value(QStringLiteral("span")).toArray();
        CHECK(nearlyEqual(span.first, expectedSpan.at(0).toDouble()));
        CHECK(nearlyEqual(span.second, expectedSpan.at(1).toDouble()));
        const QJsonArray expectedLevels = region.value(QStringLiteral("levels")).toArray();
        CHECK(nearlyEqual(channelLevels[static_cast<std::size_t>(index)].first, expectedLevels.at(0).toDouble(), 1.0));
        CHECK(nearlyEqual(channelLevels[static_cast<std::size_t>(index)].second, expectedLevels.at(1).toDouble(), 1.0));
    }

    const QJsonObject levelShift = fixture.value(QStringLiteral("level_shift")).toObject();
    const QJsonArray shiftedLevels = levelShift.value(QStringLiteral("rgba")).toArray();
    std::vector<std::pair<double, double>> requestedLevels;
    requestedLevels.reserve(static_cast<std::size_t>(shiftedLevels.size()));
    for (const QJsonValue& value : shiftedLevels) {
        const QJsonArray pair = value.toArray();
        requestedLevels.emplace_back(pair.at(0).toDouble(), pair.at(1).toDouble());
    }
    histogram->setChannelLevels(requestedLevels);
    histogram->regionChanged();
    for (int iteration = 0; iteration < 5; ++iteration) {
        QApplication::processEvents();
    }

    auto* imageItem = imageView.getImageItem();
    CHECK(imageItem != nullptr);
    CHECK(imageItem->render());
    const QJsonObject probe = fixture.value(QStringLiteral("render_probe")).toObject();
    const QJsonArray sourceRgb = probe.value(QStringLiteral("source_rgb")).toArray();
    const QJsonArray expectedDisplay = probe.value(QStringLiteral("display_rgb")).toArray();
    const auto appliedLevels = imageItem->getChannelLevels();
    CHECK(appliedLevels.has_value());
    CHECK(appliedLevels->size() >= 3);

    const int displayR = displayChannel(sourceRgb.at(0).toDouble(), (*appliedLevels)[0].minimum, (*appliedLevels)[0].maximum);
    const int displayG = displayChannel(sourceRgb.at(1).toDouble(), (*appliedLevels)[1].minimum, (*appliedLevels)[1].maximum);
    const int displayB = displayChannel(sourceRgb.at(2).toDouble(), (*appliedLevels)[2].minimum, (*appliedLevels)[2].maximum);
    CHECK(displayR == expectedDisplay.at(0).toInt());
    CHECK(displayG == expectedDisplay.at(1).toInt());
    CHECK(displayB == expectedDisplay.at(2).toInt());

    const QImage& rendered = imageItem->cachedImage();
    CHECK(!rendered.isNull());
    const QRgb pixel = rendered.pixel(0, 0);
    CHECK(qRed(pixel) == expectedDisplay.at(0).toInt());
    CHECK(qGreen(pixel) == expectedDisplay.at(1).toInt());
    CHECK(qBlue(pixel) == expectedDisplay.at(2).toInt());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard guard(argc, argv);

    if (!testImageViewRgbaHistogram()) {
        return 1;
    }

    return 0;
}
