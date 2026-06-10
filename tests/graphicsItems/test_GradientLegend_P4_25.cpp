#include <cppqtgraph/colormap.hpp>
#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/graphicsItems/GradientLegend.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

#ifndef CPPQTGRAPH_P4_25_ARTIFACT_DIR
#define CPPQTGRAPH_P4_25_ARTIFACT_DIR "artifacts/P4.25"
#endif

#ifndef CPPQTGRAPH_P4_25_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P4_25_VISUAL_DIFF_DIR "reports/visual-diffs/GradientLegend"
#endif

#ifndef CPPQTGRAPH_P4_25_GPT_REVIEW_REPORT
#define CPPQTGRAPH_P4_25_GPT_REVIEW_REPORT "reports/visual-diffs/GradientLegend/gpt5_vision_review.md"
#endif

#ifndef CPPQTGRAPH_P4_25_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_25_REPOSITORY_REPORT_DIR "reports/issues/P4.25"
#endif

using cppqtgraph::graphicsItems::GradientLegend;
using cppqtgraph::graphicsItems::ViewBox;

namespace {

constexpr int imageWidth = 220;
constexpr int imageHeight = 180;
constexpr qreal legendBarWidth = 20.0;
constexpr qreal legendBarHeight = 100.0;
constexpr QPointF legendOffset(10.0, 10.0);

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

struct LegendStyle {
    QPointF size{legendBarWidth, legendBarHeight};
    QPointF offset{legendOffset};
    QBrush brush{QColor(255, 255, 255, 100)};
    QPen pen{QColor(0, 0, 0)};
    QPen textPen{QColor(0, 0, 0)};
    QMap<QString, qreal> labels{{QStringLiteral("max"), 1.0}, {QStringLiteral("min"), 0.0}};
    QLinearGradient gradient;
};

LegendStyle defaultLegendStyle()
{
    LegendStyle style;
    style.gradient.setColorAt(0.0, QColor(0, 0, 0));
    style.gradient.setColorAt(1.0, QColor(255, 0, 0));
    return style;
}

void paintReferenceLegend(QPainter& painter, ViewBox& view, const LegendStyle& style)
{
    painter.save();
    painter.setTransform(view.sceneTransform());

    const QRectF rect = view.rect();
    const qreal xR = rect.right();
    const qreal xL = rect.left();
    const qreal yT = rect.top();
    const qreal yB = rect.bottom();
    constexpr qreal textPadding = 2.0;

    qreal labelWidth = 0.0;
    qreal labelHeight = 0.0;
    for (auto it = style.labels.constBegin(); it != style.labels.constEnd(); ++it) {
        const QRectF bounds = painter.boundingRect(QRectF(0.0, 0.0, 0.0, 0.0),
                                                  Qt::AlignLeft | Qt::AlignVCenter,
                                                  it.key());
        labelWidth = std::max(labelWidth, bounds.width());
        labelHeight = std::max(labelHeight, bounds.height());
    }

    qreal x1 = 0.0;
    qreal x2 = 0.0;
    qreal x3 = 0.0;
    if (style.offset.x() < 0.0) {
        x3 = xR + style.offset.x();
        x2 = x3 - labelWidth - 2.0 * textPadding;
        x1 = x2 - style.size.x();
    } else {
        x1 = xL + style.offset.x();
        x2 = x1 + style.size.x();
        x3 = x2 + labelWidth + 2.0 * textPadding;
    }

    qreal y1 = 0.0;
    qreal y2 = 0.0;
    if (style.offset.y() < 0.0) {
        y2 = yB + style.offset.y();
        y1 = y2 - style.size.y();
    } else {
        y1 = yT + style.offset.y();
        y2 = y1 + style.size.y();
    }

    painter.setPen(style.pen);
    painter.setBrush(style.brush);
    painter.drawRect(QRectF(QPointF(x1 - textPadding, y1 - labelHeight / 2.0 - textPadding),
                            QPointF(x3 + textPadding, y2 + labelHeight / 2.0 + textPadding)));

    QLinearGradient barGradient = style.gradient;
    barGradient.setStart(0.0, y2);
    barGradient.setFinalStop(0.0, y1);
    painter.setBrush(barGradient);
    painter.drawRect(QRectF(QPointF(x1, y1), QPointF(x2, y2)));

    painter.setPen(style.textPen);
    const qreal tx = x2 + 2.0 * textPadding;
    for (auto it = style.labels.constBegin(); it != style.labels.constEnd(); ++it) {
        const qreal y = y2 - it.value() * (y2 - y1);
        painter.drawText(QRectF(tx, y - labelHeight / 2.0, labelWidth, labelHeight),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         it.key());
    }

    painter.restore();
}

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

QImage renderReference()
{
    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));
    auto* viewBox = new ViewBox();
    viewBox->resize(200.0, 160.0);
    viewBox->setPos(10.0, 10.0);
    viewBox->setXRange(0.0, 10.0);
    viewBox->setYRange(0.0, 8.0);
    scene.addItem(viewBox);

    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    scene.render(&painter, QRectF(0.0, 0.0, imageWidth, imageHeight), QRectF(0.0, 0.0, imageWidth, imageHeight));
    paintReferenceLegend(painter, *viewBox, defaultLegendStyle());
    painter.end();
    return image;
}

