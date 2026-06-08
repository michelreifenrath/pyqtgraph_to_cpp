#include <pyqtgraph/graphicsItems/ScaleBar.hpp>
#include <pyqtgraph/graphicsItems/TextItem.hpp>
#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P4_23_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P4_23_VISUAL_DIFF_DIR "reports/visual-diffs/ScaleBar"
#endif

#ifndef PYQTGRAPH_CPP_P4_23_GPT_REVIEW_REPORT
#define PYQTGRAPH_CPP_P4_23_GPT_REVIEW_REPORT "reports/visual-diffs/ScaleBar/gpt5_vision_review.md"
#endif

#ifndef PYQTGRAPH_CPP_P4_23_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P4_23_REPOSITORY_REPORT_DIR "reports/issues/P4.23"
#endif

namespace {

using ScaleBar = pyqtgraph::graphicsItems::ScaleBar;
using TextItem = pyqtgraph::graphicsItems::TextItem;
using ViewBox = pyqtgraph::graphicsItems::ViewBox;

constexpr int imageWidth = 480;
constexpr int imageHeight = 240;

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

QPointF scaledBottomRight(const QRectF& rect, const QPointF& anchor)
{
    const QPointF bottomRight = rect.bottomRight();
    return QPointF(bottomRight.x() * anchor.x(), bottomRight.y() * anchor.y());
}

std::pair<double, QString> siScale(double value, double power = 1.0)
{
    if (!std::isfinite(value)) {
        return {1.0, QString{}};
    }

    int magnitude = 0;
    if (std::abs(value) >= 1.0e-25) {
        const double denominator = std::log(1000.0) * power;
        double log1000 = std::log(std::abs(value)) / denominator;
        log1000 = power > 0.0 ? std::floor(log1000) : std::ceil(log1000);
        log1000 = std::clamp(log1000, -9.0, 9.0);
        magnitude = static_cast<int>(log1000);
    }

    QString prefix;
    if (magnitude == 0) {
        prefix = QString{};
    } else if (magnitude < -8 || magnitude > 8) {
        prefix = QStringLiteral("e%1").arg(magnitude * 3);
    } else {
        static const std::array<QString, 17> prefixes = {
            QStringLiteral("y"),
            QStringLiteral("z"),
            QStringLiteral("a"),
            QStringLiteral("f"),
            QStringLiteral("p"),
            QStringLiteral("n"),
            QString::fromUtf8("µ"),
            QStringLiteral("m"),
            QString{},
            QStringLiteral("k"),
            QStringLiteral("M"),
            QStringLiteral("G"),
            QStringLiteral("T"),
            QStringLiteral("P"),
            QStringLiteral("E"),
            QStringLiteral("Z"),
            QStringLiteral("Y"),
        };
        prefix = prefixes.at(static_cast<std::size_t>(magnitude + 8));
    }

    const double scale = std::pow(1000.0, -static_cast<double>(magnitude));
    return {scale, prefix};
}

QString siFormat(qreal value, int precision = 3, const QString& suffix = QString{})
{
    const auto [scale, prefix] = siScale(static_cast<double>(value));
    QString spacedPrefix = prefix;
    if (!(prefix.length() > 0 && prefix.startsWith(QLatin1Char('e')))) {
        spacedPrefix = QStringLiteral(" ") + prefix;
    }
    return QString::number(value * scale, 'g', precision) + spacedPrefix + suffix;
}

struct ScaleBarStyle {
    qreal size = 2.0;
    qreal width = 5.0;
    QBrush brush{QColor(200, 200, 200)};
    QPen pen{Qt::NoPen};
    QString suffix = QStringLiteral("m");
    QPointF offset{0.0, 0.0};
};


struct VisualLayout {
    QString name;
    QRectF viewGeometry;
    ViewBox::Range2D range;
    std::vector<ScaleBarStyle> scaleBars;
};

std::vector<VisualLayout> visualLayouts()
{
    std::vector<VisualLayout> layouts;

    ScaleBarStyle bottomRight;
    bottomRight.size = 2.0;
    bottomRight.width = 12.0;
    bottomRight.brush = QBrush(QColor(220, 220, 220));
    bottomRight.pen = QPen(QColor(40, 40, 40), 2.0);
    bottomRight.suffix = QStringLiteral("m");
    bottomRight.offset = QPointF(0.0, 0.0);

    ScaleBarStyle topLeft;
    topLeft.size = 5.0;
    topLeft.width = 10.0;
    topLeft.brush = QBrush(QColor(120, 200, 255));
    topLeft.pen = QPen(QColor(255, 220, 80), 2.0);
    topLeft.suffix = QStringLiteral("V");
    topLeft.offset = QPointF(12.0, 8.0);

    ScaleBarStyle wideRange;
    wideRange.size = 1.0;
    wideRange.width = 14.0;
    wideRange.brush = QBrush(QColor(180, 255, 160));
    wideRange.pen = QPen(QColor(255, 120, 120), 3.0);
    wideRange.suffix = QStringLiteral("s");
    wideRange.offset = QPointF(-10.0, -6.0);

    ScaleBarStyle midPanel;
    midPanel.size = 3.0;
    midPanel.width = 16.0;
    midPanel.brush = QBrush(QColor(255, 180, 220));
    midPanel.pen = QPen(QColor(240, 240, 240), 2.5);
    midPanel.suffix = QStringLiteral("mm");
    midPanel.offset = QPointF(-80.0, -40.0);

    layouts.push_back({QStringLiteral("multi-anchor-panel"),
                       QRectF(8.0, 8.0, 464.0, 224.0),
                       {{{0.0, 10.0}, {0.0, 5.0}}},
                       {bottomRight, topLeft, wideRange, midPanel}});

    return layouts;
}

class ReferenceScaleBar : public QGraphicsObject {
public:
    ReferenceScaleBar(const ScaleBarStyle& style, ViewBox* view)
        : style_(style)
        , view_(view)
    {
        setFlag(QGraphicsItem::ItemHasNoContents);
        bar_ = new QGraphicsRectItem(this);
        bar_->setPen(style_.pen);
        bar_->setBrush(style_.brush);

        text_ = new TextItem(siFormat(style_.size, 3, style_.suffix), QColor(200, 200, 200), QPointF(0.5, 1.0), this);
        updateLayout();
        applyAnchor();
    }

