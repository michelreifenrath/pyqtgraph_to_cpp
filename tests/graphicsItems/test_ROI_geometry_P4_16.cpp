#include <cppqtgraph/graphicsItems/ROI.hpp>

#include <QtCore/QByteArray>
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
#include <cstdlib>
#include <iostream>
#include <type_traits>

#ifndef CPPQTGRAPH_P4_16_ARTIFACT_DIR
#define CPPQTGRAPH_P4_16_ARTIFACT_DIR "artifacts/P4.16"
#endif

#ifndef CPPQTGRAPH_P4_16_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_16_REPOSITORY_REPORT_DIR "reports/issues/P4.16"
#endif

using cppqtgraph::Point;
using cppqtgraph::graphicsItems::GraphicsObject;
using cppqtgraph::graphicsItems::ROI;
using cppqtgraph::graphicsItems::ROIState;

namespace {

struct SignalCounts {
    int changed = 0;
    int finished = 0;
    int started = 0;
};

bool nearly(qreal actual, qreal expected, qreal tolerance = 1.0e-6)
{
    return std::abs(actual - expected) <= tolerance;
}

bool samePoint(const QPointF& actual, const QPointF& expected, qreal tolerance = 1.0e-6)
{
    return nearly(actual.x(), expected.x(), tolerance) && nearly(actual.y(), expected.y(), tolerance);
}

QJsonArray pointJson(const QPointF& point)
{
    QJsonArray array;
    array.append(point.x());
    array.append(point.y());
    return array;
}

QJsonObject stateJson(const ROIState& state)
{
    return QJsonObject{{QStringLiteral("pos"), pointJson(state.pos)},
                       {QStringLiteral("size"), pointJson(state.size)},
                       {QStringLiteral("angle"), state.angle}};
}

QJsonObject rectJson(const QRectF& rect)
{
    return QJsonObject{{QStringLiteral("x"), rect.x()},
                       {QStringLiteral("y"), rect.y()},
                       {QStringLiteral("width"), rect.width()},
                       {QStringLiteral("height"), rect.height()}};
}

int countNonBackgroundPixels(const QImage& image, QRgb background)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, y) != background) {
                ++count;
            }
        }
    }
    return count;
}

bool writeReport(const QString& directory, const QJsonObject& report)
{
    if (!QDir().mkpath(directory)) {
        return false;
    }
    QFile file(directory + QStringLiteral("/ROI_geometry_interaction.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    return true;
}

int fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}

} // namespace

