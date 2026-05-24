#include <pyqtgraph/GraphicsScene/mouseEvents.hpp>

#include <QtCore/QPoint>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

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

bool samePoint(const pyqtgraph::Point& actual, qreal x, qreal y)
{
    return qFuzzyCompare(actual.x(), x) && qFuzzyCompare(actual.y(), y);
}

bool testMouseClickEventShapeAndAccessors()
{
    using pyqtgraph::GraphicsScene::MouseClickEvent;

    static_assert(std::is_constructible_v<MouseClickEvent, QGraphicsSceneMouseEvent*>);
    static_assert(std::is_constructible_v<MouseClickEvent, QGraphicsSceneMouseEvent*, bool>);
    static_assert(std::is_destructible_v<MouseClickEvent>);

    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setScenePos(QPointF(12.5, 24.0));
    pressEvent.setScreenPos(QPoint(125, 240));
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setButtons(Qt::LeftButton | Qt::RightButton);
    pressEvent.setModifiers(Qt::ShiftModifier | Qt::ControlModifier);
    pressEvent.setTimestamp(1234);

    MouseClickEvent event(&pressEvent, true);
    CHECK(samePoint(event.scenePos(), 12.5, 24.0));
    CHECK(samePoint(event.screenPos(), 125.0, 240.0));
    CHECK(event.button() == Qt::LeftButton);
    CHECK(event.buttons() == (Qt::LeftButton | Qt::RightButton));
    CHECK(event.modifiers() == (Qt::ShiftModifier | Qt::ControlModifier));
    CHECK(event.doubleClick());
    CHECK(event.time() == 1234);
    CHECK(!event.isAccepted());
    CHECK(event.currentItem() == nullptr);
    CHECK(event.acceptedItem() == nullptr);

    QGraphicsRectItem item;
    item.setPos(10.0, 20.0);
    event.setCurrentItem(&item);
    CHECK(event.currentItem() == &item);
    CHECK(samePoint(event.pos(), 2.5, 4.0));
    CHECK(samePoint(event.lastPos(), 2.5, 4.0));

    event.accept();
    CHECK(event.isAccepted());
    CHECK(event.acceptedItem() == &item);
    event.ignore();
    CHECK(!event.isAccepted());
    CHECK(event.acceptedItem() == nullptr);
    event.accept(nullptr);
    CHECK(event.acceptedItem() == &item);

    QGraphicsRectItem other;
    event.accept(&other);
    CHECK(event.acceptedItem() == &other);

    return true;
}

