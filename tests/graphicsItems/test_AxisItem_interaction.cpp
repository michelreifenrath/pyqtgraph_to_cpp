#include <cppqtgraph/graphicsItems/AxisItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QPointF>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneWheelEvent>

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

using cppqtgraph::graphicsItems::AxisItem;
using cppqtgraph::graphicsItems::PlotItem;
using cppqtgraph::graphicsItems::ViewBox;

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

class ScriptableAxisItem : public AxisItem {
public:
    using AxisItem::AxisItem;
    using AxisItem::mouseMoveEvent;
    using AxisItem::mousePressEvent;
    using AxisItem::mouseReleaseEvent;
    using AxisItem::wheelEvent;
};

class ScriptableViewBox : public ViewBox {
public:
    using ViewBox::mouseMoveEvent;
    using ViewBox::mousePressEvent;
    using ViewBox::mouseReleaseEvent;
    using ViewBox::wheelEvent;
};

bool nearlyEqual(qreal lhs, qreal rhs, qreal tolerance = 1.0e-6)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool axisRangeUnchanged(const ViewBox::AxisRange& before, const ViewBox::AxisRange& after)
{
    return nearlyEqual(before[0], after[0]) && nearlyEqual(before[1], after[1]);
}

qreal span(const ViewBox::AxisRange& axis)
{
    return axis[1] - axis[0];
}

std::unique_ptr<QGraphicsSceneWheelEvent> makeWheelEvent(const QPointF& scenePos, int delta)
{
    auto event = std::make_unique<QGraphicsSceneWheelEvent>(QEvent::GraphicsSceneWheel);
    event->setPos(scenePos);
    event->setScenePos(scenePos);
    event->setDelta(delta);
    return event;
}

std::unique_ptr<QGraphicsSceneMouseEvent> makeMouseEvent(QEvent::Type type,
                                                         const QPointF& scenePos,
                                                         const QPointF& lastScenePos,
                                                         Qt::MouseButton button,
                                                         Qt::MouseButtons buttons,
                                                         const QPointF& buttonDownScenePos)
{
    auto event = std::make_unique<QGraphicsSceneMouseEvent>(type);
    event->setScenePos(scenePos);
    event->setLastScenePos(lastScenePos);
    event->setButton(button);
    event->setButtons(buttons);
    event->setButtonDownScenePos(button, buttonDownScenePos);
    event->setScreenPos(scenePos.toPoint());
    event->setLastScreenPos(lastScenePos.toPoint());
    event->setButtonDownScreenPos(button, buttonDownScenePos.toPoint());
    return event;
}

void setItemPosFromScene(QGraphicsSceneMouseEvent& event, QGraphicsItem& item, const QPointF& scenePos)
{
    event.setPos(item.mapFromScene(scenePos));
    event.setLastPos(item.mapFromScene(event.lastScenePos()));
    event.setButtonDownPos(event.button(), item.mapFromScene(event.buttonDownScenePos(event.button())));
}

struct AxisInteractionFixture {
    QGraphicsScene scene;
    ScriptableViewBox viewBox;
    ScriptableAxisItem bottomAxis{AxisItem::Orientation::Bottom};
    ScriptableAxisItem leftAxis{AxisItem::Orientation::Left};

    AxisInteractionFixture()
    {
        scene.addItem(&viewBox);
        scene.addItem(&bottomAxis);
        scene.addItem(&leftAxis);

        leftAxis.resize(48.0, 180.0);
        viewBox.resize(280.0, 180.0);
        bottomAxis.resize(280.0, 48.0);

        leftAxis.setPos(0.0, 0.0);
        viewBox.setPos(48.0, 0.0);
        bottomAxis.setPos(48.0, 180.0);

        viewBox.setDefaultPadding(0.0);
        viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
        bottomAxis.setRange(0.0, 10.0);
        leftAxis.setRange(0.0, 10.0);
        bottomAxis.linkToView(&viewBox);
        leftAxis.linkToView(&viewBox);
    }

    [[nodiscard]] QPointF bottomSceneCenter() const
    {
        return bottomAxis.mapToScene(bottomAxis.boundingRect().center());
    }

    [[nodiscard]] QPointF leftSceneCenter() const
    {
        return leftAxis.mapToScene(leftAxis.boundingRect().center());
    }

    [[nodiscard]] QPointF viewSceneCenter() const
    {
        return viewBox.mapToScene(viewBox.boundingRect().center());
    }
};

bool testPlotItemLinksAxesToViewBox()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(320.0, 240.0);

    CHECK(plot.getAxis(QStringLiteral("bottom"))->linkedView() == plot.getViewBox());
    CHECK(plot.getAxis(QStringLiteral("left"))->linkedView() == plot.getViewBox());
    return true;
}

bool testBottomAxisDragChangesOnlyX()
{
    AxisInteractionFixture fixture;
    const auto before = fixture.viewBox.viewRange();

    const QPointF start = fixture.bottomSceneCenter();
    const QPointF end = start + QPointF(40.0, 0.0);

    auto press = makeMouseEvent(
        QEvent::GraphicsSceneMousePress, start, start, Qt::LeftButton, Qt::LeftButton, start);
    auto move = makeMouseEvent(
        QEvent::GraphicsSceneMouseMove, end, start, Qt::LeftButton, Qt::LeftButton, start);
    auto release = makeMouseEvent(
        QEvent::GraphicsSceneMouseRelease, end, end, Qt::LeftButton, Qt::NoButton, start);

    setItemPosFromScene(*press, fixture.bottomAxis, start);
    setItemPosFromScene(*move, fixture.bottomAxis, end);
    setItemPosFromScene(*release, fixture.bottomAxis, end);

    fixture.bottomAxis.mousePressEvent(press.get());
    fixture.bottomAxis.mouseMoveEvent(move.get());
    fixture.bottomAxis.mouseReleaseEvent(release.get());

    const auto after = fixture.viewBox.viewRange();
    CHECK(!axisRangeUnchanged(before[ViewBox::XAxis], after[ViewBox::XAxis]));
    CHECK(axisRangeUnchanged(before[ViewBox::YAxis], after[ViewBox::YAxis]));
    return true;
}

