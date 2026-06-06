#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>
#include <pyqtgraph/graphicsItems/InfiniteLine.hpp>
#include <pyqtgraph/graphicsItems/LinearRegionItem.hpp>
#include <pyqtgraph/graphicsItems/TargetItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

using pyqtgraph::graphicsItems::InfiniteLine;
using pyqtgraph::graphicsItems::LinearRegionItem;
using pyqtgraph::graphicsItems::TargetItem;
using pyqtgraph::GraphicsScene::MouseDragEvent;

namespace {

struct LineSignals {
    int changed = 0;
    int dragged = 0;
    int finished = 0;
};

struct RegionSignals {
    int changed = 0;
    int finished = 0;
};

struct TargetSignals {
    int changed = 0;
    int finished = 0;
};

bool nearly(qreal actual, qreal expected, qreal tolerance = 1.0e-6)
{
    return std::abs(actual - expected) <= tolerance;
}

QJsonArray pointJson(const QPointF& point)
{
    QJsonArray array;
    array.append(point.x());
    array.append(point.y());
    return array;
}

QJsonArray regionJson(const std::pair<qreal, qreal>& region)
{
    QJsonArray array;
    array.append(region.first);
    array.append(region.second);
    return array;
}

QJsonObject lineSignalsJson(const LineSignals& log)
{
    return QJsonObject{{QStringLiteral("changed"), log.changed},
                       {QStringLiteral("dragged"), log.dragged},
                       {QStringLiteral("finished"), log.finished}};
}

QJsonObject regionSignalsJson(const RegionSignals& log)
{
    return QJsonObject{{QStringLiteral("changed"), log.changed}, {QStringLiteral("finished"), log.finished}};
}

QJsonObject targetSignalsJson(const TargetSignals& log)
{
    return QJsonObject{{QStringLiteral("changed"), log.changed}, {QStringLiteral("finished"), log.finished}};
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

void addEvent(QJsonArray& events, const QString& item, const QString& name, const QPointF& from, const QPointF& to)
{
    events.append(QJsonObject{{QStringLiteral("item"), item},
                              {QStringLiteral("name"), name},
                              {QStringLiteral("from"), pointJson(from)},
                              {QStringLiteral("to"), pointJson(to)}});
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

    QGraphicsScene scene;
    QJsonObject report;
    QJsonArray events;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.09"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/InfiniteLine.py:16-34,226-277,387-417; pyqtgraph/graphicsItems/LinearRegionItem.py:9-26,96-197,242-341; pyqtgraph/graphicsItems/TargetItem.py:15-23,147-171,249-289; tests/graphicsItems/test_InfiniteLine.py:8-93; tests/graphicsItems/test_LinearRegionItem.py:23-113"));

    InfiniteLine line(0.0, 90.0, true, std::make_pair(-5.0, 5.0));
    scene.addItem(&line);
    LineSignals lineSignals;
    QObject::connect(&line, &InfiniteLine::sigPositionChanged, &line, [&lineSignals](InfiniteLine*) { ++lineSignals.changed; });
    QObject::connect(&line, &InfiniteLine::sigDragged, &line, [&lineSignals](InfiniteLine*) { ++lineSignals.dragged; });
    QObject::connect(&line, &InfiniteLine::sigPositionChangeFinished, &line, [&lineSignals](InfiniteLine*) { ++lineSignals.finished; });

    report.insert(QStringLiteral("preState"), QJsonObject{{QStringLiteral("lineValue"), line.value()},
                                                 {QStringLiteral("region"), regionJson(std::make_pair(1.0, 3.0))},
                                                 {QStringLiteral("targetPos"), pointJson(QPointF(0.0, 0.0))}});

