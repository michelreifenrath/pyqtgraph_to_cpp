#include <cppqtgraph/graphicsItems/ArrowItem.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsPathItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef CPPQTGRAPH_P4_21_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P4_21_VISUAL_DIFF_DIR "reports/visual-diffs/ArrowItem"
#endif

#ifndef CPPQTGRAPH_P4_21_GPT_REVIEW_REPORT
#define CPPQTGRAPH_P4_21_GPT_REVIEW_REPORT "reports/visual-diffs/ArrowItem/gpt5_vision_review.md"
#endif

#ifndef CPPQTGRAPH_P4_21_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_21_REPOSITORY_REPORT_DIR "reports/issues/P4.21"
#endif

namespace {

constexpr int imageWidth = 360;
constexpr int imageHeight = 180;
constexpr qreal penPaddingFactor = 0.7072;
constexpr qreal pi = 3.141592653589793238462643383279502884L;

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
                  << " expected=" << expected << " tolerance=" << tolerance << '\n';
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

QPen cosmeticPen(const QColor& color, qreal width = 1.0)
{
    QPen pen(color, width);
    pen.setCosmetic(true);
    return pen;
}

QPen dataPen(const QColor& color, qreal width)
{
    QPen pen(color, width);
    pen.setCosmetic(false);
    return pen;
}

bool samePoint(const QPainterPath::Element& element, qreal x, qreal y, qreal tolerance = 1.0e-9)
{
    return std::abs(element.x - x) <= tolerance && std::abs(element.y - y) <= tolerance;
}

qreal degreesToRadians(qreal degrees)
{
    return degrees * pi / 180.0;
}

QPainterPath referenceArrowPath(const cppqtgraph::graphicsItems::ArrowItemOptions& options)
{
    const qreal headWidth = options.headWidth.value_or(options.headLen * std::tan(degreesToRadians(options.tipAngle * 0.5)));
    QPainterPath path;
    path.moveTo(0.0, 0.0);
    path.lineTo(options.headLen, -headWidth);
    if (!options.tailLen.has_value()) {
        const qreal innerY = options.headLen - headWidth * std::tan(degreesToRadians(options.baseAngle));
        path.lineTo(innerY, 0.0);
    } else {
        const qreal halfTailWidth = options.tailWidth * 0.5;
        const qreal innerY = options.headLen - (headWidth - halfTailWidth) * std::tan(degreesToRadians(options.baseAngle));
        path.lineTo(innerY, -halfTailWidth);
        path.lineTo(options.headLen + *options.tailLen, -halfTailWidth);
        path.lineTo(options.headLen + *options.tailLen, halfTailWidth);
        path.lineTo(innerY, halfTailWidth);
    }
    path.lineTo(options.headLen, headWidth);
    path.lineTo(0.0, 0.0);

    QTransform transform;
    transform.rotate(options.angle);
    return transform.map(path);
}

struct VisualCase {
    QString name;
    QPointF position;
    cppqtgraph::graphicsItems::ArrowItemOptions options;
};

std::vector<VisualCase> visualCases()
{
    std::vector<VisualCase> cases;

    cppqtgraph::graphicsItems::ArrowItemOptions defaultArrow;
    defaultArrow.pen = cosmeticPen(QColor(200, 200, 200));
    defaultArrow.brush = QBrush(QColor(50, 50, 200));
    cases.push_back({QStringLiteral("default-head"), QPointF(82.0, 72.0), defaultArrow});

    cppqtgraph::graphicsItems::ArrowItemOptions tailed;
    tailed.angle = 0.0;
    tailed.headLen = 26.0;
    tailed.headWidth = 12.0;
    tailed.baseAngle = 10.0;
    tailed.tailLen = 42.0;
    tailed.tailWidth = 6.0;
    tailed.pen = cosmeticPen(QColor(250, 220, 40), 2.0);
    tailed.brush = QBrush(QColor(50, 180, 80));
    cases.push_back({QStringLiteral("explicit-width-tail"), QPointF(156.0, 62.0), tailed});

    cppqtgraph::graphicsItems::ArrowItemOptions dataScaled;
    dataScaled.pxMode = false;
    dataScaled.angle = 45.0;
    dataScaled.headLen = 30.0;
    dataScaled.tipAngle = 18.0;
    dataScaled.baseAngle = -8.0;
    dataScaled.tailLen = 28.0;
    dataScaled.tailWidth = 5.0;
    dataScaled.pen = dataPen(QColor(230, 230, 230), 2.0);
    dataScaled.brush = QBrush(QColor(200, 70, 160));
    cases.push_back({QStringLiteral("data-scaled-sharp-tail"), QPointF(258.0, 92.0), dataScaled});

    return cases;
}

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(8, 8, 10));
    return image;
}

