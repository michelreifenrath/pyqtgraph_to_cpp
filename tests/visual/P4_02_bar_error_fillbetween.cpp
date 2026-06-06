#include <pyqtgraph/graphicsItems/BarGraphItem.hpp>
#include <pyqtgraph/graphicsItems/ErrorBarItem.hpp>
#include <pyqtgraph/graphicsItems/FillBetweenItem.hpp>
#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPolygonF>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P4_02_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_02_ARTIFACT_DIR "reports/visual/P4.02"
#endif

#ifndef PYQTGRAPH_CPP_P4_02_CANONICAL_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_02_CANONICAL_ARTIFACT_DIR "reports/visual-diffs/P4.02-bar-error-fillbetween"
#endif

namespace {

constexpr int imageWidth = 720;
constexpr int imageHeight = 300;
const QRectF barPanel(0, 0, 240, 300);
const QRectF errorPanel(240, 0, 240, 300);
const QRectF fillPanel(480, 0, 240, 300);
const QRectF barView(-0.9, -1.6, 11.4, 4.4);
const QRectF errorView(-0.8, -2.2, 10.6, 7.0);
const QRectF fillView(-10.5, -4.8, 21.0, 9.6);

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

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

QPen cosmeticPen(const QColor& color, qreal width = 1.0)
{
    QPen pen(color, width);
    pen.setCosmetic(true);
    return pen;
}

QTransform panelTransform(const QRectF& panel, const QRectF& view)
{
    QTransform transform;
    transform.translate(panel.left(), panel.bottom() - 1.0);
    transform.scale(panel.width() / view.width(), -panel.height() / view.height());
    transform.translate(-view.left(), -view.top());
    return transform;
}

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    return image;
}

std::vector<double> range10()
{
    std::vector<double> values(10);
    for (int index = 0; index < 10; ++index) {
        values[static_cast<std::size_t>(index)] = index;
    }
    return values;
}

std::vector<double> shifted(std::span<const double> values, double offset)
{
    std::vector<double> result(values.begin(), values.end());
    for (double& value : result) {
        value += offset;
    }
    return result;
}

std::vector<double> sinValues(std::span<const double> x, double phase, double scale = 1.0)
{
    std::vector<double> y;
    y.reserve(x.size());
    for (const double value : x) {
        y.push_back(scale * std::sin(value + phase));
    }
    return y;
}

void drawBarReference(QPainter& painter)
{
    painter.save();
    painter.setTransform(panelTransform(barPanel, barView));
    painter.setPen(cosmeticPen(Qt::white));
    const auto x = range10();
    const auto y1 = sinValues(x, 0.0, 1.0);
    const auto y2 = sinValues(x, 1.0, 1.1);
    const auto y3 = sinValues(x, 2.0, 1.2);
    const std::vector<QColor> colors{Qt::red, Qt::green, Qt::blue, QColor(128, 128, 128)};
    const std::vector<std::vector<double>> xs{x, shifted(x, 0.33), shifted(x, 0.66), x};
    const std::vector<std::vector<double>> ys{y1, y2, y3, [&]() {
                                                       std::vector<double> result;
                                                       result.reserve(y1.size());
                                                       for (const double value : y1) {
                                                           result.push_back(value * 0.3 + 2.0);
                                                       }
                                                       return result;
                                                   }()};
    const std::vector<std::vector<double>> heights{y1, y2, y3, [&]() {
                                                             std::vector<double> result;
                                                             result.reserve(y1.size());
                                                             for (const double value : y1) {
                                                                 result.push_back(0.4 + value * 0.2);
                                                             }
                                                             return result;
                                                         }()};
    const std::vector<double> widths{0.3, 0.3, 0.3, 0.8};
    for (std::size_t series = 0; series < xs.size(); ++series) {
        painter.setBrush(QBrush(colors[series]));
        for (std::size_t index = 0; index < xs[series].size(); ++index) {
            const qreal x0 = xs[series][index] - widths[series] / 2.0;
            const qreal y0 = series < 3 ? 0.0 : ys[series][index] - heights[series][index] / 2.0;
            const qreal x1 = x0 + widths[series];
            const qreal y1Value = y0 + heights[series][index];
            painter.drawRect(QRectF(QPointF(std::min(x0, x1), std::min(y0, y1Value)),
                QPointF(std::max(x0, x1), std::max(y0, y1Value))));
        }
    }
    painter.restore();
}

