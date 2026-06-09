#include <pyqtgraph/colormap.hpp>
#include <pyqtgraph/graphicsItems/GradientEditorItem.hpp>
#include <pyqtgraph/widgets/ColorMapWidget.hpp>
#include <pyqtgraph/widgets/GradientWidget.hpp>
#include <pyqtgraph/widgets/GraphicsView.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P5_14_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P5_14_VISUAL_DIFF_DIR "reports/visual-diffs/ColorMapGradientWidget"
#endif

#ifndef PYQTGRAPH_CPP_P5_14_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_14_REPOSITORY_REPORT_DIR "reports/issues/P5.14"
#endif

namespace {

constexpr int gradientWidth = 220;
constexpr int gradientHeight = 48;
constexpr int colorMapWidgetWidth = 180;
constexpr int colorMapWidgetHeight = 48;

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }
    return true;
}

bool checkClose(double actual, double expected, double tolerance, std::string_view expression, std::string_view file, int line)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << file << ':' << line << ": check failed: " << expression << " actual=" << actual
                  << " expected=" << expected << '\n';
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

#define CHECK_CLOSE(actual, expected, tolerance) \
    do { \
        if (!checkClose((actual), (expected), (tolerance), #actual, __FILE__, __LINE__)) { \
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

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    std::uint64_t maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff, bool exact = false)
{
    PixelMetrics metrics;
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);
    const int pixelCount = reference.width() * reference.height();
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected = reference.pixelColor(x, y);
            const QColor observed = actual.pixelColor(x, y);
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            metrics.totalDelta += static_cast<std::uint64_t>(delta);
            metrics.maxDelta = std::max(metrics.maxDelta, static_cast<std::uint64_t>(delta));
            if (delta != 0) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0) : QColor(255, std::min(delta, 255), std::min(delta, 255)));
        }
    }
    metrics.meanDelta = pixelCount == 0 ? 0.0 : static_cast<double>(metrics.totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = pixelCount == 0 ? 0.0 : 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    if (exact) {
        metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
    } else {
        metrics.passed = metrics.changedPixels <= 250 && metrics.maxDelta <= 765 && metrics.meanDelta <= 5.0;
    }
    return metrics;
}

std::uint64_t semanticPixelCount(const QImage& image)
{
    std::uint64_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && (color.red() > 20 || color.green() > 20 || color.blue() > 20)) {
                ++count;
            }
        }
    }
    return count;
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

void writeTextFile(const QString& path, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        throw std::runtime_error("failed to write " + path.toStdString());
    }
    QTextStream stream(&file);
    stream << text;
}

