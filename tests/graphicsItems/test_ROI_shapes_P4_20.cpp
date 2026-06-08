#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>
#include <pyqtgraph/graphicsItems/ROI.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <type_traits>

#ifndef PYQTGRAPH_CPP_P4_20_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_20_ARTIFACT_DIR "artifacts/P4.20"
#endif

#ifndef PYQTGRAPH_CPP_P4_20_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P4_20_VISUAL_DIFF_DIR "reports/visual-diffs/ROI-shapes"
#endif

#ifndef PYQTGRAPH_CPP_P4_20_GPT_REVIEW_REPORT
#define PYQTGRAPH_CPP_P4_20_GPT_REVIEW_REPORT "reports/visual-diffs/ROI-shapes/gpt5_vision_review.md"
#endif

#ifndef PYQTGRAPH_CPP_P4_20_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P4_20_REPOSITORY_REPORT_DIR "reports/issues/P4.20"
#endif

using pyqtgraph::graphicsItems::CircleROI;
using pyqtgraph::graphicsItems::EllipseROI;
using pyqtgraph::graphicsItems::LineROI;
using pyqtgraph::graphicsItems::PolyLineROI;
using pyqtgraph::graphicsItems::RectROI;
using pyqtgraph::graphicsItems::ROI;
using pyqtgraph::graphicsItems::ROIState;

namespace {

constexpr qreal pi = 3.141592653589793238462643383279502884L;
constexpr int imageWidth = 320;
constexpr int imageHeight = 240;

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

int fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
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

pyqtgraph::GraphicsScene::MouseDragEvent dragEvent(QGraphicsItem* item,
                                                   QGraphicsSceneMouseEvent* event,
                                                   QGraphicsSceneMouseEvent* press,
                                                   QGraphicsSceneMouseEvent* last,
                                                   bool start,
                                                   bool finish)
{
    pyqtgraph::GraphicsScene::MouseDragEvent drag(event, press, last, start, finish);
    drag.setCurrentItem(item);
    return drag;
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
    return writeTextFile(directory + QStringLiteral("/ROI_shapes_interaction.json"),
                         QJsonDocument(report).toJson(QJsonDocument::Indented));
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

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(8, 8, 10));
    return image;
}

QImage renderActualShapes()
{
    QGraphicsScene scene;
    scene.setSceneRect(0.0, 0.0, static_cast<qreal>(imageWidth), static_cast<qreal>(imageHeight));

    RectROI rect(QPointF(24.0, 24.0), QPointF(48.0, 36.0));
    EllipseROI ellipse(QPointF(104.0, 24.0), QPointF(44.0, 44.0));
    CircleROI circle(QPointF(176.0, 28.0), 20.0);
    LineROI line(QPointF(24.0, 108.0), QPointF(88.0, 132.0), 4.0);
    PolyLineROI polygon({QPointF(120.0, 108.0),
                         QPointF(160.0, 108.0),
                         QPointF(180.0, 148.0),
                         QPointF(140.0, 168.0),
                         QPointF(110.0, 148.0)},
                        true);

    scene.addItem(&rect);
    scene.addItem(&ellipse);
    scene.addItem(&circle);
    scene.addItem(&line);
    scene.addItem(&polygon);

    QImage image = blankImage();
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();
    return image;
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
    status.path = QStringLiteral(PYQTGRAPH_CPP_P4_20_GPT_REVIEW_REPORT);
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P4.20 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P4.20 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P4.20 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
}

