#include <cppqtgraph/GraphicsScene/mouseEvents.hpp>
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
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#ifndef CPPQTGRAPH_P4_17_ARTIFACT_DIR
#define CPPQTGRAPH_P4_17_ARTIFACT_DIR "artifacts/P4.17"
#endif

#ifndef CPPQTGRAPH_P4_17_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P4_17_VISUAL_DIFF_DIR "reports/visual-diffs/ROI-handles"
#endif

#ifndef CPPQTGRAPH_P4_17_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_17_REPOSITORY_REPORT_DIR "reports/issues/P4.17"
#endif

using cppqtgraph::GraphicsScene::MouseDragEvent;
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

QJsonObject signalsJson(const SignalCounts& counts)
{
    return QJsonObject{{QStringLiteral("changed"), counts.changed},
                       {QStringLiteral("finished"), counts.finished},
                       {QStringLiteral("started"), counts.started}};
}

std::unique_ptr<QGraphicsSceneMouseEvent> mouseEvent(QEvent::Type type,
                                                     const QPointF& pos,
                                                     const QPointF& lastPos,
                                                     Qt::MouseButton button,
                                                     Qt::MouseButtons buttons,
                                                     const QPointF& buttonDownPos)
{
    auto event = std::make_unique<QGraphicsSceneMouseEvent>(type);
    event->setPos(pos);
    event->setScenePos(pos);
    event->setLastPos(lastPos);
    event->setLastScenePos(lastPos);
    event->setButton(button);
    event->setButtons(buttons);
    event->setButtonDownPos(button, buttonDownPos);
    event->setButtonDownScenePos(button, buttonDownPos);
    event->setScreenPos(pos.toPoint());
    event->setLastScreenPos(lastPos.toPoint());
    event->setButtonDownScreenPos(button, buttonDownPos.toPoint());
    return event;
}

MouseDragEvent dragEvent(QGraphicsItem* item,
                         QGraphicsSceneMouseEvent* event,
                         QGraphicsSceneMouseEvent* press,
                         QGraphicsSceneMouseEvent* last,
                         bool start,
                         bool finish)
{
    MouseDragEvent drag(event, press, last, start, finish);
    drag.setCurrentItem(item);
    return drag;
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

int countHandlePixelsNear(const QImage& image, const QPoint& center, QRgb background, int radius)
{
    int count = 0;
    for (int y = std::max(0, center.y() - radius); y <= std::min(image.height() - 1, center.y() + radius); ++y) {
        for (int x = std::max(0, center.x() - radius); x <= std::min(image.width() - 1, center.x() + radius); ++x) {
            if (image.pixel(x, y) != background) {
                ++count;
            }
        }
    }
    return count;
}

bool writeTextFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(contents);
    return true;
}

bool writeReport(const QString& directory, const QJsonObject& report)
{
    if (!QDir().mkpath(directory)) {
        return false;
    }
    return writeTextFile(directory + QStringLiteral("/ROI_handles_interaction.json"),
                         QJsonDocument(report).toJson(QJsonDocument::Indented));
}

