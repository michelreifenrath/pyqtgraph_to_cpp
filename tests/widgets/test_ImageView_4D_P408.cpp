#include <cppqtgraph/imageview/ImageView.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QImage>
#include <QtWidgets/QApplication>

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P408_FIXTURE
#define CPPQTGRAPH_P408_FIXTURE "oracle/fixtures/P408/imageview_4d_oracle.json"
#endif

namespace {

constexpr double kValueTolerance = 1.0e-9;

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

bool loadFixtureData(const QJsonObject& object,
                     std::vector<double>& data,
                     std::vector<double>& xvals,
                     std::array<std::size_t, 4>& shape,
                     QJsonObject& firstProbe,
                     QJsonObject& lastProbe)
{
    const QJsonArray shapeArray = object.value(QStringLiteral("shape")).toArray();
    CHECK(shapeArray.size() == 4);
    for (int index = 0; index < 4; ++index) {
        shape[static_cast<std::size_t>(index)] = static_cast<std::size_t>(shapeArray.at(index).toInt());
    }

    const QJsonArray dataArray = object.value(QStringLiteral("data")).toArray();
    data.clear();
    data.reserve(static_cast<std::size_t>(dataArray.size()));
    for (const QJsonValue& value : dataArray) {
        data.push_back(value.toDouble());
    }
    CHECK(data.size() == shape[0] * shape[1] * shape[2] * shape[3]);

    const QJsonArray xvalsArray = object.value(QStringLiteral("xvals")).toArray();
    xvals.clear();
    xvals.reserve(static_cast<std::size_t>(xvalsArray.size()));
    for (const QJsonValue& value : xvalsArray) {
        xvals.push_back(value.toDouble());
    }
    CHECK(xvals.size() == shape[0]);

    const QJsonObject probes = object.value(QStringLiteral("probes")).toObject();
    firstProbe = probes.value(QStringLiteral("first_frame")).toObject();
    lastProbe = probes.value(QStringLiteral("last_frame")).toObject();
    CHECK(!firstProbe.isEmpty());
    CHECK(!lastProbe.isEmpty());
    return true;
}

bool nearlyEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= kValueTolerance;
}

bool checkStoredRgb(const std::vector<double>& data,
                    const std::array<std::size_t, 4>& shape,
                    const QJsonObject& probe)
{
    const int frame = probe.value(QStringLiteral("frame")).toInt();
    const int y = probe.value(QStringLiteral("y")).toInt();
    const int x = probe.value(QStringLiteral("x")).toInt();
    const QJsonArray expected = probe.value(QStringLiteral("rgb")).toArray();
    CHECK(expected.size() == 3);

    for (int channel = 0; channel < 3; ++channel) {
        const std::size_t index = static_cast<std::size_t>(
            (((frame * static_cast<int>(shape[1]) + y) * static_cast<int>(shape[2]) + x) * 3) + channel);
        CHECK(nearlyEqual(data[index], expected.at(channel).toDouble()));
    }
    return true;
}

bool checkRgb888(const QImage& image, int x, int y, int red, int green, int blue)
{
    if (image.format() != QImage::Format_RGB888) {
        std::cerr << "expected Format_RGB888 got " << static_cast<int>(image.format()) << '\n';
        return false;
    }
    const uchar* row = image.constScanLine(y);
    const std::size_t base = static_cast<std::size_t>(x) * 3;
    if (static_cast<int>(row[base + 0]) != red || static_cast<int>(row[base + 1]) != green
        || static_cast<int>(row[base + 2]) != blue) {
        std::cerr << "rgb888(" << x << ',' << y << ") expected (" << red << ',' << green << ',' << blue << ") got ("
                  << static_cast<int>(row[base + 0]) << ',' << static_cast<int>(row[base + 1]) << ','
                  << static_cast<int>(row[base + 2]) << ")\n";
        return false;
    }
    return true;
}