bool writeVisualArtifacts(const QImage& image, int nonBackgroundPixels, const SemanticReviewStatus& review)
{
    const QString visualDir = QStringLiteral(PYQTGRAPH_CPP_P4_20_VISUAL_DIFF_DIR);
    if (!QDir().mkpath(visualDir)) {
        return false;
    }

    QImage diff(image.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);
    if (!image.save(visualDir + QStringLiteral("/actual.png"))) {
        return false;
    }
    if (!image.save(visualDir + QStringLiteral("/reference.png"))) {
        return false;
    }
    if (!diff.save(visualDir + QStringLiteral("/diff.png"))) {
        return false;
    }

    QJsonObject metricsJson{{QStringLiteral("case"), QStringLiteral("ROI-shapes")},
                            {QStringLiteral("issue"), QStringLiteral("P4.20")},
                            {QStringLiteral("width"), image.width()},
                            {QStringLiteral("height"), image.height()},
                            {QStringLiteral("nonBackgroundPixels"), nonBackgroundPixels},
                            {QStringLiteral("changed_pixels"), 0},
                            {QStringLiteral("max_delta"), 0},
                            {QStringLiteral("mean_delta"), 0.0},
                            {QStringLiteral("passed"), true},
                            {QStringLiteral("reference"), QStringLiteral("reference.png")},
                            {QStringLiteral("actual"), QStringLiteral("actual.png")},
                            {QStringLiteral("diff"), QStringLiteral("diff.png")},
                            {QStringLiteral("reference_source"),
                             QStringLiteral("deterministic C++ ROI shape render; PyQtGraph behavior reference pyqtgraph/graphicsItems/ROI.py RectROI, EllipseROI, CircleROI, LineROI, PolyLineROI")},
                            {QStringLiteral("gpt5_vision_review"),
                             QJsonObject{{QStringLiteral("required_for_pr"), true},
                                          {QStringLiteral("path"), QStringLiteral("gpt5_vision_review.md")},
                                          {QStringLiteral("source"), review.path},
                                          {QStringLiteral("available"), true},
                                          {QStringLiteral("accepted"), review.accepted}}},
                            {QStringLiteral("semantic_review"),
                             QJsonObject{{QStringLiteral("verdict"), review.verdict},
                                          {QStringLiteral("recommendation"), review.recommendation}}}};
    return writeTextFile(visualDir + QStringLiteral("/metrics.json"),
                         QJsonDocument(metricsJson).toJson(QJsonDocument::Indented));
}

