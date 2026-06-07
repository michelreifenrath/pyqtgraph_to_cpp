#include <pyqtgraph/colormap.hpp>
#include <pyqtgraph/core/ArrayView.hpp>
#include <pyqtgraph/functions.hpp>
#include <pyqtgraph/graphicsItems/ColorBarItem.hpp>
#include <pyqtgraph/graphicsItems/HistogramLUTItem.hpp>
#include <pyqtgraph/graphicsItems/ImageItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#ifndef PYQTGRAPH_CPP_P4_06_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_06_ARTIFACT_DIR "artifacts/P4.06"
#endif

namespace {

bool nearly(double actual, double expected, double tolerance = 1.0e-6)
{
    return std::abs(actual - expected) <= tolerance;
}

int fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}

QJsonArray levelsJson(const std::pair<double, double>& levels)
{
    QJsonArray array;
    array.append(levels.first);
    array.append(levels.second);
    return array;
}

QJsonArray imageLevelsJson(const std::optional<pyqtgraph::ImageLevelRange>& levels)
{
    QJsonArray array;
    if (levels.has_value()) {
        array.append(levels->minimum);
        array.append(levels->maximum);
    }
    return array;
}

QJsonObject lutJson(const std::optional<pyqtgraph::ImageLookupTable>& lut)
{
    QJsonObject object;
    object.insert(QStringLiteral("present"), lut.has_value());
    object.insert(QStringLiteral("rows"), lut.has_value() ? static_cast<int>(lut->rows) : 0);
    object.insert(QStringLiteral("channels"), lut.has_value() ? static_cast<int>(lut->channels) : 0);
    if (lut.has_value() && lut->data != nullptr && lut->rows > 0 && lut->channels > 0) {
        object.insert(QStringLiteral("firstRed"), static_cast<int>(lut->data[0]));
        const auto lastOffset = static_cast<std::ptrdiff_t>(lut->rows - 1) * lut->rowStride;
        object.insert(QStringLiteral("lastRed"), static_cast<int>(lut->data[lastOffset]));
    }
    return object;
}

QJsonObject signalJson(int lookupChanged, int histLevelsChanged, int histLevelsFinished, int colorLevelsChanged, int colorLevelsFinished)
{
    return QJsonObject{{QStringLiteral("histogramLookupChanged"), lookupChanged},
                       {QStringLiteral("histogramLevelsChanged"), histLevelsChanged},
                       {QStringLiteral("histogramLevelsFinished"), histLevelsFinished},
                       {QStringLiteral("colorBarLevelsChanged"), colorLevelsChanged},
                       {QStringLiteral("colorBarLevelsFinished"), colorLevelsFinished}};
}

pyqtgraph::ColorMap testColorMap()
{
    return pyqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 64, 32)}, QStringLiteral("P4.06-test"));
}

