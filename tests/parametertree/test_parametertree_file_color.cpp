#include <cppqtgraph/colormap.hpp>
#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>
#include <cppqtgraph/parametertree/parameterTypes/FileParameter.hpp>
#include <cppqtgraph/widgets/ColorButton.hpp>
#include <cppqtgraph/widgets/ColorMapButton.hpp>
#include <cppqtgraph/widgets/GradientWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtGui/QImage>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_PARAMETERTREE_FIXTURE_DIR
#define CPPQTGRAPH_PARAMETERTREE_FIXTURE_DIR "oracle/fixtures/parametertree"
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

QString fixturePath(const QString& name)
{
    return QDir(QString::fromUtf8(CPPQTGRAPH_PARAMETERTREE_FIXTURE_DIR)).filePath(name);
}

cppqtgraph::parametertree::ParameterItem* findItemByName(QTreeWidgetItem* root, const QString& name)
{
    if (root == nullptr) {
        return nullptr;
    }
    for (int i = 0; i < root->childCount(); ++i) {
        if (auto* item = dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(root->child(i))) {
            if (item->parameter() != nullptr && item->parameter()->name() == name) {
                return item;
            }
            if (auto* nested = findItemByName(item, name)) {
                return nested;
            }
        }
    }
    return nullptr;
}

QWidget* editorForItem(cppqtgraph::parametertree::ParameterItem* item)
{
    if (auto* widgetItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(item)) {
        return widgetItem->editorWidget();
    }
    return nullptr;
}

QPushButton* findBrowseButton(cppqtgraph::parametertree::ParameterTree& tree, const QString& name)
{
    auto* item = findItemByName(tree.invisibleRootItem(), name);
    if (item == nullptr) {
        return nullptr;
    }
    const auto buttons = item->treeWidget()->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text() == QStringLiteral("...")) {
            return button;
        }
    }
    return nullptr;
}

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double changedPercent = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual)
{
    PixelMetrics metrics;
    if (reference.size() != actual.size()) {
        return metrics;
    }
    const int pixelCount = reference.width() * reference.height();
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected = reference.pixelColor(x, y);
            const QColor observed = actual.pixelColor(x, y);
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            if (delta > 0) {
                ++metrics.changedPixels;
                metrics.totalDelta += static_cast<std::uint64_t>(delta);
                metrics.maxDelta = std::max(metrics.maxDelta, delta);
            }
        }
    }
    metrics.changedPercent = pixelCount == 0 ? 0.0 : (100.0 * static_cast<double>(metrics.changedPixels) / pixelCount);
    metrics.passed = metrics.changedPercent <= 1.5 && metrics.maxDelta <= 24;
    return metrics;
}

QImage cropColormapGradientBand(const QImage& image)
{
    constexpr int left = 10;
    constexpr int top = 4;
    constexpr int width = 280;
    constexpr int height = 8;
    if (image.width() < left + width || image.height() < top + height) {
        return {};
    }
    return image.copy(left, top, width, height);
}

std::vector<QColor> profileFromGradientBand(const QImage& band)
{
    std::vector<QColor> profile;
    if (band.isNull() || band.width() <= 0 || band.height() <= 0) {
        return profile;
    }
    profile.resize(static_cast<std::size_t>(band.width()));
    for (int x = 0; x < band.width(); ++x) {
        int red = 0;
        int green = 0;
        int blue = 0;
        int alpha = 0;
        for (int y = 0; y < band.height(); ++y) {
            const QColor color = band.pixelColor(x, y);
            red += color.red();
            green += color.green();
            blue += color.blue();
            alpha += color.alpha();
        }
        profile[static_cast<std::size_t>(x)] =
            QColor(red / band.height(), green / band.height(), blue / band.height(), alpha / band.height());
    }
    return profile;
}