bool testMouseDragEventShapeAndAccessors()
{
    using pyqtgraph::GraphicsScene::MouseClickEvent;
    using pyqtgraph::GraphicsScene::MouseDragEvent;

    static_assert(std::is_constructible_v<MouseDragEvent, QGraphicsSceneMouseEvent*, QGraphicsSceneMouseEvent*, QGraphicsSceneMouseEvent*>);
    static_assert(std::is_constructible_v<MouseDragEvent, QGraphicsSceneMouseEvent*, QGraphicsSceneMouseEvent*, QGraphicsSceneMouseEvent*, bool, bool>);
    static_assert(std::is_constructible_v<MouseDragEvent, QGraphicsSceneMouseEvent*, const MouseClickEvent&, const MouseDragEvent*>);
    static_assert(std::is_constructible_v<MouseDragEvent, QGraphicsSceneMouseEvent*, const MouseClickEvent&, const MouseDragEvent*, bool, bool>);
    static_assert(std::is_destructible_v<MouseDragEvent>);

    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setScenePos(QPointF(10.0, 20.0));
    pressEvent.setScreenPos(QPoint(100, 200));
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setButtons(Qt::LeftButton);
    pressEvent.setModifiers(Qt::AltModifier);

    QGraphicsSceneMouseEvent lastEvent(QEvent::GraphicsSceneMouseMove);
    lastEvent.setScenePos(QPointF(13.0, 25.0));
    lastEvent.setScreenPos(QPoint(130, 250));
    lastEvent.setButton(Qt::NoButton);
    lastEvent.setButtons(Qt::LeftButton);
    lastEvent.setModifiers(Qt::AltModifier);

    QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
    moveEvent.setScenePos(QPointF(15.0, 28.0));
    moveEvent.setScreenPos(QPoint(150, 280));
    moveEvent.setLastScenePos(QPointF(13.0, 25.0));
    moveEvent.setLastScreenPos(QPoint(130, 250));
    moveEvent.setButtonDownScenePos(Qt::LeftButton, QPointF(10.0, 20.0));
    moveEvent.setButtonDownScreenPos(Qt::LeftButton, QPoint(100, 200));
    moveEvent.setButtonDownScenePos(Qt::MiddleButton, QPointF(30.0, 40.0));
    moveEvent.setButtonDownScreenPos(Qt::MiddleButton, QPoint(300, 400));
    moveEvent.setButtonDownScenePos(Qt::RightButton, QPointF(45.0, 55.0));
    moveEvent.setButtonDownScreenPos(Qt::RightButton, QPoint(450, 550));
    moveEvent.setButton(Qt::NoButton);
    moveEvent.setButtons(Qt::LeftButton | Qt::RightButton);
    moveEvent.setModifiers(Qt::AltModifier | Qt::ShiftModifier);

    MouseDragEvent event(&moveEvent, &pressEvent, &lastEvent, true, false);
    CHECK(samePoint(event.scenePos(), 15.0, 28.0));
    CHECK(samePoint(event.screenPos(), 150.0, 280.0));
    CHECK(samePoint(event.lastScenePos(), 13.0, 25.0));
    CHECK(samePoint(event.lastScreenPos(), 130.0, 250.0));
    CHECK(samePoint(event.buttonDownScenePos(), 10.0, 20.0));
    CHECK(samePoint(event.buttonDownScenePos(Qt::LeftButton), 10.0, 20.0));
    CHECK(samePoint(event.buttonDownScenePos(Qt::MiddleButton), 30.0, 40.0));
    CHECK(samePoint(event.buttonDownScenePos(Qt::RightButton), 45.0, 55.0));
    CHECK(samePoint(event.buttonDownScreenPos(), 100.0, 200.0));
    CHECK(samePoint(event.buttonDownScreenPos(Qt::LeftButton), 100.0, 200.0));
    CHECK(samePoint(event.buttonDownScreenPos(Qt::MiddleButton), 300.0, 400.0));
    CHECK(samePoint(event.buttonDownScreenPos(Qt::RightButton), 450.0, 550.0));
    CHECK(event.button() == Qt::LeftButton);
    CHECK(event.buttons() == (Qt::LeftButton | Qt::RightButton));
    CHECK(event.modifiers() == (Qt::AltModifier | Qt::ShiftModifier));
    CHECK(event.isStart());
    CHECK(!event.isFinish());
    CHECK(!event.isAccepted());

    QGraphicsRectItem item;
    item.setPos(10.0, 20.0);
    event.setCurrentItem(&item);
    CHECK(samePoint(event.pos(), 5.0, 8.0));
    CHECK(samePoint(event.lastPos(), 3.0, 5.0));
    CHECK(samePoint(event.buttonDownPos(), 0.0, 0.0));
    CHECK(samePoint(event.buttonDownPos(Qt::MiddleButton), 20.0, 20.0));
    CHECK(samePoint(event.buttonDownPos(Qt::RightButton), 35.0, 35.0));
    event.accept();
    CHECK(event.isAccepted());
    CHECK(event.acceptedItem() == &item);
    event.ignore();
    CHECK(!event.isAccepted());
    CHECK(event.acceptedItem() == nullptr);

    MouseDragEvent firstDragEvent(&moveEvent, &pressEvent, nullptr, true, false);
    CHECK(samePoint(firstDragEvent.lastScenePos(), 10.0, 20.0));
    CHECK(samePoint(firstDragEvent.lastScreenPos(), 100.0, 200.0));

    MouseDragEvent finishEvent(&moveEvent, &pressEvent, &lastEvent, false, true);
    CHECK(!finishEvent.isStart());
    CHECK(finishEvent.isFinish());

    MouseClickEvent pressWrapper(&pressEvent);
    pressEvent.setScenePos(QPointF(1000.0, 2000.0));
    pressEvent.setScreenPos(QPoint(10000, 20000));
    pressEvent.setButton(Qt::RightButton);

    QGraphicsSceneMouseEvent followMove(QEvent::GraphicsSceneMouseMove);
    followMove.setScenePos(QPointF(22.0, 33.0));
    followMove.setScreenPos(QPoint(220, 330));
    followMove.setButtonDownScenePos(Qt::LeftButton, QPointF(99.0, 199.0));
    followMove.setButtonDownScreenPos(Qt::LeftButton, QPoint(990, 1990));
    followMove.setButtonDownScenePos(Qt::RightButton, QPointF(45.0, 55.0));
    followMove.setButtonDownScreenPos(Qt::RightButton, QPoint(450, 550));
    followMove.setButton(Qt::NoButton);
    followMove.setButtons(Qt::LeftButton | Qt::RightButton);
    followMove.setModifiers(Qt::ControlModifier);

    MouseDragEvent durableStart(&followMove, pressWrapper, nullptr, true, false);
    CHECK(durableStart.button() == Qt::LeftButton);
    CHECK(samePoint(durableStart.buttonDownScenePos(), 10.0, 20.0));
    CHECK(samePoint(durableStart.buttonDownScenePos(Qt::RightButton), 45.0, 55.0));
    CHECK(samePoint(durableStart.lastScenePos(), 10.0, 20.0));
    CHECK(samePoint(durableStart.lastScreenPos(), 100.0, 200.0));

    QGraphicsSceneMouseEvent continueMove(QEvent::GraphicsSceneMouseMove);
    continueMove.setScenePos(QPointF(25.0, 36.0));
    continueMove.setScreenPos(QPoint(250, 360));
    continueMove.setButtonDownScenePos(Qt::LeftButton, QPointF(10.0, 20.0));
    continueMove.setButtonDownScreenPos(Qt::LeftButton, QPoint(100, 200));
    continueMove.setButton(Qt::NoButton);
    continueMove.setButtons(Qt::LeftButton);
    continueMove.setModifiers(Qt::AltModifier);

    MouseDragEvent durableContinuation(&continueMove, pressWrapper, &durableStart, false, false);
    CHECK(samePoint(durableContinuation.lastScenePos(), 22.0, 33.0));
    CHECK(samePoint(durableContinuation.lastScreenPos(), 220.0, 330.0));
    CHECK(samePoint(durableContinuation.buttonDownScenePos(), 10.0, 20.0));

    return true;
}