bool writeReport(const QJsonObject& report)
{
    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P4_06_ARTIFACT_DIR);
    QDir().mkpath(artifactDir);
    QFile file(artifactDir + QStringLiteral("/HistogramLUT_ColorBar_interaction.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << "could not open P4.06 interaction report artifact\n";
        return false;
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    using pyqtgraph::ImageLevelRange;
    using pyqtgraph::core::ArrayView;
    using pyqtgraph::graphicsItems::ColorBarItem;
    using pyqtgraph::graphicsItems::HistogramLUTItem;
    using pyqtgraph::graphicsItems::ImageItem;

    QJsonObject report;
    QJsonArray events;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.06"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/HistogramLUTItem.py:74-91,253-304,360-443; pyqtgraph/graphicsItems/ColorBarItem.py:32-176,178-282,288-340; tests/widgets/test_histogramlutwidget.py:15-37; pyqtgraph/examples/ColorBarItem.py:24-70"));

    std::array<std::uint8_t, 4> data{{0, 64, 128, 255}};
    ImageItem image;
    image.setImage(ArrayView<const std::uint8_t, 2>(data.data(), {2, 2}));

    HistogramLUTItem histogram;
    int histogramLookupChanged = 0;
    int histogramLevelsChanged = 0;
    int histogramLevelsFinished = 0;
    QObject::connect(&histogram, &HistogramLUTItem::sigLookupTableChanged, &histogram, [&histogramLookupChanged](HistogramLUTItem*) { ++histogramLookupChanged; });
    QObject::connect(&histogram, &HistogramLUTItem::sigLevelsChanged, &histogram, [&histogramLevelsChanged](HistogramLUTItem*) { ++histogramLevelsChanged; });
    QObject::connect(&histogram, &HistogramLUTItem::sigLevelChangeFinished, &histogram, [&histogramLevelsFinished](HistogramLUTItem*) { ++histogramLevelsFinished; });

    report.insert(QStringLiteral("preState"), QJsonObject{{QStringLiteral("imageLevels"), imageLevelsJson(image.getLevels())},
                                                 {QStringLiteral("imageLut"), lutJson(image.lookupTable())},
                                                 {QStringLiteral("histogramLevels"), levelsJson(histogram.getLevels())},
                                                 {QStringLiteral("histogramLevelMode"), histogram.levelMode()},
                                                 {QStringLiteral("signals"), signalJson(histogramLookupChanged, histogramLevelsChanged, histogramLevelsFinished, 0, 0)}});

    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("HistogramLUTItem")},
                              {QStringLiteral("name"), QStringLiteral("link ImageItem and apply initial levels")}});
    histogram.setImageItem(&image);
    const auto linkedLevels = image.getLevels();
    if (!linkedLevels.has_value() || !nearly(linkedLevels->minimum, 0.0) || !nearly(linkedLevels->maximum, 1.0)) {
        return fail("HistogramLUTItem link should apply current mono levels to ImageItem");
    }

    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("HistogramLUTItem")},
                              {QStringLiteral("name"), QStringLiteral("move level region while dragging")},
                              {QStringLiteral("levels"), levelsJson(std::make_pair(10.0, 200.0))}});
    histogram.setLevels(10.0, 200.0);
    histogram.regionChanging();
    const auto changingLevels = image.getLevels();
    if (!changingLevels.has_value() || !nearly(changingLevels->minimum, 10.0) || !nearly(changingLevels->maximum, 200.0)
        || histogramLevelsChanged < 1) {
        return fail("HistogramLUTItem regionChanging should update image levels and emit sigLevelsChanged");
    }
    histogram.regionChanged();
    if (histogramLevelsFinished < 2) {
        return fail("HistogramLUTItem regionChanged should emit sigLevelChangeFinished");
    }

    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("HistogramLUTItem")},
                              {QStringLiteral("name"), QStringLiteral("change gradient/color map")}});
    histogram.setColorMap(testColorMap());
    const auto histLut = image.lookupTable();
    if (!histLut.has_value() || histLut->rows != 256 || histLut->channels != 4 || histogramLookupChanged != 1) {
        return fail("HistogramLUTItem color map change should materialize a 256-row RGBA lookup table and emit signal");
    }

    std::array<std::uint16_t, 4> secondData{{0, 128, 512, 1023}};
    ImageItem imageA;
    ImageItem imageB;
    imageA.setImage(ArrayView<const std::uint16_t, 2>(secondData.data(), {2, 2}));
    imageB.setImage(ArrayView<const std::uint16_t, 2>(secondData.data(), {2, 2}));
    ColorBarItem colorBar(std::make_pair(0.0, 100.0), 25.0, std::nullopt, true, std::make_pair(0.0, 300.0), 10.0);
    int colorLevelsChanged = 0;
    int colorLevelsFinished = 0;
    QObject::connect(&colorBar, &ColorBarItem::sigLevelsChanged, &colorBar, [&colorLevelsChanged](ColorBarItem*) { ++colorLevelsChanged; });
    QObject::connect(&colorBar, &ColorBarItem::sigLevelsChangeFinished, &colorBar, [&colorLevelsFinished](ColorBarItem*) { ++colorLevelsFinished; });

    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("ColorBarItem")},
                              {QStringLiteral("name"), QStringLiteral("link two ImageItems and apply levels/LUT")}});
    colorBar.setImageItems({&imageA, &imageB});
    colorBar.setColorMap(testColorMap());
    colorBar.setLevels(std::nullopt, 20.0, 120.0);
    for (const auto* item : {&imageA, &imageB}) {
        const auto levels = item->getLevels();
        const auto lut = item->lookupTable();
        if (!levels.has_value() || !nearly(levels->minimum, 20.0) || !nearly(levels->maximum, 120.0) || !lut.has_value()
            || lut->rows != 256 || lut->channels != 4) {
            return fail("ColorBarItem should update all linked images with levels and LUT");
        }
    }

    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("ColorBarItem")},
                              {QStringLiteral("name"), QStringLiteral("interactive handle displacement")},
                              {QStringLiteral("handleRegion"), levelsJson(std::make_pair(31.0, 319.0))}});
    colorBar.setRegionChangedEnabled(false);
    colorBar.interactionRegion()->setRegion(std::make_pair(31.0, 319.0));
    colorBar.setRegionChangedEnabled(true);
    colorBar.regionChanging();
    const auto adjusted = colorBar.levels();
    if (!nearly(adjusted.first, 200.0) || !nearly(adjusted.second, 300.0) || colorLevelsChanged < 1) {
        return fail("ColorBarItem regionChanging should apply quadratic rounded/clipped level adjustment and emit signal");
    }
    colorBar.regionChanged();
    const auto resetRegion = colorBar.interactionRegion()->getRegion();
    if (!nearly(resetRegion.first, 63.0) || !nearly(resetRegion.second, 191.0) || colorLevelsFinished != 1) {
        return fail("ColorBarItem regionChanged should reset handles and emit finished signal");
    }

    const int colorChangedBeforeNoOp = colorLevelsChanged;
    const auto levelsBeforeNoOp = colorBar.levels();
    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("ColorBarItem")},
                              {QStringLiteral("name"), QStringLiteral("disabled region-changed no-op")}});
    colorBar.setRegionChangedEnabled(false);
    colorBar.interactionRegion()->setRegion(std::make_pair(63.0, 255.0));
    colorBar.regionChanging();
    colorBar.setRegionChangedEnabled(true);
    if (colorLevelsChanged != colorChangedBeforeNoOp || colorBar.levels() != levelsBeforeNoOp) {
        return fail("ColorBarItem disabled regionChanging should be a no-op without signals");
    }

    report.insert(QStringLiteral("eventSequence"), events);
    report.insert(QStringLiteral("postState"), QJsonObject{{QStringLiteral("histogramLevels"), levelsJson(histogram.getLevels())},
                                                  {QStringLiteral("histogramLevelMode"), histogram.levelMode()},
                                                  {QStringLiteral("histogramImageLevels"), imageLevelsJson(image.getLevels())},
                                                  {QStringLiteral("histogramImageLut"), lutJson(image.lookupTable())},
                                                  {QStringLiteral("colorBarLevels"), levelsJson(colorBar.levels())},
                                                  {QStringLiteral("colorBarImageALevels"), imageLevelsJson(imageA.getLevels())},
                                                  {QStringLiteral("colorBarImageBLut"), lutJson(imageB.lookupTable())}});
    report.insert(QStringLiteral("signals"), signalJson(histogramLookupChanged, histogramLevelsChanged, histogramLevelsFinished, colorLevelsChanged, colorLevelsFinished));
    report.insert(QStringLiteral("negativeNoOp"), QJsonObject{{QStringLiteral("colorChangedBefore"), colorChangedBeforeNoOp},
                                                    {QStringLiteral("colorChangedAfter"), colorLevelsChanged},
                                                    {QStringLiteral("levelsBefore"), levelsJson(levelsBeforeNoOp)},
                                                    {QStringLiteral("levelsAfter"), levelsJson(colorBar.levels())}});

    if (!writeReport(report)) {
        return 1;
    }
    return 0;
}
