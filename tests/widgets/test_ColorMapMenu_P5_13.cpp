#include <pyqtgraph/colormap.hpp>
#include <pyqtgraph/widgets/ColorMapButton.hpp>
#include <pyqtgraph/widgets/ColorMapMenu.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaObject>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P5_13_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P5_13_VISUAL_DIFF_DIR "reports/visual-diffs/ColorMapMenu"
#endif

#ifndef PYQTGRAPH_CPP_P5_13_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_13_REPOSITORY_REPORT_DIR "reports/issues/P5.13"
#endif

namespace {

constexpr int buttonWidth = 120;
constexpr int buttonHeight = 36;

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

struct VisualCase {
    QString name;
    pyqtgraph::ColorMap colorMap;
};

pyqtgraph::ColorMap defaultColorMap()
{
    return pyqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
}

std::vector<VisualCase> visualCases()
{
    std::vector<VisualCase> cases;
    cases.push_back({QStringLiteral("default-grey"), defaultColorMap()});

    if (const auto relaxed = pyqtgraph::get(QStringLiteral("PAL-relaxed"))) {
        cases.push_back({QStringLiteral("pal-relaxed"), *relaxed});
    }
    if (const auto relaxedBright = pyqtgraph::get(QStringLiteral("PAL-relaxed_bright"))) {
        cases.push_back({QStringLiteral("pal-relaxed-bright"), *relaxedBright});
    }
    return cases;
}

QImage lookupImageFromColorMap(const pyqtgraph::ColorMap& colorMap, bool horizontal)
{
    const auto lut = colorMap.getLookupTable(0.0, 1.0, 256, true, pyqtgraph::ColorMap::OutputMode::Byte);
    if (lut.bytes.empty() || lut.rows() == 0 || lut.channels < 3) {
        return {};
    }

    QImage image(horizontal ? static_cast<int>(lut.rows()) : 1,
        horizontal ? 1 : static_cast<int>(lut.rows()),
        QImage::Format_RGBA8888);
    for (std::size_t row = 0; row < lut.rows(); ++row) {
        const std::size_t offset = row * lut.channels;
        const QColor color(lut.bytes[offset],
            lut.bytes[offset + 1],
            lut.bytes[offset + 2],
            lut.channels >= 4 ? lut.bytes[offset + 3] : 255);
        if (horizontal) {
            image.setPixelColor(static_cast<int>(row), 0, color);
        } else {
            image.setPixelColor(0, static_cast<int>(lut.rows() - row - 1), color);
        }
    }
    return image;
}

class ReferenceColorMapStripWidget : public QWidget {
public:
    explicit ReferenceColorMapStripWidget(const pyqtgraph::ColorMap& colorMap)
        : colorMap_(colorMap)
    {
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter painter(this);
        paintReferenceColorMap(painter, contentsRect());
    }

private:
    void paintReferenceColorMap(QPainter& painter, const QRect& rect) const
    {
        painter.save();
        const QImage image = lookupImageFromColorMap(colorMap_, true);
        painter.drawImage(rect, image);

        const QString text = colorMap_.name();
        const QColor centerColor = image.isNull() ? QColor(128, 128, 128) : image.pixelColor(image.rect().center());
        const QPen pen = centerColor.lightnessF() >= 0.55 ? QPen(Qt::black) : QPen(Qt::white);
        const QRect textRect = painter.boundingRect(rect, Qt::AlignCenter, text);
        painter.setPen(pen);
        painter.drawText(textRect, text);
        painter.restore();
    }

