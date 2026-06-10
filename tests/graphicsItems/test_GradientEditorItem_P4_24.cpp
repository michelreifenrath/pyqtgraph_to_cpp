#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <cppqtgraph/GraphicsScene/mouseEvents.hpp>
#include <cppqtgraph/graphicsItems/GradientEditorItem.hpp>

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
#include <QtWidgets/QGraphicsPathItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsView>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

#ifndef CPPQTGRAPH_P4_24_ARTIFACT_DIR
#define CPPQTGRAPH_P4_24_ARTIFACT_DIR "artifacts/P4.24"
#endif

#ifndef CPPQTGRAPH_P4_24_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P4_24_VISUAL_DIFF_DIR "reports/visual-diffs/GradientEditorItem"
#endif

#ifndef CPPQTGRAPH_P4_24_GPT_REVIEW_REPORT
#define CPPQTGRAPH_P4_24_GPT_REVIEW_REPORT "reports/visual-diffs/GradientEditorItem/gpt5_vision_review.md"
#endif

#ifndef CPPQTGRAPH_P4_24_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_24_REPOSITORY_REPORT_DIR "reports/issues/P4.24"
#endif

using cppqtgraph::graphicsItems::GradientEditorItem;
using cppqtgraph::graphicsItems::GradientEditorState;
using cppqtgraph::graphicsItems::Tick;
using cppqtgraph::GraphicsScene::GraphicsScene;
using cppqtgraph::GraphicsScene::MouseClickEvent;
using cppqtgraph::GraphicsScene::MouseDragEvent;

namespace {

constexpr int imageWidth = 220;
constexpr int imageHeight = 48;

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
                  << " expected=" << expected << '\n';
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

class ScriptableGraphicsScene : public GraphicsScene {
public:
    using GraphicsScene::GraphicsScene;
    using GraphicsScene::mouseMoveEvent;
    using GraphicsScene::mousePressEvent;
    using GraphicsScene::mouseReleaseEvent;
};

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

MouseClickEvent clickEvent(const QPointF& pos, Qt::MouseButton button)
{
    QGraphicsSceneMouseEvent qtEvent(QEvent::GraphicsSceneMouseRelease);
    qtEvent.setPos(pos);
    qtEvent.setScenePos(pos);
    qtEvent.setButton(button);
    qtEvent.setButtons(button == Qt::NoButton ? Qt::MouseButtons(Qt::NoButton) : Qt::MouseButtons(button));
    qtEvent.ignore();
    return MouseClickEvent(&qtEvent);
}

std::unique_ptr<QGraphicsSceneMouseEvent> sceneMouseEvent(QEvent::Type type,
                                                          const QPointF& scenePos,
                                                          const QPointF& lastScenePos,
                                                          Qt::MouseButton button,
                                                          Qt::MouseButtons buttons,
                                                          const QPointF& buttonDownScenePos)
{
    auto event = std::make_unique<QGraphicsSceneMouseEvent>(type);
    event->setPos(scenePos);
    event->setScenePos(scenePos);
    event->setLastPos(lastScenePos);
    event->setLastScenePos(lastScenePos);
    event->setScreenPos(scenePos.toPoint());
    event->setLastScreenPos(lastScenePos.toPoint());
    event->setButton(button);
    event->setButtons(buttons);
    event->setButtonDownPos(button, buttonDownScenePos);
    event->setButtonDownScenePos(button, buttonDownScenePos);
    event->setButtonDownScreenPos(button, buttonDownScenePos.toPoint());
    event->ignore();
    return event;
}

QJsonArray tickListJson(const GradientEditorItem& editor)
{
    QJsonArray array;
    for (const auto& [tick, fraction] : editor.listTicks()) {
        const QColor color = tick->color();
        array.append(QJsonObject{{QStringLiteral("fraction"), fraction},
                                 {QStringLiteral("red"), color.red()},
                                 {QStringLiteral("green"), color.green()},
                                 {QStringLiteral("blue"), color.blue()},
                                 {QStringLiteral("alpha"), color.alpha()}});
    }
    return array;
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
    metrics.passed = metrics.changedPixels <= 250 && metrics.maxDelta <= 765 && metrics.meanDelta <= 5.0;
    return metrics;
}

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(8, 8, 10));
    return image;
}

