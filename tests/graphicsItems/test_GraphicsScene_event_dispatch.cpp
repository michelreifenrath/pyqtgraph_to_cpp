#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneWheelEvent>
#include <QtWidgets/QGraphicsView>

#include <cstdlib>
#include <iostream>
#include <memory>

using pyqtgraph::GraphicsScene::GraphicsScene;
using pyqtgraph::GraphicsScene::GraphicsSceneEventHandler;
using pyqtgraph::GraphicsScene::HoverEvent;
using pyqtgraph::GraphicsScene::MouseClickEvent;
using pyqtgraph::GraphicsScene::MouseDragEvent;

namespace {

class ScriptableGraphicsScene : public GraphicsScene {
public:
    using GraphicsScene::GraphicsScene;
    using GraphicsScene::mouseMoveEvent;
    using GraphicsScene::mousePressEvent;
    using GraphicsScene::mouseReleaseEvent;
    using GraphicsScene::wheelEvent;
};

struct InteractionLog {
    int hoverSignals = 0;
    int movedSignals = 0;
    int clickedSignals = 0;
    int clickedAcceptedSignals = 0;
    int wheelCallbacks = 0;
    QPointF lastMoved;
    QGraphicsItem* lastClickedItem = nullptr;
};

class DispatchItem : public QGraphicsRectItem, public GraphicsSceneEventHandler {
public:
    DispatchItem(const QRectF& rect, bool claimInteractions, bool acceptClicks, bool acceptDrags)
        : QGraphicsRectItem(rect)
        , claimInteractions_(claimInteractions)
        , acceptClicks_(acceptClicks)
        , acceptDrags_(acceptDrags)
    {
        setAcceptedMouseButtons(Qt::NoButton);
    }

    void hoverEvent(HoverEvent* event) override
    {
        ++hoverCount;
        enterCount += event->isEnter() ? 1 : 0;
        exitCount += event->isExit() ? 1 : 0;
        if (claimInteractions_) {
            if (event->acceptClicks(Qt::LeftButton)) {
                ++clickClaims;
            } else {
                ++rejectedClickClaims;
            }
            if (event->acceptDrags(Qt::LeftButton)) {
                ++dragClaims;
            } else {
                ++rejectedDragClaims;
            }
        }
    }

    void mouseClickEvent(MouseClickEvent* event) override
    {
        ++clickCount;
        lastClickScenePos = event->scenePos();
        if (acceptClicks_) {
            event->accept();
        }
    }

    void mouseDragEvent(MouseDragEvent* event) override
    {
        ++dragCount;
        startDragCount += event->isStart() ? 1 : 0;
        finishDragCount += event->isFinish() ? 1 : 0;
        lastDragScenePos = event->scenePos();
        if (acceptDrags_) {
            event->accept();
        }
    }

    int hoverCount = 0;
    int enterCount = 0;
    int exitCount = 0;
    int clickClaims = 0;
    int dragClaims = 0;
    int rejectedClickClaims = 0;
    int rejectedDragClaims = 0;
    int clickCount = 0;
    int dragCount = 0;
    int startDragCount = 0;
    int finishDragCount = 0;
    QPointF lastClickScenePos;
    QPointF lastDragScenePos;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override { event->ignore(); }

private:
    bool claimInteractions_ = false;
    bool acceptClicks_ = false;
    bool acceptDrags_ = false;
};

class WheelItem : public QGraphicsRectItem {
public:
    explicit WheelItem(const QRectF& rect, InteractionLog& log)
        : QGraphicsRectItem(rect)
        , log_(log)
    {
        setAcceptedMouseButtons(Qt::NoButton);
    }

protected:
    void wheelEvent(QGraphicsSceneWheelEvent* event) override
    {
        ++log_.wheelCallbacks;
        event->accept();
    }

private:
    InteractionLog& log_;
};

std::unique_ptr<QGraphicsSceneMouseEvent> mouseEvent(QEvent::Type type,
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
    event->setButtonDownPos(Qt::LeftButton, buttonDownScenePos);
    event->setButtonDownScenePos(Qt::LeftButton, buttonDownScenePos);
    event->setButtonDownScreenPos(Qt::LeftButton, buttonDownScenePos.toPoint());
    event->ignore();
    return event;
}

std::unique_ptr<QGraphicsSceneWheelEvent> wheelEvent(const QPointF& scenePos, int delta)
{
    auto event = std::make_unique<QGraphicsSceneWheelEvent>(QEvent::GraphicsSceneWheel);
    event->setPos(scenePos);
    event->setScenePos(scenePos);
    event->setScreenPos(scenePos.toPoint());
    event->setDelta(delta);
    event->ignore();
    return event;
}

void addEvent(QJsonArray& events, const QString& name, const QJsonObject& details = {})
{
    QJsonObject event;
    event.insert(QStringLiteral("name"), name);
    for (auto it = details.begin(); it != details.end(); ++it) {
        event.insert(it.key(), it.value());
    }
    events.append(event);
}

int fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}

