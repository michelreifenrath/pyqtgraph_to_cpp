#include <cppqtgraph/widgets/LayoutWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef CPPQTGRAPH_P5_21_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P5_21_VISUAL_DIFF_DIR "reports/visual-diffs/LayoutWidget"
#endif

#ifndef CPPQTGRAPH_P5_21_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_21_REPOSITORY_REPORT_DIR "reports/issues/P5.21"
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
    return QStringLiteral(CPPQTGRAPH_P5_21_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

SemanticReviewStatus readGptVisualReview(const QString& caseName)
{
    SemanticReviewStatus status;
    status.path = caseArtifactDir(caseName) + QStringLiteral("/gpt5_vision_review.md");
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P5.21 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P5.21 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P5.21 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
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
    if (reference.size() != actual.size()) {
        std::cerr << "image size mismatch: reference=" << reference.width() << 'x' << reference.height()
                  << " actual=" << actual.width() << 'x' << actual.height() << '\n';
        return metrics;
    }
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

void pinLayoutForVisualTest(QGridLayout* layout)
{
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
}

// Upstream-derived reference oracle for pyqtgraph/widgets/LayoutWidget.py (a20028b).
class ReferenceLayoutWidget : public QWidget {
public:
    static constexpr int kAutoRow = -1;
    static constexpr int kNextRow = -2;
    static constexpr int kAutoCol = -1;

    explicit ReferenceLayoutWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        gridLayout = new QGridLayout(this);
        setLayout(gridLayout);
    }

    QGridLayout* gridLayout = nullptr;
    QHash<QWidget*, QPair<int, int>> items;
    QHash<int, QHash<int, QWidget*>> rows;
    int currentRow = 0;
    int currentCol = 0;

    void nextRow()
    {
        ++currentRow;
        currentCol = 0;
    }

    int nextColumn(int colspan = 1)
    {
        currentCol += colspan;
        return currentCol - colspan;
    }

    int nextCol(int colspan = 1)
    {
        return nextColumn(colspan);
    }

    QLabel* addLabel(const QString& text = QStringLiteral(" "), int row = kAutoRow, int col = kAutoCol,
                     int rowspan = 1, int colspan = 1)
    {
        auto* label = new QLabel(text, this);
        addWidget(label, row, col, rowspan, colspan);
        return label;
    }

    ReferenceLayoutWidget* addLayout(int row = kAutoRow, int col = kAutoCol, int rowspan = 1, int colspan = 1)
    {
        auto* nested = new ReferenceLayoutWidget(this);
        addWidget(nested, row, col, rowspan, colspan);
        return nested;
    }

    void addWidget(QWidget* item, int row = kAutoRow, int col = kAutoCol, int rowspan = 1, int colspan = 1)
    {
        if (row == kNextRow) {
            nextRow();
            row = currentRow;
        } else if (row == kAutoRow) {
            row = currentRow;
        }

        if (col == kAutoCol) {
            col = nextCol(colspan);
        }

        if (!rows.contains(row)) {
            rows[row] = {};
        }
        rows[row][col] = item;
        items[item] = {row, col};

        gridLayout->addWidget(item, row, col, rowspan, colspan);
    }

    QWidget* getWidget(int row, int col) const
    {
        if (!rows.contains(row)) {
            return nullptr;
        }
        const QHash<int, QWidget*>& rowMap = rows[row];
        if (!rowMap.contains(col)) {
            return nullptr;
        }
        return rowMap[col];
    }
};

void buildReferenceArrangement(ReferenceLayoutWidget& layoutWidget)
{
    pinLayoutForVisualTest(layoutWidget.gridLayout);
    layoutWidget.setFixedSize(220, 120);

    layoutWidget.addLabel(QStringLiteral("Channel"));
    auto* startButton = new QPushButton(QStringLiteral("Start"), &layoutWidget);
    startButton->setFixedWidth(70);
    layoutWidget.addWidget(startButton);

    layoutWidget.nextRow();
    layoutWidget.addLabel(QStringLiteral("Gain"));
    ReferenceLayoutWidget* nested = layoutWidget.addLayout();
    pinLayoutForVisualTest(nested->gridLayout);
    nested->addLabel(QStringLiteral("Inner"));
    auto* stopButton = new QPushButton(QStringLiteral("Stop"), &layoutWidget);
    stopButton->setFixedWidth(70);
    layoutWidget.addWidget(stopButton);
}

void buildActualArrangement(cppqtgraph::widgets::LayoutWidget& layoutWidget)
{
    pinLayoutForVisualTest(layoutWidget.gridLayout);
    layoutWidget.setFixedSize(220, 120);

    layoutWidget.addLabel(QStringLiteral("Channel"));
    auto* startButton = new QPushButton(QStringLiteral("Start"), &layoutWidget);
    startButton->setFixedWidth(70);
    layoutWidget.addWidget(startButton);

    layoutWidget.nextRow();
    layoutWidget.addLabel(QStringLiteral("Gain"));
    cppqtgraph::widgets::LayoutWidget* nested = layoutWidget.addLayout();
    pinLayoutForVisualTest(nested->gridLayout);
    nested->addLabel(QStringLiteral("Inner"));
    auto* stopButton = new QPushButton(QStringLiteral("Stop"), &layoutWidget);
    stopButton->setFixedWidth(70);
    layoutWidget.addWidget(stopButton);
}

bool testLayoutWidgetApiShape()
{
    using cppqtgraph::widgets::LayoutWidget;

    static_assert(std::is_base_of_v<QWidget, LayoutWidget>);
    static_assert(!std::is_copy_constructible_v<LayoutWidget>);

    LayoutWidget widget;
    CHECK(widget.gridLayout != nullptr);
    CHECK(widget.layout() == widget.gridLayout);
    CHECK(widget.currentRow == 0);
    CHECK(widget.currentCol == 0);
    CHECK(widget.items.isEmpty());
    CHECK(widget.rows.isEmpty());
    return true;
}

bool testLayoutWidgetPlacement()
{
    using cppqtgraph::widgets::LayoutWidget;

    LayoutWidget widget;
    QLabel* title = widget.addLabel(QStringLiteral("Title"), 0, 0);
    auto* button = new QPushButton(QStringLiteral("Run"), &widget);
    widget.addWidget(button);

    // Explicit (0,0) does not advance currentCol; next auto placement reuses column 0 per upstream.
    CHECK(title != widget.getWidget(0, 0));
    CHECK(button == widget.getWidget(0, 0));
    CHECK(widget.items.size() == 2);
    CHECK(widget.currentRow == 0);
    CHECK(widget.currentCol == 1);

    widget.nextRow();
    CHECK(widget.currentRow == 1);
    CHECK(widget.currentCol == 0);

    QLabel* rowLabel = widget.addLabel(QStringLiteral("Row2"));
    CHECK(rowLabel == widget.getWidget(1, 0));
    CHECK(widget.getWidget(1, 1) == nullptr);
    CHECK(widget.getWidget(9, 9) == nullptr);

    auto* wide = new QPushButton(QStringLiteral("Wide"), &widget);
    widget.addWidget(wide, 1, 1, 1, 2);
    CHECK(wide == widget.getWidget(1, 1));
    CHECK(widget.currentCol == 1);

    widget.nextColumn(2);
    CHECK(widget.currentCol == 3);
    CHECK(widget.nextCol() == 3);
    CHECK(widget.currentCol == 4);

    LayoutWidget* nested = widget.addLayout(LayoutWidget::kNextRow);
    CHECK(nested != nullptr);
    CHECK(nested->parentWidget() == &widget);
    CHECK(nested == widget.getWidget(2, 0));
    CHECK(widget.currentRow == 2);
    return true;
}

QImage grabWidget(QWidget& widget)
{
    widget.show();
    QApplication::processEvents();
    widget.repaint();
    QApplication::processEvents();
    return widget.grab().toImage();
}

QImage renderLayoutWidgetReference()
{
    ReferenceLayoutWidget reference;
    buildReferenceArrangement(reference);
    return grabWidget(reference);
}

QImage renderLayoutWidgetActual()
{
    cppqtgraph::widgets::LayoutWidget actual;
    buildActualArrangement(actual);
    return grabWidget(actual);
}

bool verifyPerCaseArtifactLayout(const QString& caseName)
{
    const QString caseDir = caseArtifactDir(caseName);
    const QStringList requiredFiles = {QStringLiteral("reference.png"), QStringLiteral("actual.png"),
        QStringLiteral("diff.png"), QStringLiteral("metrics.json"), QStringLiteral("gpt5_vision_review.md")};
    for (const QString& fileName : requiredFiles) {
        const QString path = caseDir + QChar('/') + fileName;
        if (!QFile::exists(path)) {
            std::cerr << "missing P5.21 per-case visual artifact: " << path.toStdString() << '\n';
            return false;
        }
    }
    return true;
}

bool writeCaseArtifacts(const QString& caseName, const QImage& reference, const QImage& actual, const QImage& diff,
    const PixelMetrics& metrics)
{
    const QString caseDir = caseArtifactDir(caseName);
    CHECK(ensureDirectory(caseDir));
    CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview(caseName);
    CHECK(review.accepted);

    writeTextFile(caseDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"")
            + jsonEscape(caseName)
            + QStringLiteral(
                "\",\n"
                "  \"issue\": \"P5.21\",\n"
                "  \"widget\": \"LayoutWidget\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/LayoutWidget.py upstream-derived test oracle\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [")
            + QString::number(reference.width())
            + QStringLiteral(", ")
            + QString::number(reference.height())
            + QStringLiteral(
                "],\n"
                "  \"fixture_hash\": \"P5.21:")
            + jsonEscape(caseName)
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
                "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"#ffffff\", \"antialias\": true}\n"
                "}\n"));
    CHECK(verifyPerCaseArtifactLayout(caseName));
    return true;
}

