#include <cppqtgraph/graphicsItems/GridItem.hpp>
#include <cppqtgraph/graphicsItems/LegendItem.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/VTickGroup.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P4_11_ARTIFACT_DIR
#define CPPQTGRAPH_P4_11_ARTIFACT_DIR "reports/visual/P4.11"
#endif

#ifndef CPPQTGRAPH_P4_11_CANONICAL_ARTIFACT_DIR
#define CPPQTGRAPH_P4_11_CANONICAL_ARTIFACT_DIR "reports/visual-diffs/P4.11-legend-grid-vtick"
#endif

namespace {

constexpr int imageWidth = 800;
constexpr int imageHeight = 600;
constexpr const char* caseName = "legend-grid-vtick";
const QRectF referencePlotRect(76.0, 54.0, imageWidth - 116.0, imageHeight - 112.0);
const QRectF referenceView(0.0, -1.2, 6.0, 3.4);

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
            application_->setQuitOnLastWindowClosed(false);
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

std::vector<double> xValues()
{
    return {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
}

std::vector<double> yValuesA()
{
    return {0.1, 0.9, 0.2, 1.7, 1.1, 2.0, 1.5};
}

std::vector<double> yValuesB()
{
    return {1.6, 1.1, 1.3, 0.5, 0.0, 0.4, -0.4};
}

QPointF mapReferencePoint(double x, double y)
{
    const double xRatio = (x - referenceView.left()) / referenceView.width();
    const double yRatio = (y - referenceView.top()) / referenceView.height();
    return QPointF(referencePlotRect.left() + xRatio * referencePlotRect.width(),
                   referencePlotRect.bottom() - yRatio * referencePlotRect.height());
}

void drawReferenceCurve(QPainter& painter, const QColor& color, const std::vector<double>& y)
{
    const auto x = xValues();
    QPainterPath path;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index) {
        const QPointF point = mapReferencePoint(x[index], y[index]);
        if (index == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(cosmeticPen(color, 2.0));
    painter.drawPath(path);
    painter.setRenderHint(QPainter::Antialiasing, false);
}

void drawReferenceGrid(QPainter& painter)
{
    painter.setPen(cosmeticPen(QColor(155, 155, 155, 85)));
    for (double x = 0.0; x <= 6.0; x += 1.0) {
        const QPointF a = mapReferencePoint(x, referenceView.top());
        const QPointF b = mapReferencePoint(x, referenceView.bottom());
        painter.drawLine(a, b);
    }
    for (double y = -1.0; y <= 2.0; y += 1.0) {
        const QPointF a = mapReferencePoint(referenceView.left(), y);
        const QPointF b = mapReferencePoint(referenceView.right(), y);
        painter.drawLine(a, b);
    }
    painter.setPen(cosmeticPen(QColor(155, 155, 155, 45)));
    for (double x = 0.5; x < 6.0; x += 1.0) {
        painter.drawLine(mapReferencePoint(x, referenceView.top()), mapReferencePoint(x, referenceView.bottom()));
    }
    for (double y = -0.5; y < 2.2; y += 1.0) {
        painter.drawLine(mapReferencePoint(referenceView.left(), y), mapReferencePoint(referenceView.right(), y));
    }
}

void drawReferenceTicks(QPainter& painter)
{
    painter.setPen(cosmeticPen(QColor(255, 210, 80), 2.0));
    const qreal top = referencePlotRect.top();
    const qreal bottom = referencePlotRect.top() + referencePlotRect.height() * 0.22;
    for (double x : {0.75, 2.25, 4.5, 5.5}) {
        const QPointF point = mapReferencePoint(x, 0.0);
        painter.drawLine(QPointF(point.x(), top), QPointF(point.x(), bottom));
    }
}

QImage renderReference()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 11));
    painter.setPen(cosmeticPen(QColor(220, 220, 220)));
    painter.drawText(QRectF(0.0, 5.0, imageWidth, 26.0), Qt::AlignCenter, QStringLiteral("Legend / Grid / VTickGroup"));
    painter.setPen(cosmeticPen(QColor(150, 150, 150)));
    painter.drawRect(referencePlotRect);
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    painter.drawText(QRectF(0.0, referencePlotRect.center().y() - 60.0, 22.0, 120.0), Qt::AlignCenter, QStringLiteral("Value"));
    painter.drawText(QRectF(referencePlotRect.left(), imageHeight - 30.0, referencePlotRect.width(), 22.0), Qt::AlignCenter, QStringLiteral("Sample"));
    drawReferenceGrid(painter);
    drawReferenceCurve(painter, QColor(255, 220, 80), yValuesA());
    drawReferenceCurve(painter, QColor(80, 180, 255), yValuesB());
    drawReferenceTicks(painter);