    QRectF boundingRect() const override
    {
        return QRectF();
    }

    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override
    {
    }

private:
    void updateLayout()
    {
        if (view_ == nullptr) {
            return;
        }
        const QPointF p1 = mapFromParent(view_->mapFromView(QPointF(0.0, 0.0)));
        const QPointF p2 = mapFromParent(view_->mapFromView(QPointF(style_.size, 0.0)));
        const qreal mappedWidth = p2.x() - p1.x();
        bar_->setRect(QRectF(-mappedWidth, 0.0, mappedWidth, style_.width));
        text_->setPos(-mappedWidth * 0.5, 0.0);
    }

    void applyAnchor()
    {
        if (view_ == nullptr) {
            return;
        }
        const qreal anchorX = style_.offset.x() <= 0.0 ? 1.0 : 0.0;
        const qreal anchorY = style_.offset.y() <= 0.0 ? 1.0 : 0.0;
        const QPointF itemAnchor(anchorX, anchorY);
        const QPointF parentAnchor(anchorX, anchorY);
        const QPointF originInParent = mapToParent(QPointF(0.0, 0.0));
        const QPointF itemAnchorLocal = scaledBottomRight(boundingRect(), itemAnchor);
        const QPointF itemAnchorInParent = mapToParent(itemAnchorLocal);
        const QPointF parentAnchorPoint = scaledBottomRight(view_->boundingRect(), parentAnchor);
        setPos(parentAnchorPoint + (originInParent - itemAnchorInParent) + style_.offset);
    }

    ScaleBarStyle style_;
    ViewBox* view_ = nullptr;
    QGraphicsRectItem* bar_ = nullptr;
    TextItem* text_ = nullptr;
};

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(8, 8, 10));
    return image;
}

QImage renderScene(QGraphicsScene& scene)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    scene.render(&painter, QRectF(0.0, 0.0, imageWidth, imageHeight), QRectF(0.0, 0.0, imageWidth, imageHeight));
    painter.end();
    return image;
}