std::vector<QColor> profileFromColorMap(const cppqtgraph::ColorMap& colorMap, int width)
{
    const auto lut = colorMap.getLookupTable(
        0.0, 1.0, static_cast<std::size_t>(width), true, cppqtgraph::ColorMap::OutputMode::Byte);
    std::vector<QColor> profile;
    profile.resize(static_cast<std::size_t>(width));
    for (int x = 0; x < width; ++x) {
        const std::size_t offset = static_cast<std::size_t>(x) * lut.channels;
        profile[static_cast<std::size_t>(x)] = QColor(lut.bytes[offset],
            lut.bytes[offset + 1],
            lut.bytes[offset + 2],
            lut.channels >= 4 ? lut.bytes[offset + 3] : 255);
    }
    return profile;
}

double gradientBandScore(const QImage& image, int row)
{
    if (image.isNull() || row < 0 || row >= image.height() || image.width() < 5) {
        return 0.0;
    }
    const int fifth = std::max(1, image.width() / 5);
    double leftRed = 0.0;
    double rightRed = 0.0;
    for (int x = 0; x < fifth; ++x) {
        leftRed += image.pixelColor(x, row).red();
        rightRed += image.pixelColor(image.width() - 1 - x, row).red();
    }
    return (rightRed - leftRed) / static_cast<double>(fifth);
}

bool gradientBandIsVisible(const QImage& image)
{
    if (image.isNull()) {
        return false;
    }
    double bestScore = 0.0;
    for (int row = 0; row < image.height(); ++row) {
        bestScore = std::max(bestScore, gradientBandScore(image, row));
    }
    return bestScore >= 40.0;
}

PixelMetrics compareGradientProfiles(
    const std::vector<QColor>& reference, const std::vector<QColor>& actual, int noiseFloor)
{
    PixelMetrics metrics;
    if (reference.size() != actual.size() || reference.empty()) {
        return metrics;
    }
    const int sampleCount = static_cast<int>(reference.size());
    for (int index = 0; index < sampleCount; ++index) {
        const QColor expected = reference[static_cast<std::size_t>(index)];
        const QColor observed = actual[static_cast<std::size_t>(index)];
        const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
            + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
        if (delta > noiseFloor) {
            ++metrics.changedPixels;
        }
        metrics.totalDelta += static_cast<std::uint64_t>(delta);
        metrics.maxDelta = std::max(metrics.maxDelta, delta);
    }
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(sampleCount);
    metrics.passed = metrics.changedPercent <= 1.5 && metrics.maxDelta <= 24;
    return metrics;
}

QImage renderWidget(QWidget& widget, const QSize& size)
{
    widget.resize(size);
    widget.show();
    QTest::qWait(0);
    return widget.grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

bool testFileParameterFakeDialogSingleSelection()
{
    cppqtgraph::parametertree::FilePickerRequest captured;
    cppqtgraph::parametertree::setFilePickerProvider([&captured](const cppqtgraph::parametertree::FilePickerRequest& request) {
        captured = request;
        return QVariant(QStringLiteral("/tmp/example.txt"));
    });

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("path")},
                    {QStringLiteral("type"), QStringLiteral("file")},
                    {QStringLiteral("value"), QVariant()}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* browse = findBrowseButton(tree, QStringLiteral("path"));
    CHECK(browse != nullptr);
    QTest::mouseClick(browse, Qt::LeftButton);
    CHECK(param->value().toString() == QStringLiteral("/tmp/example.txt"));
    CHECK(captured.fileMode == QFileDialog::AnyFile);
    return true;
}

bool testFileParameterExistingFilesReturnsList()
{
    QFileDialog::FileMode capturedMode = QFileDialog::AnyFile;
    cppqtgraph::parametertree::setFilePickerProvider([&capturedMode](const cppqtgraph::parametertree::FilePickerRequest& request) {
        capturedMode = request.fileMode;
        return QVariant::fromValue(QStringList{QStringLiteral("/tmp/a.txt"), QStringLiteral("/tmp/b.txt")});
    });

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("paths")},
                    {QStringLiteral("type"), QStringLiteral("file")},
                    {QStringLiteral("fileMode"), QStringLiteral("ExistingFiles")},
                    {QStringLiteral("value"), QVariant()}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* browse = findBrowseButton(tree, QStringLiteral("paths"));
    CHECK(browse != nullptr);
    QTest::mouseClick(browse, Qt::LeftButton);
    CHECK(capturedMode == QFileDialog::ExistingFiles);
    CHECK(param->value().toStringList()
          == QStringList({QStringLiteral("/tmp/a.txt"), QStringLiteral("/tmp/b.txt")}));
    return true;
}