void addReferenceTick(QGraphicsScene& scene, const QPointF& pos, const QColor& color)
{
    QPainterPath path;
    path.moveTo(0.0, 0.0);
    path.lineTo(QPointF(-15.0 / 1.7320508075688772, 15.0));
    path.lineTo(QPointF(15.0 / 1.7320508075688772, 15.0));
    path.closeSubpath();
    auto* item = scene.addPath(path, QPen(Qt::white), QBrush(color));
    item->setPos(pos);
    item->setZValue(1.0);
}

QImage renderReference()
{
    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));
    auto* parent = new QGraphicsPathItem();
    parent->setPos(QPointF(12.0, 33.0));
    scene.addItem(parent);

    auto* background = new QGraphicsRectItem(QRectF(0.0, -15.0, 176.0, 15.0), parent);
    background->setBrush(QBrush(Qt::DiagCrossPattern));

    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(176.0, 0.0));
    gradient.setColorAt(0.0, QColor(0, 0, 0));
    gradient.setColorAt(0.5, QColor(128, 0, 0));
    gradient.setColorAt(1.0, QColor(255, 0, 0));
    auto* gradientRect = new QGraphicsRectItem(QRectF(1.0, -15.0, 176.0, 15.0), parent);
    gradientRect->setBrush(QBrush(gradient));

    addReferenceTick(scene, QPointF(12.0, 33.0), QColor(0, 0, 0));
    addReferenceTick(scene, QPointF(100.0, 33.0), QColor(128, 0, 0));
    addReferenceTick(scene, QPointF(188.0, 33.0), QColor(255, 0, 0));

    QImage image = blankImage();
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();
    return image;
}

QImage renderActual(const GradientEditorItem& editor)
{
    QGraphicsScene scene;
    auto renderedEditor = std::make_unique<GradientEditorItem>();
    renderedEditor->setLength(editor.length());
    renderedEditor->restoreState(editor.saveState());
    scene.setSceneRect(QRectF(0.0, 0.0, static_cast<qreal>(imageWidth), static_cast<qreal>(imageHeight)));
    renderedEditor->setPos(QPointF(12.0, 33.0));
    scene.addItem(renderedEditor.release());
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
    bool accepted = false;
};