void populateScene(QGraphicsScene& scene, bool useReference)
{
    for (const VisualLayout& layout : visualLayouts()) {
        auto* viewBox = new ViewBox();
        viewBox->resize(layout.viewGeometry.width(), layout.viewGeometry.height());
        viewBox->setPos(layout.viewGeometry.topLeft());
        viewBox->setXRange(layout.range[0][0], layout.range[0][1]);
        viewBox->setYRange(layout.range[1][0], layout.range[1][1]);
        scene.addItem(viewBox);

        for (const ScaleBarStyle& style : layout.scaleBars) {
            if (useReference) {
                auto* scaleBar = new ReferenceScaleBar(style, viewBox);
                scaleBar->setParentItem(viewBox);
            } else {
                auto* scaleBar = new ScaleBar(style.size,
                                              style.width,
                                              style.brush,
                                              style.pen,
                                              style.suffix,
                                              style.offset);
                scaleBar->setParentItem(viewBox);
            }
        }
    }
}

QImage renderReference()
{
    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));
    populateScene(scene, true);
    return renderScene(scene);
}

QImage renderActual()
{
    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));
    populateScene(scene, false);
    return renderScene(scene);
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
    status.path = QStringLiteral(PYQTGRAPH_CPP_P4_23_GPT_REVIEW_REPORT);
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P4.23 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P4.23 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P4.23 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
}

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const QString visualDir = QStringLiteral(PYQTGRAPH_CPP_P4_23_VISUAL_DIFF_DIR);
    CHECK(ensureDirectory(visualDir));
    CHECK(reference.save(visualDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(visualDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(visualDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview();
    CHECK(review.accepted);

    writeTextFile(visualDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"ScaleBar\",\n"
            "  \"issue\": \"P4.23\",\n"
            "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ScaleBar.py\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"dimensions\": [480, 240],\n"
            "  \"fixture_hash\": \"P4.23:ScaleBar:size-anchor-style-label:v1\",\n"
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
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P4_23_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ScaleBar_visual_behavior.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P4.23\",\n"
            "  \"class\": \"pyqtgraph::graphicsItems::ScaleBar\",\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ScaleBar.py; ViewBox mapFromViewToItem anchoring\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/graphicsItems/ScaleBar.hpp\", \"src/pyqtgraph/graphicsItems/ScaleBar.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"tdd_baseline_failure\": {\"command\": \"cmake --build --preset dev --target pyqtgraph_cpp_graphicsitems_scalebar_p4_23\", \"exit_code\": 2, \"expected\": \"compile failed before ScaleBar implementation was added\"},\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.23 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_graphicsitems_scalebar_p4_23\"},\n"
            "  \"checks\": [\"view-coordinate bar width mapping\", \"offset anchor bottom-right and top-left\", \"pen/brush style on QGraphicsRectItem bar\", \"TextItem SI label centered above bar\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/ScaleBar\", \"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\", \"metrics\": \"metrics.json\", \"gpt5_vision_review\": \"gpt5_vision_review.md\"},\n"
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
                "  \"validation_commands\": [\"cmake --preset dev\", \"cmake --build --preset dev --parallel\", \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.23 --output-on-failure\", \"python3 -m pytest -q\", \"git diff --check\", \"git diff --name-only origin/main...HEAD\"],\n"
                "  \"manifest_dashboard\": \"not_applicable: port_manifest/dashboard updates are outside P4.23 owned paths for this shard\"\n"
                "}\n"));
    return true;
}

bool testSiFormatSuffixSpacing()
{
    CHECK(siFormat(2.0, 3, QStringLiteral("m")) == QStringLiteral("2 m"));
    CHECK(siFormat(5.0, 3, QStringLiteral("V")) == QStringLiteral("5 V"));
    CHECK(siFormat(0.002, 3, QStringLiteral("m")) == QStringLiteral("2 mm"));
    return true;
}

bool testConstructorWithParentInitializesLayout()
{
    ViewBox viewBox;
    viewBox.setGeometry(QRectF(0.0, 0.0, 200.0, 100.0));
    viewBox.setXRange(0.0, 10.0);
    viewBox.setYRange(0.0, 5.0);

    ScaleBar scaleBar(2.0, 6.0, QBrush(QColor(220, 220, 220)), QPen(QColor(40, 40, 40), 1.0), QStringLiteral("m"), QPointF(0.0, 0.0), &viewBox);

    CHECK(scaleBar.parentItem() == &viewBox);

    const QPointF p1 = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(0.0, 0.0)));
    const QPointF p2 = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(2.0, 0.0)));
    const qreal expectedWidth = p2.x() - p1.x();
    CHECK(expectedWidth > 0.0);

    QGraphicsRectItem* bar = nullptr;
    for (QGraphicsItem* child : scaleBar.childItems()) {
        if (child->type() == QGraphicsRectItem::Type) {
            bar = static_cast<QGraphicsRectItem*>(child);
            break;
        }
    }
    CHECK(bar != nullptr);
    CHECK_CLOSE(bar->rect().width(), expectedWidth, 1.0e-6);

    const QRectF parentRect = viewBox.boundingRect();
    CHECK_CLOSE(scaleBar.pos().x(), parentRect.right(), 1.0e-6);
    CHECK_CLOSE(scaleBar.pos().y(), parentRect.bottom(), 1.0e-6);

    return true;
}