void drawErrorReference(QPainter& painter)
{
    painter.save();
    painter.setTransform(panelTransform(errorPanel, errorView));
    painter.setPen(cosmeticPen(Qt::white));
    const auto x = range10();
    std::vector<double> y;
    std::vector<double> top;
    std::vector<double> bottom;
    for (std::size_t index = 0; index < x.size(); ++index) {
        y.push_back(static_cast<int>(index) % 3);
        top.push_back(1.0 + (2.0 * static_cast<double>(index) / 9.0));
        bottom.push_back(2.0 + ((0.5 - 2.0) * static_cast<double>(index) / 9.0));
    }
    for (std::size_t index = 0; index < x.size(); ++index) {
        const qreal yBottom = y[index] - bottom[index];
        const qreal yTop = y[index] + top[index];
        painter.drawLine(QPointF(x[index], yBottom), QPointF(x[index], yTop));
        painter.drawLine(QPointF(x[index] - 0.25, yTop), QPointF(x[index] + 0.25, yTop));
        painter.drawLine(QPointF(x[index] - 0.25, yBottom), QPointF(x[index] + 0.25, yBottom));
    }
    painter.restore();
}

void drawFillReference(QPainter& painter)
{
    painter.save();
    painter.setTransform(panelTransform(fillPanel, fillView));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(100, 100, 255, 180)));
    constexpr int pointCount = 200;
    QPolygonF polygon;
    polygon.reserve(pointCount * 2);
    std::vector<QPointF> lower;
    std::vector<QPointF> upper;
    lower.reserve(pointCount);
    upper.reserve(pointCount);
    for (int index = 0; index < pointCount; ++index) {
        const double x = -10.0 + 20.0 * static_cast<double>(index) / static_cast<double>(pointCount - 1);
        const double gauss = std::exp(-(x * x) / 20.0);
        lower.push_back(QPointF(x, -3.0 * gauss - 0.35 * std::cos(x * 1.7)));
        upper.push_back(QPointF(x, 3.0 * gauss + 0.25 * std::sin(x * 1.3)));
    }
    for (const QPointF& point : lower) {
        polygon << point;
    }
    for (auto it = upper.rbegin(); it != upper.rend(); ++it) {
        polygon << *it;
    }
    painter.drawPolygon(polygon, Qt::OddEvenFill);
    painter.restore();
}

QImage renderReference()
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    drawBarReference(painter);
    drawErrorReference(painter);
    drawFillReference(painter);
    painter.end();
    return image;
}

