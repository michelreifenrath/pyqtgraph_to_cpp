#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsView>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

using pyqtgraph::GraphicsScene::GraphicsScene;
using pyqtgraph::GraphicsScene::GraphicsSceneEventHandler;
using pyqtgraph::GraphicsScene::HoverEvent;
using pyqtgraph::GraphicsScene::MouseClickEvent;
using pyqtgraph::GraphicsScene::MouseDragEvent;

namespace {

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

bool samePoint(const QPointF& actual, qreal expectedX, qreal expectedY)
{
    return qFuzzyCompare(actual.x() + 1.0, expectedX + 1.0)
        && qFuzzyCompare(actual.y() + 1.0, expectedY + 1.0);
}

QPointF toPointF(const pyqtgraph::Point& point)
{
    return QPointF(point.x(), point.y());
}

struct SignalLog {
    int moved = 0;
    int hover = 0;
    int clicked = 0;
    int acceptedClicked = 0;
    int unacceptedClicked = 0;
    QPointF lastMoved;
    QGraphicsItem* lastClickedItem = nullptr;
};

class DispatchItem : public QGraphicsRectItem, public GraphicsSceneEventHandler {
public:
    explicit DispatchItem(const QString& name, QPointF scenePosition, qreal zValue)
        : QGraphicsRectItem(QRectF(0.0, 0.0, 30.0, 30.0))
        , name_(name)
    {
        setPos(scenePosition);
        setZValue(zValue);
        setAcceptedMouseButtons(Qt::NoButton);
    }