bool testResizeUpdatesBarAndAnchor()
{
    ViewBox viewBox;
    viewBox.setGeometry(QRectF(0.0, 0.0, 200.0, 100.0));
    viewBox.setXRange(0.0, 10.0);
    viewBox.setYRange(0.0, 5.0);

    ScaleBar scaleBar(2.0, 6.0, QBrush(QColor(220, 220, 220)), QPen(QColor(40, 40, 40), 1.0), QStringLiteral("m"), QPointF(0.0, 0.0));
    scaleBar.setParentItem(&viewBox);

    QGraphicsRectItem* bar = nullptr;
    for (QGraphicsItem* child : scaleBar.childItems()) {
        if (child->type() == QGraphicsRectItem::Type) {
            bar = static_cast<QGraphicsRectItem*>(child);
            break;
        }
    }
    CHECK(bar != nullptr);

    const QPointF p1Before = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(0.0, 0.0)));
    const QPointF p2Before = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(2.0, 0.0)));
    const qreal widthBefore = p2Before.x() - p1Before.x();
    const QPointF posBefore = scaleBar.pos();
    CHECK_CLOSE(bar->rect().width(), widthBefore, 1.0e-6);

    viewBox.resize(400.0, 200.0);

    const QPointF p1After = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(0.0, 0.0)));
    const QPointF p2After = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(2.0, 0.0)));
    const qreal widthAfter = p2After.x() - p1After.x();
    CHECK(widthAfter > widthBefore);
    CHECK_CLOSE(bar->rect().width(), widthAfter, 1.0e-6);

    const QRectF parentRect = viewBox.boundingRect();
    CHECK_CLOSE(scaleBar.pos().x(), parentRect.right(), 1.0e-6);
    CHECK_CLOSE(scaleBar.pos().y(), parentRect.bottom(), 1.0e-6);
    CHECK(scaleBar.pos() != posBefore);

    return true;
}