bool testLeftAxisDragChangesOnlyY()
{
    AxisInteractionFixture fixture;
    const auto before = fixture.viewBox.viewRange();

    const QPointF start = fixture.leftSceneCenter();
    const QPointF end = start + QPointF(0.0, 35.0);

    auto press = makeMouseEvent(
        QEvent::GraphicsSceneMousePress, start, start, Qt::LeftButton, Qt::LeftButton, start);
    auto move = makeMouseEvent(
        QEvent::GraphicsSceneMouseMove, end, start, Qt::LeftButton, Qt::LeftButton, start);
    auto release = makeMouseEvent(
        QEvent::GraphicsSceneMouseRelease, end, end, Qt::LeftButton, Qt::NoButton, start);

    setItemPosFromScene(*press, fixture.leftAxis, start);
    setItemPosFromScene(*move, fixture.leftAxis, end);
    setItemPosFromScene(*release, fixture.leftAxis, end);

    fixture.leftAxis.mousePressEvent(press.get());
    fixture.leftAxis.mouseMoveEvent(move.get());
    fixture.leftAxis.mouseReleaseEvent(release.get());

    const auto after = fixture.viewBox.viewRange();
    CHECK(axisRangeUnchanged(before[ViewBox::XAxis], after[ViewBox::XAxis]));
    CHECK(!axisRangeUnchanged(before[ViewBox::YAxis], after[ViewBox::YAxis]));
    return true;
}

bool testBottomAxisWheelZoomsOnlyX()
{
    AxisInteractionFixture fixture;
    const auto before = fixture.viewBox.viewRange();

    const QPointF scenePos = fixture.bottomSceneCenter();
    auto wheel = makeWheelEvent(scenePos, 120);
    wheel->setPos(fixture.bottomAxis.mapFromScene(scenePos));

    fixture.bottomAxis.wheelEvent(wheel.get());
    CHECK(wheel->isAccepted());

    const auto after = fixture.viewBox.viewRange();
    CHECK(span(after[ViewBox::XAxis]) < span(before[ViewBox::XAxis]));
    CHECK(axisRangeUnchanged(before[ViewBox::YAxis], after[ViewBox::YAxis]));
    return true;
}

bool testLeftAxisWheelZoomsOnlyY()
{
    AxisInteractionFixture fixture;
    const auto before = fixture.viewBox.viewRange();

    const QPointF scenePos = fixture.leftSceneCenter();
    auto wheel = makeWheelEvent(scenePos, 120);
    wheel->setPos(fixture.leftAxis.mapFromScene(scenePos));

    fixture.leftAxis.wheelEvent(wheel.get());
    CHECK(wheel->isAccepted());

    const auto after = fixture.viewBox.viewRange();
    CHECK(axisRangeUnchanged(before[ViewBox::XAxis], after[ViewBox::XAxis]));
    CHECK(span(after[ViewBox::YAxis]) < span(before[ViewBox::YAxis]));
    return true;
}

bool testPlotAreaInteractionUnchanged()
{
    AxisInteractionFixture fixture;
    const auto before = fixture.viewBox.viewRange();

    const QPointF start = fixture.viewSceneCenter();
    const QPointF end = start + QPointF(30.0, 20.0);

    auto press = makeMouseEvent(
        QEvent::GraphicsSceneMousePress, start, start, Qt::LeftButton, Qt::LeftButton, start);
    auto move = makeMouseEvent(
        QEvent::GraphicsSceneMouseMove, end, start, Qt::LeftButton, Qt::LeftButton, start);
    auto release = makeMouseEvent(
        QEvent::GraphicsSceneMouseRelease, end, end, Qt::LeftButton, Qt::NoButton, start);

    setItemPosFromScene(*press, fixture.viewBox, start);
    setItemPosFromScene(*move, fixture.viewBox, end);
    setItemPosFromScene(*release, fixture.viewBox, end);

    fixture.viewBox.mousePressEvent(press.get());
    fixture.viewBox.mouseMoveEvent(move.get());
    fixture.viewBox.mouseReleaseEvent(release.get());

    const auto after = fixture.viewBox.viewRange();
    CHECK(!axisRangeUnchanged(before[ViewBox::XAxis], after[ViewBox::XAxis]));
    CHECK(!axisRangeUnchanged(before[ViewBox::YAxis], after[ViewBox::YAxis]));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard app(argc, argv);

    if (!testPlotItemLinksAxesToViewBox()) {
        return 1;
    }
    if (!testBottomAxisDragChangesOnlyX()) {
        return 1;
    }
    if (!testLeftAxisDragChangesOnlyY()) {
        return 1;
    }
    if (!testBottomAxisWheelZoomsOnlyX()) {
        return 1;
    }
    if (!testLeftAxisWheelZoomsOnlyY()) {
        return 1;
    }
    if (!testPlotAreaInteractionUnchanged()) {
        return 1;
    }

    return 0;
}