QImage renderActual()
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QStyleOptionGraphicsItem option;

    const auto x = range10();
    const auto y1 = sinValues(x, 0.0, 1.0);
    const auto y2 = sinValues(x, 1.0, 1.1);
    const auto y3 = sinValues(x, 2.0, 1.2);

    painter.save();
    painter.setTransform(panelTransform(barPanel, barView));
    pyqtgraph::graphicsItems::BarGraphItem bg1(x, y1, 0.3);
    bg1.setBrush(QBrush(Qt::red));
    pyqtgraph::graphicsItems::BarGraphItem bg2(shifted(x, 0.33), y2, 0.3);
    bg2.setBrush(QBrush(Qt::green));
    pyqtgraph::graphicsItems::BarGraphItem bg3(shifted(x, 0.66), y3, 0.3);
    bg3.setBrush(QBrush(Qt::blue));
    std::vector<double> barY;
    std::vector<double> barHeight;
    for (const double value : y1) {
        barY.push_back(value * 0.3 + 2.0);
        barHeight.push_back(0.4 + value * 0.2);
    }
    pyqtgraph::graphicsItems::BarGraphItem bg4;
    bg4.setData(x, barY, barHeight, 0.8);
    for (auto* item : {&bg1, &bg2, &bg3, &bg4}) {
        item->paint(&painter, &option, nullptr);
    }
    painter.restore();

    painter.save();
    painter.setTransform(panelTransform(errorPanel, errorView));
    std::vector<double> y;
    std::vector<double> top;
    std::vector<double> bottom;
    for (std::size_t index = 0; index < x.size(); ++index) {
        y.push_back(static_cast<int>(index) % 3);
        top.push_back(1.0 + (2.0 * static_cast<double>(index) / 9.0));
        bottom.push_back(2.0 + ((0.5 - 2.0) * static_cast<double>(index) / 9.0));
    }
    pyqtgraph::graphicsItems::ErrorBarItem errors;
    errors.setData(x, y, top, bottom, 0.5);
    errors.paint(&painter, &option, nullptr);
    painter.restore();

    painter.save();
    painter.setTransform(panelTransform(fillPanel, fillView));
    constexpr int pointCount = 200;
    std::vector<double> fillX;
    std::vector<double> lower;
    std::vector<double> upper;
    for (int index = 0; index < pointCount; ++index) {
        const double value = -10.0 + 20.0 * static_cast<double>(index) / static_cast<double>(pointCount - 1);
        const double gauss = std::exp(-(value * value) / 20.0);
        fillX.push_back(value);
        lower.push_back(-3.0 * gauss - 0.35 * std::cos(value * 1.7));
        upper.push_back(3.0 * gauss + 0.25 * std::sin(value * 1.3));
    }
    pyqtgraph::graphicsItems::PlotCurveItem lowerCurve;
    lowerCurve.setData(fillX, lower);
    pyqtgraph::graphicsItems::PlotCurveItem upperCurve;
    upperCurve.setData(fillX, upper);
    pyqtgraph::graphicsItems::FillBetweenItem fill(&lowerCurve, &upperCurve, QBrush(QColor(100, 100, 255, 180)), QPen(Qt::NoPen));
    fill.paint(&painter, &option, nullptr);
    painter.restore();

    painter.end();
    return image;
}

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
    metrics.passed = metrics.changedPixels <= 48 && metrics.maxDelta <= 4;
    return metrics;
}

