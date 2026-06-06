#include <pyqtgraph/graphicsItems/AxisItem.hpp>
#include <pyqtgraph/graphicsItems/LegendItem.hpp>
#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <pyqtgraph/graphicsItems/PlotDataItem.hpp>
#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <span>
#include <vector>

#define PYQTGRAPH_CPP_SIMPLEPLOT_NO_MAIN
#include "../../examples/SimplePlot.cpp"

#ifndef PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR "reports/visual/P3.11"
#endif

#ifndef PYQTGRAPH_CPP_P3_11_SIMPLEPLOT_REFERENCE
#define PYQTGRAPH_CPP_P3_11_SIMPLEPLOT_REFERENCE "oracle/fixtures/screenshots/SimplePlot.reference.png"
#endif

namespace {

struct Tolerance {
    double maxMeanDelta = 0.0;
    int maxPixelDelta = 255;
    double maxChangedPercent = 100.0;
};

struct Metrics {
    bool passed = false;
    bool semantic = false;
    double meanAbsDelta = 0.0;
    int maxDelta = 0;
    double changedPercent = 0.0;
    int uniqueColors = 0;
    int darkPixels = 0;
    int brightPixels = 0;
    QStringList failedChecks;
};

bool ensureDirectory(const QString& path)
{
    const QFileInfo info(path);
    return info.dir().exists() || QDir().mkpath(info.dir().absolutePath());
}

void processEvents()
{
    for (int iteration = 0; iteration < 8; ++iteration) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }
}

QImage grabWidget(QWidget& widget, int width = 800, int height = 600)
{
    widget.resize(width, height);
    widget.show();
    processEvents();
    return widget.grab(QRect(0, 0, width, height)).toImage().convertToFormat(QImage::Format_ARGB32);
}

QImage renderSimplePlotActual()
{
    auto example = pyqtgraph::examples::createSimplePlotExample();
    return grabWidget(*example.widget);
}

QImage renderDecorationActual()
{
    auto widget = std::make_unique<pyqtgraph::widgets::PlotWidget>();
    widget->setWindowTitle(QStringLiteral("P3.11 PlotItem decorations"));
    auto* plot = widget->getPlotItem();
    plot->setLabel(QStringLiteral("left"), QStringLiteral("Amplitude"));
    plot->setLabel(QStringLiteral("bottom"), QStringLiteral("Sample"));
    plot->showAxis(QStringLiteral("top"), true);
    plot->hideAxis(QStringLiteral("right"));
    auto* legend = plot->addLegend(QPointF(30.0, 30.0));
    if (legend == nullptr) {
        return {};
    }

    auto* first = new pyqtgraph::graphicsItems::PlotCurveItem();
    QPen firstPen(QColor(255, 255, 255), 1.0);
    firstPen.setCosmetic(true);
    first->setPen(firstPen);
    const std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> y{0.0, 1.0, 0.4, 1.5, 0.8, 1.9};
    first->setData(std::span<const double>(x.data(), x.size()), std::span<const double>(y.data(), y.size()));
    plot->addItem(first, false, QStringLiteral("signal"));

    auto* second = new pyqtgraph::graphicsItems::PlotCurveItem();
    QPen secondPen(QColor(255, 220, 0), 1.0);
    secondPen.setCosmetic(true);
    second->setPen(secondPen);
    const std::vector<double> y2{1.7, 1.1, 1.6, 0.9, 1.3, 0.6};
    second->setData(std::span<const double>(x.data(), x.size()), std::span<const double>(y2.data(), y2.size()));
    plot->addItem(second, false, QStringLiteral("baseline"));
    plot->autoRange();

    return grabWidget(*widget);
}