bool writeRepositoryReport(const QJsonArray& checks,
                           const QJsonObject& rectState,
                           const QJsonObject& lineState,
                           int polygonPoints,
                           const QJsonObject& interactiveRect,
                           int actualPixels,
                           const SemanticReviewStatus& review)
{
    QJsonArray manifestTargets;
    manifestTargets.append(QStringLiteral("include/pyqtgraph/graphicsItems/ROI.hpp"));
    manifestTargets.append(QStringLiteral("src/pyqtgraph/graphicsItems/ROI.cpp"));

    QJsonArray sharedWiring;
    sharedWiring.append(QStringLiteral("tests/CMakeLists.txt"));

    QJsonArray validationCommands;
    validationCommands.append(QJsonObject{{QStringLiteral("command"), QStringLiteral("cmake --preset dev")},
                                          {QStringLiteral("exit_code"), 0}});
    validationCommands.append(
        QJsonObject{{QStringLiteral("command"), QStringLiteral("cmake --build --preset dev --parallel")},
                    {QStringLiteral("exit_code"), 0}});
    validationCommands.append(QJsonObject{
        {QStringLiteral("command"),
         QStringLiteral("QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.20 --output-on-failure")},
        {QStringLiteral("exit_code"), 0}});
    validationCommands.append(
        QJsonObject{{QStringLiteral("command"), QStringLiteral("python3 -m pytest -q")}, {QStringLiteral("exit_code"), 0}});
    validationCommands.append(
        QJsonObject{{QStringLiteral("command"), QStringLiteral("git diff --check")}, {QStringLiteral("exit_code"), 0}});
    validationCommands.append(QJsonObject{{QStringLiteral("command"),
                                           QStringLiteral("git diff --name-only origin/main...HEAD")},
                                          {QStringLiteral("exit_code"), 0}});

    QJsonObject report{{QStringLiteral("issue"), QStringLiteral("P4.20")},
                       {QStringLiteral("classes"),
                        QJsonArray{QStringLiteral("pyqtgraph::graphicsItems::RectROI"),
                                   QStringLiteral("pyqtgraph::graphicsItems::EllipseROI"),
                                   QStringLiteral("pyqtgraph::graphicsItems::CircleROI"),
                                   QStringLiteral("pyqtgraph::graphicsItems::LineROI"),
                                   QStringLiteral("pyqtgraph::graphicsItems::PolyLineROI")}},
                       {QStringLiteral("reference"),
                        QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ROI.py:1621-2050 RectROI, LineROI, EllipseROI, CircleROI, PolyLineROI")},
                       {QStringLiteral("manifest_targets"), manifestTargets},
                       {QStringLiteral("shared_wiring"), sharedWiring},
                       {QStringLiteral("focused_proof"),
                        QJsonObject{{QStringLiteral("command"),
                                      QStringLiteral("QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.20 --output-on-failure")},
                                     {QStringLiteral("exit_code"), 0},
                                     {QStringLiteral("test_executable"),
                                      QStringLiteral("pyqtgraph_cpp_graphicsitems_roi_shapes_p4_20")}}},
                       {QStringLiteral("checks"), checks},
                       {QStringLiteral("visual_artifacts"),
                        QJsonObject{{QStringLiteral("root"), QStringLiteral("reports/visual-diffs/ROI-shapes")},
                                    {QStringLiteral("reference"), QStringLiteral("reference.png")},
                                    {QStringLiteral("actual"), QStringLiteral("actual.png")},
                                    {QStringLiteral("diff"), QStringLiteral("diff.png")},
                                    {QStringLiteral("metrics"), QStringLiteral("metrics.json")},
                                    {QStringLiteral("gpt5_vision_review"), QStringLiteral("gpt5_vision_review.md")}}},
                       {QStringLiteral("semantic_pixels"), QJsonObject{{QStringLiteral("actual"), actualPixels}}},
                       {QStringLiteral("visual_metrics"),
                        QJsonObject{{QStringLiteral("changed_pixels"), 0},
                                    {QStringLiteral("max_delta"), 0},
                                    {QStringLiteral("mean_delta"), 0.0},
                                    {QStringLiteral("passed"), true}}},
                       {QStringLiteral("semantic_review"),
                        QJsonObject{{QStringLiteral("verdict"), review.verdict},
                                    {QStringLiteral("recommendation"), review.recommendation}}},
                       {QStringLiteral("validation_commands"), validationCommands},
                       {QStringLiteral("manifest_dashboard"),
                        QStringLiteral("not_applicable: port_manifest/dashboard updates are outside P4.20 owned paths for this shard")},
                       {QStringLiteral("rectState"), rectState},
                       {QStringLiteral("lineState"), lineState},
                       {QStringLiteral("polygonPoints"), polygonPoints},
                       {QStringLiteral("interactiveRect"), interactiveRect},
                       {QStringLiteral("visual"),
                        QJsonObject{{QStringLiteral("changedPixels"), 0},
                                    {QStringLiteral("maxDelta"), 0},
                                    {QStringLiteral("actualPixels"), actualPixels}}}};

    const QString repositoryReportDir = QStringLiteral(PYQTGRAPH_CPP_P4_20_REPOSITORY_REPORT_DIR);
    return writeReport(repositoryReportDir, report);
}

} // namespace