    auto linePress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(0.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    auto lineStartMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(0.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    auto lineFinishMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(3.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    addEvent(events, QStringLiteral("InfiniteLine"), QStringLiteral("left drag movable vertical line"), QPointF(0.0, 0.0), QPointF(3.0, 0.0));
    MouseDragEvent lineStart = dragEvent(&line, lineStartMove.get(), linePress.get(), nullptr, true, false);
    line.mouseDragEvent(&lineStart);
    MouseDragEvent lineFinish = dragEvent(&line, lineFinishMove.get(), linePress.get(), lineStartMove.get(), false, true);
    line.mouseDragEvent(&lineFinish);
    if (!lineStart.isAccepted() || !lineFinish.isAccepted()) {
        return fail("movable InfiniteLine drag should accept left-button events");
    }
    if (!nearly(line.value(), 3.0) || lineSignals.changed < 1 || lineSignals.dragged < 1 || lineSignals.finished != 1) {
        return fail("movable InfiniteLine drag should update value and emit changed/dragged/finished signals");
    }

    const int lineChangedBeforeNoOp = lineSignals.changed;
    const int lineDraggedBeforeNoOp = lineSignals.dragged;
    line.setMovable(false);
    auto lineNoOpMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(4.0, 0.0), QPointF(3.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(3.0, 0.0));
    MouseDragEvent lineNoOp = dragEvent(&line, lineNoOpMove.get(), linePress.get(), nullptr, true, false);
    addEvent(events, QStringLiteral("InfiniteLine"), QStringLiteral("non-movable left drag no-op"), QPointF(3.0, 0.0), QPointF(4.0, 0.0));
    line.mouseDragEvent(&lineNoOp);
    if (lineNoOp.isAccepted() || !nearly(line.value(), 3.0) || lineSignals.changed != lineChangedBeforeNoOp || lineSignals.dragged != lineDraggedBeforeNoOp) {
        return fail("non-movable InfiniteLine drag should be ignored without state or signal changes");
    }

    LinearRegionItem region(std::make_pair(1.0, 3.0), LinearRegionItem::Orientation::Vertical, true);
    scene.addItem(&region);
    RegionSignals regionSignals;
    QObject::connect(&region, &LinearRegionItem::sigRegionChanged, &region, [&regionSignals](LinearRegionItem*) { ++regionSignals.changed; });
    QObject::connect(&region, &LinearRegionItem::sigRegionChangeFinished, &region, [&regionSignals](LinearRegionItem*) { ++regionSignals.finished; });

    auto regionPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(0.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    auto regionStartMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(0.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    auto regionFinishMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(2.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    addEvent(events, QStringLiteral("LinearRegionItem"), QStringLiteral("left drag whole vertical region"), QPointF(0.0, 0.0), QPointF(2.0, 0.0));
    MouseDragEvent regionStart = dragEvent(&region, regionStartMove.get(), regionPress.get(), nullptr, true, false);
    region.mouseDragEvent(&regionStart);
    MouseDragEvent regionFinish = dragEvent(&region, regionFinishMove.get(), regionPress.get(), regionStartMove.get(), false, true);
    region.mouseDragEvent(&regionFinish);
    const auto draggedRegion = region.getRegion();
    if (!regionStart.isAccepted() || !regionFinish.isAccepted()) {
        return fail("movable LinearRegionItem drag should accept left-button events");
    }
    if (!nearly(draggedRegion.first, 3.0) || !nearly(draggedRegion.second, 5.0) || regionSignals.changed < 1 || regionSignals.finished != 1) {
        return fail("whole-region drag should shift both edges and emit changed/finished signals");
    }

    region.setRegion(std::make_pair(5.0, 2.0));
    const auto sortedRegion = region.getRegion();
    if (!nearly(sortedRegion.first, 2.0) || !nearly(sortedRegion.second, 5.0) || regionSignals.finished < 2) {
        return fail("sort-mode LinearRegionItem setRegion should return ascending region and emit finished");
    }
    const int regionChangedBeforeNoOp = regionSignals.changed;
    const int regionFinishedBeforeNoOp = regionSignals.finished;
    region.setRegion(std::make_pair(region.line(0)->value(), region.line(1)->value()));
    if (regionSignals.changed != regionChangedBeforeNoOp || regionSignals.finished != regionFinishedBeforeNoOp) {
        return fail("LinearRegionItem setRegion to existing line values should be a no-op");
    }

    TargetItem target(QPointF(0.0, 0.0), 10.0, true);
    scene.addItem(&target);
    TargetSignals targetSignals;
    QObject::connect(&target, &TargetItem::sigPositionChanged, &target, [&targetSignals](TargetItem*) { ++targetSignals.changed; });
    QObject::connect(&target, &TargetItem::sigPositionChangeFinished, &target, [&targetSignals](TargetItem*) { ++targetSignals.finished; });

    auto targetPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(0.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    auto targetStartMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(0.0, 0.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    auto targetFinishMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(4.0, 5.0), QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, QPointF(0.0, 0.0));
    addEvent(events, QStringLiteral("TargetItem"), QStringLiteral("left drag target"), QPointF(0.0, 0.0), QPointF(4.0, 5.0));
    MouseDragEvent targetStart = dragEvent(&target, targetStartMove.get(), targetPress.get(), nullptr, true, false);
    target.mouseDragEvent(&targetStart);
    MouseDragEvent targetFinish = dragEvent(&target, targetFinishMove.get(), targetPress.get(), targetStartMove.get(), false, true);
    target.mouseDragEvent(&targetFinish);
    if (!targetStart.isAccepted() || !targetFinish.isAccepted() || !nearly(target.pos().x(), 4.0) || !nearly(target.pos().y(), 5.0) || targetSignals.changed < 1 || targetSignals.finished != 1) {
        return fail("TargetItem drag should update position and emit changed/finished signals");
    }

    const int targetChangedBeforeNoOp = targetSignals.changed;
    target.setMovable(false);
    auto targetNoOpMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(6.0, 7.0), QPointF(4.0, 5.0), Qt::LeftButton, Qt::LeftButton, QPointF(4.0, 5.0));
    MouseDragEvent targetNoOp = dragEvent(&target, targetNoOpMove.get(), targetPress.get(), nullptr, true, false);
    addEvent(events, QStringLiteral("TargetItem"), QStringLiteral("non-movable target drag no-op"), QPointF(4.0, 5.0), QPointF(6.0, 7.0));
    target.mouseDragEvent(&targetNoOp);
    if (targetNoOp.isAccepted() || !nearly(target.pos().x(), 4.0) || !nearly(target.pos().y(), 5.0) || targetSignals.changed != targetChangedBeforeNoOp) {
        return fail("non-movable TargetItem drag should be ignored without state or signal changes");
    }

    report.insert(QStringLiteral("eventSequence"), events);
    report.insert(QStringLiteral("postState"), QJsonObject{{QStringLiteral("lineValue"), line.value()},
                                                  {QStringLiteral("region"), regionJson(region.getRegion())},
                                                  {QStringLiteral("targetPos"), pointJson(target.pos())}});
    report.insert(QStringLiteral("signals"), QJsonObject{{QStringLiteral("InfiniteLine"), lineSignalsJson(lineSignals)},
                                                {QStringLiteral("LinearRegionItem"), regionSignalsJson(regionSignals)},
                                                {QStringLiteral("TargetItem"), targetSignalsJson(targetSignals)}});
    report.insert(QStringLiteral("negativeNoOp"), QJsonObject{{QStringLiteral("lineAccepted"), lineNoOp.isAccepted()},
                                                    {QStringLiteral("lineValueAfterNoOp"), line.value()},
                                                    {QStringLiteral("targetAccepted"), targetNoOp.isAccepted()},
                                                    {QStringLiteral("targetPosAfterNoOp"), pointJson(target.pos())},
                                                    {QStringLiteral("regionNoOpChangedBefore"), regionChangedBeforeNoOp},
                                                    {QStringLiteral("regionNoOpChangedAfter"), regionSignals.changed},
                                                    {QStringLiteral("regionNoOpFinishedBefore"), regionFinishedBeforeNoOp},
                                                    {QStringLiteral("regionNoOpFinishedAfter"), regionSignals.finished}});

    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P4_09_ARTIFACT_DIR);
    QDir().mkpath(artifactDir);
    QFile file(artifactDir + QStringLiteral("/LinearRegion_InfiniteLine_TargetItem_interaction.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail("could not open P4.09 interaction report artifact");
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));

    return 0;
}