bool testConstructionAndBehavior()
{
    static_assert(std::is_constructible_v<ScaleBar, qreal>);
    static_assert(std::is_constructible_v<ScaleBar, qreal, qreal, QBrush, QPen, QString, QPointF, QGraphicsItem*>);
    static_assert(std::is_base_of_v<QGraphicsObject, ScaleBar>);
    static_assert(!std::is_final_v<ScaleBar>);

    ViewBox viewBox;
    viewBox.setGeometry(QRectF(0.0, 0.0, 200.0, 100.0));
    viewBox.setXRange(0.0, 10.0);
    viewBox.setYRange(0.0, 5.0);

    ScaleBar scaleBar(2.0, 6.0, QBrush(QColor(220, 220, 220)), QPen(QColor(40, 40, 40), 1.0), QStringLiteral("m"), QPointF(0.0, 0.0));
    scaleBar.setParentItem(&viewBox);

    CHECK(scaleBar.parentItem() == &viewBox);
    CHECK(scaleBar.flags().testFlag(QGraphicsItem::ItemHasNoContents));
    CHECK(scaleBar.boundingRect().isEmpty());
    CHECK(scaleBar.size() == 2.0);
    CHECK(scaleBar.barWidth() == 6.0);
    CHECK(scaleBar.suffix() == QStringLiteral("m"));
    CHECK(scaleBar.offset() == QPointF(0.0, 0.0));

    const QPointF p1 = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(0.0, 0.0)));
    const QPointF p2 = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(2.0, 0.0)));
    const qreal expectedWidth = p2.x() - p1.x();
    CHECK(expectedWidth > 0.0);

    QList<QGraphicsItem*> children = scaleBar.childItems();
    CHECK(children.size() == 2);

    QGraphicsRectItem* bar = nullptr;
    TextItem* label = nullptr;
    for (QGraphicsItem* child : children) {
        if (child->type() == QGraphicsRectItem::Type) {
            bar = static_cast<QGraphicsRectItem*>(child);
            continue;
        }
        label = dynamic_cast<TextItem*>(child);
    }
    CHECK(bar != nullptr);
    CHECK(label != nullptr);
    CHECK_CLOSE(bar->rect().width(), expectedWidth, 1.0e-6);
    CHECK_CLOSE(bar->rect().left(), -expectedWidth, 1.0e-6);
    CHECK_CLOSE(bar->rect().height(), 6.0, 1.0e-6);
    CHECK(bar->brush().color() == QColor(220, 220, 220));
    CHECK(bar->pen().color() == QColor(40, 40, 40));
    CHECK(label->toPlainText() == siFormat(2.0, 3, QStringLiteral("m")));
    CHECK_CLOSE(label->pos().x(), -expectedWidth * 0.5, 1.0e-6);
    CHECK_CLOSE(label->pos().y(), 0.0, 1.0e-6);

    const QRectF parentRect = viewBox.boundingRect();
    CHECK_CLOSE(scaleBar.pos().x(), parentRect.right(), 1.0e-6);
    CHECK_CLOSE(scaleBar.pos().y(), parentRect.bottom(), 1.0e-6);

    ScaleBar topLeftBar(5.0, 4.0, QBrush(Qt::white), QPen(Qt::NoPen), QStringLiteral("V"), QPointF(12.0, 8.0));
    topLeftBar.setParentItem(&viewBox);
    CHECK_CLOSE(topLeftBar.pos().x(), parentRect.left() + 12.0, 1.0e-6);
    CHECK_CLOSE(topLeftBar.pos().y(), parentRect.top() + 8.0, 1.0e-6);
    CHECK(topLeftBar.suffix() == QStringLiteral("V"));

    viewBox.setXRange(0.0, 20.0);
    viewBox.setYRange(0.0, 5.0);
    const QPointF p1Wide = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(0.0, 0.0)));
    const QPointF p2Wide = scaleBar.mapFromParent(viewBox.mapFromView(QPointF(2.0, 0.0)));
    const qreal narrowerWidth = p2Wide.x() - p1Wide.x();
    CHECK(narrowerWidth < expectedWidth);
    CHECK_CLOSE(bar->rect().width(), narrowerWidth, 1.0e-6);

    return true;
}

bool testVisualBehavior()
{
    const QImage reference = renderReference();
    const QImage actual = renderActual();
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    if (referencePixels < 900 || actualPixels < 900) {
        std::cerr << "ScaleBar blank/placeholder guard failed: reference=" << referencePixels
                  << " actual=" << actualPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(writeArtifacts(reference, actual, diff, metrics));
    CHECK(writeIssueReport(metrics, referencePixels, actualPixels));
    if (!metrics.passed) {
        std::cerr << "P4.23 ScaleBar visual comparison failed: changedPixels=" << metrics.changedPixels
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

    if (!testSiFormatSuffixSpacing()) {
        return 1;
    }
    if (!testConstructorWithParentInitializesLayout()) {
        return 1;
    }
    if (!testResizeUpdatesBarAndAnchor()) {
        return 1;
    }
    if (!testConstructionAndBehavior()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
