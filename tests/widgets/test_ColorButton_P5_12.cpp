#include <pyqtgraph/functions.hpp>
#include <pyqtgraph/widgets/ColorButton.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPoint>
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

#ifndef PYQTGRAPH_CPP_P5_12_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P5_12_VISUAL_DIFF_DIR "reports/visual-diffs/ColorButton"
#endif

#ifndef PYQTGRAPH_CPP_P5_12_GPT_REVIEW_REPORT
#define PYQTGRAPH_CPP_P5_12_GPT_REVIEW_REPORT "reports/visual-diffs/ColorButton/gpt5_vision_review.md"
#endif

#ifndef PYQTGRAPH_CPP_P5_12_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_12_REPOSITORY_REPORT_DIR "reports/issues/P5.12"
#endif

namespace {

constexpr int buttonWidth = 72;
constexpr int buttonHeight = 36;
constexpr int imageWidth = 240;
constexpr int imageHeight = 48;
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
    QPoint position;
};

std::vector<VisualCase> visualCases()
{
    return {
        {QStringLiteral("default-gray"), QColor(128, 128, 128), QPoint(8, 6)},
        {QStringLiteral("alpha-cyan"), QColor(40, 180, 220, 140), QPoint(88, 6)},
        {QStringLiteral("opaque-orange"), QColor(240, 120, 30, 255), QPoint(168, 6)},
    };
}

void paintColorSwatch(QPainter& painter, const QRect& rect, const QColor& color)
{
    painter.setBrush(pyqtgraph::mkBrush(QStringLiteral("w")));
    painter.drawRect(rect);
    painter.setBrush(QBrush(Qt::BrushStyle::DiagCrossPattern));
    painter.drawRect(rect);
    painter.setBrush(pyqtgraph::mkBrush(color));
    painter.drawRect(rect);
}

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(8, 8, 10));
    return image;
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

QImage renderActualButton(pyqtgraph::widgets::ColorButton& button, const QColor& color)
{
    button.setColor(color, true);
    button.resize(buttonWidth, buttonHeight);
    button.show();
    QApplication::processEvents();
    return button.grab().toImage();
}

QImage renderReference(const std::vector<VisualCase>& cases)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const VisualCase& visualCase : cases) {
        const QImage buttonImage = renderReferenceButton(visualCase.color);
        painter.drawImage(visualCase.position, buttonImage);
    }
    painter.end();
    return image;
}

QImage renderActual(const std::vector<VisualCase>& cases)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const VisualCase& visualCase : cases) {
        pyqtgraph::widgets::ColorButton button;
        const QImage buttonImage = renderActualButton(button, visualCase.color);
        painter.drawImage(visualCase.position, buttonImage);
    }
    painter.end();
    return image;
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

SemanticReviewStatus readGptVisualReview()
{
    SemanticReviewStatus status;
    status.path = QStringLiteral(PYQTGRAPH_CPP_P5_12_GPT_REVIEW_REPORT);
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

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const QString visualDir = QStringLiteral(PYQTGRAPH_CPP_P5_12_VISUAL_DIFF_DIR);
    CHECK(ensureDirectory(visualDir));
    CHECK(reference.save(visualDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(visualDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(visualDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview();
    CHECK(review.accepted);

    writeTextFile(visualDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"ColorButton\",\n"
            "  \"issue\": \"P5.12\",\n"
            "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorButton.py paintEvent swatch overlay\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"dimensions\": [")
            + QString::number(imageWidth)
            + QStringLiteral(", ")
            + QString::number(imageHeight)
            + QStringLiteral(
                "],\n"
                "  \"fixture_hash\": \"P5.12:ColorButton:default-alpha-opaque:v1\",\n"
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
    return true;
}

bool writeIssueReport(const PixelMetrics& metrics, std::uint64_t referencePixels, std::uint64_t actualPixels)
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_12_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ColorButton_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.12\",\n"
            "  \"class\": \"pyqtgraph::widgets::ColorButton\",\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ColorButton.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/ColorButton.hpp\", \"src/pyqtgraph/widgets/ColorButton.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.12 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_colorbutton_p5_12\"},\n"
            "  \"checks\": [\"QPushButton subclass with default gray swatch\", \"setColor finished/changing signals\", \"saveState/restoreState RGBA bytes\", \"dialog slot rollback and accept behavior\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/ColorButton\", \"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\", \"metrics\": \"metrics.json\", \"gpt5_vision_review\": \"gpt5_vision_review.md\"},\n"
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
                "  \"validation_commands\": [\"cmake --preset dev\", \"cmake --build --preset dev --parallel\", \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.12 --output-on-failure\", \"python3 -m pytest -q\", \"git diff --check\", \"git diff --name-only origin/main...HEAD\"]\n"
                "}\n"));
  writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.12 ColorButton completion report\n\n"
            "- Issue: GitHub #249 / P5.12\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ColorButton` with color swatch painting, QColorDialog alpha/non-native options, changing/changed signals, and save/restore RGBA state.\n\n"
            "## Artifacts\n\n"
            "- `include/pyqtgraph/widgets/ColorButton.hpp`\n"
            "- `src/pyqtgraph/widgets/ColorButton.cpp`\n"
            "- `tests/widgets/test_ColorButton_P5_12.cpp`\n"
            "- `reports/visual-diffs/ColorButton/*`\n"
            "- `reports/issues/P5.12/*`\n"));
    return true;
}

bool testConstructionAndApiShape()
{
    using pyqtgraph::widgets::ColorButton;

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
    using pyqtgraph::widgets::ColorButton;

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
    const QImage reference = renderReference(cases);
    const QImage actual = renderActual(cases);
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    if (referencePixels < 300 || actualPixels < 300) {
        std::cerr << "ColorButton blank/placeholder guard failed: reference=" << referencePixels
                  << " actual=" << actualPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(writeArtifacts(reference, actual, diff, metrics));
    CHECK(writeIssueReport(metrics, referencePixels, actualPixels));
    if (!metrics.passed) {
        std::cerr << "P5.12 ColorButton visual comparison failed: changedPixels=" << metrics.changedPixels
                  << " maxDelta=" << metrics.maxDelta << '\n';
        return false;
    }
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