bool testHoverEventShapeAccessorsAndClaims()
{
    using pyqtgraph::GraphicsScene::HoverEvent;

    static_assert(std::is_constructible_v<HoverEvent, QGraphicsSceneMouseEvent*, bool>);
    static_assert(std::is_destructible_v<HoverEvent>);

    QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
    moveEvent.setScenePos(QPointF(18.0, 29.0));
    moveEvent.setScreenPos(QPoint(180, 290));
    moveEvent.setLastScenePos(QPointF(16.0, 27.0));
    moveEvent.setLastScreenPos(QPoint(160, 270));
    moveEvent.setButtons(Qt::MiddleButton);
    moveEvent.setModifiers(Qt::MetaModifier);

    HoverEvent event(&moveEvent, true);
    CHECK(!event.isExit());
    CHECK(!event.isEnter());
    CHECK(samePoint(event.scenePos(), 18.0, 29.0));
    CHECK(samePoint(event.screenPos(), 180.0, 290.0));
    CHECK(samePoint(event.lastScenePos(), 16.0, 27.0));
    CHECK(samePoint(event.lastScreenPos(), 160.0, 270.0));
    CHECK(event.buttons() == Qt::MiddleButton);
    CHECK(event.modifiers() == Qt::MetaModifier);

    QGraphicsRectItem item;
    item.setPos(10.0, 20.0);
    event.setCurrentItem(&item);
    CHECK(event.currentItem() == &item);
    CHECK(samePoint(event.pos(), 8.0, 9.0));
    CHECK(samePoint(event.lastPos(), 6.0, 7.0));
    CHECK(event.acceptClicks(Qt::LeftButton));
    CHECK(event.clickItems().value(Qt::LeftButton) == &item);
    CHECK(!event.acceptClicks(Qt::LeftButton));
    CHECK(event.acceptDrags(Qt::RightButton, &item));
    CHECK(event.dragItems().value(Qt::RightButton) == &item);
    CHECK(!event.acceptDrags(Qt::RightButton));

    auto deletedClickItem = std::make_unique<QGraphicsRectItem>();
    CHECK(event.acceptClicks(Qt::MiddleButton, deletedClickItem.get()));
    CHECK(event.clickItems().value(Qt::MiddleButton) == deletedClickItem.get());
    deletedClickItem.reset();
    CHECK(!event.clickItems().contains(Qt::MiddleButton));
    auto replacementClickItem = std::make_unique<QGraphicsRectItem>();
    CHECK(event.acceptClicks(Qt::MiddleButton, replacementClickItem.get()));
    CHECK(event.clickItems().value(Qt::MiddleButton) == replacementClickItem.get());

    auto deletedDragItem = std::make_unique<QGraphicsRectItem>();
    CHECK(event.acceptDrags(Qt::LeftButton, deletedDragItem.get()));
    CHECK(event.dragItems().value(Qt::LeftButton) == deletedDragItem.get());
    deletedDragItem.reset();
    CHECK(!event.dragItems().contains(Qt::LeftButton));
    auto replacementDragItem = std::make_unique<QGraphicsRectItem>();
    CHECK(event.acceptDrags(Qt::LeftButton, replacementDragItem.get()));
    CHECK(event.dragItems().value(Qt::LeftButton) == replacementDragItem.get());

    HoverEvent unavailable(&moveEvent, false);
    CHECK(!unavailable.acceptClicks(Qt::LeftButton, &item));
    CHECK(!unavailable.acceptDrags(Qt::LeftButton, &item));
    CHECK(unavailable.clickItems().isEmpty());
    CHECK(unavailable.dragItems().isEmpty());

    HoverEvent exitEvent(nullptr, true);
    CHECK(exitEvent.isExit());
    CHECK(!exitEvent.isEnter());
    CHECK(samePoint(exitEvent.scenePos(), 0.0, 0.0));
    CHECK(samePoint(exitEvent.screenPos(), 0.0, 0.0));

    QGraphicsSceneMouseEvent enterMove(QEvent::GraphicsSceneMouseMove);
    enterMove.setScenePos(QPointF(3.0, 4.0));
    enterMove.setScreenPos(QPoint(30, 40));
    enterMove.setLastScenePos(QPointF(3.0, 4.0));
    enterMove.setLastScreenPos(QPoint(30, 40));
    HoverEvent enterEvent(&enterMove, true);
    CHECK(!enterEvent.isEnter());
    CHECK(!enterEvent.isExit());
    enterEvent.setEnter(true);
    CHECK(enterEvent.isEnter());
    enterEvent.setEnter(false);
    CHECK(!enterEvent.isEnter());
    enterEvent.setExit(true);
    CHECK(enterEvent.isExit());
    CHECK(samePoint(enterEvent.scenePos(), 3.0, 4.0));
    CHECK(samePoint(enterEvent.lastScenePos(), 3.0, 4.0));
    enterEvent.setExit(false);
    CHECK(!enterEvent.isExit());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testMouseClickEventShapeAndAccessors()) {
        return 1;
    }
    if (!testMouseDragEventShapeAndAccessors()) {
        return 1;
    }
    if (!testHoverEventShapeAccessorsAndClaims()) {
        return 1;
    }

    return 0;
}