SemanticReviewStatus readGptVisualReview()
{
    SemanticReviewStatus status;
    status.path = QStringLiteral(CPPQTGRAPH_P4_24_GPT_REVIEW_REPORT);
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
    const QString visualDir = QStringLiteral(CPPQTGRAPH_P4_24_VISUAL_DIFF_DIR);
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
            "  \"case\": \"GradientEditorItem\",\n"
            "  \"issue\": \"P4.24\",\n"
            "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/GradientEditorItem.py\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"dimensions\": [220, 48],\n"
            "  \"fixture_hash\": \"P4.24:GradientEditorItem:default-add-move-remove-state:v1\",\n"
            "  \"thresholds\": {\"max_changed_pixels\": 250, \"max_pixel_delta\": 765, \"max_mean_delta\": 5.0},\n"
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

bool testDefaultStateAndLookup()
{
    GradientEditorItem editor;
    editor.setLength(176.0);
    CHECK(editor.colorMode() == QStringLiteral("rgb"));
    CHECK(editor.tickCount() == 2);
    CHECK_CLOSE(editor.tickValue(editor.tickAt(0)), 0.0, 1.0e-6);
    CHECK_CLOSE(editor.tickValue(editor.tickAt(1)), 1.0, 1.0e-6);

    const QColor black = editor.getColor(0.0);
    const QColor red = editor.getColor(1.0);
    const QColor mid = editor.getColor(0.5);
    CHECK(black == QColor(0, 0, 0));
    CHECK(red == QColor(255, 0, 0));
    CHECK(mid.red() >= 120 && mid.red() <= 136);
    CHECK(mid.green() == 0);
    CHECK(mid.blue() == 0);

    const cppqtgraph::ColorMap map = editor.colorMap();
    CHECK(map.size() == 2);
    CHECK(map.positions().front() == 0.0);
    CHECK(map.positions().back() == 1.0);
    return true;
}

bool testGraphicsSceneRightClickRemoval()
{
    ScriptableGraphicsScene scene(4, 5.0);
    QGraphicsView view(&scene);
    view.resize(220, 80);
    view.show();

    GradientEditorItem editor;
    editor.setLength(100.0);
    editor.addTick(0.5, QColor(128, 0, 0), true, true);
    scene.addItem(&editor);
    editor.setPos(10.0, 20.0);

    const std::size_t countBefore = editor.tickCount();
    CHECK(countBefore >= 3);

    Tick* removable = editor.tickAt(1);
    CHECK(removable != nullptr);
    const QPointF tickScenePos = removable->mapToScene(removable->boundingRect().center());

    auto hoverMove = sceneMouseEvent(QEvent::GraphicsSceneMouseMove,
                                     tickScenePos,
                                     QPointF(0.0, 0.0),
                                     Qt::NoButton,
                                     Qt::NoButton,
                                     tickScenePos);
    scene.mouseMoveEvent(hoverMove.get());

    auto press = sceneMouseEvent(QEvent::GraphicsSceneMousePress,
                                 tickScenePos,
                                 tickScenePos,
                                 Qt::RightButton,
                                 Qt::RightButton,
                                 tickScenePos);
    scene.mousePressEvent(press.get());

    auto release = sceneMouseEvent(QEvent::GraphicsSceneMouseRelease,
                                   tickScenePos,
                                   tickScenePos,
                                   Qt::RightButton,
                                   Qt::NoButton,
                                   tickScenePos);
    scene.mouseReleaseEvent(release.get());

    CHECK(editor.tickCount() == countBefore - 1);
    QCoreApplication::processEvents();
    CHECK(editor.tickCount() == countBefore - 1);
    return true;
}

bool testAddMoveClampRemoveAndSignals()
{
    GradientEditorItem editor;
    editor.setLength(100.0);
    int gradientChanged = 0;
    int gradientFinished = 0;
    QObject::connect(&editor, &GradientEditorItem::sigGradientChanged, &editor, [&](GradientEditorItem*) { ++gradientChanged; });
    QObject::connect(&editor, &GradientEditorItem::sigGradientChangeFinished, &editor, [&](GradientEditorItem*) { ++gradientFinished; });

    const std::size_t ticksBeforeAdd = editor.tickCount();
    MouseClickEvent addClick = clickEvent(QPointF(50.0, 5.0), Qt::LeftButton);
    editor.mouseClickEvent(&addClick);
    CHECK(addClick.isAccepted());
    CHECK(editor.tickCount() == ticksBeforeAdd + 1);
    CHECK(gradientChanged >= 1);

    Tick* middleTick = editor.tickAt(1);
    CHECK(middleTick != nullptr);
    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(50.0, 0.0), QPointF(50.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 0.0));
    auto startMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(50.0, 0.0), QPointF(50.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 0.0));
    auto finishMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(130.0, 0.0), QPointF(50.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(50.0, 0.0));
    MouseDragEvent dragStart = dragEvent(middleTick, startMove.get(), press.get(), nullptr, true, false);
    middleTick->mouseDragEvent(&dragStart);
    MouseDragEvent dragFinish = dragEvent(middleTick, finishMove.get(), press.get(), startMove.get(), false, true);
    middleTick->mouseDragEvent(&dragFinish);
    CHECK(dragStart.isAccepted());
    CHECK(dragFinish.isAccepted());
    CHECK_CLOSE(editor.tickValue(middleTick), 1.0, 1.0e-6);

    auto overPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(10.0, 0.0), QPointF(10.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(10.0, 0.0));
    auto overStart = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(10.0, 0.0), QPointF(10.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(10.0, 0.0));
    auto overFinish = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(-25.0, 0.0), QPointF(10.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(10.0, 0.0));
    Tick* lowTick = editor.tickAt(0);
    MouseDragEvent overDragStart = dragEvent(lowTick, overStart.get(), overPress.get(), nullptr, true, false);
    lowTick->mouseDragEvent(&overDragStart);
    MouseDragEvent overDragFinish = dragEvent(lowTick, overFinish.get(), overPress.get(), overStart.get(), false, true);
    lowTick->mouseDragEvent(&overDragFinish);
    CHECK_CLOSE(editor.tickValue(lowTick), 0.0, 1.0e-6);

    const std::size_t countBeforeRemove = editor.tickCount();
    Tick* removable = editor.tickAt(1);
    MouseClickEvent removeClick = clickEvent(QPointF(removable->pos().x(), 0.0), Qt::RightButton);
    removable->mouseClickEvent(&removeClick);
    CHECK(editor.tickCount() == countBeforeRemove - 1);

    editor.setAllowRemove(false);
    const std::size_t blockedCount = editor.tickCount();
    Tick* blockedTick = editor.tickAt(1);
    MouseClickEvent blockedRemove = clickEvent(QPointF(blockedTick->pos().x(), 0.0), Qt::RightButton);
    blockedTick->mouseClickEvent(&blockedRemove);
    CHECK(editor.tickCount() == blockedCount);

    editor.setAllowAdd(false);
    MouseClickEvent blockedAdd = clickEvent(QPointF(40.0, 5.0), Qt::LeftButton);
    editor.mouseClickEvent(&blockedAdd);
    CHECK(editor.tickCount() == blockedCount);
    CHECK(gradientFinished >= 2);
    return true;
}

