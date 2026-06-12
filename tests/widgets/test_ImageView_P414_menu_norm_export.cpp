#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/imageview/ImageView.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P414_FIXTURE
#define CPPQTGRAPH_P414_FIXTURE "oracle/fixtures/P414/imageview_menu_norm_export_oracle.json"
#endif

namespace {

constexpr double kTolerance = 1.0e-5;
constexpr std::size_t kFrames = 4;
constexpr std::size_t kHeight = 8;
constexpr std::size_t kWidth = 8;

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
    std::vector<float> data(kFrames * kHeight * kWidth);
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        const float value = 100.0F + 10.0F * static_cast<float>(frame);
        for (std::size_t pixel = 0; pixel < kHeight * kWidth; ++pixel) {
            data[frame * kHeight * kWidth + pixel] = value;
        }
    }
    return data;
}

std::vector<double> makeFixtureXVals()
{
    return {0.0, 1.0, 2.0, 3.0};
}

std::unique_ptr<cppqtgraph::imageview::ImageView> makeImageView()
{
    const std::vector<float> data = makeFixtureData();
    const std::vector<double> xvals = makeFixtureXVals();
    auto imageView = std::make_unique<cppqtgraph::imageview::ImageView>();
    imageView->setImage(cppqtgraph::core::ArrayView<const float, 3>(
                            data.data(), {kFrames, kHeight, kWidth}),
                        false,
                        false);
    imageView->setCurrentIndex(0);
    return imageView;
}

QString sha256File(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

bool compareCurvesToFixture(const cppqtgraph::imageview::ImageView& imageView, const QJsonArray& expectedCurves)
{
    CHECK(imageView.roiCurveCount() == static_cast<std::size_t>(expectedCurves.size()));
    for (int curveIndex = 0; curveIndex < expectedCurves.size(); ++curveIndex) {
        const QJsonObject expectedCurve = expectedCurves.at(curveIndex).toObject();
        const auto* curve = imageView.roiCurve(static_cast<std::size_t>(curveIndex));
        CHECK(curve != nullptr);
        const auto xData = curve->xData();
        const auto yData = curve->yData();
        const QJsonArray expectedX = expectedCurve.value(QStringLiteral("x")).toArray();
        const QJsonArray expectedY = expectedCurve.value(QStringLiteral("y")).toArray();
        CHECK(static_cast<int>(xData.size()) == expectedX.size());
        CHECK(static_cast<int>(yData.size()) == expectedY.size());
        for (int index = 0; index < expectedX.size(); ++index) {
            CHECK(nearlyEqual(xData[static_cast<std::size_t>(index)], expectedX.at(index).toDouble()));
            CHECK(nearlyEqual(yData[static_cast<std::size_t>(index)], expectedY.at(index).toDouble()));
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    ApplicationGuard guard(argc, argv);

    QJsonObject fixture;
    CHECK(readJsonFixture(QString::fromUtf8(CPPQTGRAPH_P414_FIXTURE), fixture));

    auto imageView = makeImageView();
    CHECK(imageView->menuButton() != nullptr);

    imageView->menuClicked();
    CHECK(imageView->normGroup() != nullptr);
    CHECK(!imageView->normGroup()->isVisible());

    imageView->menu();
    QMenu* menu = imageView->menu();
    CHECK(menu != nullptr);
    QStringList menuActions;
    for (QAction* action : menu->actions()) {
        if (action != nullptr) {
            menuActions.push_back(action->text());
        }
    }
    const QJsonArray expectedMenuActions = fixture.value(QStringLiteral("menu_actions")).toArray();
    CHECK(menuActions.size() == expectedMenuActions.size());
    for (int index = 0; index < expectedMenuActions.size(); ++index) {
        CHECK(menuActions.at(index) == expectedMenuActions.at(index).toString());
    }

    imageView->normToggled(true);
    CHECK(imageView->normGroup()->isVisible() == fixture.value(QStringLiteral("norm_group_visible")).toBool());

    auto* divideRadio = imageView->normGroup()->findChild<QRadioButton*>(QStringLiteral("normDivideRadio"));
    auto* frameCheck = imageView->normGroup()->findChild<QCheckBox*>(QStringLiteral("normFrameCheck"));
    CHECK(divideRadio != nullptr);
    CHECK(frameCheck != nullptr);
    divideRadio->setChecked(true);
    frameCheck->setChecked(true);
    imageView->normRadioChanged();

    const double expectedPixel = fixture.value(QStringLiteral("normalized_sample_pixel")).toDouble();
    CHECK(nearlyEqual(imageView->normalizedSamplePixel(0, 1, 1), expectedPixel));

    imageView->roiButton()->setChecked(true);
    imageView->roiClicked();
    QCoreApplication::processEvents();

    CHECK(compareCurvesToFixture(*imageView, fixture.value(QStringLiteral("startup_curves")).toArray()));

    QTemporaryDir tempDir;
    CHECK(tempDir.isValid());
    const QString exportBase = tempDir.filePath(QStringLiteral("stack.png"));
    imageView->exportImage(exportBase);
    const QStringList exported = QDir(tempDir.path()).entryList(QStringList{QStringLiteral("stack*.png")}, QDir::Files);
    const QJsonArray expectedHashes = fixture.value(QStringLiteral("export_hashes")).toArray();
    CHECK(exported.size() == fixture.value(QStringLiteral("export_file_count")).toInt());
    CHECK(exported.size() == expectedHashes.size());
    for (int index = 0; index < exported.size(); ++index) {
        CHECK(sha256File(tempDir.filePath(exported.at(index))) == expectedHashes.at(index).toString());
    }

    return 0;
}