std::uint64_t semanticPixelCount(const QImage& image)
{
    std::uint64_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && (color.red() > 12 || color.green() > 12 || color.blue() > 12)) {
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

struct SemanticReviewStatus {
    QString sourcePath;
    QString verdict;
    QString recommendation;
    bool configured = false;
    bool sourceExists = false;
    bool accepted = false;
};

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

SemanticReviewStatus readOptionalGptVisualReview()
{
    SemanticReviewStatus status;
#ifdef PYQTGRAPH_CPP_P4_02_GPT_REVIEW_REPORT
    status.sourcePath = QStringLiteral(PYQTGRAPH_CPP_P4_02_GPT_REVIEW_REPORT);
#endif
    if (status.sourcePath.isEmpty()) {
        status.sourcePath = qEnvironmentVariable("PG_P4_02_VISUAL_REVIEW_REPORT");
    }
    const bool explicitReviewSource = !status.sourcePath.isEmpty();
    if (status.sourcePath.isEmpty()) {
        const std::vector<QString> existingReviewCandidates{
#ifdef PYQTGRAPH_CPP_P4_02_REPOSITORY_CANONICAL_ARTIFACT_DIR
            QStringLiteral(PYQTGRAPH_CPP_P4_02_REPOSITORY_CANONICAL_ARTIFACT_DIR) + QStringLiteral("/gpt5_vision_review.md"),
#endif
#ifdef PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR
            QStringLiteral(PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR) + QStringLiteral("/bar-error-fillbetween/gpt5_vision_review.md"),
#endif
        };
        for (const QString& candidate : existingReviewCandidates) {
            if (QFile::exists(candidate)) {
                status.sourcePath = candidate;
                break;
            }
        }
    }
    status.configured = explicitReviewSource;
    if (status.sourcePath.isEmpty()) {
        return status;
    }
    if (!QFile::exists(status.sourcePath)) {
        std::cerr << "configured P4.02 GPT visual review is missing: " << status.sourcePath.toStdString() << '\n';
        return status;
    }
    QFile file(status.sourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "configured P4.02 GPT visual review is unreadable: " << status.sourcePath.toStdString() << '\n';
        return status;
    }
    status.sourceExists = true;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
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
    status.accepted = status.verdict == QStringLiteral("pass") && status.recommendation == QStringLiteral("merge_ok");
    if (!status.accepted) {
        std::cerr << "configured P4.02 GPT visual review is not accepted in " << status.sourcePath.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString() << ")\n";
    }
    return status;
}

bool copyOptionalGptVisualReview(const SemanticReviewStatus& status, const QString& destinationPath)
{
    if (!status.sourceExists) {
        if (status.configured) {
            std::cerr << "P4.02 GPT visual review is required but was not available\n";
            return false;
        }
        Q_UNUSED(destinationPath);
        return true;
    }
    if (!status.accepted) {
        return false;
    }
    if (QFileInfo(status.sourcePath).absoluteFilePath() == QFileInfo(destinationPath).absoluteFilePath()) {
        return true;
    }
    QFile::remove(destinationPath);
    if (!QFile::copy(status.sourcePath, destinationPath)) {
        std::cerr << "failed to copy P4.02 GPT visual review from " << status.sourcePath.toStdString() << " to "
                  << destinationPath.toStdString() << '\n';
        return false;
    }
    return true;
}

QString fixtureHash()
{
    return QStringLiteral("P4.02:bargraph-example:errorbar-example:fillbetween-static-gaussian:v1");
}

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    std::vector<QString> caseDirs;
    auto addUniqueCaseDir = [&caseDirs](const QString& dir) {
        if (std::find(caseDirs.begin(), caseDirs.end(), dir) == caseDirs.end()) {
            caseDirs.push_back(dir);
        }
    };
    addUniqueCaseDir(QStringLiteral(PYQTGRAPH_CPP_P4_02_ARTIFACT_DIR) + QStringLiteral("/bar-error-fillbetween"));
    addUniqueCaseDir(QStringLiteral(PYQTGRAPH_CPP_P4_02_CANONICAL_ARTIFACT_DIR));
#ifdef PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR
    addUniqueCaseDir(QStringLiteral(PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR) + QStringLiteral("/bar-error-fillbetween"));
#endif
#ifdef PYQTGRAPH_CPP_P4_02_REPOSITORY_CANONICAL_ARTIFACT_DIR
    addUniqueCaseDir(QStringLiteral(PYQTGRAPH_CPP_P4_02_REPOSITORY_CANONICAL_ARTIFACT_DIR));
#endif
    const SemanticReviewStatus reviewStatus = readOptionalGptVisualReview();
    for (const QString& caseDir : caseDirs) {
        CHECK(ensureDirectory(caseDir));
        CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
        CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
        CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));
        CHECK(copyOptionalGptVisualReview(reviewStatus, caseDir + QStringLiteral("/gpt5_vision_review.md")));
        writeTextFile(caseDir + QStringLiteral("/metrics.json"),
            QStringLiteral(
                "{\n"
                "  \"case\": \"bar-error-fillbetween\",\n"
                "  \"issue\": \"P4.02\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/examples/BarGraphItem.py; pyqtgraph/examples/ErrorBarItem.py; pyqtgraph/examples/FillBetweenItem.py; pyqtgraph/graphicsItems/BarGraphItem.py; pyqtgraph/graphicsItems/ErrorBarItem.py; pyqtgraph/graphicsItems/FillBetweenItem.py\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [720, 300],\n"
                "  \"fixture_hash\": \"")
                + fixtureHash()
                + QStringLiteral(
                    "\",\n"
                    "  \"thresholds\": {\"max_changed_pixels\": 48, \"max_pixel_delta\": 4},\n"
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
                + jsonEscape(reviewStatus.sourcePath)
                + QStringLiteral("\", \"generated_by_test\": false, \"copied_by_test\": ")
                + (reviewStatus.sourceExists && reviewStatus.accepted ? QStringLiteral("true") : QStringLiteral("false"))
                + QStringLiteral(", \"available\": ")
                + (reviewStatus.sourceExists ? QStringLiteral("true") : QStringLiteral("false"))
                + QStringLiteral(
                    ", \"configured\": ")
                + (reviewStatus.configured ? QStringLiteral("true") : QStringLiteral("false"))
                + QStringLiteral(
                    "},\n"
                    "  \"semantic_review\": ")
                + (reviewStatus.sourceExists
                        ? QStringLiteral("{\"verdict\": \"") + jsonEscape(reviewStatus.verdict)
                            + QStringLiteral("\", \"recommendation\": \"") + jsonEscape(reviewStatus.recommendation)
                            + QStringLiteral("\", \"accepted\": ") + (reviewStatus.accepted ? QStringLiteral("true") : QStringLiteral("false"))
                            + QStringLiteral("}")
                        : QStringLiteral("null"))
                + QStringLiteral(
                    ",\n"
                    "  \"passed\": ")
                + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
                + QStringLiteral(
                    ",\n"
                    "  \"blank_placeholder_guard\": \"passed\",\n"
                    "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"black\", \"antialias\": false}\n"
                    "}\n"));
    }
    std::vector<QString> reportRoots;
    auto addUniqueReportRoot = [&reportRoots](const QString& dir) {
        if (std::find(reportRoots.begin(), reportRoots.end(), dir) == reportRoots.end()) {
            reportRoots.push_back(dir);
        }
    };
    addUniqueReportRoot(QStringLiteral(PYQTGRAPH_CPP_P4_02_ARTIFACT_DIR));