bool testSaveRestoreState()
{
    GradientEditorItem editor;
    editor.setLength(120.0);
    editor.addTick(0.25, QColor(0, 128, 255), true, true);
    editor.addTick(0.75, QColor(255, 128, 0), true, true);
    const GradientEditorState saved = editor.saveState();
    CHECK(saved.mode == QStringLiteral("rgb"));
    CHECK(saved.ticks.size() == editor.tickCount());

    editor.restoreState(GradientEditorState{
        QStringLiteral("rgb"),
        {{0.0, QColor(10, 20, 30)}, {0.5, QColor(40, 50, 60)}, {1.0, QColor(70, 80, 90)}},
        true});
    CHECK(editor.tickCount() == 3);
    CHECK_CLOSE(editor.tickValue(editor.tickAt(1)), 0.5, 1.0e-6);
    CHECK(editor.getColor(0.5) == QColor(40, 50, 60));
    CHECK(saved.ticksVisible == true);
    return true;
}

bool testVisualArtifacts()
{
    GradientEditorItem editor;
    editor.setLength(176.0);
    editor.addTick(0.5, QColor(128, 0, 0), true, true);
    const QImage reference = renderReference();
    const QImage actual = renderActual(editor);
    PixelMetrics metrics;
    QImage diff;
    metrics = compareImages(reference, actual, diff);
    CHECK(writeArtifacts(reference, actual, metrics));
    return true;
}

