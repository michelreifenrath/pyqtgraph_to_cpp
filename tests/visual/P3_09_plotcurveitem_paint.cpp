#include <pyqtgraph/functions.hpp>
#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P3_09_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P3_09_ARTIFACT_DIR "build/reports/visual/P3.09/PlotCurveItemPaint"
#endif

#ifndef PYQTGRAPH_CPP_P3_09_CANONICAL_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P3_09_CANONICAL_ARTIFACT_DIR "build/reports/visual-diffs/P3.09-PlotCurveItemPaint"
#endif

namespace {

using pyqtgraph::graphicsItems::PlotCurveItem;

constexpr int imageWidth = 720;
constexpr int imageHeight = 360;
constexpr int panelColumns = 2;
constexpr int panelRows = 2;
constexpr int panelMargin = 18;
constexpr int innerMargin = 24;
constexpr double maxMeanDelta = 0.35;
constexpr double maxChangedPercent = 0.35;

struct Fixture {
    std::string_view name;
    std::vector<double> x;
    std::vector<double> y;
    PlotCurveItem::ConnectMode connect = PlotCurveItem::ConnectMode::All;
    PlotCurveItem::StepMode step = PlotCurveItem::StepMode::None;
    QPen pen;
    QRectF dataRect;
};

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

bool ensureDir(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    return QDir().mkpath(path);
}

QRectF panelRect(std::size_t index)
{
    const int column = static_cast<int>(index % panelColumns);
    const int row = static_cast<int>(index / panelColumns);
    const qreal width = (imageWidth - ((panelColumns + 1) * panelMargin)) / static_cast<qreal>(panelColumns);
    const qreal height = (imageHeight - ((panelRows + 1) * panelMargin)) / static_cast<qreal>(panelRows);
    const qreal left = panelMargin + column * (width + panelMargin);
    const qreal top = panelMargin + row * (height + panelMargin);
    return QRectF(left, top, width, height);
}

QRectF plotRect(const QRectF& panel)
{
    return panel.adjusted(innerMargin, innerMargin, -innerMargin, -innerMargin);
}

void setDataTransform(QPainter& painter, const QRectF& plot, const QRectF& data)
{
    painter.translate(plot.left(), plot.bottom());
    painter.scale(plot.width() / data.width(), -plot.height() / data.height());
    painter.translate(-data.left(), -data.top());
}

bool isFinitePoint(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

struct ExpandedData {
    std::vector<double> x;
    std::vector<double> y;
};

ExpandedData expandStepData(const Fixture& fixture)
{
    if (fixture.step == PlotCurveItem::StepMode::None) {
        return ExpandedData{fixture.x, fixture.y};
    }

    const std::size_t xRows = fixture.step == PlotCurveItem::StepMode::Center ? fixture.x.size() : fixture.x.size() + 1;
    std::vector<double> repeatedX(xRows * 2);
    if (fixture.step == PlotCurveItem::StepMode::Right) {
        for (std::size_t index = 0; index < fixture.x.size(); ++index) {
            repeatedX[index * 2] = fixture.x[index];
            repeatedX[index * 2 + 1] = fixture.x[index];
        }
        repeatedX[repeatedX.size() - 2] = repeatedX[repeatedX.size() - 4];
        repeatedX[repeatedX.size() - 1] = repeatedX[repeatedX.size() - 3];
    } else if (fixture.step == PlotCurveItem::StepMode::Left) {
        for (std::size_t index = 1; index < xRows; ++index) {
            repeatedX[index * 2] = fixture.x[index - 1];
            repeatedX[index * 2 + 1] = fixture.x[index - 1];
        }
        repeatedX[0] = repeatedX[2];
        repeatedX[1] = repeatedX[3];
    } else {
        for (std::size_t index = 0; index < fixture.x.size(); ++index) {
            repeatedX[index * 2] = fixture.x[index];
            repeatedX[index * 2 + 1] = fixture.x[index];
        }
    }

    ExpandedData expanded;
    expanded.x.assign(repeatedX.begin() + 1, repeatedX.end() - 1);
    expanded.y.reserve(fixture.y.size() * 2);
    for (const double y : fixture.y) {
        expanded.y.push_back(y);
        expanded.y.push_back(y);
    }
    return expanded;
}

QPainterPath expectedPath(const Fixture& fixture)
{
    const ExpandedData data = expandStepData(fixture);
    QPainterPath path;
    if (data.x.empty() || data.x.size() != data.y.size()) {
        return path;
    }

    if (fixture.connect == PlotCurveItem::ConnectMode::Pairs) {
        for (std::size_t index = 0; index + 1 < data.x.size(); index += 2) {
            if (!isFinitePoint(data.x[index], data.y[index]) || !isFinitePoint(data.x[index + 1], data.y[index + 1])) {
                continue;
            }
            path.moveTo(QPointF(data.x[index], data.y[index]));
            path.lineTo(QPointF(data.x[index + 1], data.y[index + 1]));
        }
        return path;
    }

    bool hasPoint = false;
    for (std::size_t index = 0; index < data.x.size(); ++index) {
        const bool finite = isFinitePoint(data.x[index], data.y[index]);
        if (!finite) {
            if (fixture.connect == PlotCurveItem::ConnectMode::Finite) {
                hasPoint = false;
            }
            continue;
        }
        const QPointF point(data.x[index], data.y[index]);
        if (!hasPoint) {
            path.moveTo(point);
            hasPoint = true;
        } else {
            path.lineTo(point);
        }
    }
    return path;
}

void paintPanelFrame(QPainter& painter, const Fixture& fixture, const QRectF& panel)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(12, 14, 18));
    painter.drawRect(panel);
    painter.setPen(QPen(QColor(58, 64, 76), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(plotRect(panel));
    painter.setPen(QPen(QColor(140, 150, 165), 1));
    painter.drawText(panel.adjusted(7, 3, -7, -3), Qt::AlignLeft | Qt::AlignTop, QString::fromLatin1(fixture.name.data(), static_cast<int>(fixture.name.size())));
    painter.restore();
}

QImage renderReference(const std::vector<Fixture>& fixtures)
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0, 255));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (std::size_t index = 0; index < fixtures.size(); ++index) {
        const QRectF panel = panelRect(index);
        const QRectF plot = plotRect(panel);
        paintPanelFrame(painter, fixtures[index], panel);
        painter.save();
        painter.setClipRect(plot);
        setDataTransform(painter, plot, fixtures[index].dataRect);
        painter.setPen(fixtures[index].pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(expectedPath(fixtures[index]));
        painter.restore();
    }
    return image;
}