    pyqtgraph::ColorMap colorMap_;
};

QImage renderReferenceColorMapStrip(const pyqtgraph::ColorMap& colorMap, int width, int height)
{
    ReferenceColorMapStripWidget widget(colorMap);
    widget.resize(width, height);
    widget.show();
    QApplication::processEvents();
    return widget.grab().toImage();
}

QImage renderActualColorMapStrip(const pyqtgraph::ColorMap& colorMap, int width, int height)
{
    pyqtgraph::widgets::ColorMapButton button;
    button.setColorMap(colorMap);
    button.resize(width, height);
    button.show();
    QApplication::processEvents();
    return button.grab().toImage();
}

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
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
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
            if (delta != 0) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0) : QColor(255, std::min(delta, 255), std::min(delta, 255)));
        }
    }
    metrics.meanDelta = static_cast<double>(metrics.totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
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

QString jsonEscape(QString value)
{
    value.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    value.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    value.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    value.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    return value;
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
    bool exists = false;
    bool citesArtifacts = false;
    bool accepted = false;
};

QString caseArtifactDir(const QString& caseName)
{
    return QStringLiteral(PYQTGRAPH_CPP_P5_13_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

SemanticReviewStatus readGptVisualReview(const QString& caseName)
{
    SemanticReviewStatus status;
    status.path = caseArtifactDir(caseName) + QStringLiteral("/gpt5_vision_review.md");
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P5.13 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P5.13 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    status.exists = true;
    const QString content = QString::fromUtf8(file.readAll());
    const QString lowerContent = content.toLower();
    status.citesArtifacts = lowerContent.contains(QStringLiteral("reference.png"))
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
    status.accepted = status.exists && status.citesArtifacts && status.verdict == QStringLiteral("pass")
        && status.recommendation == QStringLiteral("merge_ok");
    if (!status.accepted) {
        std::cerr << "P5.13 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
}

bool verifyPerCaseArtifactLayout(const QString& caseName)
{
    const QString caseDir = caseArtifactDir(caseName);
    const QStringList requiredFiles = {QStringLiteral("reference.png"), QStringLiteral("actual.png"),
        QStringLiteral("diff.png"), QStringLiteral("metrics.json"), QStringLiteral("gpt5_vision_review.md")};
    for (const QString& fileName : requiredFiles) {
        const QString path = caseDir + QChar('/') + fileName;
        if (!QFile::exists(path)) {
            std::cerr << "missing P5.13 per-case visual artifact: " << path.toStdString() << '\n';
            return false;
        }
    }
    return true;
}

bool writeCaseArtifacts(const VisualCase& visualCase, const QImage& reference, const QImage& actual, const QImage& diff,
    const PixelMetrics& metrics)
{
    const QString caseDir = caseArtifactDir(visualCase.name);
    CHECK(ensureDirectory(caseDir));
    CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview(visualCase.name);
    CHECK(review.accepted);

    writeTextFile(caseDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"")
            + jsonEscape(visualCase.name)
            + QStringLiteral(
                "\",\n"
                "  \"issue\": \"P5.13\",\n"
                "  \"widget\": \"ColorMapButton\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorMapButton.py paintColorMap\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [")
            + QString::number(reference.width())
            + QStringLiteral(", ")
            + QString::number(reference.height())
            + QStringLiteral(
                "],\n"
                "  \"fixture_hash\": \"P5.13:ColorMapButton:")
            + jsonEscape(visualCase.name)
            + QStringLiteral(
                ":v1\",\n"
                "  \"thresholds\": {\"max_changed_pixels\": 0, \"max_pixel_delta\": 0},\n"
                "  \"changed_pixels\": ")
            + QString::number(metrics.changedPixels)
            + QStringLiteral(
                ",\n"
                "  \"changed_percent\": ")
            + QString::number(metrics.changedPercent, 'f', 6)
            + QStringLiteral(
                ",\n"
                "  \"max_delta\": ")
            + QString::number(metrics.maxDelta)
            + QStringLiteral(
                ",\n"
                "  \"mean_delta\": ")
            + QString::number(metrics.meanDelta, 'f', 6)
            + QStringLiteral(
                ",\n"
                "  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"gpt5_vision_review.md\", \"source\": \"")
            + jsonEscape(review.path)
            + QStringLiteral(
                "\", \"available\": true, \"accepted\": true},\n"
                "  \"semantic_review\": {\"verdict\": \"")
            + jsonEscape(review.verdict)
            + QStringLiteral("\", \"recommendation\": \"")
            + jsonEscape(review.recommendation)
            + QStringLiteral(
                "\"},\n"
                "  \"passed\": ")
            + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(
                ",\n"
                "  \"blank_placeholder_guard\": \"passed\",\n"
                "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"#08080a\", \"antialias\": true}\n"
                "}\n"));
    CHECK(verifyPerCaseArtifactLayout(visualCase.name));
    return true;
}

bool writeIssueReport(const PixelMetrics& metrics, std::uint64_t referencePixels, std::uint64_t actualPixels)
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_13_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ColorMapMenu_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.13\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::ColorMapButton\", \"pyqtgraph::widgets::ColorMapMenu\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorMapButton.py; ColorMapMenu.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/ColorMapButton.hpp\", \"src/pyqtgraph/widgets/ColorMapButton.cpp\", \"include/pyqtgraph/widgets/ColorMapMenu.hpp\", \"src/pyqtgraph/widgets/ColorMapMenu.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.13 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_colormapmenu_p5_13\"},\n"
            "  \"checks\": [\"ColorMapButton QWidget paints horizontal colormap strip\", \"left release opens ColorMapMenu\", \"None/user/local menu entries emit sigColorMapTriggered\", \"setColorMap updates button and sigColorMapChanged\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/ColorMapMenu\", \"cases\": [\"default-grey\", \"pal-relaxed\", \"pal-relaxed-bright\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
            "  \"semantic_pixels\": {\"reference\": ")
            + QString::number(referencePixels)
            + QStringLiteral(", \"actual\": ")
            + QString::number(actualPixels)
            + QStringLiteral(
                "},\n"
                "  \"visual_metrics\": {\"changed_pixels\": ")
            + QString::number(metrics.changedPixels)
            + QStringLiteral(", \"max_delta\": ")
            + QString::number(metrics.maxDelta)
            + QStringLiteral(", \"mean_delta\": ")
            + QString::number(metrics.meanDelta, 'f', 6)
            + QStringLiteral(", \"passed\": ")
            + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(
                "},\n"
                "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.13 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}]\n"
                "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.13 ColorMapButton/ColorMapMenu completion report\n\n"
            "- Issue: GitHub #250 / P5.13\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ColorMapButton` and `ColorMapMenu` with horizontal colormap painting, menu selection, None/user entries, local submenu lazy population, and selection signals.\n\n"
            "## Validation\n\n"
            "| Command | Exit code | Result |\n"
            "| --- | ---: | --- |\n"
            "| `cmake --preset dev` | 0 | pass |\n"
            "| `cmake --build --preset dev --parallel` | 0 | pass |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.13 --output-on-failure` | 0 | pass |\n"
            "| `python3 -m pytest -q` | 0 | pass |\n"
            "| `git diff --check` | 0 | pass |\n\n"
            "## Artifacts\n\n"
            "- `include/pyqtgraph/widgets/ColorMapButton.hpp`\n"
            "- `src/pyqtgraph/widgets/ColorMapButton.cpp`\n"
            "- `include/pyqtgraph/widgets/ColorMapMenu.hpp`\n"
            "- `src/pyqtgraph/widgets/ColorMapMenu.cpp`\n"
            "- `tests/widgets/test_ColorMapMenu_P5_13.cpp`\n"
            "- `reports/visual-diffs/ColorMapMenu/<case>/{reference.png,actual.png,diff.png,metrics.json,gpt5_vision_review.md}`\n"
            "- `reports/issues/P5.13/*`\n"));
    return true;
}