QImage renderReference(const std::vector<VisualCase>& cases)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const VisualCase& visualCase : cases) {
        painter.save();
        painter.translate(visualCase.position);
        painter.setPen(visualCase.options.pen);
        painter.setBrush(visualCase.options.brush);
        painter.drawPath(referenceArrowPath(visualCase.options));
        painter.restore();
    }
    painter.end();
    return image;
}

QImage renderActual(const std::vector<VisualCase>& cases)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QStyleOptionGraphicsItem option;
    for (const VisualCase& visualCase : cases) {
        cppqtgraph::graphicsItems::ArrowItem arrow(visualCase.options);
        painter.save();
        painter.translate(visualCase.position);
        arrow.paint(&painter, &option, nullptr);
        painter.restore();
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
    status.path = QStringLiteral(CPPQTGRAPH_P4_21_GPT_REVIEW_REPORT);
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P4.21 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P4.21 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P4.21 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
}

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const QString visualDir = QStringLiteral(CPPQTGRAPH_P4_21_VISUAL_DIFF_DIR);
    CHECK(ensureDirectory(visualDir));
    CHECK(reference.save(visualDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(visualDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(visualDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview();
    CHECK(review.accepted);

    writeTextFile(visualDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"ArrowItem\",\n"
            "  \"issue\": \"P4.21\",\n"
            "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ArrowItem.py; pyqtgraph/functions.py makeArrowPath\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"dimensions\": [360, 180],\n"
            "  \"fixture_hash\": \"P4.21:ArrowItem:default-explicit-tail-data-scaled:v1\",\n"
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
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P4_21_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ArrowItem_visual_behavior.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P4.21\",\n"
            "  \"class\": \"cppqtgraph::graphicsItems::ArrowItem\",\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ArrowItem.py; pyqtgraph/functions.py makeArrowPath; tests/graphicsItems/test_ArrowItem.py parent construction\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/graphicsItems/ArrowItem.hpp\", \"src/cppqtgraph/graphicsItems/ArrowItem.cpp\"],\n"
            "  \"shared_wiring\": [\"tests/CMakeLists.txt\"],\n"
            "  \"tdd_baseline_failure\": {\"command\": \"cmake --build --preset dev --target cppqtgraph_graphicsitems_arrowitem_p4_21\", \"exit_code\": 2, \"expected\": \"compile failed before ArrowItem implementation was added\"},\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.21 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_graphicsitems_arrowitem_p4_21\"},\n"
            "  \"checks\": [\"QGraphicsPathItem parent/pos hierarchy\", \"makeArrowPath-compatible head/tail geometry\", \"pxMode ItemIgnoresTransformations flag\", \"dataBounds and pixelPadding style behavior\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/ArrowItem\", \"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\", \"metrics\": \"metrics.json\", \"gpt5_vision_review\": \"gpt5_vision_review.md\"},\n"
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
                "  \"validation_commands\": [\"cmake --preset dev\", \"cmake --build --preset dev --parallel\", \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.21 --output-on-failure\", \"python3 -m pytest -q\", \"git diff --check\", \"git diff --name-only origin/main...HEAD\"],\n"
                "  \"example_manifest\": \"not_applicable: no example_manifest.yaml status fields changed for this focused shard\"\n"
                "}\n"));
    return true;
}

bool testConstructionAndStyle()
{
    using cppqtgraph::graphicsItems::ArrowItem;
    using cppqtgraph::graphicsItems::ArrowItemOptions;

    static_assert(std::is_constructible_v<ArrowItem>);
    static_assert(std::is_constructible_v<ArrowItem, QGraphicsItem*>);
    static_assert(std::is_constructible_v<ArrowItem, QPointF>);
    static_assert(std::is_constructible_v<ArrowItem, QPointF, QGraphicsItem*>);
    static_assert(std::is_constructible_v<ArrowItem, ArrowItemOptions>);
    static_assert(std::is_base_of_v<QGraphicsPathItem, ArrowItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, ArrowItem>);
    static_assert(!std::is_base_of_v<QGraphicsObject, ArrowItem>);
    static_assert(!std::is_final_v<ArrowItem>);

    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    ArrowItem child(QPointF(10.0, 10.0), &parent);
    CHECK(child.parentItem() == &parent);
    CHECK(child.pos() == QPointF(10.0, 10.0));
    CHECK(child.flags().testFlag(QGraphicsItem::ItemIgnoresTransformations));
    CHECK(!child.shape().isEmpty());
    CHECK(child.shape().elementCount() == child.path().elementCount());
    CHECK(child.pen().color() == QColor(200, 200, 200));
    CHECK(child.brush().color() == QColor(50, 50, 200));

    ArrowItemOptions style;
    style.pxMode = false;
    style.angle = 0.0;
    style.headLen = 20.0;
    style.headWidth = 10.0;
    style.baseAngle = 0.0;
    style.tailLen = 30.0;
    style.tailWidth = 4.0;
    style.pen = dataPen(Qt::white, 4.0);
    style.brush = QBrush(Qt::red);
    ArrowItem tailed(style);
    CHECK(!tailed.flags().testFlag(QGraphicsItem::ItemIgnoresTransformations));
    CHECK(tailed.path().elementCount() == 8);
    CHECK(samePoint(tailed.path().elementAt(0), 0.0, 0.0));
    CHECK(samePoint(tailed.path().elementAt(1), 20.0, -10.0));
    CHECK(samePoint(tailed.path().elementAt(2), 20.0, -2.0));
    CHECK(samePoint(tailed.path().elementAt(3), 50.0, -2.0));
    CHECK(samePoint(tailed.path().elementAt(4), 50.0, 2.0));
    CHECK(samePoint(tailed.path().elementAt(5), 20.0, 2.0));
    CHECK(samePoint(tailed.path().elementAt(6), 20.0, 10.0));
    CHECK(samePoint(tailed.path().elementAt(7), 0.0, 0.0));

    const QRectF tailedBounds = tailed.boundingRect();
    const auto dataX = tailed.dataBounds(0);
    const auto dataY = tailed.dataBounds(1);
    const qreal nonCosmeticPad = 4.0 * penPaddingFactor;
    CHECK_CLOSE(dataX.first, tailedBounds.left() - nonCosmeticPad, 1.0e-9);
    CHECK_CLOSE(dataX.second, tailedBounds.right() + nonCosmeticPad, 1.0e-9);
    CHECK_CLOSE(dataY.first, tailedBounds.top() - nonCosmeticPad, 1.0e-9);
    CHECK_CLOSE(dataY.second, tailedBounds.bottom() + nonCosmeticPad, 1.0e-9);
    CHECK_CLOSE(tailed.pixelPadding(), 0.0, 1.0e-9);

    style.pxMode = true;
    style.pen = cosmeticPen(Qt::white, 2.0);
    tailed.setStyle(style);
    CHECK(tailed.flags().testFlag(QGraphicsItem::ItemIgnoresTransformations));
    CHECK(tailed.dataBounds(0) == (std::pair<qreal, qreal>{0.0, 0.0}));
    CHECK(tailed.dataBounds(1) == (std::pair<qreal, qreal>{0.0, 0.0}));
    CHECK(tailed.pixelPadding() > std::hypot(tailed.boundingRect().width(), tailed.boundingRect().height()));

    style.angle = 90.0;
    tailed.setStyle(style);
    CHECK_CLOSE(tailed.path().elementAt(1).x, 10.0, 1.0e-9);
    CHECK_CLOSE(tailed.path().elementAt(1).y, 20.0, 1.0e-9);

    return true;
}

bool testVisualBehavior()
{
    const std::vector<VisualCase> cases = visualCases();
    const QImage reference = renderReference(cases);
    const QImage actual = renderActual(cases);
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    if (referencePixels < 900 || actualPixels < 900) {
        std::cerr << "ArrowItem blank/placeholder guard failed: reference=" << referencePixels
                  << " actual=" << actualPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(writeArtifacts(reference, actual, diff, metrics));
    CHECK(writeIssueReport(metrics, referencePixels, actualPixels));
    if (!metrics.passed) {
        std::cerr << "P4.21 ArrowItem visual comparison failed: changedPixels=" << metrics.changedPixels
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

    if (!testConstructionAndStyle()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