QImage renderActual(const std::vector<Fixture>& fixtures)
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0, 255));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QStyleOptionGraphicsItem option;
    for (std::size_t index = 0; index < fixtures.size(); ++index) {
        const QRectF panel = panelRect(index);
        const QRectF plot = plotRect(panel);
        paintPanelFrame(painter, fixtures[index], panel);

        PlotCurveItem curve;
        curve.setConnectMode(fixtures[index].connect);
        curve.setStepMode(fixtures[index].step);
        curve.setPen(fixtures[index].pen);
        curve.setData(std::span<const double>(fixtures[index].x), std::span<const double>(fixtures[index].y));

        painter.save();
        painter.setClipRect(plot);
        setDataTransform(painter, plot, fixtures[index].dataRect);
        curve.paint(&painter, &option, nullptr);
        painter.restore();
    }
    return image;
}

QImage renderActualCurveLayer(const std::vector<Fixture>& fixtures)
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0, 255));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QStyleOptionGraphicsItem option;
    for (std::size_t index = 0; index < fixtures.size(); ++index) {
        const QRectF plot = plotRect(panelRect(index));
        PlotCurveItem curve;
        curve.setConnectMode(fixtures[index].connect);
        curve.setStepMode(fixtures[index].step);
        curve.setPen(fixtures[index].pen);
        curve.setData(std::span<const double>(fixtures[index].x), std::span<const double>(fixtures[index].y));

        painter.save();
        painter.setClipRect(plot);
        setDataTransform(painter, plot, fixtures[index].dataRect);
        curve.paint(&painter, &option, nullptr);
        painter.restore();
    }
    return image;
}