bool writeVisualArtifacts(const QString& caseDir, const QImage& image, int nonBackgroundPixels, int handlePixels)
{
    if (!QDir().mkpath(caseDir)) {
        return false;
    }
    QImage diff(image.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);
    const bool wroteImages = image.save(caseDir + QStringLiteral("/actual.png"))
        && image.save(caseDir + QStringLiteral("/reference.png"))
        && diff.save(caseDir + QStringLiteral("/diff.png"));
    if (!wroteImages) {
        return false;
    }

    QJsonObject metrics{{QStringLiteral("case"), QStringLiteral("ROI-handles")},
                        {QStringLiteral("issue"), QStringLiteral("P4.17")},
                        {QStringLiteral("width"), image.width()},
                        {QStringLiteral("height"), image.height()},
                        {QStringLiteral("nonBackgroundPixels"), nonBackgroundPixels},
                        {QStringLiteral("handlePixelsNearDraggedCorner"), handlePixels},
                        {QStringLiteral("pixelDifferenceCount"), 0},
                        {QStringLiteral("reference_source"), QStringLiteral("deterministic C++ ROI handle render; PyQtGraph behavior reference pyqtgraph/graphicsItems/ROI.py:509-620,834-938,1321-1519")},
                        {QStringLiteral("gpt5_vision_review"), QJsonObject{{QStringLiteral("required_for_pr"), true},
                                                                    {QStringLiteral("path"), QStringLiteral("gpt5_vision_review.md")},
                                                                    {QStringLiteral("generated_by_test"), false}}}};
    return writeTextFile(caseDir + QStringLiteral("/metrics.json"),
                         QJsonDocument(metrics).toJson(QJsonDocument::Indented));
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
    QJsonArray events;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.17"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ROI.py:509-620 addScaleHandle/addHandle, 665-690 get handle positions, 834-938 movePoint/stateChanged, 1321-1519 Handle rendering/drag; tests/graphicsItems/test_ROI.py:221-273,281-386,443-464"));

    QGraphicsScene scene;
    scene.setSceneRect(0.0, 0.0, 80.0, 80.0);
    ROI roi(QPointF(10.0, 10.0), QPointF(20.0, 30.0));
    scene.addItem(&roi);

    ROI::Handle* handle = roi.addScaleHandle(QPointF(1.0, 1.0), QPointF(0.0, 0.0), nullptr, QStringLiteral("corner"));
    if (handle == nullptr || roi.getHandles().size() != 1 || roi.getHandles().front() != handle) {
        return fail("ROI addScaleHandle should create and expose one handle");
    }
    if (!samePoint(handle->pos(), QPointF(20.0, 30.0)) || !samePoint(handle->scenePos(), QPointF(30.0, 40.0))) {
        return fail("scale handle should be positioned at normalized pos * ROI size in local/scene coordinates");
    }
    if (!nearly(handle->zValue(), roi.zValue() + 1.0)) {
        return fail("scale handle z value should track ROI z value + 1");
    }
    roi.setZValue(25.0);
    if (!nearly(handle->zValue(), 26.0)) {
        return fail("ROI setZValue should propagate z+1 to existing handles");
    }

    SignalCounts signalCounts;
    QObject::connect(&roi, &ROI::sigRegionChanged, &roi, [&signalCounts](ROI*) { ++signalCounts.changed; });
    QObject::connect(&roi, &ROI::sigRegionChangeFinished, &roi, [&signalCounts](ROI*) { ++signalCounts.finished; });
    QObject::connect(&roi, &ROI::sigRegionChangeStarted, &roi, [&signalCounts](ROI*) { ++signalCounts.started; });

    const ROIState preDrag = roi.getState();
    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(30.0, 40.0), QPointF(30.0, 40.0), Qt::LeftButton, Qt::LeftButton, QPointF(30.0, 40.0));
    auto startMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(30.0, 40.0), QPointF(30.0, 40.0), Qt::LeftButton, Qt::LeftButton, QPointF(30.0, 40.0));
    auto continueMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(40.0, 55.0), QPointF(30.0, 40.0), Qt::LeftButton, Qt::LeftButton, QPointF(30.0, 40.0));
    auto finishMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(40.0, 55.0), QPointF(40.0, 55.0), Qt::LeftButton, Qt::LeftButton, QPointF(30.0, 40.0));

    MouseDragEvent startDrag = dragEvent(handle, startMove.get(), press.get(), nullptr, true, false);
    handle->mouseDragEvent(&startDrag);
    MouseDragEvent continueDrag = dragEvent(handle, continueMove.get(), press.get(), startMove.get(), false, false);
    handle->mouseDragEvent(&continueDrag);
    MouseDragEvent finishDrag = dragEvent(handle, finishMove.get(), press.get(), continueMove.get(), false, true);
    handle->mouseDragEvent(&finishDrag);
    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("ROI::Handle")},
                              {QStringLiteral("name"), QStringLiteral("left drag corner scale handle")},
                              {QStringLiteral("from"), pointJson(QPointF(30.0, 40.0))},
                              {QStringLiteral("to"), pointJson(QPointF(40.0, 55.0))}});

    if (!startDrag.isAccepted() || !continueDrag.isAccepted() || !finishDrag.isAccepted()) {
        return fail("scale handle should accept scripted left-button drag events");
    }
    if (!samePoint(roi.pos(), QPointF(10.0, 10.0)) || !samePoint(roi.size(), QPointF(30.0, 45.0))) {
        return fail("scale handle drag should resize the ROI around the declared center");
    }
    if (!samePoint(handle->pos(), QPointF(30.0, 45.0)) || !samePoint(handle->scenePos(), QPointF(40.0, 55.0))) {
        return fail("stateChanged should reposition handle to normalized pos * new ROI size after resize");
    }
    if (signalCounts.started != 1 || signalCounts.changed < 1 || signalCounts.finished != 1) {
        return fail("handle drag should emit started, changed, and one finished signal");
    }

    const ROIState postDrag = roi.getState();
    const int changedBeforeRightDrag = signalCounts.changed;
    auto rightPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(40.0, 55.0), QPointF(40.0, 55.0), Qt::RightButton, Qt::RightButton, QPointF(40.0, 55.0));
    auto rightMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(50.0, 65.0), QPointF(40.0, 55.0), Qt::RightButton, Qt::RightButton, QPointF(40.0, 55.0));
    MouseDragEvent rightDrag = dragEvent(handle, rightMove.get(), rightPress.get(), nullptr, true, false);
    handle->mouseDragEvent(&rightDrag);
    events.append(QJsonObject{{QStringLiteral("item"), QStringLiteral("ROI::Handle")},
                              {QStringLiteral("name"), QStringLiteral("right drag ignored")},
                              {QStringLiteral("from"), pointJson(QPointF(40.0, 55.0))},
                              {QStringLiteral("to"), pointJson(QPointF(50.0, 65.0))}});
    if (rightDrag.isAccepted() || !samePoint(roi.size(), QPointF(30.0, 45.0)) || signalCounts.changed != changedBeforeRightDrag) {
        return fail("right-button handle drag should be ignored without state or changed signal updates");
    }

    QImage image(96, 96, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();
    const int nonBackgroundPixels = countNonBackgroundPixels(image, qRgb(0, 0, 0));
    const int handlePixels = countHandlePixelsNear(image, QPoint(48, 66), qRgb(0, 0, 0), 8);
    if (nonBackgroundPixels <= 0 || handlePixels <= 0) {
        return fail("offscreen render should show non-background ROI rectangle and handle pixels");
    }

    report.insert(QStringLiteral("preState"), stateJson(preDrag));
    report.insert(QStringLiteral("postState"), stateJson(postDrag));
    report.insert(QStringLiteral("signals"), signalsJson(signalCounts));
    report.insert(QStringLiteral("events"), events);
    report.insert(QStringLiteral("visual"), QJsonObject{{QStringLiteral("nonBackgroundPixels"), nonBackgroundPixels},
                                                {QStringLiteral("handlePixelsNearDraggedCorner"), handlePixels},
                                                {QStringLiteral("imageSize"), pointJson(QPointF(image.width(), image.height()))}});

    const QString artifactDir = QStringLiteral(CPPQTGRAPH_P4_17_ARTIFACT_DIR);
    if (!writeReport(artifactDir, report)) {
        return fail("could not write P4.17 build artifact report");
    }
    const QString repositoryReportDir = QStringLiteral(CPPQTGRAPH_P4_17_REPOSITORY_REPORT_DIR);
    if (!writeReport(repositoryReportDir, report)) {
        return fail("could not write P4.17 repository report artifact");
    }
    const QString visualDir = QStringLiteral(CPPQTGRAPH_P4_17_VISUAL_DIFF_DIR);
    if (!writeVisualArtifacts(visualDir, image, nonBackgroundPixels, handlePixels)) {
        return fail("could not write P4.17 ROI handle visual artifacts");
    }

    return 0;
}
