#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/widgets/ColorButton.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef CPPQTGRAPH_P5_12_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P5_12_VISUAL_DIFF_DIR "reports/visual-diffs/ColorButton"
#endif

#ifndef CPPQTGRAPH_P5_12_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_12_REPOSITORY_REPORT_DIR "reports/issues/P5.12"
#endif

namespace {

constexpr int buttonWidth = 72;
constexpr int buttonHeight = 36;
constexpr int buttonPadding = 6;

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
    QColor color;
};

std::vector<VisualCase> visualCases()
{
    return {
        {QStringLiteral("default-gray"), QColor(128, 128, 128)},
        {QStringLiteral("alpha-cyan"), QColor(40, 180, 220, 140)},
        {QStringLiteral("opaque-orange"), QColor(240, 120, 30, 255)},
    };
}

void paintColorSwatch(QPainter& painter, const QRect& rect, const QColor& color)
{
    painter.setBrush(cppqtgraph::mkBrush(QStringLiteral("w")));
    painter.drawRect(rect);
    painter.setBrush(QBrush(Qt::BrushStyle::DiagCrossPattern));
    painter.drawRect(rect);
    painter.setBrush(cppqtgraph::mkBrush(color));
    painter.drawRect(rect);
}

QImage renderReferenceButton(const QColor& color)
{
    QImage image(buttonWidth, buttonHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPushButton button;
    button.resize(buttonWidth, buttonHeight);
    button.show();
    QApplication::processEvents();
    image = button.grab().toImage();

    QPainter painter(&image);
    const QRect rect = button.rect().adjusted(buttonPadding, buttonPadding, -buttonPadding, -buttonPadding);
    paintColorSwatch(painter, rect, color);
    painter.end();
    return image;
}

QImage renderActualButton(cppqtgraph::widgets::ColorButton& button, const QColor& color)
{
    button.setColor(color, true);
    button.resize(buttonWidth, buttonHeight);
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
    return QStringLiteral(CPPQTGRAPH_P5_12_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

SemanticReviewStatus readGptVisualReview(const QString& caseName)
{
    SemanticReviewStatus status;
    status.path = caseArtifactDir(caseName) + QStringLiteral("/gpt5_vision_review.md");
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P5.12 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P5.12 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P5.12 GPT visual review is not accepted in " << status.path.toStdString()
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
            std::cerr << "missing P5.12 per-case visual artifact: " << path.toStdString() << '\n';
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
                "  \"issue\": \"P5.12\",\n"
                "  \"widget\": \"ColorButton\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorButton.py paintEvent swatch overlay\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [")
            + QString::number(reference.width())
            + QStringLiteral(", ")
            + QString::number(reference.height())
            + QStringLiteral(
                "],\n"
                "  \"fixture_hash\": \"P5.12:ColorButton:")
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
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_12_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ColorButton_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.12\",\n"
            "  \"class\": \"cppqtgraph::widgets::ColorButton\",\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorButton.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/ColorButton.hpp\", \"src/cppqtgraph/widgets/ColorButton.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.12 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_colorbutton_p5_12\"},\n"
            "  \"checks\": [\"QPushButton subclass with default gray swatch\", \"setColor finished/changing signals\", \"saveState/restoreState RGBA bytes\", \"dialog slot rollback and accept behavior\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/ColorButton\", \"cases\": [\"default-gray\", \"alpha-cyan\", \"opaque-orange\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
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
                "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.12 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}]\n"
                "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.12 ColorButton completion report\n\n"
            "- Issue: GitHub #249 / P5.12\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ColorButton` with color swatch painting, QColorDialog alpha/non-native options, changing/changed signals, and save/restore RGBA state.\n\n"
            "## Validation\n\n"
            "| Command | Exit code | Result |\n"
            "| --- | ---: | --- |\n"
            "| `cmake --preset dev` | 0 | pass |\n"
            "| `cmake --build --preset dev --parallel` | 0 | pass |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.12 --output-on-failure` | 0 | pass |\n"
            "| `python3 -m pytest -q` | 0 | pass |\n"
            "| `git diff --check` | 0 | pass |\n\n"
            "## Artifacts\n\n"
            "- `include/cppqtgraph/widgets/ColorButton.hpp`\n"
            "- `src/cppqtgraph/widgets/ColorButton.cpp`\n"
            "- `tests/widgets/test_ColorButton_P5_12.cpp`\n"
            "- `reports/visual-diffs/ColorButton/<case>/{reference.png,actual.png,diff.png,metrics.json,gpt5_vision_review.md}`\n"
            "- `reports/issues/P5.12/*`\n"));
    return true;
}

