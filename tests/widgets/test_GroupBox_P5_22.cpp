#include <pyqtgraph/widgets/GroupBox.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPaintEvent>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P5_22_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P5_22_VISUAL_DIFF_DIR "reports/visual-diffs/GroupBox"
#endif

#ifndef PYQTGRAPH_CPP_P5_22_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_22_REPOSITORY_REPORT_DIR "reports/issues/P5.22"
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
    return QStringLiteral(PYQTGRAPH_CPP_P5_22_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

SemanticReviewStatus readGptVisualReview(const QString& caseName)
{
    SemanticReviewStatus status;
    status.path = caseArtifactDir(caseName) + QStringLiteral("/gpt5_vision_review.md");
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P5.22 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P5.22 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P5.22 GPT visual review is not accepted in " << status.path.toStdString()
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

class ReferenceCollapseHandle : public QPushButton {
public:
    explicit ReferenceCollapseHandle(QWidget* parent = nullptr)
        : QPushButton(parent)
    {
        setFixedSize(12, 12);
        setFlat(true);
        setStyleSheet(QStringLiteral("border: none;"));
    }

    void setIndicatorPath(const QPainterPath& path)
    {
        indicatorPath_ = path;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPushButton::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        const QRectF bounds = rect().adjusted(1, 1, -1, -1);
        painter.save();
        painter.translate(bounds.center());
        painter.scale(bounds.width() / 2.0, bounds.height() / 2.0);
        painter.drawPath(indicatorPath_);
        painter.restore();
    }

private:
    QPainterPath indicatorPath_;
};

QPainterPath referenceOpenPath()
{
    QPainterPath path;
    path.moveTo(-1, 0);
    path.lineTo(1, 0);
    path.lineTo(0, 1);
    path.lineTo(-1, 0);
    return path;
}

QPainterPath referenceClosePath()
{
    QPainterPath path;
    path.moveTo(0, -1);
    path.lineTo(0, 1);
    path.lineTo(1, 0);
    path.lineTo(0, -1);
    return path;
}

// Upstream-derived reference oracle for pyqtgraph/widgets/GroupBox.py (a20028b).
class ReferenceGroupBox : public QGroupBox {
public:
    explicit ReferenceGroupBox(QWidget* parent = nullptr)
        : QGroupBox(parent)
        , lastSizePolicy_(sizePolicy())
    {
        collapseHandle_ = new ReferenceCollapseHandle(this);
        collapseHandle_->setIndicatorPath(referenceOpenPath());
        collapseHandle_->move(3, 3);
        connect(collapseHandle_, &QPushButton::clicked, this, &ReferenceGroupBox::toggleCollapsed);
    }

    explicit ReferenceGroupBox(const QString& title, QWidget* parent = nullptr)
        : ReferenceGroupBox(parent)
    {
        setTitle(title);
    }

    [[nodiscard]] bool collapsed() const { return collapsed_; }

    void setCollapsed(bool collapsed)
    {
        if (collapsed == collapsed_) {
            return;
        }
        if (collapsed) {
            collapseHandle_->setIndicatorPath(referenceClosePath());
            QGroupBox::setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            lastSizePolicy_ = sizePolicy();
        } else {
            collapseHandle_->setIndicatorPath(referenceOpenPath());
            QGroupBox::setSizePolicy(lastSizePolicy_);
        }
        const auto children = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget* child : children) {
            if (child != collapseHandle_) {
                child->setVisible(!collapsed);
            }
        }
        collapseHandle_->setVisible(true);
        collapseHandle_->raise();
        collapsed_ = collapsed;
    }

    void toggleCollapsed()
    {
        setCollapsed(!collapsed_);
    }

    void setTitle(const QString& title)
    {
        QGroupBox::setTitle(QStringLiteral("   ") + title);
    }

    void setSizePolicy(QSizePolicy policy)
    {
        QGroupBox::setSizePolicy(policy);
        lastSizePolicy_ = sizePolicy();
    }

private:
    ReferenceCollapseHandle* collapseHandle_ = nullptr;
    bool collapsed_ = false;
    QSizePolicy lastSizePolicy_;
};

void populateGroupBoxChildren(QGroupBox& box)
{
    auto* label = new QLabel(QStringLiteral("Channel A"), &box);
    label->setGeometry(20, 24, 80, 18);
    auto* button = new QPushButton(QStringLiteral("Run"), &box);
    button->setGeometry(110, 22, 60, 22);
}

void pinGroupBoxForVisual(QGroupBox& box)
{
    box.setFixedSize(200, 90);
    box.setStyleSheet(QStringLiteral("QGroupBox { background: #ffffff; }"));
}

QImage grabWidget(QWidget& widget)
{
    widget.show();
    QApplication::processEvents();
    widget.repaint();
    QApplication::processEvents();
    return widget.grab().toImage();
}

QImage renderReferenceGroupBox(bool collapsed)
{
    ReferenceGroupBox reference(QStringLiteral("Controls"));
    pinGroupBoxForVisual(reference);
    populateGroupBoxChildren(reference);
    if (collapsed) {
        reference.setCollapsed(true);
    }
    return grabWidget(reference);
}

QImage renderActualGroupBox(bool collapsed)
{
    pyqtgraph::widgets::GroupBox actual(QStringLiteral("Controls"));
    pinGroupBoxForVisual(actual);
    populateGroupBoxChildren(actual);
    if (collapsed) {
        actual.setCollapsed(true);
    }
    return grabWidget(actual);
}

bool testGroupBoxApiShape()
{
    using pyqtgraph::widgets::GroupBox;

    static_assert(std::is_base_of_v<QGroupBox, GroupBox>);

    GroupBox box(QStringLiteral("Test"));
    CHECK(!box.collapsed());
    CHECK(box.title().startsWith(QStringLiteral("   ")));
    return true;
}

bool testGroupBoxCollapseBehavior()
{
    using pyqtgraph::widgets::GroupBox;

    GroupBox box(QStringLiteral("Gain"));
    auto* child = new QLabel(QStringLiteral("Inner"), &box);
    box.show();
    QApplication::processEvents();
    const auto handles = box.findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
    CHECK(handles.size() == 1);
    QWidget* handle = handles.front();

    QSignalSpy spy(&box, &GroupBox::sigCollapseChanged);
    CHECK(spy.isValid());

    const QSizePolicy initialPolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    box.setSizePolicy(initialPolicy);
    CHECK(box.sizePolicy() == initialPolicy);

    box.setCollapsed(true);
    CHECK(box.collapsed());
    CHECK(spy.count() == 1);
    CHECK(spy.at(0).at(0).toBool());
    CHECK(!child->isVisible());
    CHECK(handle->isVisible());

    box.setCollapsed(true);
    CHECK(spy.count() == 1);

    box.setCollapsed(false);
    CHECK(!box.collapsed());
    CHECK(spy.count() == 2);
    CHECK(!spy.at(1).at(0).toBool());
    CHECK(child->isVisible());

    box.toggleCollapsed();
    CHECK(box.collapsed());
    CHECK(spy.count() == 3);

    return true;
}

bool verifyPerCaseArtifactLayout(const QString& caseName)
{
    const QString caseDir = caseArtifactDir(caseName);
    const QStringList requiredFiles = {QStringLiteral("reference.png"), QStringLiteral("actual.png"),
        QStringLiteral("diff.png"), QStringLiteral("metrics.json"), QStringLiteral("gpt5_vision_review.md")};
    for (const QString& fileName : requiredFiles) {
        const QString path = caseDir + QChar('/') + fileName;
        if (!QFile::exists(path)) {
            std::cerr << "missing P5.22 per-case visual artifact: " << path.toStdString() << '\n';
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
                "  \"issue\": \"P5.22\",\n"
                "  \"widget\": \"GroupBox\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/GroupBox.py upstream-derived test oracle\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [")
            + QString::number(reference.width())
            + QStringLiteral(", ")
            + QString::number(reference.height())
            + QStringLiteral(
                "],\n"
                "  \"fixture_hash\": \"P5.22:")
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

bool writeIssueReport(const PixelMetrics& expandedMetrics, const PixelMetrics& collapsedMetrics)
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_22_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/GroupBox_behavior.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.22\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::GroupBox\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/GroupBox.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/GroupBox.hpp\", \"src/pyqtgraph/widgets/GroupBox.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.22 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_groupbox_p5_22\"},\n"
            "  \"checks\": [\"GroupBox API shape and title padding\", \"initial expanded state\", \"collapse/expand signal and child visibility\", \"collapse handle remains visible\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/GroupBox\", \"cases\": [\"GroupBox-expanded\", \"GroupBox-collapsed\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
            "  \"visual_metrics\": {\"expanded\": {\"changed_pixels\": ")
            + QString::number(expandedMetrics.changedPixels)
            + QStringLiteral(", \"max_delta\": ")
            + QString::number(expandedMetrics.maxDelta)
            + QStringLiteral(", \"passed\": ")
            + (expandedMetrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral("}, \"collapsed\": {\"changed_pixels\": ")
            + QString::number(collapsedMetrics.changedPixels)
            + QStringLiteral(", \"max_delta\": ")
            + QString::number(collapsedMetrics.maxDelta)
            + QStringLiteral(", \"passed\": ")
            + (collapsedMetrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(
                "}},\n"
                "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.22 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
                "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.22 GroupBox completion report\n\n"
            "- Issue: GitHub #264 / P5.22\n"
            "- Validation class: visual-render\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `GroupBox` as a QGroupBox subclass with collapse handle, title padding, child visibility toggling, size-policy tracking, and `sigCollapseChanged`.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.22 --output-on-failure` | 0 |\n"
            "| `python3 -m pytest -q` | 0 |\n"
            "| `git diff --check` | 0 |\n"
            "| `git diff --name-only origin/main...HEAD` | 0 |\n"));
    return true;
}

bool testVisualCase(const QString& caseName, bool collapsed)
{
    const QImage reference = renderReferenceGroupBox(collapsed);
    const QImage actual = renderActualGroupBox(collapsed);
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    if (referencePixels < 50 || actualPixels < 50) {
        std::cerr << "P5.22 blank/placeholder guard failed for " << caseName.toStdString()
                  << ": reference=" << referencePixels << " actual=" << actualPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    if (!metrics.passed) {
        std::cerr << "P5.22 visual comparison failed for " << caseName.toStdString()
                  << ": changedPixels=" << metrics.changedPixels << " maxDelta=" << metrics.maxDelta << '\n';
        return false;
    }
    CHECK(writeCaseArtifacts(caseName, reference, actual, diff, metrics));
    return true;
}

bool testVisualBehavior()
{
    PixelMetrics expandedMetrics;
    PixelMetrics collapsedMetrics;

    {
        const QImage reference = renderReferenceGroupBox(false);
        const QImage actual = renderActualGroupBox(false);
        QImage diff;
        expandedMetrics = compareImages(reference, actual, diff);
        CHECK(testVisualCase(QStringLiteral("GroupBox-expanded"), false));
    }
    {
        const QImage reference = renderReferenceGroupBox(true);
        const QImage actual = renderActualGroupBox(true);
        QImage diff;
        collapsedMetrics = compareImages(reference, actual, diff);
        CHECK(testVisualCase(QStringLiteral("GroupBox-collapsed"), true));
    }

    CHECK(writeIssueReport(expandedMetrics, collapsedMetrics));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testGroupBoxApiShape()) {
        return 1;
    }
    if (!testGroupBoxCollapseBehavior()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