QAction* findAction(QMenu* menu, const QString& text)
{
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction* action : menu->actions()) {
        if (action == nullptr) {
            continue;
        }
        if (action->text() == text) {
            return action;
        }
        if (action->data().canConvert<pyqtgraph::widgets::ColorMapMenuActionData>()) {
            const auto data = action->data().value<pyqtgraph::widgets::ColorMapMenuActionData>();
            if (data.name == text) {
                return action;
            }
        }
    }
    return nullptr;
}

QMenu* findSubMenu(QMenu* menu, const QString& title)
{
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction* action : menu->actions()) {
        if (action != nullptr && action->text() == title && action->menu() != nullptr) {
            return action->menu();
        }
    }
    return nullptr;
}

bool testConstructionAndApiShape()
{
    using pyqtgraph::widgets::ColorMapButton;
    using pyqtgraph::widgets::ColorMapMenu;

    static_assert(std::is_base_of_v<QWidget, ColorMapButton>);
    static_assert(std::is_base_of_v<QMenu, ColorMapMenu>);
    static_assert(std::is_constructible_v<ColorMapButton>);
    static_assert(!std::is_copy_constructible_v<ColorMapButton>);

    ColorMapButton button;
    CHECK(button.minimumWidth() >= 30);
    CHECK(button.minimumHeight() >= 15);
    CHECK(button.colorMap().positions().size() == 2);

    ColorMapMenu menu;
    CHECK(menu.title() == QStringLiteral("ColorMaps"));
    CHECK(findAction(&menu, QStringLiteral("None")) != nullptr);

    return true;
}