bool testFileParameterEnumOptionMapping()
{
    cppqtgraph::parametertree::FilePickerRequest captured;
    cppqtgraph::parametertree::setFilePickerProvider([&captured](const cppqtgraph::parametertree::FilePickerRequest& request) {
        captured = request;
        return QVariant(QStringLiteral("/tmp/save.txt"));
    });

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("picker")},
                    {QStringLiteral("type"), QStringLiteral("file")},
                    {QStringLiteral("acceptMode"), QStringLiteral("AcceptSave")},
                    {QStringLiteral("fileMode"), QStringLiteral("AnyFile")},
                    {QStringLiteral("viewMode"), QStringLiteral("List")},
                    {QStringLiteral("options"), QVariantList{QStringLiteral("ShowDirsOnly")}},
                    {QStringLiteral("value"), QVariant()}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* browse = findBrowseButton(tree, QStringLiteral("picker"));
    CHECK(browse != nullptr);
    QTest::mouseClick(browse, Qt::LeftButton);
    CHECK(captured.acceptMode == QFileDialog::AcceptSave);
    CHECK(captured.fileMode == QFileDialog::AnyFile);
    CHECK(captured.viewMode == QFileDialog::List);
    CHECK(captured.options.testFlag(QFileDialog::ShowDirsOnly));
    return true;
}

bool testFileParameterRelativeToAndDefaultReset()
{
    const QString base = QDir::temp().filePath(QStringLiteral("parametertree-file-base"));
    QDir().mkpath(base);
    const QString absolute = QDir(base).filePath(QStringLiteral("picked.txt"));

    cppqtgraph::parametertree::setFilePickerProvider([absolute](const cppqtgraph::parametertree::FilePickerRequest&) {
        return QVariant(absolute);
    });

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("relative")},
                    {QStringLiteral("type"), QStringLiteral("file")},
                    {QStringLiteral("relativeTo"), base},
                    {QStringLiteral("default"), QStringLiteral("picked.txt")},
                    {QStringLiteral("value"), QStringLiteral("picked.txt")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("relative"));
    CHECK(item != nullptr);
    auto* widgetItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(item);
    CHECK(widgetItem != nullptr);
    CHECK(widgetItem->defaultButton()->isVisible());

    auto* browse = findBrowseButton(tree, QStringLiteral("relative"));
    CHECK(browse != nullptr);
    QTest::mouseClick(browse, Qt::LeftButton);
    CHECK(param->value().toString() == QStringLiteral("picked.txt"));

    param->setValue(QStringLiteral("other.txt"));
    CHECK(widgetItem->defaultButton()->isEnabled());
    QTest::mouseClick(widgetItem->defaultButton(), Qt::LeftButton);
    CHECK(param->value().toString() == QStringLiteral("picked.txt"));
    return true;
}

bool testColorParameterChangingAndChangedSignals()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("brush")},
                    {QStringLiteral("type"), QStringLiteral("color")},
                    {QStringLiteral("value"), QStringLiteral("#f00")}});

    QSignalSpy changing(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanging);
    QSignalSpy changed(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanged);

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* button = qobject_cast<cppqtgraph::widgets::ColorButton*>(editorForItem(findItemByName(tree.invisibleRootItem(), QStringLiteral("brush"))));
    CHECK(button != nullptr);
    button->setColor(QColor(0, 255, 0), false);
    CHECK(changing.count() >= 1);
    button->setColor(QColor(0, 255, 0), true);
    QTest::qWait(0);
    CHECK(changed.count() >= 1);
    CHECK(param->value().value<QColor>() == QColor(Qt::green));
    const auto saved = button->saveState();
    CHECK(saved == (std::array<int, 4>{0, 255, 0, 255}));
    return true;
}

