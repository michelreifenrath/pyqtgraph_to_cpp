#include <pyqtgraph/graphicsItems/ROI.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>

#ifndef PYQTGRAPH_CPP_P4_18_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_18_ARTIFACT_DIR "artifacts/P4.18"
#endif

#ifndef PYQTGRAPH_CPP_P4_18_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P4_18_REPOSITORY_REPORT_DIR "reports/issues/P4.18"
#endif

using pyqtgraph::graphicsItems::ROI;
using pyqtgraph::graphicsItems::ROIAffineSliceParams;
using pyqtgraph::graphicsItems::ROIState;

namespace {

struct SignalCounts {
    int changed = 0;
    int finished = 0;
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

QJsonObject affineJson(const ROIAffineSliceParams& params)
{
    QJsonArray vectors;
    vectors.append(pointJson(params.vectors[0]));
    vectors.append(pointJson(params.vectors[1]));
    return QJsonObject{{QStringLiteral("shape"), pointJson(params.shape)},
                       {QStringLiteral("origin"), pointJson(params.origin)},
                       {QStringLiteral("vectors"), vectors}};
}

bool writeReport(const QString& directory, const QJsonObject& report)
{
    if (!QDir().mkpath(directory)) {
        return false;
    }
    QFile file(directory + QStringLiteral("/ROI_transform_interaction.json"));
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
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    QJsonObject report;
    QJsonArray checks;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.18"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ROI.py:300-445 setSize/setAngle/scale/translate/rotate, 1227-1265 getAffineSliceParams; tests/graphicsItems/test_ROI.py:99-105,136-165"));

    SignalCounts scaleSignals;
    ROI scaled(QPointF(10.0, 20.0), QPointF(10.0, 20.0), 0.0);
    QObject::connect(&scaled, &ROI::sigRegionChanged, &scaled, [&scaleSignals](ROI*) { ++scaleSignals.changed; });
    QObject::connect(&scaled, &ROI::sigRegionChangeFinished, &scaled, [&scaleSignals](ROI*) { ++scaleSignals.finished; });
    const QPointF scaledAnchorBefore = scaled.mapToParent(QPointF(5.0, 10.0));
    scaled.scale(QPointF(2.0, 2.0), std::optional<QPointF>(QPointF(0.5, 0.5)), std::nullopt, false, true, false);
    if (!samePoint(scaled.size(), QPointF(20.0, 40.0)) || !samePoint(scaled.pos(), QPointF(5.0, 10.0))) {
        return fail("ROI scale around a normalized center should update size and compensate position");
    }
    if (!samePoint(scaled.mapToParent(QPointF(10.0, 20.0)), scaledAnchorBefore)) {
        return fail("ROI scale should keep the requested center anchored in parent coordinates");
    }
    if (scaleSignals.changed != 1 || scaleSignals.finished != 0) {
        return fail("ROI scale update=true finish=false should emit changed once and suppress finished");
    }
    checks.append(QStringLiteral("scale-normalized-center-anchor"));

    SignalCounts angleSignals;
    ROI angled(QPointF(10.0, 20.0), QPointF(10.0, 20.0), 0.0);
    QObject::connect(&angled, &ROI::sigRegionChanged, &angled, [&angleSignals](ROI*) { ++angleSignals.changed; });
    QObject::connect(&angled, &ROI::sigRegionChangeFinished, &angled, [&angleSignals](ROI*) { ++angleSignals.finished; });
    const QPointF angleAnchorBefore = angled.mapToParent(QPointF(5.0, 10.0));
    angled.setAngle(90.0, std::optional<QPointF>(QPointF(0.5, 0.5)), std::nullopt, false, true, false);
    if (!nearly(angled.angle(), 90.0) || !samePoint(angled.pos(), QPointF(25.0, 25.0))) {
        return fail("ROI setAngle around a normalized center should set angle and compensate position");
    }
    if (!nearly(angled.transform().m11(), 0.0) || !nearly(angled.transform().m12(), 1.0)
        || !nearly(angled.transform().m21(), -1.0) || !nearly(angled.transform().m22(), 0.0)) {
        return fail("ROI setAngle should leave a rotation-only Qt transform");
    }
    if (!samePoint(angled.mapToParent(QPointF(5.0, 10.0)), angleAnchorBefore)) {
        return fail("ROI setAngle should keep the requested center anchored in parent coordinates");
    }
    if (angleSignals.changed != 1 || angleSignals.finished != 0) {
        return fail("ROI setAngle update=true finish=false should emit changed once and suppress finished");
    }
    checks.append(QStringLiteral("setAngle-normalized-center-anchor"));