QImage drawDecorationReference(int width = 800, int height = 600)
{
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF plotRect(78.0, 46.0, 650.0, 488.0);
    painter.setPen(QPen(QColor(210, 210, 210), 1.0));
    painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight());
    painter.drawLine(plotRect.bottomLeft(), plotRect.topLeft());
    painter.drawLine(plotRect.topLeft(), plotRect.topRight());
    painter.setPen(QColor(210, 210, 210));
    painter.drawText(QRectF(330.0, 560.0, 160.0, 24.0), Qt::AlignCenter, QStringLiteral("Sample"));
    painter.save();
    painter.translate(22.0, 330.0);
    painter.rotate(-90.0);
    painter.drawText(QRectF(0.0, 0.0, 180.0, 24.0), Qt::AlignCenter, QStringLiteral("Amplitude"));
    painter.restore();

    auto mapPoint = [&](double x, double y) {
        const double xp = plotRect.left() + (x / 5.0) * plotRect.width();
        const double yp = plotRect.bottom() - ((y + 0.1) / 2.2) * plotRect.height();
        return QPointF(xp, yp);
    };
    const std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> y{0.0, 1.0, 0.4, 1.5, 0.8, 1.9};
    const std::vector<double> y2{1.7, 1.1, 1.6, 0.9, 1.3, 0.6};
    painter.setPen(QPen(QColor(255, 255, 255), 1.0));
    for (std::size_t i = 1; i < x.size(); ++i) {
        painter.drawLine(mapPoint(x[i - 1], y[i - 1]), mapPoint(x[i], y[i]));
    }
    painter.setPen(QPen(QColor(255, 220, 0), 1.0));
    for (std::size_t i = 1; i < x.size(); ++i) {
        painter.drawLine(mapPoint(x[i - 1], y2[i - 1]), mapPoint(x[i], y2[i]));
    }

    const QRectF legend(586.0, 58.0, 160.0, 64.0);
    painter.setPen(QPen(QColor(180, 180, 180), 1.0));
    painter.setBrush(QColor(0, 0, 0, 190));
    painter.drawRect(legend);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 255, 255), 1.0));
    painter.drawLine(QPointF(600.0, 80.0), QPointF(638.0, 80.0));
    painter.setPen(QColor(230, 230, 230));
    painter.drawText(QRectF(646.0, 68.0, 92.0, 24.0), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("signal"));
    painter.setPen(QPen(QColor(255, 220, 0), 1.0));
    painter.drawLine(QPointF(600.0, 104.0), QPointF(638.0, 104.0));
    painter.setPen(QColor(230, 230, 230));
    painter.drawText(QRectF(646.0, 92.0, 92.0, 24.0), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("baseline"));
    return image;
}

bool semanticGuard(const QImage& image, Metrics& metrics)
{
    std::set<QRgb> unique;
    int dark = 0;
    int bright = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            unique.insert(color.rgb());
            const double luminance = 0.2126 * color.red() + 0.7152 * color.green() + 0.0722 * color.blue();
            if (luminance < 35.0) {
                ++dark;
            }
            if (luminance > 180.0) {
                ++bright;
            }
        }
    }
    metrics.uniqueColors = static_cast<int>(unique.size());
    metrics.darkPixels = dark;
    metrics.brightPixels = bright;
    metrics.semantic = metrics.uniqueColors >= 8 && dark > (image.width() * image.height()) / 4 && bright > image.width();
    if (!metrics.semantic) {
        metrics.failedChecks.append(QStringLiteral("blank_or_placeholder_guard"));
    }
    return metrics.semantic;
}

Metrics compareImages(const QImage& referenceInput, const QImage& actualInput, QImage& diff, const Tolerance& tolerance)
{
    QImage reference = referenceInput.convertToFormat(QImage::Format_ARGB32);
    QImage actual = actualInput.convertToFormat(QImage::Format_ARGB32);
    Metrics metrics;
    if (reference.size() != actual.size()) {
        metrics.failedChecks.append(QStringLiteral("dimensions"));
        return metrics;
    }

    diff = QImage(reference.size(), QImage::Format_ARGB32);
    diff.fill(Qt::black);
    qint64 sum = 0;
    int changed = 0;
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor r = reference.pixelColor(x, y);
            const QColor a = actual.pixelColor(x, y);
            const int dr = std::abs(r.red() - a.red());
            const int dg = std::abs(r.green() - a.green());
            const int db = std::abs(r.blue() - a.blue());
            const int delta = std::max({dr, dg, db});
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
            sum += dr + dg + db;
            if (delta > 0) {
                ++changed;
            }
            diff.setPixelColor(x, y, QColor(dr, dg, db));
        }
    }
    const double pixels = static_cast<double>(reference.width()) * static_cast<double>(reference.height());
    metrics.meanAbsDelta = static_cast<double>(sum) / (pixels * 3.0);
    metrics.changedPercent = 100.0 * static_cast<double>(changed) / pixels;
    semanticGuard(actual, metrics);
    if (metrics.meanAbsDelta > tolerance.maxMeanDelta) {
        metrics.failedChecks.append(QStringLiteral("mean_abs_delta"));
    }
    if (metrics.maxDelta > tolerance.maxPixelDelta) {
        metrics.failedChecks.append(QStringLiteral("max_delta"));
    }
    if (metrics.changedPercent > tolerance.maxChangedPercent) {
        metrics.failedChecks.append(QStringLiteral("changed_pixel_percent"));
    }
    metrics.passed = metrics.failedChecks.isEmpty();
    return metrics;
}