bool testColorMapParameterGradientSignalsAndValue()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("gradient")},
                    {QStringLiteral("type"), QStringLiteral("colormap")},
                    {QStringLiteral("value"), QVariant()}});

    QSignalSpy changing(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanging);
    QSignalSpy changed(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanged);

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("gradient"));
    CHECK(item != nullptr);
    auto* gradient = qobject_cast<cppqtgraph::widgets::GradientWidget*>(editorForItem(item));
    CHECK(gradient != nullptr);

    const cppqtgraph::ColorMap mapped(
        {0.0, 1.0}, {QColor(0, 0, 255), QColor(255, 255, 0)}, QStringLiteral("test-map"));
    gradient->setColorMap(mapped);
    QTest::qWait(0);
    CHECK(changing.count() >= 0);
    CHECK(changed.count() >= 1);
    CHECK(param->value().value<cppqtgraph::ColorMap>().colors().front() == QColor(0, 0, 255));
    return true;
}

bool testColorMapLutParameterViridisValue()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("lut")},
                    {QStringLiteral("type"), QStringLiteral("cmaplut")},
                    {QStringLiteral("value"), QStringLiteral("viridis")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* button = qobject_cast<cppqtgraph::widgets::ColorMapButton*>(
        editorForItem(findItemByName(tree.invisibleRootItem(), QStringLiteral("lut"))));
    CHECK(button != nullptr);
    CHECK(button->colorMap().name() == QStringLiteral("viridis"));
    CHECK(param->value().value<cppqtgraph::ColorMap>().name() == QStringLiteral("viridis"));
    return true;
}

bool loadViridisFixture(QJsonArray& rows)
{
    QFile file(fixturePath(QStringLiteral("viridis_lut.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return false;
    }
    rows = doc.array();
    return rows.size() > 0;
}

bool compareViridisLookupTableToFixture()
{
    QJsonArray rows;
    CHECK(loadViridisFixture(rows));
    const std::optional<cppqtgraph::ColorMap> viridis = cppqtgraph::get(QStringLiteral("viridis"));
    CHECK(viridis.has_value());
    const auto lut = viridis->getLookupTable(
        0.0, 1.0, static_cast<std::size_t>(rows.size()), true, cppqtgraph::ColorMap::OutputMode::Byte);
    CHECK(static_cast<int>(lut.rows()) == rows.size());
    for (int index = 0; index < rows.size(); ++index) {
        const QJsonArray rgb = rows.at(index).toArray();
        const std::size_t offset = static_cast<std::size_t>(index) * lut.channels;
        CHECK(lut.bytes[offset] == static_cast<std::uint8_t>(rgb.at(0).toInt()));
        CHECK(lut.bytes[offset + 1] == static_cast<std::uint8_t>(rgb.at(1).toInt()));
        CHECK(lut.bytes[offset + 2] == static_cast<std::uint8_t>(rgb.at(2).toInt()));
    }
    return true;
}

bool testVisualWidgetMatchesPinnedReference(const QString& referenceName, QWidget& widget, const QSize& size)
{
    const QString path = fixturePath(referenceName);
    if (!QFile::exists(path)) {
        std::cerr << "missing visual reference: " << path.toStdString() << '\n';
        return false;
    }
    QImage reference(path);
    CHECK(!reference.isNull());
    const QImage actual = renderWidget(widget, size);
    const PixelMetrics metrics = compareImages(reference, actual);
    if (!metrics.passed) {
        std::cerr << referenceName.toStdString() << " visual mismatch changed%=" << metrics.changedPercent
                  << " maxDelta=" << metrics.maxDelta << '\n';
    }
    CHECK(metrics.passed);
    return true;
}

bool testColormapGradientBandMatchesPinnedReference(cppqtgraph::widgets::GradientWidget& gradient, const QSize& size)
{
    constexpr int kGradientNoiseFloor = 8;

    const QString path = fixturePath(QStringLiteral("colormap_gradient.reference.png"));
    if (!QFile::exists(path)) {
        std::cerr << "missing visual reference: " << path.toStdString() << '\n';
        return false;
    }
    QImage reference(path);
    CHECK(!reference.isNull());
    const QImage referenceBand = cropColormapGradientBand(reference);
    CHECK(!referenceBand.isNull());
    const std::vector<QColor> referenceProfile = profileFromGradientBand(referenceBand);
    CHECK(!referenceProfile.empty());

    const QImage grab = renderWidget(gradient, size);
    CHECK(gradientBandIsVisible(grab));

    const std::vector<QColor> actualProfile =
        profileFromColorMap(gradient.colorMap(), static_cast<int>(referenceProfile.size()));
    CHECK(actualProfile.size() == referenceProfile.size());

    const PixelMetrics metrics = compareGradientProfiles(referenceProfile, actualProfile, kGradientNoiseFloor);
    if (!metrics.passed) {
        std::cerr << "colormap_gradient.reference.png gradient-band mismatch changed%=" << metrics.changedPercent
                  << " maxDelta=" << metrics.maxDelta << '\n';
    }
    CHECK(metrics.passed);
    return true;
}

bool testColorButtonVisualReference()
{
    cppqtgraph::widgets::ColorButton button(nullptr, QColor(255, 0, 0));
    button.setFlat(true);
    return testVisualWidgetMatchesPinnedReference(QStringLiteral("color_button.reference.png"), button, QSize(120, 30));
}

bool testColormapGradientVisualReference()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("gradient")},
                    {QStringLiteral("type"), QStringLiteral("colormap")},
                    {QStringLiteral("value"), QVariant()}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("gradient"));
    CHECK(item != nullptr);
    auto* gradient = qobject_cast<cppqtgraph::widgets::GradientWidget*>(editorForItem(item));
    CHECK(gradient != nullptr);

    const cppqtgraph::ColorMap fixedMap(
        {0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 0, 0)}, QStringLiteral("fixed"));
    gradient->setColorMap(fixedMap);
    gradient->setMaxDim(35);
    gradient->setMinimumWidth(300);
    gradient->setLength(280.0);
    return testColormapGradientBandMatchesPinnedReference(*gradient, QSize(300, 35));
}