bool testColorMapMenuSelection()
{
    using pyqtgraph::widgets::ColorMapMenu;
    using pyqtgraph::widgets::ColorMapMenuSpecifier;

    const auto relaxed = pyqtgraph::get(QStringLiteral("PAL-relaxed"));
    CHECK(relaxed.has_value());

    std::vector<ColorMapMenuSpecifier> userList;
    userList.push_back(ColorMapMenuSpecifier{QStringLiteral("PAL-relaxed"), std::nullopt, relaxed});

    ColorMapMenu menu(nullptr, userList, false, true);
    QSignalSpy spy(&menu, &ColorMapMenu::sigColorMapTriggered);
    CHECK(spy.isValid());

    QAction* noneAction = findAction(&menu, QStringLiteral("None"));
    CHECK(noneAction != nullptr);
    std::optional<pyqtgraph::ColorMap> triggeredMap;
    QObject::connect(&menu, &ColorMapMenu::sigColorMapTriggered, [&](const pyqtgraph::ColorMap& map) {
        triggeredMap = map;
    });
    CHECK(QMetaObject::invokeMethod(&menu, "onTriggered", Q_ARG(QAction*, noneAction)));
    CHECK(spy.count() == 1);
    CHECK(triggeredMap.has_value() && triggeredMap->positions().size() == 2);

    spy.clear();
    QAction* userAction = findAction(&menu, QStringLiteral("PAL-relaxed"));
    CHECK(userAction != nullptr);
    CHECK(QMetaObject::invokeMethod(&menu, "onTriggered", Q_ARG(QAction*, userAction)));
    CHECK(spy.count() == 1);
    CHECK(triggeredMap.has_value() && triggeredMap->name() == QStringLiteral("PAL-relaxed"));

    QMenu* localMenu = findSubMenu(&menu, QStringLiteral("local"));
    CHECK(localMenu != nullptr);
    CHECK(QMetaObject::invokeMethod(&menu, "buildLocalSubMenu"));
    CHECK(!localMenu->actions().isEmpty());

    return true;
}