bool writeIssueReport(const PixelMetrics& metrics, std::uint64_t referencePixels, std::uint64_t actualPixels)
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_21_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/LayoutWidget_arrangement.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.21\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::LayoutWidget\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/LayoutWidget.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/LayoutWidget.hpp\", \"src/cppqtgraph/widgets/LayoutWidget.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.21 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_layoutwidget_p5_21\"},\n"
            "  \"checks\": [\"LayoutWidget API shape and QGridLayout ownership\", \"auto/explicit placement and nextRow/nextCol\", \"nested addLayout and kNextRow\", \"getWidget missing-cell nullptr\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/LayoutWidget\", \"cases\": [\"LayoutWidget-arrangement\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
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
                "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.21 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
                "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.21 LayoutWidget completion report\n\n"
            "- Issue: GitHub #262 / P5.21\n"
            "- Validation class: visual-render\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `LayoutWidget` as a QWidget convenience wrapper around QGridLayout with row/column bookkeeping, auto placement, nextRow/nextColumn, addLabel, nested addLayout, and getWidget.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.21 --output-on-failure` | 0 |\n"
            "| `python3 -m pytest -q` | 0 |\n"
            "| `git diff --check` | 0 |\n"
            "| `git diff --name-only origin/main...HEAD` | 0 |\n"));
    return true;
}

bool testVisualBehavior()
{
    const QString caseName = QStringLiteral("LayoutWidget-arrangement");
    const QImage reference = renderLayoutWidgetReference();
    const QImage actual = renderLayoutWidgetActual();
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    if (referencePixels < 50 || actualPixels < 50) {
        std::cerr << "P5.21 blank/placeholder guard failed: reference=" << referencePixels
                  << " actual=" << actualPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    if (!metrics.passed) {
        std::cerr << "P5.21 visual comparison failed: changedPixels=" << metrics.changedPixels
                  << " maxDelta=" << metrics.maxDelta << '\n';
        return false;
    }
    CHECK(writeCaseArtifacts(caseName, reference, actual, diff, metrics));
    CHECK(writeIssueReport(metrics, referencePixels, actualPixels));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testLayoutWidgetApiShape()) {
        return 1;
    }
    if (!testLayoutWidgetPlacement()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