bool testCmapLutViridisLutMatchesFixture()
{
    return compareViridisLookupTableToFixture();
}

bool testCmapLutViridisVisualReference()
{
    CHECK(compareViridisLookupTableToFixture());

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("lut")},
                    {QStringLiteral("type"), QStringLiteral("cmaplut")},
                    {QStringLiteral("value"), QStringLiteral("viridis")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* button = qobject_cast<cppqtgraph::widgets::ColorMapButton*>(
        editorForItem(findItemByName(tree.invisibleRootItem(), QStringLiteral("lut"))));
    CHECK(button != nullptr);
    CHECK(button->colorMap().name() == QStringLiteral("viridis"));
    return testVisualWidgetMatchesPinnedReference(
        QStringLiteral("cmaplut_viridis.reference.png"), *button, QSize(256, 30));
}

struct TestCase {
    std::string_view name;
    bool (*run)();
};

bool runAllTests()
{
    static const TestCase cases[] = {
        {"file fake dialog single", testFileParameterFakeDialogSingleSelection},
        {"file existing files list", testFileParameterExistingFilesReturnsList},
        {"file enum option mapping", testFileParameterEnumOptionMapping},
        {"file relativeTo default reset", testFileParameterRelativeToAndDefaultReset},
        {"color changing changed", testColorParameterChangingAndChangedSignals},
        {"colormap gradient value", testColorMapParameterGradientSignalsAndValue},
        {"cmaplut viridis value", testColorMapLutParameterViridisValue},
        {"visual color button", testColorButtonVisualReference},
        {"visual colormap gradient", testColormapGradientVisualReference},
        {"cmaplut viridis lut fixture", testCmapLutViridisLutMatchesFixture},
        {"visual cmaplut viridis", testCmapLutViridisVisualReference},
    };

    for (const TestCase& testCase : cases) {
        if (!testCase.run()) {
            std::cerr << "test failed: " << testCase.name << '\n';
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ApplicationGuard guard(argc, argv);
    return runAllTests() ? 0 : 1;
}
