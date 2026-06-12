#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>
#include <cppqtgraph/widgets/GraphicsView.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#define CPPQTGRAPH_IMAGEVIEW_NO_MAIN
#include "../../examples/ImageView.cpp"

#ifndef CPPQTGRAPH_P409_FIXTURE
#define CPPQTGRAPH_P409_FIXTURE "oracle/fixtures/P409/imageview_view_oracle.json"
#endif

namespace {

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

bool nearlyEqual(double lhs, double rhs, double tolerance)
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

bool checkViewRange(const QJsonObject& fixture,
                    const cppqtgraph::graphicsItems::ViewBox& viewBoxState,
                    const cppqtgraph::widgets::GraphicsView& graphicsView)
{
    const QJsonObject oracleRange = fixture.value(QStringLiteral("view_range")).toObject();
    const double tolerance = fixture.value(QStringLiteral("view_range_tolerance")).toDouble(0.5);
    const double aspectTolerance = fixture.value(QStringLiteral("aspect_ratio_tolerance")).toDouble(0.02);
    const auto actualRange = viewBoxState.viewRange();

    const QJsonArray shape = fixture.value(QStringLiteral("shape")).toArray();
    CHECK(shape.size() == 4);
    const double imageHeight = static_cast<double>(shape.at(1).toInt());
    const double imageWidth = static_cast<double>(shape.at(2).toInt());
    constexpr double kDefaultPadding = 0.02;

    double expectedX0 = -kDefaultPadding * imageWidth;
    double expectedX1 = imageWidth * (1.0 + kDefaultPadding);
    double expectedY0 = -kDefaultPadding * imageHeight;
    double expectedY1 = imageHeight * (1.0 + kDefaultPadding);

    const double widgetAspect =
        static_cast<double>(graphicsView.width()) / static_cast<double>(graphicsView.height());
    const double paddedAspect = (expectedX1 - expectedX0) / (expectedY1 - expectedY0);
    if (paddedAspect < widgetAspect) {
        const double expandedSpan = (expectedY1 - expectedY0) * widgetAspect;
        const double center = 0.5 * (expectedX0 + expectedX1);
        expectedX0 = center - 0.5 * expandedSpan;
        expectedX1 = center + 0.5 * expandedSpan;
    } else if (paddedAspect > widgetAspect) {
        const double expandedSpan = (expectedX1 - expectedX0) / widgetAspect;
        const double center = 0.5 * (expectedY0 + expectedY1);
        expectedY0 = center - 0.5 * expandedSpan;
        expectedY1 = center + 0.5 * expandedSpan;
    }

    CHECK(nearlyEqual(actualRange[0][0], expectedX0, tolerance));
    CHECK(nearlyEqual(actualRange[0][1], expectedX1, tolerance));
    CHECK(nearlyEqual(actualRange[1][0], expectedY0, tolerance));
    CHECK(nearlyEqual(actualRange[1][1], expectedY1, tolerance));

    const QJsonArray oracleX = oracleRange.value(QStringLiteral("x")).toArray();
    const QJsonArray oracleY = oracleRange.value(QStringLiteral("y")).toArray();
    CHECK(oracleX.size() == 2);
    CHECK(oracleY.size() == 2);
    const QJsonObject expectedCrop = fixture.value(QStringLiteral("image_crop")).toObject();
    const double oracleWidgetAspect = static_cast<double>(expectedCrop.value(QStringLiteral("width")).toInt())
        / static_cast<double>(expectedCrop.value(QStringLiteral("height")).toInt());
    const double oracleXSpan = oracleX.at(1).toDouble() - oracleX.at(0).toDouble();
    const double oracleXCenter = 0.5 * (oracleX.at(0).toDouble() + oracleX.at(1).toDouble());
    const double scaledOracleXSpan = oracleXSpan * (widgetAspect / oracleWidgetAspect);
    CHECK(nearlyEqual(actualRange[0][0], oracleXCenter - 0.5 * scaledOracleXSpan, tolerance));
    CHECK(nearlyEqual(actualRange[0][1], oracleXCenter + 0.5 * scaledOracleXSpan, tolerance));
    CHECK(nearlyEqual(actualRange[1][0], oracleY.at(0).toDouble(), tolerance));
    CHECK(nearlyEqual(actualRange[1][1], oracleY.at(1).toDouble(), tolerance));

    const double dataWidth = actualRange[0][1] - actualRange[0][0];
    const double dataHeight = actualRange[1][1] - actualRange[1][0];
    CHECK(dataWidth > 0.0);
    CHECK(dataHeight > 0.0);
    const double dataAspect = dataWidth / dataHeight;
    CHECK(nearlyEqual(dataAspect, widgetAspect, aspectTolerance));
    return true;
}

bool testImageViewExampleDefaults()
{
    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P409_FIXTURE), fixture));
    CHECK(fixture.value(QStringLiteral("y_inverted")).toBool());

    auto example = cppqtgraph::examples::createImageViewExample();
    CHECK(example.window != nullptr);
    CHECK(example.imageView != nullptr);
    example.window->show();
    for (int iteration = 0; iteration < 5; ++iteration) {
        QApplication::processEvents();
    }

    auto* viewBox = example.imageView->getViewBox();
    auto* graphicsView = example.imageView->getView();
    CHECK(viewBox != nullptr);
    CHECK(graphicsView != nullptr);
    CHECK(viewBox->yInverted() == fixture.value(QStringLiteral("y_inverted")).toBool());
    CHECK(checkViewRange(fixture, *viewBox, *graphicsView));

    const QJsonObject window = fixture.value(QStringLiteral("window")).toObject();
    CHECK(example.window->width() == window.value(QStringLiteral("width")).toInt());
    CHECK(example.window->height() == window.value(QStringLiteral("height")).toInt());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard guard(argc, argv);

    if (!testImageViewExampleDefaults()) {
        return 1;
    }

    return 0;
}