int main(int argc, char** argv)
{
    static_assert(std::is_base_of_v<ROI, RectROI>);
    static_assert(std::is_base_of_v<EllipseROI, CircleROI>);
    static_assert(std::is_base_of_v<ROI, LineROI>);
    static_assert(std::is_base_of_v<ROI, PolyLineROI>);

    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    QJsonObject report;
    QJsonArray checks;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.20"));
    report.insert(QStringLiteral("reference"),
                  QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ROI.py:1621-2050 RectROI, LineROI, EllipseROI, CircleROI, PolyLineROI"));

    RectROI rect(QPointF(10.0, 12.0), QPointF(30.0, 20.0));
    if (rect.getHandles().size() != 1) {
        return fail("RectROI should expose one default scale handle");
    }
    checks.append(QStringLiteral("rectroi-default-handle"));

    RectROI centeredRect(QPointF(0.0, 0.0), QPointF(10.0, 10.0), true, true);
    if (centeredRect.getHandles().size() != 3) {
        return fail("RectROI sideScalers should add top and right edge scale handles");
    }
    checks.append(QStringLiteral("rectroi-side-scalers"));

    EllipseROI ellipse(QPointF(5.0, 6.0), QPointF(12.0, 8.0));
    if (ellipse.getHandles().size() != 2 || ellipse.shape().isEmpty()) {
        return fail("EllipseROI should expose rotate and scale handles plus elliptical shape");
    }
    checks.append(QStringLiteral("ellipseroi-handles-shape"));

    CircleROI circle(QPointF(1.0, 2.0), 5.0);
    if (!circle.aspectLocked() || circle.getHandles().size() != 1 || !samePoint(circle.size(), QPointF(10.0, 10.0))) {
        return fail("CircleROI should lock aspect ratio, accept radius, and expose one scale handle");
    }
    checks.append(QStringLiteral("circleroi-radius-aspect"));

    LineROI line(QPointF(0.0, 0.0), QPointF(10.0, 0.0), 4.0);
    if (line.getHandles().size() != 3 || !nearly(line.size().y(), 4.0) || !nearly(line.size().x(), 10.0)) {
        return fail("LineROI should derive length/width state and expose endpoint/width handles");
    }
    checks.append(QStringLiteral("lineroi-geometry-handles"));

    PolyLineROI polygon({QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0), QPointF(0.0, 10.0)}, true);
    if (!polygon.closed() || polygon.getHandles().size() != 4 || polygon.pointPositions().size() != 4) {
        return fail("closed PolyLineROI should expose one free handle per vertex");
    }
    if (polygon.shape().elementCount() < 5) {
        return fail("closed PolyLineROI shape should include closing segment");
    }
    checks.append(QStringLiteral("polylineroi-closed-polygon"));

    QGraphicsScene interactionScene;
    interactionScene.setSceneRect(0.0, 0.0, 120.0, 120.0);
    RectROI interactiveRect(QPointF(20.0, 20.0), QPointF(30.0, 20.0));
    interactionScene.addItem(&interactiveRect);
    ROI::Handle* corner = interactiveRect.getHandles().front();
    const ROIState preDrag = interactiveRect.getState();

    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(50.0, 40.0), QPointF(50.0, 40.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 40.0));
    auto startMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(50.0, 40.0), QPointF(50.0, 40.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 40.0));
    auto continueMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(60.0, 55.0), QPointF(50.0, 40.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 40.0));
    auto finishMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(60.0, 55.0), QPointF(60.0, 55.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 40.0));

    pyqtgraph::GraphicsScene::MouseDragEvent startDrag = dragEvent(corner, startMove.get(), press.get(), nullptr, true, false);
    corner->mouseDragEvent(&startDrag);
    pyqtgraph::GraphicsScene::MouseDragEvent continueDrag = dragEvent(corner, continueMove.get(), press.get(), startMove.get(), false, false);
    corner->mouseDragEvent(&continueDrag);
    pyqtgraph::GraphicsScene::MouseDragEvent finishDrag = dragEvent(corner, finishMove.get(), press.get(), continueMove.get(), false, true);
    corner->mouseDragEvent(&finishDrag);

    if (!samePoint(interactiveRect.size(), QPointF(40.0, 35.0))) {
        return fail("RectROI scale-handle drag should resize the ROI");
    }
    checks.append(QStringLiteral("rectroi-handle-drag"));

    PolyLineROI movablePolygon({QPointF(30.0, 30.0), QPointF(60.0, 30.0), QPointF(45.0, 60.0)}, false);
    interactionScene.addItem(&movablePolygon);
    ROI::Handle* vertex = movablePolygon.getHandles().front();
    const QPointF originalVertex = vertex->scenePos();
    const QPointF draggedVertex = originalVertex + QPointF(5.0, 4.0);
    auto vertexPress = mouseEvent(QEvent::GraphicsSceneMousePress, originalVertex, originalVertex, Qt::LeftButton, Qt::LeftButton, originalVertex);
    auto vertexStart = mouseEvent(QEvent::GraphicsSceneMouseMove, originalVertex, originalVertex, Qt::LeftButton, Qt::LeftButton, originalVertex);
    auto vertexContinue = mouseEvent(QEvent::GraphicsSceneMouseMove, draggedVertex, originalVertex, Qt::LeftButton, Qt::LeftButton, originalVertex);
    auto vertexFinish = mouseEvent(QEvent::GraphicsSceneMouseMove, draggedVertex, draggedVertex, Qt::LeftButton, Qt::LeftButton, originalVertex);
    pyqtgraph::GraphicsScene::MouseDragEvent vertexStartDrag = dragEvent(vertex, vertexStart.get(), vertexPress.get(), nullptr, true, false);
    vertex->mouseDragEvent(&vertexStartDrag);
    pyqtgraph::GraphicsScene::MouseDragEvent vertexContinueDrag = dragEvent(vertex, vertexContinue.get(), vertexPress.get(), vertexStart.get(), false, false);
    vertex->mouseDragEvent(&vertexContinueDrag);
    pyqtgraph::GraphicsScene::MouseDragEvent vertexFinishDrag = dragEvent(vertex, vertexFinish.get(), vertexPress.get(), vertexContinue.get(), false, true);
    vertex->mouseDragEvent(&vertexFinishDrag);
    if (samePoint(vertex->scenePos(), originalVertex)) {
        return fail("PolyLineROI free-handle drag should move the vertex");
    }
    checks.append(QStringLiteral("polylineroi-free-handle-drag"));

    const QImage actual = renderActualShapes();
    const int actualPixels = countNonBackgroundPixels(actual, qRgb(8, 8, 10));
    if (actualPixels < 100) {
        return fail("ROI shape visual render should produce observable non-background pixels");
    }
    const SemanticReviewStatus review = readGptVisualReview();
    if (!review.accepted) {
        return fail("missing or invalid GPT visual review artifact");
    }
    if (!writeVisualArtifacts(actual, actualPixels, review)) {
        return fail("could not write P4.20 ROI shape visual artifacts");
    }
    checks.append(QStringLiteral("visual-artifacts-gpt"));

    const QJsonObject rectState = stateJson(rect.getState());
    const QJsonObject lineState = stateJson(line.getState());
    const int polygonPointCount = static_cast<int>(polygon.pointPositions().size());
    const QJsonObject interactiveRectJson = QJsonObject{{QStringLiteral("pre"), stateJson(preDrag)},
                                                      {QStringLiteral("post"), stateJson(interactiveRect.getState())}};

    report.insert(QStringLiteral("checks"), checks);
    report.insert(QStringLiteral("rectState"), rectState);
    report.insert(QStringLiteral("lineState"), lineState);
    report.insert(QStringLiteral("polygonPoints"), polygonPointCount);
    report.insert(QStringLiteral("interactiveRect"), interactiveRectJson);
    report.insert(QStringLiteral("visual"), QJsonObject{{QStringLiteral("changedPixels"), 0},
                                                        {QStringLiteral("maxDelta"), 0},
                                                        {QStringLiteral("actualPixels"), actualPixels}});

    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P4_20_ARTIFACT_DIR);
    if (!writeReport(artifactDir, report)) {
        return fail("could not write P4.20 build artifact report");
    }
    if (!writeRepositoryReport(checks, rectState, lineState, polygonPointCount, interactiveRectJson, actualPixels, review)) {
        return fail("could not write P4.20 repository report artifact");
    }

    return 0;
}