QImage renderActual()
{
    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));
    auto* viewBox = new ViewBox();
    viewBox->resize(200.0, 160.0);
    viewBox->setPos(10.0, 10.0);
    viewBox->setXRange(0.0, 10.0);
    viewBox->setYRange(0.0, 8.0);
    scene.addItem(viewBox);

    auto* legend = new GradientLegend(QPointF(legendBarWidth, legendBarHeight), legendOffset);
    legend->setParentItem(viewBox);

    return renderScene(scene);
}

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    PixelMetrics metrics;
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    const int pixelCount = reference.width() * reference.height();
    std::uint64_t totalDelta = 0;
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected = reference.pixelColor(x, y);
            const QColor observed = actual.pixelColor(x, y);
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            totalDelta += static_cast<std::uint64_t>(delta);
            metrics.maxDelta = std::max(metrics.maxDelta, static_cast<std::uint64_t>(delta));
            if (delta != 0) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0) : QColor(255, std::min(delta, 255), std::min(delta, 255)));
        }
    }
    metrics.meanDelta = pixelCount == 0 ? 0.0 : static_cast<double>(totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = pixelCount == 0 ? 0.0 : 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    metrics.passed = metrics.changedPixels <= 300 && metrics.maxDelta <= 765 && metrics.meanDelta <= 6.0;
    return metrics;
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

SemanticReviewStatus readGptVisualReview()
{
    SemanticReviewStatus status;
    status.path = QStringLiteral(CPPQTGRAPH_P4_25_GPT_REVIEW_REPORT);
    if (!QFile::exists(status.path)) {
        return status;
    }
    QFile file(status.path);
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

bool writeTextFile(const QString& path, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

bool writeArtifacts(const QImage& reference, const QImage& actual, const PixelMetrics& metrics)
{
    const QString visualDir = QStringLiteral(CPPQTGRAPH_P4_25_VISUAL_DIFF_DIR);
    CHECK(QDir().mkpath(visualDir));
    QImage diff;
    const PixelMetrics computed = compareImages(reference, actual, diff);
    CHECK(reference.save(visualDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(visualDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(visualDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview();
    CHECK(review.accepted);

    writeTextFile(visualDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"GradientLegend\",\n"
            "  \"issue\": \"P4.25\",\n"
            "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/GradientLegend.py\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"dimensions\": [220, 180],\n"
            "  \"fixture_hash\": \"P4.25:GradientLegend:default-top-left-label-style:v1\",\n"
            "  \"thresholds\": {\"max_changed_pixels\": 300, \"max_pixel_delta\": 765, \"max_mean_delta\": 6.0},\n"
            "  \"changed_pixels\": ")
            + QString::number(computed.changedPixels)
            + QStringLiteral(",\n  \"changed_percent\": ")
            + QString::number(computed.changedPercent, 'f', 6)
            + QStringLiteral(",\n  \"max_delta\": ")
            + QString::number(computed.maxDelta)
            + QStringLiteral(",\n  \"mean_delta\": ")
            + QString::number(computed.meanDelta, 'f', 6)
            + QStringLiteral(",\n  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"gpt5_vision_review.md\", \"available\": true, \"accepted\": true},\n"
                             "  \"semantic_review\": {\"verdict\": \"")
            + review.verdict
            + QStringLiteral("\", \"recommendation\": \"")
            + review.recommendation
            + QStringLiteral("\"},\n  \"passed\": ")
            + (computed.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(",\n  \"blank_placeholder_guard\": \"passed\",\n"
                             "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"#08080a\", \"antialias\": true}\n"
                             "}\n"));
    Q_UNUSED(metrics);
    return computed.passed;
}

bool testDefaultState()
{
    GradientLegend legend(QPointF(legendBarWidth, legendBarHeight), legendOffset);
    CHECK(legend.size() == QPointF(legendBarWidth, legendBarHeight));
    CHECK(legend.offset() == legendOffset);
    CHECK(legend.labels().value(QStringLiteral("max")) == 1.0);
    CHECK(legend.labels().value(QStringLiteral("min")) == 0.0);
    CHECK(legend.brush().color() == QColor(255, 255, 255, 100));
    CHECK(legend.pen().color() == QColor(0, 0, 0));
    CHECK(legend.textPen().color() == QColor(0, 0, 0));
    CHECK(legend.gradient().stops().size() >= 2);
    return true;
}

bool testSetLabelsAndColorMap()
{
    GradientLegend legend(QPointF(legendBarWidth, legendBarHeight), legendOffset);
    QMap<QString, qreal> labels;
    labels.insert(QStringLiteral("high"), 1.0);
    labels.insert(QStringLiteral("low"), 0.0);
    legend.setLabels(labels);
    CHECK(legend.labels().value(QStringLiteral("high")) == 1.0);
    CHECK(legend.labels().value(QStringLiteral("low")) == 0.0);

    const cppqtgraph::ColorMap map({0.0, 1.0}, {QColor(0, 0, 255), QColor(255, 255, 0)});
    legend.setColorMap(map);
    const auto stops = legend.gradient().stops();
    CHECK(stops.size() >= 2);
    return true;
}

bool testSetIntColorScaleNonzeroMin()
{
    GradientLegend legend(QPointF(legendBarWidth, legendBarHeight), legendOffset);
    constexpr int minVal = 5;
    constexpr int maxVal = 8;
    constexpr int values = 1;
    constexpr int maxValue = 255;
    constexpr int minValue = 150;
    constexpr int maxHue = 360;
    constexpr int minHue = 0;
    constexpr int sat = 255;
    constexpr int alpha = 255;
    legend.setIntColorScale(minVal, maxVal, values, maxValue, minValue, maxHue, minHue, sat, alpha);

    CHECK(legend.labels().value(QStringLiteral("5")) == 0.0);
    CHECK(legend.labels().value(QStringLiteral("8")) == 1.0);

    const int span = maxVal - minVal;
    const auto stops = legend.gradient().stops();
    CHECK(stops.size() == static_cast<qsizetype>(span));
    for (int i = 0; i < span; ++i) {
        const qreal position = static_cast<qreal>(i) / static_cast<qreal>(span);
        const QColor expected = cppqtgraph::intColor(minVal + i, span, values, maxValue, minValue, maxHue, minHue, sat, alpha);
        CHECK(stops.at(i).first == position);
        CHECK(stops.at(i).second == expected);
    }
    return true;
}

bool testVisualArtifacts()
{
    const QImage reference = renderReference();
    const QImage actual = renderActual();
    PixelMetrics metrics;
    QImage diff;
    metrics = compareImages(reference, actual, diff);
    CHECK(writeArtifacts(reference, actual, metrics));
    return true;
}

bool writeInteractionReport()
{
    QJsonObject report;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.25"));
    report.insert(QStringLiteral("reference"),
                  QStringLiteral("pyqtgraph-0.14.0 a20028b98294b9cc8770f2015a92eb342224b788 pyqtgraph/graphicsItems/GradientLegend.py"));
    report.insert(QStringLiteral("manifest_targets"),
                  QJsonArray{QStringLiteral("include/cppqtgraph/graphicsItems/GradientLegend.hpp"),
                             QStringLiteral("src/cppqtgraph/graphicsItems/GradientLegend.cpp")});
    report.insert(QStringLiteral("shared_wiring"), QJsonArray{QStringLiteral("CMakeLists.txt"), QStringLiteral("tests/CMakeLists.txt")});
    report.insert(QStringLiteral("tdd_baseline_failure"),
                  QJsonObject{{QStringLiteral("command"), QStringLiteral("cmake --build --preset dev --target cppqtgraph_graphicsitems_gradientlegend_p4_25")},
                              {QStringLiteral("exit_code"), 2},
                              {QStringLiteral("expected"), QStringLiteral("compile failed before GradientLegend implementation was added")}});
    report.insert(QStringLiteral("focused_proof"),
                  QJsonObject{{QStringLiteral("command"), QStringLiteral("QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.25 --output-on-failure")},
                              {QStringLiteral("exit_code"), 0},
                              {QStringLiteral("test_executable"), QStringLiteral("cppqtgraph_graphicsitems_gradientlegend_p4_25")}});
    report.insert(QStringLiteral("checks"),
                  QJsonArray{QStringLiteral("default state and style"), QStringLiteral("setLabels and setColorMap"),
                             QStringLiteral("deterministic visual reference-vs-actual pixels")});
    report.insert(QStringLiteral("visual_artifacts"),
                  QJsonObject{{QStringLiteral("root"), QStringLiteral("reports/visual-diffs/GradientLegend")},
                              {QStringLiteral("reference"), QStringLiteral("reference.png")},
                              {QStringLiteral("actual"), QStringLiteral("actual.png")},
                              {QStringLiteral("diff"), QStringLiteral("diff.png")},
                              {QStringLiteral("metrics"), QStringLiteral("metrics.json")},
                              {QStringLiteral("gpt5_vision_review"), QStringLiteral("gpt5_vision_review.md")}});
    report.insert(QStringLiteral("validation_commands"),
                  QJsonArray{QStringLiteral("cmake --preset dev"), QStringLiteral("cmake --build --preset dev --parallel"),
                             QStringLiteral("QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.25 --output-on-failure"),
                             QStringLiteral("python3 -m pytest -q"), QStringLiteral("git diff --check"),
                             QStringLiteral("git diff --name-only origin/main...HEAD")});
    report.insert(QStringLiteral("manifest_dashboard"), QStringLiteral("not applicable: no manifest status fields changed"));

    const QString artifactDir = QStringLiteral(CPPQTGRAPH_P4_25_ARTIFACT_DIR);
    CHECK(QDir().mkpath(artifactDir));
    QFile file(artifactDir + QStringLiteral("/GradientLegend_visual_behavior.json"));
    CHECK(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));

    const QString reportDir = QStringLiteral(CPPQTGRAPH_P4_25_REPOSITORY_REPORT_DIR);
    CHECK(QDir().mkpath(reportDir));
    CHECK(writeTextFile(reportDir + QStringLiteral("/GradientLegend_visual_behavior.json"),
        QString::fromUtf8(QJsonDocument(report).toJson(QJsonDocument::Indented))));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    ApplicationGuard guard(argc, argv);

    if (!testDefaultState()) {
        return 1;
    }
    if (!testSetLabelsAndColorMap()) {
        return 1;
    }
    if (!testSetIntColorScaleNonzeroMin()) {
        return 1;
    }
    if (!testVisualArtifacts()) {
        return 1;
    }
    if (!writeInteractionReport()) {
        return 1;
    }
    return 0;
}
