#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneWheelEvent>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>

using ViewBox = pyqtgraph::graphicsItems::ViewBox;

namespace {

class ScriptableViewBox : public ViewBox {
public:
    using ViewBox::mouseMoveEvent;
    using ViewBox::mousePressEvent;
    using ViewBox::mouseReleaseEvent;
    using ViewBox::wheelEvent;
};

struct SignalLog {
    int xRangeChanged = 0;
    int yRangeChanged = 0;
    int rangeChanged = 0;
    int manualChanged = 0;
    int stateChanged = 0;
    int transformChanged = 0;
    int resized = 0;
    ViewBox::Range2D lastRange{};
    std::array<bool, 2> lastChanged{{false, false}};
    std::array<bool, 2> lastManualMask{{false, false}};
};

QJsonArray axisJson(const ViewBox::AxisRange& axis)
{
    QJsonArray array;
    array.append(axis[0]);
    array.append(axis[1]);
    return array;
}

QJsonObject rangeJson(const ViewBox::Range2D& range)
{
    QJsonObject object;
    object.insert(QStringLiteral("x"), axisJson(range[ViewBox::XAxis]));
    object.insert(QStringLiteral("y"), axisJson(range[ViewBox::YAxis]));
    return object;
}

QJsonArray maskJson(const std::array<bool, 2>& mask)
{
    QJsonArray array;
    array.append(mask[ViewBox::XAxis]);
    array.append(mask[ViewBox::YAxis]);
    return array;
}

qreal span(const ViewBox::AxisRange& axis)
{
    return axis[1] - axis[0];
}

qreal center(const ViewBox::AxisRange& axis)
{
    return axis[0] + span(axis) / 2.0;
}

bool nearly(qreal actual, qreal expected, qreal tolerance = 1.0e-6)
{
    return std::abs(actual - expected) <= tolerance;
}

bool rangeNearly(const ViewBox::AxisRange& actual, const ViewBox::AxisRange& expected, qreal tolerance = 1.0e-6)
{
    return nearly(actual[0], expected[0], tolerance) && nearly(actual[1], expected[1], tolerance);
}

void connectLog(ViewBox& viewBox, SignalLog& log)
{
    QObject::connect(&viewBox, &ViewBox::sigXRangeChanged, &viewBox, [&log](ViewBox*, ViewBox::AxisRange) {
        ++log.xRangeChanged;
    });
    QObject::connect(&viewBox, &ViewBox::sigYRangeChanged, &viewBox, [&log](ViewBox*, ViewBox::AxisRange) {
        ++log.yRangeChanged;
    });
    QObject::connect(&viewBox, &ViewBox::sigRangeChanged, &viewBox, [&log](ViewBox*, ViewBox::Range2D range, std::array<bool, 2> changed) {
        ++log.rangeChanged;
        log.lastRange = range;
        log.lastChanged = changed;
    });
    QObject::connect(&viewBox, &ViewBox::sigRangeChangedManually, &viewBox, [&log](std::array<bool, 2> mask) {
        ++log.manualChanged;
        log.lastManualMask = mask;
    });
    QObject::connect(&viewBox, &ViewBox::sigStateChanged, &viewBox, [&log](ViewBox*) {
        ++log.stateChanged;
    });
    QObject::connect(&viewBox, &ViewBox::sigTransformChanged, &viewBox, [&log](ViewBox*) {
        ++log.transformChanged;
    });
    QObject::connect(&viewBox, &ViewBox::sigResized, &viewBox, [&log](ViewBox*) {
        ++log.resized;
    });
}

std::unique_ptr<QGraphicsSceneWheelEvent> wheelEvent(const QPointF& pos, int delta)
{
    auto event = std::make_unique<QGraphicsSceneWheelEvent>(QEvent::GraphicsSceneWheel);
    event->setPos(pos);
    event->setScenePos(pos);
    event->setDelta(delta);
    return event;
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

void addEvent(QJsonArray& events, const QString& name, const QJsonObject& details = {})
{
    QJsonObject event;
    event.insert(QStringLiteral("name"), name);
    for (auto it = details.begin(); it != details.end(); ++it) {
        event.insert(it.key(), it.value());
    }
    events.append(event);
}

QJsonObject signalJson(const SignalLog& log)
{
    QJsonObject object;
    object.insert(QStringLiteral("xRangeChanged"), log.xRangeChanged);
    object.insert(QStringLiteral("yRangeChanged"), log.yRangeChanged);
    object.insert(QStringLiteral("rangeChanged"), log.rangeChanged);
    object.insert(QStringLiteral("manualChanged"), log.manualChanged);
    object.insert(QStringLiteral("stateChanged"), log.stateChanged);
    object.insert(QStringLiteral("transformChanged"), log.transformChanged);
    object.insert(QStringLiteral("resized"), log.resized);
    object.insert(QStringLiteral("lastChanged"), maskJson(log.lastChanged));
    object.insert(QStringLiteral("lastManualMask"), maskJson(log.lastManualMask));
    object.insert(QStringLiteral("lastRange"), rangeJson(log.lastRange));
    return object;
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
    ScriptableViewBox viewBox;
    scene.addItem(&viewBox);
    viewBox.resize(200.0, 100.0);
    viewBox.setDefaultPadding(0.0);
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);

    SignalLog log;
    connectLog(viewBox, log);

    QJsonObject report;
    QJsonArray events;
    report.insert(QStringLiteral("issue"), QStringLiteral("P3.06"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ViewBox/ViewBox.py:90-96, 393-405, 1015-1132, 1297-1316, 1335-1395, 1575-1703"));
    report.insert(QStringLiteral("preState"), rangeJson(viewBox.viewRange()));
    report.insert(QStringLiteral("preMouseEnabled"), maskJson(viewBox.mouseEnabled()));

    const auto beforeWheel = viewBox.viewRange();
    auto wheel = wheelEvent(QPointF(100.0, 50.0), 120);
    addEvent(events, QStringLiteral("wheel zoom"), QJsonObject{{QStringLiteral("delta"), 120}});
    viewBox.wheelEvent(wheel.get());
    const auto afterWheel = viewBox.viewRange();
    if (!wheel->isAccepted()) {
        return fail("wheel interaction should accept enabled mouse event");
    }
    if (!(span(afterWheel[ViewBox::XAxis]) < span(beforeWheel[ViewBox::XAxis]) && span(afterWheel[ViewBox::YAxis]) < span(beforeWheel[ViewBox::YAxis]))) {
        return fail("wheel interaction should zoom both enabled axes");
    }
    if (!nearly(center(afterWheel[ViewBox::XAxis]), center(beforeWheel[ViewBox::XAxis]), 1.0e-5)) {
        return fail("centered wheel interaction should preserve x center");
    }
    if (log.manualChanged < 1 || !log.lastManualMask[ViewBox::XAxis] || !log.lastManualMask[ViewBox::YAxis]) {
        return fail("wheel interaction should emit manual range signal for both axes");
    }

    const auto beforePan = viewBox.viewRange();
    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(100.0, 50.0), QPointF(100.0, 50.0), Qt::LeftButton, Qt::LeftButton, QPointF(100.0, 50.0));
    auto move = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(125.0, 50.0), QPointF(100.0, 50.0), Qt::LeftButton, Qt::LeftButton, QPointF(100.0, 50.0));
    auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(125.0, 50.0), QPointF(125.0, 50.0), Qt::LeftButton, Qt::NoButton, QPointF(100.0, 50.0));
    addEvent(events, QStringLiteral("left drag pan"), QJsonObject{{QStringLiteral("from"), QStringLiteral("100,50")}, {QStringLiteral("to"), QStringLiteral("125,50")}});
    viewBox.mousePressEvent(press.get());
    viewBox.mouseMoveEvent(move.get());
    viewBox.mouseReleaseEvent(release.get());
    const auto afterPan = viewBox.viewRange();
    if (!press->isAccepted() || !move->isAccepted() || !release->isAccepted()) {
        return fail("pan drag events should be accepted");
    }
    if (nearly(center(afterPan[ViewBox::XAxis]), center(beforePan[ViewBox::XAxis]), 1.0e-5)) {
        return fail("left drag should pan x range");
    }
    if (log.transformChanged < 1) {
        return fail("range-changing interactions should emit transform changed signal");
    }