int main(int argc, char** argv)
{
    static_assert(std::is_base_of_v<GraphicsObject, ROI>);
    static_assert(std::is_base_of_v<QGraphicsObject, ROI>);
    static_assert(!std::is_copy_constructible_v<ROI>);
    static_assert(!std::is_move_constructible_v<ROI>);

    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    QJsonObject report;
    QJsonArray checks;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.16"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ROI.py:41,145-231,254-386,388-445,1010-1079; tests/graphicsItems/test_ROI.py:43-183"));

    ROI roi(QPointF(2.0, 3.0), QPointF(4.0, 5.0), 30.0);
    if (!samePoint(roi.pos(), QPointF(2.0, 3.0)) || !samePoint(roi.size(), QPointF(4.0, 5.0)) || !nearly(roi.angle(), 30.0)) {
        return fail("ROI construction should expose requested position, size, and angle in state accessors");
    }
    if (!samePoint(static_cast<const QGraphicsItem&>(roi).pos(), QPointF(2.0, 3.0)) || !nearly(roi.zValue(), 10.0)) {
        return fail("ROI construction should synchronize Qt position and default z value");
    }
    if (roi.boundingRect() != QRectF(0.0, 0.0, 4.0, 5.0)) {
        return fail("ROI boundingRect should reflect positive state size");
    }
    constexpr qreal pi = 3.141592653589793238462643383279502884L;
    const qreal radians = 30.0 * pi / 180.0;
    if (!nearly(roi.transform().m11(), std::cos(radians)) || !nearly(roi.transform().m12(), std::sin(radians))) {
        return fail("ROI transform should contain a rotation-only angle in degrees");
    }
    checks.append(QStringLiteral("construction-position-size-angle-z-transform"));

    SignalCounts signalCounts;
    QObject::connect(&roi, &ROI::sigRegionChanged, &roi, [&signalCounts](ROI*) { ++signalCounts.changed; });
    QObject::connect(&roi, &ROI::sigRegionChangeFinished, &roi, [&signalCounts](ROI*) { ++signalCounts.finished; });
    QObject::connect(&roi, &ROI::sigRegionChangeStarted, &roi, [&signalCounts](ROI*) { ++signalCounts.started; });

    roi.setPos(QPointF(7.0, 8.0), false);
    roi.setSize(QPointF(9.0, 10.0), false);
    roi.setAngle(45.0, false);
    if (signalCounts.changed != 0 || signalCounts.finished != 0) {
        return fail("ROI update=false changes should batch without emitting region signals");
    }
    if (!samePoint(roi.pos(), QPointF(7.0, 8.0)) || !samePoint(roi.size(), QPointF(9.0, 10.0)) || !nearly(roi.angle(), 45.0)) {
        return fail("ROI update=false changes should still update state and Qt geometry");
    }
    roi.stateChanged(false);
    if (signalCounts.changed != 1 || signalCounts.finished != 0) {
        return fail("ROI stateChanged(false) should emit changed but suppress finished");
    }
    roi.stateChangeFinished();
    if (signalCounts.finished != 1) {
        return fail("ROI stateChangeFinished should emit finished exactly once when requested");
    }
    checks.append(QStringLiteral("batched-update-and-finish"));

    const ROIState saved = roi.saveState();
    ROI restored(QPointF(-1.0, -2.0), QPointF(1.0, 1.0), 0.0);
    restored.setState(saved);
    if (!samePoint(restored.pos(), QPointF(7.0, 8.0)) || !samePoint(restored.size(), QPointF(9.0, 10.0)) || !nearly(restored.angle(), 45.0)) {
        return fail("ROI saveState/setState should restore pos, size, and angle");
    }
    if (!samePoint(static_cast<const QGraphicsItem&>(restored).pos(), QPointF(7.0, 8.0)) || restored.boundingRect() != QRectF(0.0, 0.0, 9.0, 10.0)) {
        return fail("ROI setState should synchronize Qt position and bounding rectangle");
    }
    checks.append(QStringLiteral("save-restore"));

    const int changedBeforeNoOp = signalCounts.changed;
    const int finishedBeforeNoOp = signalCounts.finished;
    roi.setPos(roi.pos());
    roi.setSize(roi.size());
    roi.setAngle(roi.angle());
    if (signalCounts.changed != changedBeforeNoOp) {
        return fail("ROI no-op setters should not emit sigRegionChanged when state is unchanged");
    }
    if (signalCounts.finished != finishedBeforeNoOp + 3) {
        return fail("ROI no-op setters should preserve PyQtGraph's finished emission when finish=true");
    }
    checks.append(QStringLiteral("no-op-state-change"));

    roi.setSize(QPointF(-4.0, 5.0));
    if (!samePoint(roi.size(), QPointF(-4.0, 5.0)) || roi.boundingRect() != QRectF(-4.0, 0.0, 4.0, 5.0)) {
        return fail("ROI should store negative size and expose a normalized bounding rectangle");
    }
    const QPointF snapped = roi.getSnapPosition(QPointF(2.2, 3.7), QPointF(0.5, 2.0));
    if (!samePoint(snapped, QPointF(2.0, 4.0))) {
        return fail("ROI getSnapPosition should snap to the nearest rectangular grid point");
    }
    roi.translate(QPointF(1.2, -0.2), true, false);
    if (!samePoint(roi.pos(), QPointF(8.0, 8.0)) || signalCounts.finished != finishedBeforeNoOp + 4) {
        return fail("ROI translate with snap and finish=false should update snapped position without finished emission");
    }
    checks.append(QStringLiteral("negative-size-snap-translate"));

    QGraphicsScene scene;
    scene.setSceneRect(-20.0, -20.0, 80.0, 80.0);
    ROI rendered(QPointF(20.0, 20.0), QPointF(20.0, 15.0), 15.0);
    scene.addItem(&rendered);
    QImage image(96, 96, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();
    const int nonBackgroundPixels = countNonBackgroundPixels(image, qRgb(0, 0, 0));
    if (nonBackgroundPixels <= 0) {
        return fail("ROI scene render should produce observable non-background rectangle pixels");
    }
    checks.append(QStringLiteral("offscreen-scene-render"));

    report.insert(QStringLiteral("checks"), checks);
    report.insert(QStringLiteral("savedState"), stateJson(saved));
    report.insert(QStringLiteral("finalState"), stateJson(roi.getState()));
    report.insert(QStringLiteral("negativeBoundingRect"), rectJson(roi.boundingRect()));
    report.insert(QStringLiteral("signals"), QJsonObject{{QStringLiteral("changed"), signalCounts.changed},
                                                 {QStringLiteral("finished"), signalCounts.finished},
                                                 {QStringLiteral("started"), signalCounts.started}});
    report.insert(QStringLiteral("render"), QJsonObject{{QStringLiteral("nonBackgroundPixels"), nonBackgroundPixels},
                                              {QStringLiteral("width"), image.width()},
                                              {QStringLiteral("height"), image.height()}});

    const QString artifactDir = QStringLiteral(CPPQTGRAPH_P4_16_ARTIFACT_DIR);
    if (!writeReport(artifactDir, report)) {
        return fail("could not write P4.16 build artifact report");
    }
    const QString repositoryReportDir = QStringLiteral(CPPQTGRAPH_P4_16_REPOSITORY_REPORT_DIR);
    if (!writeReport(repositoryReportDir, report)) {
        return fail("could not write P4.16 repository report artifact");
    }

    return 0;
}