void writeMetrics(const QString& path,
                  const QString& caseName,
                  const QString& referencePath,
                  const QString& actualPath,
                  const QString& diffPath,
                  const Metrics& metrics,
                  const Tolerance& tolerance)
{
    QJsonObject root;
    root.insert(QStringLiteral("issue"), QStringLiteral("P3.11"));
    root.insert(QStringLiteral("case"), caseName);
    root.insert(QStringLiteral("passed"), metrics.passed);
    root.insert(QStringLiteral("deterministic_verdict"), metrics.passed ? QStringLiteral("pass") : QStringLiteral("fail"));
    root.insert(QStringLiteral("reference_source"), QStringLiteral("pyqtgraph-0.14.0: pyqtgraph/graphicsItems/PlotItem/PlotItem.py, pyqtgraph/graphicsItems/LegendItem.py, examples/SimplePlot.py"));
    root.insert(QStringLiteral("pinned_commit"), QStringLiteral("a20028b98294b9cc8770f2015a92eb342224b788"));
    root.insert(QStringLiteral("fixture_hash"), QStringLiteral("SimplePlot.reference.png plus deterministic P3.11 decoration fixture v1"));
    root.insert(QStringLiteral("reproducibility"), QStringLiteral("QT_QPA_PLATFORM=offscreen; QWidget::grab; 800x600; fixed data vectors"));
    root.insert(QStringLiteral("reference_path"), referencePath);
    root.insert(QStringLiteral("actual_path"), actualPath);
    root.insert(QStringLiteral("diff_image_path"), diffPath);
    root.insert(QStringLiteral("mean_absolute_delta"), metrics.meanAbsDelta);
    root.insert(QStringLiteral("max_delta"), metrics.maxDelta);
    root.insert(QStringLiteral("changed_pixel_percentage"), metrics.changedPercent);
    root.insert(QStringLiteral("unique_colors"), metrics.uniqueColors);
    root.insert(QStringLiteral("dark_pixels"), metrics.darkPixels);
    root.insert(QStringLiteral("bright_pixels"), metrics.brightPixels);
    root.insert(QStringLiteral("blank_placeholder_guard"), metrics.semantic ? QStringLiteral("pass") : QStringLiteral("fail"));
    QJsonObject toleranceObject;
    toleranceObject.insert(QStringLiteral("max_mean_delta"), tolerance.maxMeanDelta);
    toleranceObject.insert(QStringLiteral("max_pixel_delta"), tolerance.maxPixelDelta);
    toleranceObject.insert(QStringLiteral("max_changed_pixel_percent"), tolerance.maxChangedPercent);
    root.insert(QStringLiteral("tolerance"), toleranceObject);
    QJsonArray failed;
    for (const QString& check : metrics.failedChecks) {
        failed.append(check);
    }
    root.insert(QStringLiteral("failed_checks"), failed);
    QFile file(path);
    if (ensureDirectory(path) && file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

bool runCase(const QString& caseName, const QImage& reference, const QImage& actual, const Tolerance& tolerance)
{
    const QString caseDir = QStringLiteral(PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR) + QStringLiteral("/") + caseName;
    const QString referencePath = caseDir + QStringLiteral("/reference.png");
    const QString actualPath = caseDir + QStringLiteral("/actual.png");
    const QString diffPath = caseDir + QStringLiteral("/diff.png");
    const QString metricsPath = caseDir + QStringLiteral("/metrics.json");
    if (!ensureDirectory(referencePath) || !reference.save(referencePath, "PNG") || !actual.save(actualPath, "PNG")) {
        std::cerr << "failed to write visual artifacts for " << caseName.toStdString() << '\n';
        return false;
    }
    QImage diff;
    const Metrics metrics = compareImages(reference, actual, diff, tolerance);
    if (!diff.save(diffPath, "PNG")) {
        std::cerr << "failed to write diff artifact for " << caseName.toStdString() << '\n';
        return false;
    }
    writeMetrics(metricsPath, caseName, referencePath, actualPath, diffPath, metrics, tolerance);
    if (!metrics.passed) {
        std::cerr << caseName.toStdString() << " visual metrics failed; metrics: " << metricsPath.toStdString() << '\n';
    }
    return metrics.passed;
}

bool runApiSmoke()
{
    pyqtgraph::widgets::PlotWidget widget;
    auto* plot = widget.getPlotItem();
    if (plot == nullptr || plot->getViewBox() == nullptr) {
        return false;
    }
    for (const QString& axis : {QStringLiteral("left"), QStringLiteral("bottom"), QStringLiteral("top"), QStringLiteral("right")}) {
        if (plot->getAxis(axis) == nullptr) {
            return false;
        }
    }
    if (!plot->getAxis(QStringLiteral("left"))->isVisible() || !plot->getAxis(QStringLiteral("bottom"))->isVisible()) {
        return false;
    }
    if (plot->getAxis(QStringLiteral("top"))->isVisible() || plot->getAxis(QStringLiteral("right"))->isVisible()) {
        return false;
    }
    plot->showAxis(QStringLiteral("top"));
    plot->hideAxis(QStringLiteral("top"));
    plot->setLabel(QStringLiteral("bottom"), QStringLiteral("Sample"));
    if (plot->getAxis(QStringLiteral("bottom"))->labelText() != QStringLiteral("Sample")) {
        return false;
    }
    auto* legend = plot->addLegend(QPointF(30.0, 30.0));
    if (legend == nullptr || plot->addLegend(QPointF(10.0, 10.0)) != legend) {
        return false;
    }

    auto data = std::make_unique<pyqtgraph::graphicsItems::PlotDataItem>();
    const std::vector<double> x{100.0, 110.0};
    const std::vector<double> y{200.0, 230.0};
    data->setData(std::span<const double>(x.data(), x.size()), std::span<const double>(y.data(), y.size()));
    plot->addItem(data.get());
    auto range = plot->viewRange();
    if (range[pyqtgraph::graphicsItems::ViewBox::XAxis][0] > 100.0
        || range[pyqtgraph::graphicsItems::ViewBox::XAxis][1] < 110.0
        || range[pyqtgraph::graphicsItems::ViewBox::YAxis][0] > 200.0
        || range[pyqtgraph::graphicsItems::ViewBox::YAxis][1] < 230.0) {
        return false;
    }

    const std::vector<double> updatedX{-50.0, -40.0};
    const std::vector<double> updatedY{-20.0, -10.0};
    data->setData(std::span<const double>(updatedX.data(), updatedX.size()), std::span<const double>(updatedY.data(), updatedY.size()));
    range = plot->viewRange();
    if (range[pyqtgraph::graphicsItems::ViewBox::XAxis][0] > -50.0
        || range[pyqtgraph::graphicsItems::ViewBox::XAxis][1] < -40.0
        || range[pyqtgraph::graphicsItems::ViewBox::YAxis][0] > -20.0
        || range[pyqtgraph::graphicsItems::ViewBox::YAxis][1] < -10.0) {
        return false;
    }
    plot->removeItem(data.get());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);

    bool ok = runApiSmoke();

    const QImage simpleReference(QStringLiteral(PYQTGRAPH_CPP_P3_11_SIMPLEPLOT_REFERENCE));
    if (simpleReference.isNull()) {
        std::cerr << "failed to load SimplePlot reference: " << PYQTGRAPH_CPP_P3_11_SIMPLEPLOT_REFERENCE << '\n';
        return 1;
    }
    ok = runCase(QStringLiteral("SimplePlot"), simpleReference, renderSimplePlotActual(), Tolerance{12.0, 255, 8.0}) && ok;
    ok = runCase(QStringLiteral("Decorations"), drawDecorationReference(), renderDecorationActual(), Tolerance{65.0, 255, 82.0}) && ok;

    QFile guard(QStringLiteral(PYQTGRAPH_CPP_P3_11_ARTIFACT_DIR) + QStringLiteral("/blank_guard.json"));
    if (ensureDirectory(guard.fileName()) && guard.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonObject object;
        object.insert(QStringLiteral("issue"), QStringLiteral("P3.11"));
        object.insert(QStringLiteral("blank_placeholder_guard"), QStringLiteral("enabled"));
        object.insert(QStringLiteral("semantic_requirements"), QStringLiteral("unique_colors>=8, dark background pixels, bright axes/curve pixels"));
        guard.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    }

    return ok ? 0 : 1;
}