    const auto beforeRectZoom = viewBox.viewRange();
    viewBox.setMouseMode(ViewBox::RectMode);
    auto rectPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(25.0, 25.0), QPointF(25.0, 25.0), Qt::LeftButton, Qt::LeftButton, QPointF(25.0, 25.0));
    auto rectMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(175.0, 75.0), QPointF(25.0, 25.0), Qt::LeftButton, Qt::LeftButton, QPointF(25.0, 25.0));
    auto rectRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(175.0, 75.0), QPointF(175.0, 75.0), Qt::LeftButton, Qt::NoButton, QPointF(25.0, 25.0));
    addEvent(events, QStringLiteral("left drag rect zoom"));
    viewBox.mousePressEvent(rectPress.get());
    viewBox.mouseMoveEvent(rectMove.get());
    viewBox.mouseReleaseEvent(rectRelease.get());
    const auto afterRectZoom = viewBox.viewRange();
    if (!rectPress->isAccepted() || !rectMove->isAccepted() || !rectRelease->isAccepted()) {
        return fail("rect mode drag events should be accepted");
    }
    if (!(span(afterRectZoom[ViewBox::XAxis]) < span(beforeRectZoom[ViewBox::XAxis]) && span(afterRectZoom[ViewBox::YAxis]) < span(beforeRectZoom[ViewBox::YAxis]))) {
        return fail("rect mode left drag should zoom both axes");
    }
    viewBox.setMouseMode(ViewBox::PanMode);

    const auto beforeRightDrag = viewBox.viewRange();
    auto rightPress = mouseEvent(QEvent::GraphicsSceneMousePress, QPointF(100.0, 50.0), QPointF(100.0, 50.0), Qt::RightButton, Qt::RightButton, QPointF(100.0, 50.0));
    auto rightMove = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(115.0, 60.0), QPointF(100.0, 50.0), Qt::RightButton, Qt::RightButton, QPointF(100.0, 50.0));
    auto rightRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(115.0, 60.0), QPointF(115.0, 60.0), Qt::RightButton, Qt::NoButton, QPointF(100.0, 50.0));
    addEvent(events, QStringLiteral("right drag scale"), QJsonObject{{QStringLiteral("from"), QStringLiteral("100,50")}, {QStringLiteral("to"), QStringLiteral("115,60")}});
    viewBox.mousePressEvent(rightPress.get());
    viewBox.mouseMoveEvent(rightMove.get());
    viewBox.mouseReleaseEvent(rightRelease.get());
    const auto afterRightDrag = viewBox.viewRange();
    if (!(span(afterRightDrag[ViewBox::XAxis]) != span(beforeRightDrag[ViewBox::XAxis]) || span(afterRightDrag[ViewBox::YAxis]) != span(beforeRightDrag[ViewBox::YAxis]))) {
        return fail("right drag should scale at least one axis");
    }

    ScriptableViewBox source;
    ScriptableViewBox follower;
    scene.addItem(&source);
    scene.addItem(&follower);
    source.setPos(300.0, 0.0);
    follower.setPos(600.0, 0.0);
    source.resize(200.0, 100.0);
    follower.resize(200.0, 100.0);
    source.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    follower.setRange(QRectF(50.0, 0.0, 10.0, 10.0), 0.0);
    SignalLog followerLog;
    connectLog(follower, followerLog);
    addEvent(events, QStringLiteral("setXLink then source setXRange"));
    follower.setXLink(&source);
    source.setXRange(20.0, 30.0, 0.0);
    if (!rangeNearly(follower.viewRange()[ViewBox::XAxis], ViewBox::AxisRange{20.0, 30.0})) {
        return fail("linked x view should follow source x range");
    }
    if (followerLog.xRangeChanged < 1 || followerLog.rangeChanged < 1) {
        return fail("linked view update should emit x and aggregate range signals");
    }

    follower.setXLink(nullptr);
    const auto unlinkedRange = follower.viewRange();
    addEvent(events, QStringLiteral("unlink then source setXRange no-op for follower"));
    source.setXRange(100.0, 110.0, 0.0);
    if (!rangeNearly(follower.viewRange()[ViewBox::XAxis], unlinkedRange[ViewBox::XAxis])) {
        return fail("unlinked follower should not update when source range changes");
    }

    ScriptableViewBox ySource;
    ScriptableViewBox yFollower;
    scene.addItem(&ySource);
    scene.addItem(&yFollower);
    ySource.setPos(0.0, 200.0);
    yFollower.setPos(0.0, 250.0);
    ySource.resize(100.0, 100.0);
    yFollower.resize(100.0, 100.0);
    ySource.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    yFollower.setRange(QRectF(0.0, 50.0, 10.0, 10.0), 0.0);
    addEvent(events, QStringLiteral("overlapped y link alignment"));
    yFollower.setYLink(&ySource);
    if (!rangeNearly(yFollower.viewRange()[ViewBox::YAxis], ViewBox::AxisRange{-5.0, 5.0})) {
        return fail("overlapping y-linked follower should account for inverted screen-y direction");
    }

    viewBox.setMouseEnabled(false, false);
    const auto beforeDisabled = viewBox.viewRange();
    const int manualBeforeDisabled = log.manualChanged;
    auto disabledWheel = wheelEvent(QPointF(100.0, 50.0), 120);
    addEvent(events, QStringLiteral("disabled mouse wheel no-op"));
    viewBox.wheelEvent(disabledWheel.get());
    if (disabledWheel->isAccepted()) {
        return fail("disabled wheel should be ignored");
    }
    if (viewBox.viewRange() != beforeDisabled || log.manualChanged != manualBeforeDisabled) {
        return fail("disabled wheel should not change range or emit manual signal");
    }

    report.insert(QStringLiteral("eventSequence"), events);
    report.insert(QStringLiteral("postState"), rangeJson(viewBox.viewRange()));
    report.insert(QStringLiteral("signals"), signalJson(log));
    report.insert(QStringLiteral("linkedFollowerPostState"), rangeJson(follower.viewRange()));
    report.insert(QStringLiteral("linkedSignals"), signalJson(followerLog));
    report.insert(QStringLiteral("negativeNoOp"), QJsonObject{{QStringLiteral("disabledWheelAccepted"), disabledWheel->isAccepted()}, {QStringLiteral("manualCountBefore"), manualBeforeDisabled}, {QStringLiteral("manualCountAfter"), log.manualChanged}});

    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P3_06_ARTIFACT_DIR);
    QDir().mkpath(artifactDir);
    QFile file(artifactDir + QStringLiteral("/ViewBox_interaction_linking.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail("could not open P3.06 interaction report artifact");
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));

    return 0;
}