bool checkRenderedProbe(cppqtgraph::imageview::ImageView& imageView, const QJsonObject& probe)
{
    const int y = probe.value(QStringLiteral("y")).toInt();
    const int x = probe.value(QStringLiteral("x")).toInt();
    const QJsonArray display = probe.value(QStringLiteral("display_rgb")).toArray();
    CHECK(display.size() == 3);

    auto* imageItem = imageView.getImageItem();
    CHECK(imageItem != nullptr);
    CHECK(imageItem->render());
    const QImage& rendered = imageItem->cachedImage();
    CHECK(!rendered.isNull());
    CHECK(checkRgb888(rendered,
                      x,
                      y,
                      display.at(0).toInt(),
                      display.at(1).toInt(),
                      display.at(2).toInt()));
    return true;
}

bool testImageViewFloat4D()
{
    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P408_FIXTURE), fixture));

    std::vector<double> data;
    std::vector<double> xvals;
    std::array<std::size_t, 4> shape{};
    QJsonObject firstProbe;
    QJsonObject lastProbe;
    CHECK(loadFixtureData(fixture, data, xvals, shape, firstProbe, lastProbe));

    std::vector<float> floatData(data.begin(), data.end());
    cppqtgraph::core::ArrayView<const float, 4> imageData(
        floatData.data(),
        {shape[0], shape[1], shape[2], shape[3]});
    cppqtgraph::core::ArrayView<const double, 1> xView(xvals.data(), {xvals.size()});

    cppqtgraph::imageview::ImageView imageViewWidget;
    imageViewWidget.setAxisOrder(cppqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageViewWidget.setImage(imageData, xView, false, false);

    CHECK(imageViewWidget.hasImage());
    CHECK(imageViewWidget.currentIndex() == 0);
    CHECK(imageViewWidget.xValues().size() == xvals.size());
    for (std::size_t index = 0; index < xvals.size(); ++index) {
        CHECK(nearlyEqual(imageViewWidget.xValues()[index], xvals[index]));
    }

    CHECK(checkStoredRgb(data, shape, firstProbe));
    CHECK(checkRenderedProbe(imageViewWidget, firstProbe));

    const int lastFrame = lastProbe.value(QStringLiteral("frame")).toInt();
    imageViewWidget.setCurrentIndex(lastFrame);
    CHECK(imageViewWidget.currentIndex() == lastFrame);
    CHECK(checkStoredRgb(data, shape, lastProbe));
    CHECK(checkRenderedProbe(imageViewWidget, lastProbe));

    return true;
}

bool testImageViewDouble4D()
{
    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P408_FIXTURE), fixture));

    std::vector<double> data;
    std::vector<double> xvals;
    std::array<std::size_t, 4> shape{};
    QJsonObject firstProbe;
    QJsonObject lastProbe;
    CHECK(loadFixtureData(fixture, data, xvals, shape, firstProbe, lastProbe));

    cppqtgraph::core::ArrayView<const double, 4> imageData(data.data(), {shape[0], shape[1], shape[2], shape[3]});
    cppqtgraph::core::ArrayView<const double, 1> xView(xvals.data(), {xvals.size()});

    cppqtgraph::imageview::ImageView imageViewWidget;
    imageViewWidget.setAxisOrder(cppqtgraph::graphicsItems::ImageItem::AxisOrder::RowMajor);
    imageViewWidget.setImage(imageData, xView, false, false);

    CHECK(imageViewWidget.hasImage());
    CHECK(imageViewWidget.currentIndex() == 0);
    CHECK(checkRenderedProbe(imageViewWidget, firstProbe));

    const int lastFrame = lastProbe.value(QStringLiteral("frame")).toInt();
    imageViewWidget.setCurrentIndex(lastFrame);
    CHECK(imageViewWidget.currentIndex() == lastFrame);
    CHECK(checkRenderedProbe(imageViewWidget, lastProbe));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard guard(argc, argv);

    if (!testImageViewFloat4D() || !testImageViewDouble4D()) {
        return 1;
    }

    return 0;
}