    ROI rotated(QPointF(10.0, 20.0), QPointF(10.0, 20.0), 15.0);
    const QPointF rotateCenterLocal(5.0, 10.0);
    const QPointF rotateAnchorBefore = rotated.mapToParent(rotateCenterLocal);
    rotated.rotate(45.0, std::optional<QPointF>(rotateCenterLocal), false, true, false);
    if (!nearly(rotated.angle(), 60.0) || !samePoint(rotated.mapToParent(rotateCenterLocal), rotateAnchorBefore)) {
        return fail("ROI rotate should increment the angle and anchor the supplied local center");
    }
    checks.append(QStringLiteral("rotate-local-center-increment"));

    ROI translated(QPointF(0.0, 0.0), QPointF(2.0, 2.0), 0.0);
    translated.translate(QPointF(1.2, 3.1), QPointF(0.5, 2.0), false);
    if (!samePoint(translated.pos(), QPointF(1.0, 4.0))) {
        return fail("ROI translate should snap to a rectangular grid when given a point snap spacing");
    }
    checks.append(QStringLiteral("translate-rectangular-snap"));

    QGraphicsScene scene;
    QGraphicsRectItem target(QRectF(0.0, 0.0, 40.0, 40.0));
    ROI affine(QPointF(1.0, 1.0), QPointF(27.0, 28.0), 0.0);
    scene.addItem(&target);
    scene.addItem(&affine);
    const ROIAffineSliceParams identityParams = affine.getAffineSliceParams(&target);
    if (!samePoint(identityParams.shape, QPointF(27.0, 28.0))
        || !samePoint(identityParams.vectors[0], QPointF(1.0, 0.0))
        || !samePoint(identityParams.vectors[1], QPointF(0.0, 1.0))
        || !samePoint(identityParams.origin, QPointF(1.0, 1.0))) {
        return fail("ROI getAffineSliceParams identity case should match PyQtGraph RectROI oracle shape/vectors/origin");
    }

    ROI affineRotated(QPointF(3.0, 4.0), QPointF(6.0, 8.0), 90.0);
    scene.addItem(&affineRotated);
    const ROIAffineSliceParams rotatedParams = affineRotated.getAffineSliceParams(&target);
    if (!samePoint(rotatedParams.shape, QPointF(6.0, 8.0))
        || !samePoint(rotatedParams.vectors[0], QPointF(0.0, 1.0))
        || !samePoint(rotatedParams.vectors[1], QPointF(-1.0, 0.0))
        || !samePoint(rotatedParams.origin, QPointF(3.0, 4.0))) {
        return fail("ROI getAffineSliceParams should normalize mapped ROI basis vectors for rotated ROI state");
    }
    checks.append(QStringLiteral("affine-slice-params-identity-and-rotated"));

    report.insert(QStringLiteral("checks"), checks);
    report.insert(QStringLiteral("scaledState"), stateJson(scaled.getState()));
    report.insert(QStringLiteral("angledState"), stateJson(angled.getState()));
    report.insert(QStringLiteral("rotatedState"), stateJson(rotated.getState()));
    report.insert(QStringLiteral("translatedState"), stateJson(translated.getState()));
    report.insert(QStringLiteral("identityAffine"), affineJson(identityParams));
    report.insert(QStringLiteral("rotatedAffine"), affineJson(rotatedParams));
    report.insert(QStringLiteral("signals"), QJsonObject{{QStringLiteral("scaleChanged"), scaleSignals.changed},
                                                 {QStringLiteral("scaleFinished"), scaleSignals.finished},
                                                 {QStringLiteral("angleChanged"), angleSignals.changed},
                                                 {QStringLiteral("angleFinished"), angleSignals.finished}});

    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P4_18_ARTIFACT_DIR);
    if (!writeReport(artifactDir, report)) {
        return fail("could not write P4.18 build artifact report");
    }
    const QString repositoryReportDir = QStringLiteral(PYQTGRAPH_CPP_P4_18_REPOSITORY_REPORT_DIR);
    if (!writeReport(repositoryReportDir, report)) {
        return fail("could not write P4.18 repository report artifact");
    }

    return 0;
}