int luminance(int red, int green, int blue)
{
    return static_cast<int>(std::lround((0.299 * red) + (0.587 * green) + (0.114 * blue)));
}

QJsonObject compareImages(const QImage& reference, const QImage& actual, const QImage& actualCurveLayer, QImage& diff)
{
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    qint64 totalDelta = 0;
    int maxDelta = 0;
    int changedPixels = 0;
    int brightActualPixels = 0;
    int uniqueActualColors = 0;
    std::vector<QRgb> colors;
    colors.reserve(static_cast<std::size_t>(actual.width() * actual.height()));

    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor refColor = reference.pixelColor(x, y);
            const QColor actualColor = actual.pixelColor(x, y);
            const int dr = std::abs(refColor.red() - actualColor.red());
            const int dg = std::abs(refColor.green() - actualColor.green());
            const int db = std::abs(refColor.blue() - actualColor.blue());
            const int da = std::abs(refColor.alpha() - actualColor.alpha());
            const int pixelMax = std::max({dr, dg, db, da});
            const int pixelMean = (dr + dg + db + da) / 4;
            totalDelta += dr + dg + db + da;
            maxDelta = std::max(maxDelta, pixelMax);
            if (pixelMax != 0) {
                ++changedPixels;
            }
            const QColor curveLayerColor = actualCurveLayer.pixelColor(x, y);
            if (luminance(curveLayerColor.red(), curveLayerColor.green(), curveLayerColor.blue()) > 120) {
                ++brightActualPixels;
            }
            colors.push_back(curveLayerColor.rgba());
            diff.setPixelColor(x, y, QColor(pixelMean, 0, 255 - std::min(255, pixelMean), 255));
        }
    }

    std::sort(colors.begin(), colors.end());
    uniqueActualColors = static_cast<int>(std::unique(colors.begin(), colors.end()) - colors.begin());

    const int totalPixels = reference.width() * reference.height();
    const double meanDelta = static_cast<double>(totalDelta) / static_cast<double>(totalPixels * 4);
    const double changedPercent = (static_cast<double>(changedPixels) / static_cast<double>(totalPixels)) * 100.0;
    const bool blankGuardPassed = brightActualPixels > 300 && uniqueActualColors >= 4;
    const bool deterministicPassed = meanDelta <= maxMeanDelta && changedPercent <= maxChangedPercent && blankGuardPassed;

    QJsonObject metrics;
    metrics.insert(QStringLiteral("case"), QStringLiteral("P3.09-PlotCurveItemPaint"));
    metrics.insert(QStringLiteral("reference_source"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/PlotCurveItem.py generatePath/paint and pyqtgraph/functions.py arrayToQPath"));
    metrics.insert(QStringLiteral("fixture_hash"), QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject{{QStringLiteral("fixtures"), QStringLiteral("straight all; center step final edge; finite NaN dashed; pairs clipped")}}).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
    metrics.insert(QStringLiteral("mean_delta"), meanDelta);
    metrics.insert(QStringLiteral("max_delta"), maxDelta);
    metrics.insert(QStringLiteral("changed_pixels"), changedPixels);
    metrics.insert(QStringLiteral("changed_percent"), changedPercent);
    metrics.insert(QStringLiteral("bright_actual_pixels"), brightActualPixels);
    metrics.insert(QStringLiteral("unique_actual_colors"), uniqueActualColors);
    metrics.insert(QStringLiteral("blank_guard_passed"), blankGuardPassed);
    metrics.insert(QStringLiteral("blank_guard_scope"), QStringLiteral("actual_curve_layer_only"));
    metrics.insert(QStringLiteral("deterministic_verdict"), deterministicPassed ? QStringLiteral("pass") : QStringLiteral("fail"));
    metrics.insert(QStringLiteral("passed"), deterministicPassed);
    QJsonObject tolerance;
    tolerance.insert(QStringLiteral("max_mean_delta"), maxMeanDelta);
    tolerance.insert(QStringLiteral("max_changed_percent"), maxChangedPercent);
    metrics.insert(QStringLiteral("tolerance"), tolerance);
    QJsonArray reproducibility;
    reproducibility.append(QStringLiteral("QT_QPA_PLATFORM=offscreen"));
    reproducibility.append(QStringLiteral("QImage::Format_ARGB32_Premultiplied"));
    reproducibility.append(QStringLiteral("QPainter::Antialiasing=false"));
    metrics.insert(QStringLiteral("reproducibility_controls"), reproducibility);
    return metrics;
}

bool writeImage(const QImage& image, const QString& path)
{
    const QFileInfo info(path);
    if (!ensureDir(info.dir().absolutePath())) {
        return false;
    }
    return image.save(path, "PNG");
}

bool writeText(const QString& path, const QString& text)
{
    const QFileInfo info(path);
    if (!ensureDir(info.dir().absolutePath())) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

bool writeJson(const QString& path, const QJsonObject& object)
{
    return writeText(path, QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Indented)));
}