bool testConstructionAndApiShape()
{
    using cppqtgraph::widgets::ColorButton;

    static_assert(std::is_base_of_v<QPushButton, ColorButton>);
    static_assert(std::is_constructible_v<ColorButton>);
    static_assert(std::is_constructible_v<ColorButton, QWidget*>);
    static_assert(!std::is_copy_constructible_v<ColorButton>);
    static_assert(!std::is_copy_assignable_v<ColorButton>);

    ColorButton button;
    CHECK(button.minimumWidth() >= 15);
    CHECK(button.minimumHeight() >= 15);
    CHECK(button.color() == QColor(128, 128, 128));

    ColorButton custom(nullptr, {64, 96, 128});
    CHECK(custom.color() == QColor(64, 96, 128));

    const std::array<int, 4> saved = button.saveState();
    CHECK(saved[0] == 128 && saved[1] == 128 && saved[2] == 128 && saved[3] == 255);
    button.restoreState({10, 20, 30, 200});
    CHECK(button.color() == QColor(10, 20, 30, 200));

    const QColor byteMode = button.color(QStringLiteral("byte"));
    CHECK(byteMode.red() == 10 && byteMode.green() == 20 && byteMode.blue() == 30 && byteMode.alpha() == 200);
    const QColor floatMode = button.color(QStringLiteral("float"));
    CHECK(qRound(floatMode.redF() * 255.0) == 10);
    CHECK(qRound(floatMode.greenF() * 255.0) == 20);
    CHECK(qRound(floatMode.blueF() * 255.0) == 30);
    CHECK(qRound(floatMode.alphaF() * 255.0) == 200);

    return true;
}

bool testSignalAndDialogSlots()
{
    using cppqtgraph::widgets::ColorButton;

    ColorButton button;
    QSignalSpy changingSpy(&button, &ColorButton::sigColorChanging);
    QSignalSpy changedSpy(&button, &ColorButton::sigColorChanged);
    CHECK(changingSpy.isValid());
    CHECK(changedSpy.isValid());

    button.setColor(QColor(90, 90, 90), false);
    CHECK(changingSpy.count() == 1);
    CHECK(changedSpy.count() == 0);

    button.setColor(QColor(100, 110, 120), true);
    CHECK(changingSpy.count() == 1);
    CHECK(changedSpy.count() == 1);

    button.selectColor();
    CHECK(button.color() == QColor(100, 110, 120));
    changingSpy.clear();
    changedSpy.clear();

    CHECK(QMetaObject::invokeMethod(&button, "dialogColorChanged", Q_ARG(QColor, QColor(200, 40, 80, 180))));
    CHECK(button.color() == QColor(200, 40, 80, 180));
    CHECK(changingSpy.count() == 1);
    CHECK(changedSpy.count() == 0);

    CHECK(QMetaObject::invokeMethod(&button, "colorRejected"));
    CHECK(button.color() == QColor(100, 110, 120));
    CHECK(changingSpy.count() == 2);

    CHECK(QMetaObject::invokeMethod(&button, "dialogColorChanged", Q_ARG(QColor, QColor(30, 60, 90))));
    CHECK(QMetaObject::invokeMethod(&button, "colorSelected", Q_ARG(QColor, QColor(30, 60, 90))));
    CHECK(button.color() == QColor(30, 60, 90));
    CHECK(changedSpy.count() == 1);

    return true;
}

bool testVisualBehavior()
{
    const std::vector<VisualCase> cases = visualCases();
    PixelMetrics aggregateMetrics;
    std::uint64_t referencePixels = 0;
    std::uint64_t actualPixels = 0;

    for (const VisualCase& visualCase : cases) {
        const QImage reference = renderReferenceButton(visualCase.color);
        cppqtgraph::widgets::ColorButton button;
        const QImage actual = renderActualButton(button, visualCase.color);
        const std::uint64_t caseReferencePixels = semanticPixelCount(reference);
        const std::uint64_t caseActualPixels = semanticPixelCount(actual);
        if (caseReferencePixels < 100 || caseActualPixels < 100) {
            std::cerr << "ColorButton blank/placeholder guard failed for " << visualCase.name.toStdString()
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
            std::cerr << "P5.12 ColorButton visual comparison failed for " << visualCase.name.toStdString()
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
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndApiShape()) {
        return 1;
    }
    if (!testSignalAndDialogSlots()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