#ifdef PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR
    addUniqueReportRoot(QStringLiteral(PYQTGRAPH_CPP_P4_02_REPOSITORY_ARTIFACT_DIR));
#endif
    for (const QString& reportRoot : reportRoots) {
        CHECK(ensureDirectory(reportRoot));
        writeTextFile(reportRoot + QStringLiteral("/manual_semantic_inspection.md"),
            QStringLiteral(
                "# P4.02 manual semantic inspection note\n\n"
                "Deterministic visual artifacts cover the upstream BarGraphItem, ErrorBarItem, and FillBetweenItem example shapes. "
                "The implementing agent must open/read reference.png, actual.png, and diff.png and record the semantic inspection in implementation.md. "
                "If an external GPT visual review artifact is configured, this test validates and copies it rather than fabricating a semantic verdict.\n"));
        writeTextFile(reportRoot + QStringLiteral("/summary.json"),
            QStringLiteral("{\n  \"issue\": \"P4.02\",\n  \"passed_cases\": ") + (metrics.passed ? QStringLiteral("1") : QStringLiteral("0"))
                + QStringLiteral(",\n  \"total_cases\": 1,\n  \"blank_placeholder_guard\": \"passed\"\n}\n"));
    }
    return true;
}

bool testDataGuardsAndBounds()
{
    pyqtgraph::graphicsItems::BarGraphItemOptions defaultBarOptions;
    pyqtgraph::graphicsItems::ErrorBarItemOptions defaultErrorOptions;
    CHECK(defaultBarOptions.pen.isCosmetic());
    CHECK(defaultErrorOptions.pen.isCosmetic());
    pyqtgraph::graphicsItems::BarGraphItem optionBars(defaultBarOptions);
    pyqtgraph::graphicsItems::ErrorBarItem optionErrors(defaultErrorOptions);
    CHECK(optionBars.pen().isCosmetic());
    CHECK(optionErrors.pen().isCosmetic());

    pyqtgraph::graphicsItems::ErrorBarItem deferred;
    CHECK(!deferred.isVisible());
    CHECK(deferred.boundingRect().isNull());

    const std::vector<double> x{1.0, 2.0};
    const std::vector<double> y{3.0, 4.0};
    const std::vector<double> top{0.5, 0.75};
    const std::vector<double> bottom{0.25, 0.5};
    deferred.setData(x, y, top, bottom, 0.4);
    CHECK(deferred.isVisible());
    CHECK(deferred.boundingRect().left() <= 0.8);
    CHECK(deferred.boundingRect().right() >= 2.2);
    CHECK(deferred.boundingRect().top() <= 2.5);
    CHECK(deferred.boundingRect().bottom() >= 4.5);
    deferred.clear();
    CHECK(!deferred.isVisible());
    CHECK(deferred.boundingRect().isNull());

    pyqtgraph::graphicsItems::ErrorBarItemOptions asymmetricVerticalOptions;
    asymmetricVerticalOptions.x = {10.0};
    asymmetricVerticalOptions.y = {20.0};
    asymmetricVerticalOptions.top = {3.0};
    asymmetricVerticalOptions.bottom = {1.0};
    asymmetricVerticalOptions.beam = 0.0;
    pyqtgraph::graphicsItems::ErrorBarItem asymmetricVertical(asymmetricVerticalOptions);
    const QRectF asymmetricPathBounds = asymmetricVertical.path().boundingRect();
    CHECK(std::abs(asymmetricPathBounds.top() - 19.0) < 1.0e-9);
    CHECK(std::abs(asymmetricPathBounds.bottom() - 23.0) < 1.0e-9);

    pyqtgraph::graphicsItems::ErrorBarItemOptions verticalOnlyOptions;
    verticalOnlyOptions.x = {1.0};
    verticalOnlyOptions.y = {2.0};
    verticalOnlyOptions.top = {1.0};
    verticalOnlyOptions.bottom = {1.0};
    verticalOnlyOptions.pen = QPen(Qt::white, 6.0);
    pyqtgraph::graphicsItems::ErrorBarItem verticalOnly(verticalOnlyOptions);
    CHECK(verticalOnly.path().boundingRect().width() == 0.0);
    CHECK(verticalOnly.boundingRect().width() >= 6.0);
    verticalOnly.setPen(QPen(Qt::white, 10.0));
    CHECK(verticalOnly.boundingRect().width() >= 10.0);

    pyqtgraph::graphicsItems::BarGraphItem bars;
    const std::vector<double> heights{2.0, -3.0};
    bars.setData(x, heights, -0.5);
    const auto [xMin, xMax] = bars.dataBounds(0);
    const auto [yMin, yMax] = bars.dataBounds(1);
    CHECK(xMin <= 0.75);
    CHECK(xMax >= 2.25);
    CHECK(yMin <= -3.0);
    CHECK(yMax >= 2.0);
    CHECK(!bars.shape().isEmpty());

    pyqtgraph::graphicsItems::BarGraphItemOptions explicitCoords;
    explicitCoords.x0 = {20.0};
    explicitCoords.x1 = {21.0};
    explicitCoords.height = {1.0};
    bars.setOpts(explicitCoords);
    bars.setData(x, heights, 0.5);
    const auto [resetXMin, resetXMax] = bars.dataBounds(0);
    CHECK(resetXMin < 2.0);
    CHECK(resetXMax < 3.0);

    const std::vector<QPen> twoPens{cosmeticPen(Qt::red), cosmeticPen(Qt::green)};
    const std::vector<QBrush> twoBrushes{QBrush(Qt::red), QBrush(Qt::green)};
    bars.setPens(twoPens);
    bars.setBrushes(twoBrushes);
    const std::vector<double> expandedX{1.0, 2.0, 3.0};
    const std::vector<double> expandedHeights{1.0, 2.0, 3.0};
    bars.setData(expandedX, expandedHeights, 0.5);
    bool paintAfterStyleMismatchOk = true;
    try {
        QImage styleProbe(64, 64, QImage::Format_ARGB32_Premultiplied);
        styleProbe.fill(Qt::black);
        QPainter stylePainter(&styleProbe);
        bars.paint(&stylePainter, nullptr, nullptr);
        stylePainter.end();
    } catch (...) {
        paintAfterStyleMismatchOk = false;
    }
    CHECK(paintAfterStyleMismatchOk);

    pyqtgraph::graphicsItems::PlotCurveItem lowerStep;
    pyqtgraph::graphicsItems::PlotCurveItem upperStep;
    const std::vector<double> stepX{0.0, 1.0, 2.0};
    const std::vector<double> stepLower{0.0, 1.0, 0.0};
    const std::vector<double> stepUpper{2.0, 3.0, 2.0};
    lowerStep.setStepMode(pyqtgraph::graphicsItems::PlotCurveItem::StepMode::Right);
    upperStep.setStepMode(pyqtgraph::graphicsItems::PlotCurveItem::StepMode::Right);
    lowerStep.setData(stepX, stepLower);
    upperStep.setData(stepX, stepUpper);
    pyqtgraph::graphicsItems::FillBetweenItem stepFill(&lowerStep, &upperStep, QBrush(Qt::blue));
    CHECK(stepFill.zValue() < lowerStep.zValue());
    {
        pyqtgraph::graphicsItems::ViewBox viewBox;
        viewBox.addItem(&stepFill);
        CHECK(stepFill.parentItem() != nullptr);
        CHECK(stepFill.zValue() < lowerStep.zValue());
        viewBox.removeItem(&stepFill);
    }
    const QPainterPath stepPath = stepFill.path();
    CHECK(stepPath.elementCount() >= 4);
    CHECK(std::abs(stepPath.elementAt(1).x - 1.0) < 1.0e-9);
    CHECK(std::abs(stepPath.elementAt(1).y - 0.0) < 1.0e-9);

    pyqtgraph::graphicsItems::PlotCurveItem singleLowerStep;
    pyqtgraph::graphicsItems::PlotCurveItem singleUpperStep;
    singleLowerStep.setStepMode(pyqtgraph::graphicsItems::PlotCurveItem::StepMode::Center);
    singleUpperStep.setStepMode(pyqtgraph::graphicsItems::PlotCurveItem::StepMode::Center);
    singleLowerStep.setConnectMode(pyqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Finite);
    singleUpperStep.setConnectMode(pyqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Finite);
    const std::vector<double> centerStepX{0.0, 1.0, 2.0, 3.0};
    singleLowerStep.setData(centerStepX, std::vector<double>{std::nan(""), 0.0, std::nan("")});
    singleUpperStep.setData(centerStepX, std::vector<double>{std::nan(""), 2.0, std::nan("")});
    pyqtgraph::graphicsItems::FillBetweenItem singleStepFill(&singleLowerStep, &singleUpperStep, QBrush(Qt::blue));
    const QRectF singleStepBounds = singleStepFill.path().boundingRect();
    CHECK(!singleStepFill.path().isEmpty());
    CHECK(singleStepBounds.left() <= 1.0);
    CHECK(singleStepBounds.right() >= 2.0);
    CHECK(singleStepBounds.height() >= 2.0);

    const QRectF oldFillPathBounds = stepFill.path().boundingRect();
    const QRectF oldFillBounds = stepFill.boundingRect();
    const std::vector<double> movedLower{-4.0, -4.0, -4.0};
    lowerStep.setData(stepX, movedLower);
    CHECK(static_cast<const QGraphicsPathItem&>(stepFill).path().boundingRect() == oldFillPathBounds);
    QThread::msleep(20);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    const QRectF scheduledFillPathBounds = static_cast<const QGraphicsPathItem&>(stepFill).path().boundingRect();
    CHECK(scheduledFillPathBounds.top() < oldFillPathBounds.top() - 1.0);
    const QRectF refreshedFillPathBounds = stepFill.path().boundingRect();
    const QRectF refreshedFillBounds = stepFill.boundingRect();
    CHECK(refreshedFillPathBounds.top() < oldFillPathBounds.top() - 1.0);
    CHECK(refreshedFillBounds.top() < oldFillBounds.top() - 1.0);

    auto moveToCount = [](const QPainterPath& path) {
        int count = 0;
        for (int index = 0; index < path.elementCount(); ++index) {
            if (path.elementAt(index).type == QPainterPath::MoveToElement) {
                ++count;
            }
        }
        return count;
    };

    pyqtgraph::graphicsItems::PlotCurveItem lowerFinite;
    pyqtgraph::graphicsItems::PlotCurveItem upperFinite;
    lowerFinite.setConnectMode(pyqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Finite);
    upperFinite.setConnectMode(pyqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Finite);
    const double nan = std::nan("");
    const std::vector<double> gapX{0.0, 1.0, 2.0, 3.0, 4.0};
    lowerFinite.setData(gapX, std::vector<double>{0.0, 1.0, nan, 1.0, 0.0});
    upperFinite.setData(gapX, std::vector<double>{2.0, 3.0, nan, 3.0, 2.0});
    pyqtgraph::graphicsItems::FillBetweenItem finiteFill(&lowerFinite, &upperFinite, QBrush(Qt::blue));
    CHECK(moveToCount(finiteFill.path()) >= 2);

    pyqtgraph::graphicsItems::PlotCurveItem lowerPairs;
    pyqtgraph::graphicsItems::PlotCurveItem upperPairs;
    lowerPairs.setConnectMode(pyqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Pairs);
    upperPairs.setConnectMode(pyqtgraph::graphicsItems::PlotCurveItem::ConnectMode::Pairs);
    const std::vector<double> pairX{0.0, 1.0, 2.0, 3.0};
    lowerPairs.setData(pairX, std::vector<double>{0.0, 1.0, 0.0, 1.0});
    upperPairs.setData(pairX, std::vector<double>{2.0, 3.0, 2.0, 3.0});
    pyqtgraph::graphicsItems::FillBetweenItem pairsFill(&lowerPairs, &upperPairs, QBrush(Qt::blue));
    CHECK(moveToCount(pairsFill.path()) >= 2);

    pyqtgraph::graphicsItems::PlotCurveItem shorterLower;
    pyqtgraph::graphicsItems::PlotCurveItem longerUpper;
    shorterLower.setData(std::vector<double>{0.0, 1.0, 2.0}, std::vector<double>{0.0, 0.0, 0.0});
    longerUpper.setData(std::vector<double>{0.0, 1.0, 2.0, 3.0}, std::vector<double>{1.0, 1.0, 1.0, 1.0});
    pyqtgraph::graphicsItems::FillBetweenItem unequalFill(&shorterLower, &longerUpper, QBrush(Qt::blue));
    CHECK(unequalFill.path().boundingRect().right() > 2.9);

    auto transientLower = std::make_unique<pyqtgraph::graphicsItems::PlotCurveItem>();
    auto transientUpper = std::make_unique<pyqtgraph::graphicsItems::PlotCurveItem>();
    transientLower->setData(std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 0.0});
    transientUpper->setData(std::vector<double>{0.0, 1.0}, std::vector<double>{1.0, 1.0});
    pyqtgraph::graphicsItems::FillBetweenItem guardedFill(transientLower.get(), transientUpper.get(), QBrush(Qt::blue));
    CHECK(!guardedFill.path().isEmpty());
    transientLower.reset();
    CHECK(guardedFill.curve1() == nullptr);
    CHECK(guardedFill.path().isEmpty());
    CHECK(guardedFill.boundingRect().isEmpty());
    transientUpper.reset();
    CHECK(guardedFill.curve2() == nullptr);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testDataGuardsAndBounds()) {
        return 1;
    }

    const QImage reference = renderReference();
    const QImage actual = renderActual();
    const std::uint64_t referenceSemanticPixels = semanticPixelCount(reference);
    const std::uint64_t actualSemanticPixels = semanticPixelCount(actual);
    if (referenceSemanticPixels < 1200 || actualSemanticPixels < 1200) {
        std::cerr << "blank/placeholder guard failed: reference=" << referenceSemanticPixels << " actual=" << actualSemanticPixels
                  << '\n';
        return 1;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    if (!writeArtifacts(reference, actual, diff, metrics)) {
        return 1;
    }
    if (!metrics.passed) {
        std::cerr << "P4.02 visual comparison failed: changedPixels=" << metrics.changedPixels
                  << " maxDelta=" << metrics.maxDelta << '\n';
        return 1;
    }
    return 0;
}