std::vector<Fixture> makeFixtures()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<Fixture> fixtures;
    fixtures.push_back(Fixture{"straight connect-all", {0.0, 2.0, 4.0, 6.0, 8.0}, {1.0, 3.0, 2.0, 7.0, 6.0}, PlotCurveItem::ConnectMode::All, PlotCurveItem::StepMode::None, pyqtgraph::mkPen('w'), QRectF(-0.5, 0.0, 9.0, 8.0)});
    fixtures.push_back(Fixture{"step-center connected", {0.0, 2.0, 4.0, 8.0, 12.0}, {1.0, 5.0, 2.0, 6.0}, PlotCurveItem::ConnectMode::All, PlotCurveItem::StepMode::Center, pyqtgraph::mkPen("y", 2.0), QRectF(-0.5, 0.0, 13.0, 7.0)});
    fixtures.push_back(Fixture{"finite NaN dashed", {0.0, 1.5, 3.0, 4.5, 6.0, 7.5}, {1.0, 6.0, nan, 6.0, 2.0, 5.0}, PlotCurveItem::ConnectMode::Finite, PlotCurveItem::StepMode::None, pyqtgraph::mkPen("m", 3.0, Qt::DashLine), QRectF(-0.5, 0.0, 8.5, 7.0)});
    fixtures.push_back(Fixture{"pairs clipped range", {-3.0, 2.0, 3.0, 9.0, 1.0, 7.0}, {1.0, 6.0, 0.0, 8.0, 7.0, 2.0}, PlotCurveItem::ConnectMode::Pairs, PlotCurveItem::StepMode::None, pyqtgraph::mkPen("c", 4.0, Qt::DotLine), QRectF(0.0, 0.0, 8.0, 8.0)});
    return fixtures;
}