    void hoverEvent(HoverEvent* event) override
    {
        ++hoverCount;
        enterCount += event->isEnter() ? 1 : 0;
        exitCount += event->isExit() ? 1 : 0;
        lastHoverLocalPos = toPointF(event->pos());

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

    void mouseClickEvent(MouseClickEvent* event) override
    {
        ++clickCount;
        lastClickLocalPos = toPointF(event->pos());
        lastClickScenePos = toPointF(event->scenePos());
        event->accept();
    }

    void mouseDragEvent(MouseDragEvent* event) override
    {
        ++dragCount;
        startDragCount += event->isStart() ? 1 : 0;
        finishDragCount += event->isFinish() ? 1 : 0;
        lastDragLocalPos = toPointF(event->pos());
        lastDragButtonDownLocalPos = toPointF(event->buttonDownPos());
        event->accept();
    }

    QString name() const { return name_; }

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
    QPointF lastHoverLocalPos;
    QPointF lastClickLocalPos;
    QPointF lastClickScenePos;
    QPointF lastDragLocalPos;
    QPointF lastDragButtonDownLocalPos;

private:
    QString name_;
};

void addEvent(QJsonArray& events, const QString& name, const QJsonObject& details = {})
{
    QJsonObject event;
    event.insert(QStringLiteral("name"), name);
    for (auto it = details.begin(); it != details.end(); ++it) {
        event.insert(it.key(), it.value());
    }
    events.append(event);
}

QJsonObject pointObject(const QPointF& point)
{
    return QJsonObject {
        {QStringLiteral("x"), point.x()},
        {QStringLiteral("y"), point.y()},
    };
}

QJsonObject itemState(const DispatchItem& item)
{
    return QJsonObject {
        {QStringLiteral("name"), item.name()},
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
        {QStringLiteral("lastHoverLocalPos"), pointObject(item.lastHoverLocalPos)},
        {QStringLiteral("lastClickLocalPos"), pointObject(item.lastClickLocalPos)},
        {QStringLiteral("lastClickScenePos"), pointObject(item.lastClickScenePos)},
        {QStringLiteral("lastDragLocalPos"), pointObject(item.lastDragLocalPos)},
        {QStringLiteral("lastDragButtonDownLocalPos"), pointObject(item.lastDragButtonDownLocalPos)},
    };
}

QJsonObject signalState(const SignalLog& log)
{
    const QString lastClickedItem = log.lastClickedItem == nullptr
        ? QStringLiteral("none")
        : QStringLiteral("accepted-item");
    return QJsonObject {
        {QStringLiteral("moved"), log.moved},
        {QStringLiteral("hover"), log.hover},
        {QStringLiteral("clicked"), log.clicked},
        {QStringLiteral("acceptedClicked"), log.acceptedClicked},
        {QStringLiteral("unacceptedClicked"), log.unacceptedClicked},
        {QStringLiteral("lastMoved"), pointObject(log.lastMoved)},
        {QStringLiteral("lastClickedItem"), lastClickedItem},
    };
}

void sendMouseToViewport(QGraphicsView& view,
    QEvent::Type type,
    const QPointF& scenePos,
    Qt::MouseButton button,
    Qt::MouseButtons buttons)
{
    const QPoint viewportPoint = view.mapFromScene(scenePos);
    const QPoint globalPoint = view.viewport()->mapToGlobal(viewportPoint);
    QMouseEvent event(type,
        QPointF(viewportPoint),
        QPointF(globalPoint),
        button,
        buttons,
        Qt::NoModifier);
    QCoreApplication::sendEvent(view.viewport(), &event);
    QCoreApplication::processEvents();
}

bool writeReport(const QJsonObject& report)
{
#ifdef PYQTGRAPH_CPP_P3_04_ARTIFACT_DIR
    const QString artifactDir = QString::fromUtf8(PYQTGRAPH_CPP_P3_04_ARTIFACT_DIR);
#else
    const QString artifactDir = QStringLiteral("artifacts/P3.04");
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

bool runP304InteractionProof()
{
    GraphicsScene scene(4, 5.0);
    scene.setSceneRect(QRectF(0.0, 0.0, 160.0, 160.0));

    QGraphicsView view(&scene);
    view.setSceneRect(scene.sceneRect());
    view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setMouseTracking(true);
    view.viewport()->setMouseTracking(true);
    view.resize(200, 200);
    view.show();
    QCoreApplication::processEvents();

    DispatchItem bottom(QStringLiteral("bottom"), QPointF(10.0, 10.0), 0.0);
    DispatchItem top(QStringLiteral("top"), QPointF(10.0, 10.0), 10.0);
    scene.addItem(&bottom);
    scene.addItem(&top);

    SignalLog signalLog;
    QObject::connect(&scene, &GraphicsScene::sigMouseMoved, &scene, [&signalLog](const QPointF& pos) {
        ++signalLog.moved;
        signalLog.lastMoved = pos;
    });
    QObject::connect(&scene, &GraphicsScene::sigMouseHover, &scene, [&signalLog](const QList<QGraphicsItem*>& items) {
        ++signalLog.hover;
        if (items.isEmpty()) {
            std::cerr << "sigMouseHover emitted no items for an in-item hover move\n";
        }
    });
    QObject::connect(&scene, &GraphicsScene::sigMouseClicked, &scene, [&signalLog](MouseClickEvent* event) {
        ++signalLog.clicked;
        if (event->isAccepted()) {
            ++signalLog.acceptedClicked;
        } else {
            ++signalLog.unacceptedClicked;
        }
        signalLog.lastClickedItem = event->acceptedItem();
    });

    QJsonObject report;
    QJsonArray eventSequence;
    report.insert(QStringLiteral("issue"), QStringLiteral("P3.04"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/GraphicsScene/GraphicsScene.py:47-71,138-243,250-387,401-469; pyqtgraph/GraphicsScene/mouseEvents.py:10-128,155-241,246-380"));
    report.insert(QStringLiteral("dispatchPath"), QStringLiteral("QMouseEvent sent to QGraphicsView viewport; QGraphicsView dispatches QGraphicsSceneMouseEvent to pyqtgraph::GraphicsScene"));
    report.insert(QStringLiteral("preState"), QJsonObject {
        {QStringLiteral("top"), itemState(top)},
        {QStringLiteral("bottom"), itemState(bottom)},
        {QStringLiteral("signals"), signalState(signalLog)},
    });

    addEvent(eventSequence, QStringLiteral("hover-over-overlapping-items"), QJsonObject {
        {QStringLiteral("scenePos"), QStringLiteral("15,15")},
        {QStringLiteral("expected"), QStringLiteral("top and bottom receive hover; top claims left click/drag; bottom claim attempts are rejected")},
    });
    sendMouseToViewport(view, QEvent::MouseMove, QPointF(15.0, 15.0), Qt::NoButton, Qt::NoButton);
    CHECK(signalLog.moved == 1);
    CHECK(signalLog.hover == 1);
    CHECK(samePoint(signalLog.lastMoved, 15.0, 15.0));
    CHECK(top.hoverCount == 1);
    CHECK(top.enterCount == 1);
    CHECK(top.clickClaims == 1);
    CHECK(top.dragClaims == 1);
    CHECK(samePoint(top.lastHoverLocalPos, 5.0, 5.0));
    CHECK(bottom.hoverCount == 1);
    CHECK(bottom.enterCount == 1);
    CHECK(bottom.rejectedClickClaims == 1);
    CHECK(bottom.rejectedDragClaims == 1);

    addEvent(eventSequence, QStringLiteral("claimed-click-press-release"), QJsonObject {
        {QStringLiteral("scenePos"), QStringLiteral("15,15")},
        {QStringLiteral("expected"), QStringLiteral("hover-claimed left click dispatches only to top item and emits accepted sigMouseClicked")},
    });
    sendMouseToViewport(view, QEvent::MouseButtonPress, QPointF(15.0, 15.0), Qt::LeftButton, Qt::LeftButton);
    sendMouseToViewport(view, QEvent::MouseButtonRelease, QPointF(15.0, 15.0), Qt::LeftButton, Qt::NoButton);
    CHECK(top.clickCount == 1);
    CHECK(bottom.clickCount == 0);
    CHECK(samePoint(top.lastClickLocalPos, 5.0, 5.0));
    CHECK(samePoint(top.lastClickScenePos, 15.0, 15.0));
    CHECK(signalLog.clicked == 1);
    CHECK(signalLog.acceptedClicked == 1);
    CHECK(signalLog.lastClickedItem == &top);

    addEvent(eventSequence, QStringLiteral("claimed-drag-press-move-release"), QJsonObject {
        {QStringLiteral("from"), QStringLiteral("15,15")},
        {QStringLiteral("to"), QStringLiteral("28,15")},
        {QStringLiteral("expected"), QStringLiteral("hover-claimed drag sends start and finish wrappers only to top item")},
    });
    sendMouseToViewport(view, QEvent::MouseButtonPress, QPointF(15.0, 15.0), Qt::LeftButton, Qt::LeftButton);
    sendMouseToViewport(view, QEvent::MouseMove, QPointF(28.0, 15.0), Qt::NoButton, Qt::LeftButton);
    sendMouseToViewport(view, QEvent::MouseButtonRelease, QPointF(28.0, 15.0), Qt::LeftButton, Qt::NoButton);
    CHECK(top.dragCount == 2);
    CHECK(top.startDragCount == 1);
    CHECK(top.finishDragCount == 1);
    CHECK(bottom.dragCount == 0);
    CHECK(samePoint(top.lastDragLocalPos, 18.0, 5.0));
    CHECK(samePoint(top.lastDragButtonDownLocalPos, 5.0, 5.0));

    addEvent(eventSequence, QStringLiteral("negative-outside-click-radius"), QJsonObject {
        {QStringLiteral("scenePos"), QStringLiteral("90,90")},
        {QStringLiteral("expected"), QStringLiteral("no item callback; sigMouseClicked still emits an unaccepted wrapper")},
    });
    const int itemClicksBeforeNegative = top.clickCount + bottom.clickCount;
    sendMouseToViewport(view, QEvent::MouseButtonPress, QPointF(90.0, 90.0), Qt::LeftButton, Qt::LeftButton);
    sendMouseToViewport(view, QEvent::MouseButtonRelease, QPointF(90.0, 90.0), Qt::LeftButton, Qt::NoButton);
    CHECK(top.clickCount + bottom.clickCount == itemClicksBeforeNegative);
    CHECK(signalLog.clicked == 2);
    CHECK(signalLog.acceptedClicked == 1);
    CHECK(signalLog.unacceptedClicked == 1);
    CHECK(signalLog.lastClickedItem == nullptr);

    report.insert(QStringLiteral("eventSequence"), eventSequence);
    report.insert(QStringLiteral("postState"), QJsonObject {
        {QStringLiteral("top"), itemState(top)},
        {QStringLiteral("bottom"), itemState(bottom)},
        {QStringLiteral("signals"), signalState(signalLog)},
        {QStringLiteral("negativeNoOp"), QJsonObject {
            {QStringLiteral("outsideRadiusItemCallbacks"), 0},
            {QStringLiteral("unacceptedSignalEmitted"), true},
        }},
    });
    report.insert(QStringLiteral("result"), QStringLiteral("passed"));

    CHECK(writeReport(report));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    ApplicationGuard application(argc, argv);

    return runP304InteractionProof() ? 0 : 1;
}