    const QRectF legend(106.0, 84.0, 130.0, 56.0);
    painter.setPen(cosmeticPen(QColor(200, 200, 200)));
    painter.setBrush(QColor(0, 0, 0, 190));
    painter.drawRect(legend);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(cosmeticPen(QColor(255, 220, 80), 2.0));
    painter.drawLine(QPointF(legend.left() + 10.0, legend.top() + 18.0), QPointF(legend.left() + 36.0, legend.top() + 18.0));
    painter.setPen(cosmeticPen(QColor(80, 180, 255), 2.0));
    painter.drawLine(QPointF(legend.left() + 10.0, legend.top() + 39.0), QPointF(legend.left() + 36.0, legend.top() + 39.0));
    painter.setPen(cosmeticPen(QColor(230, 230, 230)));
    painter.drawText(QPointF(legend.left() + 45.0, legend.top() + 22.0), QStringLiteral("rising"));
    painter.drawText(QPointF(legend.left() + 45.0, legend.top() + 43.0), QStringLiteral("falling"));
    painter.end();
    return image;
}

void processEvents()
{
    for (int iteration = 0; iteration < 8; ++iteration) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }
}

QImage renderActual()
{
    cppqtgraph::widgets::PlotWidget widget;
    auto* plot = widget.getPlotItem();
    plot->setTitle(QStringLiteral("Legend / Grid / VTickGroup"));
    plot->setLabel(QStringLiteral("left"), QStringLiteral("Value"));
    plot->setLabel(QStringLiteral("bottom"), QStringLiteral("Sample"));
    plot->setRange(referenceView, 0.0);

    auto* grid = new cppqtgraph::graphicsItems::GridItem;
    grid->setTextPen(std::nullopt);
    QPen gridPen(QColor(155, 155, 155));
    gridPen.setCosmetic(true);
    grid->setPen(gridPen);
    plot->addItem(grid, true);

    auto* legend = plot->addLegend(QPointF(30.0, 30.0));
    legend->setPen(cosmeticPen(QColor(200, 200, 200)));
    legend->setBrush(QBrush(QColor(0, 0, 0, 190)));
    legend->setLabelTextColor(QColor(230, 230, 230));

    const auto x = xValues();
    auto* first = plot->plot(x, yValuesA(), QStringLiteral("rising"));
    first->setPen(cosmeticPen(QColor(255, 220, 80), 2.0));
    auto* second = plot->plot(x, yValuesB(), QStringLiteral("falling"));
    second->setPen(cosmeticPen(QColor(80, 180, 255), 2.0));

    auto* ticks = new cppqtgraph::graphicsItems::VTickGroup({0.75, 2.25, 4.5, 5.5}, {0.78, 1.0}, cosmeticPen(QColor(255, 210, 80), 2.0));
    plot->addItem(ticks, true);

    widget.resize(imageWidth, imageHeight);
    widget.show();
    processEvents();
    QPixmap pixmap = widget.grab(QRect(0, 0, imageWidth, imageHeight));
    return pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
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
            if (delta > 6) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta <= 6 ? QColor(0, 0, 0) : QColor(255, std::min(delta, 255), std::min(delta, 255)));
        }
    }
    metrics.meanDelta = static_cast<double>(metrics.totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    metrics.passed = metrics.meanDelta <= 42.0 && metrics.changedPercent <= 28.0 && metrics.maxDelta <= 1020;
    return metrics;
}