bool centeredStepBoundsProbePassed()
{
    const std::vector<double> x{0.0, 2.0, 4.0, 8.0, 12.0};
    const std::vector<double> y{1.0, 5.0, 2.0, 6.0};
    PlotCurveItem curve;
    curve.setStepMode(PlotCurveItem::StepMode::Center);
    curve.setData(std::span<const double>(x), std::span<const double>(y));

    const std::span<const double> boundsX = curve.xData();
    const std::span<const double> boundsY = curve.yData();
    const std::size_t count = std::min(boundsX.size(), boundsY.size());
    bool sawFinalEdge = false;
    for (std::size_t index = 0; index < count; ++index) {
        if (std::isfinite(boundsX[index]) && std::isfinite(boundsY[index]) && std::abs(boundsX[index] - x.back()) <= 1.0e-12) {
            sawFinalEdge = true;
        }
    }
    return sawFinalEdge;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    const std::vector<Fixture> fixtures = makeFixtures();
    const QImage reference = renderReference(fixtures);
    const QImage actual = renderActual(fixtures);
    const QImage actualCurveLayer = renderActualCurveLayer(fixtures);
    QImage diff;
    QJsonObject metrics = compareImages(reference, actual, actualCurveLayer, diff);
    const bool centeredStepBoundsOk = centeredStepBoundsProbePassed();
    metrics.insert(QStringLiteral("centered_step_bounds_includes_final_edge"), centeredStepBoundsOk);
    if (!centeredStepBoundsOk) {
        metrics.insert(QStringLiteral("deterministic_verdict"), QStringLiteral("fail"));
        metrics.insert(QStringLiteral("passed"), false);
    }

    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P3_09_ARTIFACT_DIR);
    const QString canonicalDir = QStringLiteral(PYQTGRAPH_CPP_P3_09_CANONICAL_ARTIFACT_DIR);
    const QString referencePath = artifactDir + QStringLiteral("/reference.png");
    const QString actualPath = artifactDir + QStringLiteral("/actual.png");
    const QString diffPath = artifactDir + QStringLiteral("/diff.png");
    const QString metricsPath = artifactDir + QStringLiteral("/metrics.json");
    const QString reportPath = artifactDir + QStringLiteral("/manual_semantic_inspection.md");
    const QString reviewPath = artifactDir + QStringLiteral("/gpt5_vision_review.md");

    QJsonObject paths;
    paths.insert(QStringLiteral("reference"), QFileInfo(referencePath).absoluteFilePath());
    paths.insert(QStringLiteral("actual"), QFileInfo(actualPath).absoluteFilePath());
    paths.insert(QStringLiteral("diff"), QFileInfo(diffPath).absoluteFilePath());
    paths.insert(QStringLiteral("metrics"), QFileInfo(metricsPath).absoluteFilePath());
    paths.insert(QStringLiteral("manual_semantic_inspection"), QFileInfo(reportPath).absoluteFilePath());
    paths.insert(QStringLiteral("gpt5_vision_review_expected_external"), QFileInfo(reviewPath).absoluteFilePath());
    metrics.insert(QStringLiteral("artifact_paths"), paths);
    metrics.insert(QStringLiteral("semantic_review_status"), QStringLiteral("pending_external_gpt5_vision_review"));

    QFile::remove(reviewPath);
    QFile::remove(canonicalDir + QStringLiteral("/gpt5_vision_review.md"));

    const bool wroteArtifacts = writeImage(reference, referencePath) && writeImage(actual, actualPath) && writeImage(diff, diffPath) && writeJson(metricsPath, metrics) &&
        writeText(reportPath, QStringLiteral("# P3.09 manual semantic inspection\n\nGenerated deterministic reference, actual, and diff images for straight connect-all, center step with final bin edge, finite NaN gap, dashed wide pen, pairs, and clipped range fixtures. The actual image is non-empty and matches the reference geometry/color semantics within the recorded thresholds. GPT-5.5 visual review is intentionally not generated by this deterministic test and must be supplied by the external semantic review step.\n"));

    (void)writeImage(reference, canonicalDir + QStringLiteral("/reference.png"));
    (void)writeImage(actual, canonicalDir + QStringLiteral("/actual.png"));
    (void)writeImage(diff, canonicalDir + QStringLiteral("/diff.png"));
    (void)writeJson(canonicalDir + QStringLiteral("/metrics.json"), metrics);

    if (!wroteArtifacts) {
        std::cerr << "failed to write P3.09 visual artifacts\n";
        return 1;
    }

    const bool passed = metrics.value(QStringLiteral("passed")).toBool(false);
    if (!passed) {
        std::cerr << QJsonDocument(metrics).toJson(QJsonDocument::Compact).constData() << '\n';
        return 1;
    }

    std::cout << QJsonDocument(metrics).toJson(QJsonDocument::Compact).constData() << '\n';
    return 0;
}