bool writeReport(const QJsonObject& report)
{
#ifdef PYQTGRAPH_CPP_P3_03_ARTIFACT_DIR
    const QString artifactDir = QString::fromUtf8(PYQTGRAPH_CPP_P3_03_ARTIFACT_DIR);
#else
    const QString artifactDir = QStringLiteral("artifacts/P3.03");
#endif
    QDir().mkpath(artifactDir);
    QFile file(artifactDir + QStringLiteral("/interaction_report.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << "failed to open interaction report: " << qPrintable(file.errorString()) << '\n';
        return false;
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    file.write("\n");
    return true;
}

QJsonObject itemState(const DispatchItem& item)
{
    return QJsonObject {
        {QStringLiteral("hoverCount"), item.hoverCount},
        {QStringLiteral("enterCount"), item.enterCount},
        {QStringLiteral("exitCount"), item.exitCount},
        {QStringLiteral("clickClaims"), item.clickClaims},
        {QStringLiteral("dragClaims"), item.dragClaims},
        {QStringLiteral("rejectedClickClaims"), item.rejectedClickClaims},
        {QStringLiteral("rejectedDragClaims"), item.rejectedDragClaims},
        {QStringLiteral("clickCount"), item.clickCount},
        {QStringLiteral("dragCount"), item.dragCount},
        {QStringLiteral("startDragCount"), item.startDragCount},
        {QStringLiteral("finishDragCount"), item.finishDragCount},
    };
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    ScriptableGraphicsScene scene(4, 5.0);
    QGraphicsView view(&scene);
    view.resize(160, 160);
    view.show();

    DispatchItem bottom(QRectF(10.0, 10.0, 30.0, 30.0), true, true, true);
    bottom.setZValue(0.0);
    DispatchItem top(QRectF(10.0, 10.0, 30.0, 30.0), true, true, true);
    top.setZValue(10.0);
    DispatchItem nearMiss(QRectF(55.0, 50.0, 1.0, 20.0), false, true, false);
    nearMiss.setZValue(1.0);

    InteractionLog log;
    WheelItem wheelReceiver(QRectF(90.0, 90.0, 20.0, 20.0), log);

    scene.addItem(&bottom);
    scene.addItem(&top);
    scene.addItem(&nearMiss);
    scene.addItem(&wheelReceiver);

    QObject::connect(&scene, &GraphicsScene::sigMouseMoved, &scene, [&log](const QPointF& pos) {
        ++log.movedSignals;
        log.lastMoved = pos;
    });
    QObject::connect(&scene, &GraphicsScene::sigMouseHover, &scene, [&log](const QList<QGraphicsItem*>& items) {
        ++log.hoverSignals;
        if (items.isEmpty()) {
            std::cerr << "hover signal emitted without items\n";
        }
    });
    QObject::connect(&scene, &GraphicsScene::sigMouseClicked, &scene, [&log](MouseClickEvent* event) {
        ++log.clickedSignals;
        log.clickedAcceptedSignals += event->isAccepted() ? 1 : 0;
        log.lastClickedItem = event->acceptedItem();
    });

    QJsonObject report;
    QJsonArray events;
    report.insert(QStringLiteral("issue"), QStringLiteral("P3.03"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/GraphicsScene/GraphicsScene.py:18-31,74-80,138-243,250-387,401-469; pyqtgraph/GraphicsScene/mouseEvents.py:248-320"));
    report.insert(QStringLiteral("preState"), QJsonObject {
        {QStringLiteral("top"), itemState(top)},
        {QStringLiteral("bottom"), itemState(bottom)},
        {QStringLiteral("nearMiss"), itemState(nearMiss)},
        {QStringLiteral("signals"), QJsonObject{{QStringLiteral("moved"), 0}, {QStringLiteral("hover"), 0}, {QStringLiteral("clicked"), 0}}},
    });

    addEvent(events, QStringLiteral("hover move over overlapping items"), QJsonObject{{QStringLiteral("scenePos"), QStringLiteral("15,15")}});
    auto hoverMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(15.0, 15.0), QPointF(0.0, 0.0), Qt::NoButton, Qt::NoButton, QPointF(15.0, 15.0));
    scene.mouseMoveEvent(hoverMove.get());
    if (log.movedSignals != 1 || log.hoverSignals != 1 || log.lastMoved != QPointF(15.0, 15.0)) {
        return fail("mouse move should emit moved and hover signals with scene position");
    }
    if (top.hoverCount != 1 || top.enterCount != 1 || top.clickClaims != 1 || top.dragClaims != 1) {
        return fail("top hover item should enter and claim click/drag");
    }
    if (bottom.hoverCount != 1 || bottom.enterCount != 1 || bottom.rejectedClickClaims != 1 || bottom.rejectedDragClaims != 1) {
        return fail("lower hover item should receive hover but lose duplicate claims");
    }

    addEvent(events, QStringLiteral("claimed click press/release"), QJsonObject{{QStringLiteral("scenePos"), QStringLiteral("15,15")}});
    auto clickPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(15.0, 15.0), QPointF(15.0, 15.0), Qt::LeftButton, Qt::LeftButton, QPointF(15.0, 15.0));
    scene.mousePressEvent(clickPress.get());
    auto clickRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(15.0, 15.0), QPointF(15.0, 15.0), Qt::LeftButton, Qt::NoButton, QPointF(15.0, 15.0));
    scene.mouseReleaseEvent(clickRelease.get());
    if (top.clickCount != 1 || bottom.clickCount != 0 || log.clickedSignals != 1 || log.clickedAcceptedSignals != 1 || log.lastClickedItem != &top) {
        return fail("claimed click should dispatch only to top item and emit accepted signal");
    }

    addEvent(events, QStringLiteral("claimed drag start/finish"), QJsonObject{{QStringLiteral("from"), QStringLiteral("16,16")}, {QStringLiteral("to"), QStringLiteral("30,16")}});
    auto dragPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(16.0, 16.0), QPointF(16.0, 16.0), Qt::LeftButton, Qt::LeftButton, QPointF(16.0, 16.0));
    scene.mousePressEvent(dragPress.get());
    auto dragStart = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(30.0, 16.0), QPointF(16.0, 16.0), Qt::NoButton, Qt::LeftButton, QPointF(16.0, 16.0));
    scene.mouseMoveEvent(dragStart.get());
    auto dragContinue = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(34.0, 16.0), QPointF(30.0, 16.0), Qt::NoButton, Qt::LeftButton, QPointF(16.0, 16.0));
    scene.mouseMoveEvent(dragContinue.get());
    auto dragRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(34.0, 16.0), QPointF(34.0, 16.0), Qt::LeftButton, Qt::NoButton, QPointF(16.0, 16.0));
    scene.mouseReleaseEvent(dragRelease.get());
    if (top.dragCount != 3 || top.startDragCount != 1 || top.finishDragCount != 1 || bottom.dragCount != 0) {
        return fail("claimed drag should send start, continuation, and finish to top item only");
    }

    addEvent(events, QStringLiteral("click-radius hit-test"), QJsonObject{{QStringLiteral("scenePos"), QStringLiteral("53,60")}});
    auto nearPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(53.0, 60.0), QPointF(53.0, 60.0), Qt::LeftButton, Qt::LeftButton, QPointF(53.0, 60.0));
    scene.mousePressEvent(nearPress.get());
    auto nearRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(53.0, 60.0), QPointF(53.0, 60.0), Qt::LeftButton, Qt::NoButton, QPointF(53.0, 60.0));
    scene.mouseReleaseEvent(nearRelease.get());
    if (nearMiss.clickCount != 1 || log.lastClickedItem != &nearMiss) {
        return fail("click radius should dispatch to eligible near-miss item");
    }

    addEvent(events, QStringLiteral("negative outside click radius"), QJsonObject{{QStringLiteral("scenePos"), QStringLiteral("80,80")}});
    const int clicksBeforeNegative = top.clickCount + bottom.clickCount + nearMiss.clickCount;
    auto negativePress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(80.0, 80.0), QPointF(80.0, 80.0), Qt::LeftButton, Qt::LeftButton, QPointF(80.0, 80.0));
    scene.mousePressEvent(negativePress.get());
    auto negativeRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(80.0, 80.0), QPointF(80.0, 80.0), Qt::LeftButton, Qt::NoButton, QPointF(80.0, 80.0));
    scene.mouseReleaseEvent(negativeRelease.get());
    if ((top.clickCount + bottom.clickCount + nearMiss.clickCount) != clicksBeforeNegative || log.clickedSignals != 3 || log.clickedAcceptedSignals != 2) {
        return fail("outside-radius negative click should emit unaccepted signal without item callback");
    }

    addEvent(events, QStringLiteral("wheel passthrough"), QJsonObject{{QStringLiteral("scenePos"), QStringLiteral("95,95")}, {QStringLiteral("delta"), 120}});
    auto wheel = wheelEvent(QPointF(95.0, 95.0), 120);
    scene.wheelEvent(wheel.get());
    if (log.wheelCallbacks != 1 || !wheel->isAccepted()) {
        return fail("wheel event should continue through normal QGraphicsScene delivery");
    }

    report.insert(QStringLiteral("eventSequence"), events);
    report.insert(QStringLiteral("postState"), QJsonObject {
        {QStringLiteral("top"), itemState(top)},
        {QStringLiteral("bottom"), itemState(bottom)},
        {QStringLiteral("nearMiss"), itemState(nearMiss)},
        {QStringLiteral("signals"), QJsonObject{
            {QStringLiteral("moved"), log.movedSignals},
            {QStringLiteral("hover"), log.hoverSignals},
            {QStringLiteral("clicked"), log.clickedSignals},
            {QStringLiteral("clickedAccepted"), log.clickedAcceptedSignals},
            {QStringLiteral("wheelCallbacks"), log.wheelCallbacks},
        }},
        {QStringLiteral("negativeNoOp"), QJsonObject{{QStringLiteral("outsideRadiusItemCallbacks"), 0}, {QStringLiteral("unacceptedSignalEmitted"), true}}},
    });
    report.insert(QStringLiteral("result"), QStringLiteral("passed"));

    if (!writeReport(report)) {
        return 1;
    }

    return 0;
}