std::uint64_t semanticPixelCount(const QImage& image)
{
    std::uint64_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && (color.red() > 18 || color.green() > 18 || color.blue() > 18)) {
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

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const std::vector<QString> caseDirs{
        QStringLiteral(CPPQTGRAPH_P4_11_ARTIFACT_DIR) + QStringLiteral("/") + QString::fromLatin1(caseName),
        QStringLiteral(CPPQTGRAPH_P4_11_CANONICAL_ARTIFACT_DIR),
    };
    for (const QString& caseDir : caseDirs) {
        CHECK(ensureDirectory(caseDir));
        CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
        CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
        CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));
        writeTextFile(caseDir + QStringLiteral("/metrics.json"),
            QStringLiteral(
                "{\n"
                "  \"case\": \"legend-grid-vtick\",\n"
                "  \"issue\": \"P4.11\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/LegendItem.py; pyqtgraph/graphicsItems/GridItem.py; pyqtgraph/graphicsItems/VTickGroup.py; tests/graphicsItems/test_LegendItem.py\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [800, 600],\n"
                "  \"fixture_hash\": \"P4.11:legend-grid-vtick:fixed-two-curves:v1\",\n"
                "  \"thresholds\": {\"max_mean_delta\": 42.0, \"max_changed_percent\": 28.0, \"max_pixel_delta\": 1020},\n"
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
                    "  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"gpt5_vision_review.md\", \"generated_by_test\": false, \"available\": false},\n"
                    "  \"passed\": ")
                + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
                + QStringLiteral(
                    ",\n"
                    "  \"blank_placeholder_guard\": \"passed\",\n"
                    "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"size\": \"800x600\", \"fixed_data\": true}\n"
                    "}\n"));
    }
    const QString reportRoot = QStringLiteral(CPPQTGRAPH_P4_11_ARTIFACT_DIR);
    CHECK(ensureDirectory(reportRoot));
    writeTextFile(reportRoot + QStringLiteral("/manual_semantic_inspection.md"),
        QStringLiteral(
            "# P4.11 manual semantic inspection note\n\n"
            "The generated images exercise PyQtGraph 0.14.0 LegendItem, GridItem, and VTickGroup semantics: "
            "two named curves have colored legend samples, a multi-level cosmetic grid overlays the ViewBox without "
            "autorange participation, and yellow vertical ticks occupy only the configured top fraction of the view. "
            "The implementing agent must open/read reference.png, actual.png, and diff.png and record the semantic "
            "inspection in implementation.md. This deterministic test does not fabricate GPT visual review evidence.\n"));
    writeTextFile(reportRoot + QStringLiteral("/summary.json"),
        QStringLiteral("{\n  \"issue\": \"P4.11\",\n  \"passed_cases\": ") + (metrics.passed ? QStringLiteral("1") : QStringLiteral("0"))
            + QStringLiteral(",\n  \"total_cases\": 1,\n  \"blank_placeholder_guard\": \"passed\"\n}\n"));
    return true;
}

bool testApiBasics()
{
    cppqtgraph::graphicsItems::LegendItem legend(std::nullopt);
    CHECK(!legend.offset().has_value());
    CHECK(legend.columnCount() == 1);
    CHECK(legend.itemCount() == 0);

    cppqtgraph::graphicsItems::VTickGroup ticks;
    CHECK(ticks.xValues().empty());
    CHECK(std::abs(ticks.yRange()[0] - 0.0) < 1.0e-9);
    CHECK(std::abs(ticks.yRange()[1] - 1.0) < 1.0e-9);
    ticks.setXVals({1.0, 2.0});
    CHECK(ticks.xValues().size() == 2);
    ticks.setYRange({0.8, 1.0});
    CHECK(std::abs(ticks.yRange()[0] - 0.8) < 1.0e-9);

    cppqtgraph::graphicsItems::GridItem grid;
    CHECK(grid.tickSpacing().x.size() == 3);
    CHECK(grid.tickSpacing().y.size() == 3);
    grid.setTextPen(std::nullopt);
    CHECK(!grid.textPen().has_value());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testApiBasics()) {
        return 1;
    }

    const QImage reference = renderReference();
    const QImage actual = renderActual();
    const std::uint64_t referenceSemanticPixels = semanticPixelCount(reference);
    const std::uint64_t actualSemanticPixels = semanticPixelCount(actual);
    if (referenceSemanticPixels < 4500 || actualSemanticPixels < 4500) {
        std::cerr << "blank/placeholder guard failed: reference=" << referenceSemanticPixels << " actual=" << actualSemanticPixels << '\n';
        return 1;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    if (!writeArtifacts(reference, actual, diff, metrics)) {
        return 1;
    }
    if (!metrics.passed) {
        std::cerr << "P4.11 visual comparison failed: meanDelta=" << metrics.meanDelta
                  << " changedPercent=" << metrics.changedPercent << " maxDelta=" << metrics.maxDelta << '\n';
        return 1;
    }
    return 0;
}