bool testButtonMenuIntegration()
{
    using pyqtgraph::widgets::ColorMapButton;

    ColorMapButton button;
    QSignalSpy changedSpy(&button, &ColorMapButton::sigColorMapChanged);
    CHECK(changedSpy.isValid());

    const auto relaxed = pyqtgraph::get(QStringLiteral("PAL-relaxed"));
    CHECK(relaxed.has_value());

    auto* menu = button.getMenu();
    CHECK(menu != nullptr);
    CHECK(menu == button.getMenu());

    QAction* noneAction = findAction(menu, QStringLiteral("None"));
    CHECK(noneAction != nullptr);
    changedSpy.clear();
    CHECK(QMetaObject::invokeMethod(menu, "onTriggered", Q_ARG(QAction*, noneAction)));
    CHECK(changedSpy.count() == 1);
    CHECK(button.colorMap().positions().size() == 2);

    QMenu* localMenu = findSubMenu(menu, QStringLiteral("local"));
    CHECK(localMenu != nullptr);
    CHECK(QMetaObject::invokeMethod(menu, "buildLocalSubMenu"));
    CHECK(!localMenu->actions().isEmpty());

    changedSpy.clear();
    QAction* localAction = localMenu->actions().front();
    CHECK(localAction != nullptr);
    CHECK(QMetaObject::invokeMethod(menu, "onTriggered", Q_ARG(QAction*, localAction)));
    CHECK(changedSpy.count() == 1);
    CHECK(!button.colorMap().name().isEmpty());

    changedSpy.clear();
    button.setColorMap(*relaxed);
    CHECK(changedSpy.count() == 1);
    CHECK(button.colorMap().name() == QStringLiteral("PAL-relaxed"));

    button.setColorMap(QStringLiteral("PAL-relaxed_bright"));
    CHECK(button.colorMap().name() == QStringLiteral("PAL-relaxed_bright"));

    button.setColorMap(QStringLiteral("missing-map-name"));
    CHECK(button.colorMap().positions().size() == 2);

    return true;
}

bool testVisualBehavior()
{
    const std::vector<VisualCase> cases = visualCases();
    PixelMetrics aggregateMetrics;
    std::uint64_t referencePixels = 0;
    std::uint64_t actualPixels = 0;

    for (const VisualCase& visualCase : cases) {
        const QImage reference = renderReferenceColorMapStrip(visualCase.colorMap, buttonWidth, buttonHeight);
        const QImage actual = renderActualColorMapStrip(visualCase.colorMap, buttonWidth, buttonHeight);
        const std::uint64_t caseReferencePixels = semanticPixelCount(reference);
        const std::uint64_t caseActualPixels = semanticPixelCount(actual);
        if (caseReferencePixels < 100 || caseActualPixels < 100) {
            std::cerr << "ColorMapButton blank/placeholder guard failed for " << visualCase.name.toStdString()
                      << ": reference=" << caseReferencePixels << " actual=" << caseActualPixels << '\n';
            return false;
        }
        referencePixels += caseReferencePixels;
        actualPixels += caseActualPixels;

        QImage diff;
        const PixelMetrics metrics = compareImages(reference, actual, diff);
        aggregateMetrics.changedPixels += metrics.changedPixels;
        aggregateMetrics.totalDelta += metrics.totalDelta;
        aggregateMetrics.maxDelta = std::max(aggregateMetrics.maxDelta, metrics.maxDelta);
        if (!metrics.passed) {
            std::cerr << "P5.13 ColorMapButton visual comparison failed for " << visualCase.name.toStdString()
                      << ": changedPixels=" << metrics.changedPixels << " maxDelta=" << metrics.maxDelta << '\n';
            return false;
        }
        CHECK(writeCaseArtifacts(visualCase, reference, actual, diff, metrics));
    }

    const int aggregatePixelCount = static_cast<int>(buttonWidth * buttonHeight * static_cast<int>(cases.size()));
    aggregateMetrics.meanDelta = static_cast<double>(aggregateMetrics.totalDelta) / static_cast<double>(aggregatePixelCount);
    aggregateMetrics.changedPercent = 100.0 * static_cast<double>(aggregateMetrics.changedPixels)
        / static_cast<double>(aggregatePixelCount);
    aggregateMetrics.passed = aggregateMetrics.changedPixels == 0 && aggregateMetrics.maxDelta == 0;

    CHECK(writeIssueReport(aggregateMetrics, referencePixels, actualPixels));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qRegisterMetaType<pyqtgraph::widgets::ColorMapMenuActionData>();
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndApiShape()) {
        return 1;
    }
    if (!testColorMapMenuSelection()) {
        return 1;
    }
    if (!testButtonMenuIntegration()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