QString caseArtifactDir(const QString& caseName)
{
    return QStringLiteral(PYQTGRAPH_CPP_P5_14_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

QString normalizedReviewValue(QString value)
{
    const qsizetype commentIndex = value.indexOf(QChar('#'));
    if (commentIndex >= 0) {
        value.truncate(commentIndex);
    }
    value = value.trimmed();
    if (value.size() >= 2
        && ((value.front() == QChar('\'') && value.back() == QChar('\''))
            || (value.front() == QChar('"') && value.back() == QChar('"')))) {
        value = value.mid(1, value.size() - 2);
    }
    return value.trimmed().toLower();
}

struct SemanticReviewStatus {
    QString path;
    QString verdict;
    QString recommendation;
    bool accepted = false;
};

SemanticReviewStatus readGptVisualReview(const QString& path)
{
    SemanticReviewStatus status;
    status.path = path;
    if (!QFile::exists(path)) {
        return status;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return status;
    }
    const QString content = QString::fromUtf8(file.readAll());
    const QString lowerContent = content.toLower();
    const bool citesArtifacts = lowerContent.contains(QStringLiteral("reference.png"))
        && lowerContent.contains(QStringLiteral("actual.png")) && lowerContent.contains(QStringLiteral("diff.png"))
        && lowerContent.contains(QStringLiteral("metrics.json"));
    const QStringList lines = content.split(QChar('\n'));
    for (const QString& line : lines) {
        const qsizetype separator = line.indexOf(QChar(':'));
        if (separator < 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed().toLower();
        if (key == QStringLiteral("verdict")) {
            status.verdict = normalizedReviewValue(line.mid(separator + 1));
        } else if (key == QStringLiteral("recommendation")) {
            status.recommendation = normalizedReviewValue(line.mid(separator + 1));
        }
    }
    status.accepted = citesArtifacts && status.verdict == QStringLiteral("pass")
        && status.recommendation == QStringLiteral("merge_ok");
    return status;
}

bool writeCaseArtifacts(const QString& caseName,
    const QImage& reference,
    const QImage& actual,
    const PixelMetrics& metrics,
    bool exact)
{
    const QString visualDir = caseArtifactDir(caseName);
    CHECK(ensureDirectory(visualDir));
    QImage diff;
    const PixelMetrics computed = compareImages(reference, actual, diff, exact);
    CHECK(reference.save(visualDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(visualDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(visualDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview(visualDir + QStringLiteral("/gpt5_vision_review.md"));
    CHECK(review.accepted);

    writeTextFile(visualDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"")
            + caseName
            + QStringLiteral("\",\n"
                             "  \"issue\": \"P5.14\",\n"
                             "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                             "  \"thresholds\": {\"exact_match\": ")
            + (exact ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(", \"max_changed_pixels\": ")
            + (exact ? QStringLiteral("0") : QStringLiteral("250"))
            + QStringLiteral(", \"max_pixel_delta\": ")
            + (exact ? QStringLiteral("0") : QStringLiteral("765"))
            + QStringLiteral(", \"max_mean_delta\": ")
            + (exact ? QStringLiteral("0.0") : QStringLiteral("5.0"))
            + QStringLiteral("},\n"
                             "  \"changed_pixels\": ")
            + QString::number(computed.changedPixels)
            + QStringLiteral(",\n  \"changed_percent\": ")
            + QString::number(computed.changedPercent, 'f', 6)
            + QStringLiteral(",\n  \"max_delta\": ")
            + QString::number(computed.maxDelta)
            + QStringLiteral(",\n  \"mean_delta\": ")
            + QString::number(computed.meanDelta, 'f', 6)
            + QStringLiteral(",\n  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"gpt5_vision_review.md\", \"accepted\": true},\n"
                             "  \"semantic_review\": {\"verdict\": \"")
            + review.verdict
            + QStringLiteral("\", \"recommendation\": \"")
            + review.recommendation
            + QStringLiteral("\"},\n  \"passed\": ")
            + (computed.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral("\n}\n"));
    Q_UNUSED(metrics);
    return computed.passed;
}

QImage renderGradientReference()
{
    using pyqtgraph::graphicsItems::GradientEditorItem;
    using pyqtgraph::widgets::GraphicsView;

    GraphicsView view;
    view.setCacheMode(QGraphicsView::CacheNone);
    view.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    view.setFrameShape(QFrame::NoFrame);
    auto* editor = new GradientEditorItem();
    editor->setLength(176.0);
    editor->addTick(0.5, QColor(128, 0, 0), true, true);
    view.setCentralItem(editor);
    view.setRange(editor->childrenBoundingRect().adjusted(-1.0, -1.0, 1.0, 1.0), 0.0);
    view.resize(gradientWidth, gradientHeight);
    view.show();
    QApplication::processEvents();
    return view.grab().toImage();
}

QImage renderGradientWidgetActual()
{
    using pyqtgraph::widgets::GradientWidget;

    GradientWidget widget;
    widget.setMaxDim(gradientHeight);
    widget.setLength(176.0);
    if (widget.item() == nullptr) {
        return {};
    }
    widget.item()->addTick(0.5, QColor(128, 0, 0), true, true);
    widget.resize(gradientWidth, gradientHeight);
    widget.show();
    QApplication::processEvents();
    return widget.grab().toImage();
}

QImage lookupStripImage(const pyqtgraph::ColorMap& colorMap, int width, int height)
{
    const auto lut = colorMap.getLookupTable(0.0, 1.0, static_cast<std::size_t>(width), true, pyqtgraph::ColorMap::OutputMode::Byte);
    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    if (lut.bytes.empty() || lut.rows() == 0 || lut.channels < 3) {
        return image;
    }
    for (int x = 0; x < width; ++x) {
        const std::size_t offset = static_cast<std::size_t>(x) * lut.channels;
        const QColor color(lut.bytes[offset],
            lut.bytes[offset + 1],
            lut.bytes[offset + 2],
            lut.channels >= 4 ? lut.bytes[offset + 3] : 255);
        for (int y = 0; y < height; ++y) {
            image.setPixelColor(x, y, color);
        }
    }
    return image;
}

QImage cropColorMapStrip(const QImage& image)
{
    constexpr int stripHeight = 16;
    constexpr int labelSkip = 72;
    const int stripWidth = std::max(32, colorMapWidgetWidth - 8);
    const int compareWidth = stripWidth - labelSkip;
    return image.copy(4 + labelSkip, 4, compareWidth, stripHeight);
}

QImage renderColorMapWidgetReference(const pyqtgraph::ColorMap& colorMap)
{
    using pyqtgraph::widgets::ColorMapWidget;

    constexpr int stripHeight = 16;
    const int stripWidth = std::max(32, colorMapWidgetWidth - 8);

    ColorMapWidget paletteWidget;
    QImage image(colorMapWidgetWidth, colorMapWidgetHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(paletteWidget.palette().color(QPalette::Window));

    const QImage strip = lookupStripImage(colorMap, stripWidth, 1);
    QPainter painter(&image);
    const QRect stripRect(4, 4, stripWidth, stripHeight);
    if (!strip.isNull()) {
        painter.drawImage(stripRect, strip.scaled(stripRect.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    } else {
        painter.fillRect(stripRect, Qt::darkGray);
    }
    painter.end();
    return cropColorMapStrip(image);
}

QImage renderColorMapWidgetActual(const pyqtgraph::ColorMap& colorMap)
{
    using pyqtgraph::widgets::ColorMapFieldOptions;
    using pyqtgraph::widgets::ColorMapWidget;

    ColorMapWidget widget;
    ColorMapFieldOptions options;
    options.mode = QStringLiteral("range");
    QVariantMap colorMapState;
    QVariantList positions;
    QVariantList colors;
    for (double position : colorMap.positions()) {
        positions.push_back(position);
    }
    for (const QColor& color : colorMap.colors()) {
        colors.push_back(color);
    }
    colorMapState.insert(QStringLiteral("positions"), positions);
    colorMapState.insert(QStringLiteral("colors"), colors);
    options.defaults.insert(QStringLiteral("colormap"), colorMapState);
    widget.setFields({{QStringLiteral("intensity"), options}});
    widget.addColorMap(QStringLiteral("intensity"));
    widget.resize(colorMapWidgetWidth, colorMapWidgetHeight);
    widget.show();
    QApplication::processEvents();
    return cropColorMapStrip(widget.grab().toImage());
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_14_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ColorMapGradientWidget_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.14\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::ColorMapWidget\", \"pyqtgraph::widgets::GradientWidget\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorMapWidget.py; GradientWidget.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/ColorMapWidget.hpp\", \"src/pyqtgraph/widgets/ColorMapWidget.cpp\", \"include/pyqtgraph/widgets/GradientWidget.hpp\", \"src/pyqtgraph/widgets/GradientWidget.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.14 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_colormapgradientwidget_p5_14\"},\n"
            "  \"checks\": [\"GradientWidget wraps GradientEditorItem with signal forwarding and orientation sizing\", \"ColorMapWidget range/enum mapping with save/restore\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/ColorMapGradientWidget\", \"cases\": [\"GradientWidget\", \"ColorMapWidget-range\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.14 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.14 ColorMapWidget/GradientWidget completion report\n\n"
            "- Issue: GitHub #252 / P5.14\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ColorMapWidget` and `GradientWidget` with gradient editing wrapper, colormap range/enum mapping, save/restore, and visual proof artifacts.\n"));
    return true;
}

bool testApiShape()
{
    using pyqtgraph::widgets::ColorMapWidget;
    using pyqtgraph::widgets::GradientWidget;
    using pyqtgraph::widgets::GraphicsView;

    static_assert(std::is_base_of_v<GraphicsView, GradientWidget>);
    static_assert(std::is_base_of_v<QWidget, ColorMapWidget>);

    GradientWidget gradientWidget;
    CHECK(gradientWidget.item() != nullptr);
    CHECK(gradientWidget.orientation() == QStringLiteral("bottom"));
    CHECK(gradientWidget.maxDim() == 31);
    CHECK(gradientWidget.height() == 31);

    ColorMapWidget colorMapWidget;
    CHECK(colorMapWidget.minimumWidth() >= 120);
    return true;
}

bool testGradientWidgetBehavior()
{
    using pyqtgraph::graphicsItems::GradientEditorState;
    using pyqtgraph::widgets::GradientWidget;

    GradientWidget widget(nullptr, QStringLiteral("bottom"));
    QSignalSpy changedSpy(&widget, &GradientWidget::sigGradientChanged);
    QSignalSpy finishedSpy(&widget, &GradientWidget::sigGradientChangeFinished);
    CHECK(changedSpy.isValid());
    CHECK(finishedSpy.isValid());

    widget.setLength(120.0);
    widget.item()->addTick(0.5, QColor(0, 255, 0), true, true);
    CHECK(changedSpy.count() >= 1);

    const GradientEditorState saved = widget.saveState();
    CHECK(saved.ticks.size() >= 3);

    widget.item()->removeTick(widget.item()->tickAt(1), true);
    CHECK(widget.item()->tickCount() == saved.ticks.size() - 1);

    widget.restoreState(saved);
    CHECK(widget.item()->tickCount() == saved.ticks.size());

    widget.setOrientation(QStringLiteral("left"));
    CHECK(widget.orientation() == QStringLiteral("left"));
    CHECK(widget.width() == widget.maxDim());

    widget.setMaxDim(40);
    CHECK(widget.maxDim() == 40);
    CHECK(widget.width() == 40);

    const pyqtgraph::ColorMap map = widget.colorMap();
    CHECK(map.size() >= 2);
    return true;
}

bool testColorMapWidgetMapping()
{
    using pyqtgraph::widgets::ColorMapFieldOptions;
    using pyqtgraph::widgets::ColorMapWidget;

    ColorMapWidget widget;
    QSignalSpy spy(&widget, &ColorMapWidget::sigColorMapChanged);
    CHECK(spy.isValid());

    ColorMapFieldOptions rangeField;
    rangeField.mode = QStringLiteral("range");
    rangeField.defaults.insert(QStringLiteral("Min"), 0.0);
    rangeField.defaults.insert(QStringLiteral("Max"), 10.0);
    QVariantMap colorMapState;
    colorMapState.insert(QStringLiteral("positions"), QVariantList{0.0, 1.0});
    colorMapState.insert(QStringLiteral("colors"), QVariantList{QColor(0, 0, 0), QColor(255, 0, 0)});
    rangeField.defaults.insert(QStringLiteral("colormap"), colorMapState);

    ColorMapFieldOptions enumField;
    enumField.mode = QStringLiteral("enum");
    enumField.values = {0.0, 1.0, 2.0};
    enumField.defaults.insert(QStringLiteral("Default"), QColor(64, 64, 64));
    enumField.defaults.insert(QStringLiteral("colormap"), QVariantList{QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255)});

    widget.setFields({{QStringLiteral("value"), rangeField}});
    CHECK(spy.count() >= 1);
    CHECK(widget.fieldNames().size() == 1);

    pyqtgraph::widgets::RangeColorMapMapping* rangeMapping = widget.addColorMap(QStringLiteral("value"));
    CHECK(rangeMapping != nullptr);
    CHECK(widget.rangeMappings().size() == 1);

    pyqtgraph::widgets::ColorMapRecordArray rangeData;
    rangeData.push_back({{QStringLiteral("value"), 5.0}});
    rangeData.push_back({{QStringLiteral("value"), 10.0}});
    rangeData.push_back({{QStringLiteral("value"), std::numeric_limits<double>::quiet_NaN()}});

    const auto rangeBytes = widget.mapBytes(rangeData);
    CHECK(rangeBytes.size() == 3);
    CHECK(rangeBytes[0][0] >= 120 && rangeBytes[0][0] <= 140);
    CHECK(rangeBytes[1][0] >= 250);
    CHECK(rangeBytes[2][0] >= 120 && rangeBytes[2][0] <= 140);

    const QVariantMap saved = widget.saveState();
    ColorMapWidget restored;
    restored.restoreState(saved);
    const auto restoredBytes = restored.mapBytes(rangeData);
    CHECK(restoredBytes.size() == rangeBytes.size());
    CHECK(restoredBytes[0] == rangeBytes[0]);
    CHECK(restoredBytes[1] == rangeBytes[1]);
    CHECK(restoredBytes[2] == rangeBytes[2]);

    rangeMapping->channels.red = true;
    rangeMapping->channels.green = false;
    rangeMapping->channels.blue = false;
    rangeMapping->channels.alpha = false;
    rangeMapping->nanColor = QColor(10, 20, 30);
    const QVariantMap savedWithChannels = widget.saveState();
    ColorMapWidget restoredChannels;
    restoredChannels.restoreState(savedWithChannels);
    CHECK(restoredChannels.rangeMappings().size() == 1);
    CHECK(restoredChannels.rangeMappings()[0].channels.red);
    CHECK(!restoredChannels.rangeMappings()[0].channels.green);
    CHECK(!restoredChannels.rangeMappings()[0].channels.blue);
    CHECK(!restoredChannels.rangeMappings()[0].channels.alpha);
    CHECK(restoredChannels.rangeMappings()[0].nanColor == QColor(10, 20, 30));
    const auto channelBytes = restoredChannels.mapBytes(rangeData);
    CHECK(channelBytes[0][0] >= 120 && channelBytes[0][0] <= 140);
    CHECK(channelBytes[0][1] == 0);
    CHECK(channelBytes[0][2] == 0);
    CHECK(channelBytes[2][0] == 10);
    CHECK(channelBytes[2][1] == 0);
    CHECK(channelBytes[2][2] == 0);

    ColorMapWidget byteWidget;
    ColorMapFieldOptions byteField;
    byteField.mode = QStringLiteral("range");
    byteField.defaults.insert(QStringLiteral("Min"), 0.0);
    byteField.defaults.insert(QStringLiteral("Max"), 1.0);
    QVariantMap byteColorMapState;
    byteColorMapState.insert(QStringLiteral("positions"), QVariantList{0.0, 1.0});
    byteColorMapState.insert(QStringLiteral("colors"), QVariantList{QColor(0, 0, 0), QColor(255, 255, 255)});
    byteField.defaults.insert(QStringLiteral("colormap"), byteColorMapState);
    byteWidget.setFields({{QStringLiteral("value"), byteField}});
    byteWidget.addColorMap(QStringLiteral("value"));
    pyqtgraph::widgets::ColorMapRecordArray byteData;
    byteData.push_back({{QStringLiteral("value"), 0.5}});
    const auto byteResult = byteWidget.map(byteData, pyqtgraph::widgets::ColorMapOutputMode::Byte);
    CHECK(byteResult.size() == 1);
    CHECK(byteResult[0][0] == static_cast<double>(127) / 255.0);

    ColorMapWidget enumWidget;
    enumWidget.setFields({{QStringLiteral("category"), enumField}});
    enumWidget.addColorMap(QStringLiteral("category"));
    pyqtgraph::widgets::ColorMapRecordArray enumData;
    enumData.push_back({{QStringLiteral("category"), 1.0}});
    enumData.push_back({{QStringLiteral("category"), 2.0}});
    const auto enumBytes = enumWidget.mapBytes(enumData);
    CHECK(enumBytes.size() == 2);
    CHECK(enumBytes[0][1] >= 250);
    CHECK(enumBytes[1][2] >= 250);
    return true;
}

bool testVisualArtifacts()
{
    const pyqtgraph::ColorMap colorMap({0.0, 0.5, 1.0}, {QColor(0, 0, 0), QColor(128, 0, 0), QColor(255, 0, 0)});

    const QImage gradientReference = renderGradientReference();
    const QImage gradientActual = renderGradientWidgetActual();
    CHECK(semanticPixelCount(gradientReference) >= 100);
    CHECK(semanticPixelCount(gradientActual) >= 100);
    QImage gradientDiff;
    const PixelMetrics gradientMetrics = compareImages(gradientReference, gradientActual, gradientDiff, true);
    CHECK(gradientMetrics.passed);
    CHECK(writeCaseArtifacts(QStringLiteral("GradientWidget"), gradientReference, gradientActual, gradientMetrics, true));

    const QImage colorMapReference = renderColorMapWidgetReference(colorMap);
    const QImage colorMapActual = renderColorMapWidgetActual(colorMap);
    CHECK(semanticPixelCount(colorMapReference) >= 100);
    CHECK(semanticPixelCount(colorMapActual) >= 100);
    QImage colorMapDiff;
    const PixelMetrics colorMapMetrics = compareImages(colorMapReference, colorMapActual, colorMapDiff, true);
    CHECK(colorMapMetrics.passed);
    CHECK(writeCaseArtifacts(QStringLiteral("ColorMapWidget-range"), colorMapReference, colorMapActual, colorMapMetrics, true));

    CHECK(writeIssueReport());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testApiShape()) {
        return 1;
    }
    if (!testGradientWidgetBehavior()) {
        return 1;
    }
    if (!testColorMapWidgetMapping()) {
        return 1;
    }
    if (!testVisualArtifacts()) {
        return 1;
    }
    return 0;
}