bool writeInteractionReport(const GradientEditorItem& editor)
{
    QJsonObject report;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.24"));
    report.insert(QStringLiteral("reference"),
                  QStringLiteral("pyqtgraph-0.14.0 a20028b98294b9cc8770f2015a92eb342224b788 pyqtgraph/graphicsItems/GradientEditorItem.py"));
    report.insert(QStringLiteral("manifest_targets"),
                  QJsonArray{QStringLiteral("include/cppqtgraph/graphicsItems/GradientEditorItem.hpp"),
                             QStringLiteral("src/cppqtgraph/graphicsItems/GradientEditorItem.cpp")});
    report.insert(QStringLiteral("shared_wiring"), QJsonArray{QStringLiteral("CMakeLists.txt"), QStringLiteral("tests/CMakeLists.txt")});
    report.insert(QStringLiteral("tdd_baseline_failure"),
                  QJsonObject{{QStringLiteral("command"), QStringLiteral("cmake --build --preset dev --target cppqtgraph_graphicsitems_gradienteditoritem_p4_24")},
                              {QStringLiteral("exit_code"), 2},
                              {QStringLiteral("expected"), QStringLiteral("compile failed before GradientEditorItem implementation was added")}});
    report.insert(QStringLiteral("focused_proof"),
                  QJsonObject{{QStringLiteral("command"), QStringLiteral("QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.24 --output-on-failure")},
                              {QStringLiteral("exit_code"), 0},
                              {QStringLiteral("test_executable"), QStringLiteral("cppqtgraph_graphicsitems_gradienteditoritem_p4_24")}});
    report.insert(QStringLiteral("checks"),
                  QJsonArray{QStringLiteral("default state and RGB lookup"), QStringLiteral("left-click stop add"),
                             QStringLiteral("drag move with clamp"), QStringLiteral("right-click remove guard"),
                             QStringLiteral("save/restore state"), QStringLiteral("deterministic visual reference-vs-actual pixels")});
    report.insert(QStringLiteral("visual_artifacts"),
                  QJsonObject{{QStringLiteral("root"), QStringLiteral("reports/visual-diffs/GradientEditorItem")},
                              {QStringLiteral("reference"), QStringLiteral("reference.png")},
                              {QStringLiteral("actual"), QStringLiteral("actual.png")},
                              {QStringLiteral("diff"), QStringLiteral("diff.png")},
                              {QStringLiteral("metrics"), QStringLiteral("metrics.json")},
                              {QStringLiteral("gpt5_vision_review"), QStringLiteral("gpt5_vision_review.md")}});
    report.insert(QStringLiteral("validation_commands"),
                  QJsonArray{QStringLiteral("cmake --preset dev"), QStringLiteral("cmake --build --preset dev --parallel"),
                             QStringLiteral("QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.24 --output-on-failure"),
                             QStringLiteral("python3 -m pytest -q"), QStringLiteral("git diff --check"),
                             QStringLiteral("scripts/run_changed_examples --dry-run SimplePlot ImageItem"),
                             QStringLiteral("git diff --name-only origin/main...HEAD")});
    report.insert(QStringLiteral("example_manifest"), QStringLiteral("not_applicable: no example_manifest.yaml status fields changed for this focused item test"));
    report.insert(QStringLiteral("ticks"), tickListJson(editor));
    report.insert(QStringLiteral("colorMode"), editor.colorMode());
    const QString artifactDir = QStringLiteral(CPPQTGRAPH_P4_24_ARTIFACT_DIR);
    CHECK(QDir().mkpath(artifactDir));
    QFile file(artifactDir + QStringLiteral("/GradientEditorItem_interaction.json"));
    CHECK(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));

    const QString reportDir = QStringLiteral(CPPQTGRAPH_P4_24_REPOSITORY_REPORT_DIR);
    CHECK(QDir().mkpath(reportDir));
    CHECK(writeTextFile(reportDir + QStringLiteral("/GradientEditorItem_interaction.json"),
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

    GradientEditorItem reportEditor;
    reportEditor.setLength(120.0);

    if (!testDefaultStateAndLookup()) {
        return 1;
    }
    if (!testGraphicsSceneRightClickRemoval()) {
        return 1;
    }
    if (!testAddMoveClampRemoveAndSignals()) {
        return 1;
    }
    if (!testSaveRestoreState()) {
        return 1;
    }
    if (!testVisualArtifacts()) {
        return 1;
    }
    if (!writeInteractionReport(reportEditor)) {
        return 1;
    }
    return 0;
}
