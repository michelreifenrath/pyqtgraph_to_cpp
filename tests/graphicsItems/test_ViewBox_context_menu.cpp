#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QPointF>
#include <QtGui/QAction>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QWidgetAction>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

using ViewBox = cppqtgraph::graphicsItems::ViewBox;
using ViewBoxMenu = cppqtgraph::graphicsItems::ViewBoxMenu;

namespace {

class ScriptableViewBox : public ViewBox {
public:
    using ViewBox::mouseMoveEvent;
    using ViewBox::mousePressEvent;
    using ViewBox::mouseReleaseEvent;
};

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
            return 1; \
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

bool nearlyEqual(qreal lhs, qreal rhs, qreal tolerance = 1.0e-6)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool rangeNearly(const ViewBox::AxisRange& actual, const ViewBox::AxisRange& expected, qreal tolerance = 1.0e-6)
{
    return nearlyEqual(actual[0], expected[0], tolerance) && nearlyEqual(actual[1], expected[1], tolerance);
}

qreal span(const ViewBox::AxisRange& axis)
{
    return axis[1] - axis[0];
}

std::unique_ptr<QGraphicsSceneMouseEvent> mouseEvent(QEvent::Type type,
                                                     const QPointF& pos,
                                                     const QPointF& lastPos,
                                                     Qt::MouseButton button,
                                                     Qt::MouseButtons buttons,
                                                     const QPointF& buttonDownPos,
                                                     const QPoint& screenPos,
                                                     const QPoint& lastScreenPos,
                                                     const QPoint& buttonDownScreenPos)
{
    auto event = std::make_unique<QGraphicsSceneMouseEvent>(type);
    event->setPos(pos);
    event->setScenePos(pos);
    event->setLastPos(lastPos);
    event->setLastScenePos(lastPos);
    event->setButton(button);
    event->setButtons(buttons);
    event->setButtonDownPos(button, buttonDownPos);
    event->setScreenPos(screenPos);
    event->setLastScreenPos(lastScreenPos);
    event->setButtonDownScreenPos(button, buttonDownScreenPos);
    return event;
}

QAction* findAction(QMenu* menu, const QString& text)
{
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction* action : menu->actions()) {
        if (action != nullptr && action->text() == text) {
            return action;
        }
    }
    return nullptr;
}

QMenu* findSubMenu(QMenu* menu, const QString& subMenuTitle)
{
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction* action : menu->actions()) {
        if (action != nullptr && action->menu() != nullptr && action->text() == subMenuTitle) {
            return action->menu();
        }
    }
    return nullptr;
}

QAction* findSubMenuAction(QMenu* menu, const QString& subMenuTitle, const QString& actionText)
{
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction* action : menu->actions()) {
        if (action == nullptr || action->menu() == nullptr || action->text() != subMenuTitle) {
            continue;
        }
        for (QAction* subAction : action->menu()->actions()) {
            if (subAction != nullptr && subAction->text() == actionText) {
                return subAction;
            }
        }
    }
    return nullptr;
}

QRadioButton* findAxisAutoRadio(QMenu* menu, const QString& subMenuTitle)
{
    QMenu* subMenu = findSubMenu(menu, subMenuTitle);
    if (subMenu == nullptr) {
        return nullptr;
    }
    for (QAction* action : subMenu->actions()) {
        auto* widgetAction = qobject_cast<QWidgetAction*>(action);
        if (widgetAction == nullptr) {
            continue;
        }
        QWidget* widget = widgetAction->defaultWidget();
        if (widget == nullptr) {
            continue;
        }
        if (auto* autoRadio = widget->findChild<QRadioButton*>(QStringLiteral("autoRadio")); autoRadio != nullptr) {
            return autoRadio;
        }
    }
    return nullptr;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    ApplicationGuard guard(argc, argv);

    QGraphicsScene scene;
    ScriptableViewBox viewBox;
    scene.addItem(&viewBox);
    viewBox.resize(200.0, 100.0);
    viewBox.setDefaultPadding(0.0);
    viewBox.disableAutoRange(ViewBox::XYAxes);
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);

    const QPoint pressScreen(100, 50);
    auto rightPress = mouseEvent(QEvent::GraphicsSceneMousePress,
                                 QPointF(100.0, 50.0),
                                 QPointF(100.0, 50.0),
                                 Qt::RightButton,
                                 Qt::RightButton,
                                 QPointF(100.0, 50.0),
                                 pressScreen,
                                 pressScreen,
                                 pressScreen);
    auto rightRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease,
                                   QPointF(100.0, 50.0),
                                   QPointF(100.0, 50.0),
                                   Qt::RightButton,
                                   Qt::NoButton,
                                   QPointF(100.0, 50.0),
                                   pressScreen,
                                   pressScreen,
                                   pressScreen);
    viewBox.mousePressEvent(rightPress.get());
    viewBox.mouseReleaseEvent(rightRelease.get());
    CHECK(rightPress->isAccepted());
    CHECK(rightRelease->isAccepted());
    CHECK(viewBox.contextMenuRaiseCount() == 1);
    CHECK(viewBox.menu() != nullptr);
    CHECK(viewBox.menu()->title() == QStringLiteral("ViewBox options"));

    ScriptableViewBox dragViewBox;
    scene.addItem(&dragViewBox);
    dragViewBox.resize(200.0, 100.0);
    dragViewBox.setDefaultPadding(0.0);
    dragViewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    const auto beforeRightDrag = dragViewBox.viewRange();
    const QPoint dragPressScreen(100, 50);
    const QPoint dragMoveScreen(130, 80);
    auto dragPress = mouseEvent(QEvent::GraphicsSceneMousePress,
                                QPointF(100.0, 50.0),
                                QPointF(100.0, 50.0),
                                Qt::RightButton,
                                Qt::RightButton,
                                QPointF(100.0, 50.0),
                                dragPressScreen,
                                dragPressScreen,
                                dragPressScreen);
    auto dragMove = mouseEvent(QEvent::GraphicsSceneMouseMove,
                               QPointF(130.0, 80.0),
                               QPointF(100.0, 50.0),
                               Qt::RightButton,
                               Qt::RightButton,
                               QPointF(100.0, 50.0),
                               dragMoveScreen,
                               dragPressScreen,
                               dragPressScreen);
    auto dragRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease,
                                  QPointF(130.0, 80.0),
                                  QPointF(130.0, 80.0),
                                  Qt::RightButton,
                                  Qt::NoButton,
                                  QPointF(100.0, 50.0),
                                  dragMoveScreen,
                                  dragMoveScreen,
                                  dragPressScreen);
    dragViewBox.mousePressEvent(dragPress.get());
    dragViewBox.mouseMoveEvent(dragMove.get());
    dragViewBox.mouseReleaseEvent(dragRelease.get());
    const auto afterRightDrag = dragViewBox.viewRange();
    CHECK(dragViewBox.contextMenuRaiseCount() == 0);
    CHECK(span(afterRightDrag[ViewBox::XAxis]) != span(beforeRightDrag[ViewBox::XAxis])
           || span(afterRightDrag[ViewBox::YAxis]) != span(beforeRightDrag[ViewBox::YAxis]));

    QGraphicsRectItem dataRect(QRectF(2.0, 3.0, 6.0, 4.0));
    dataRect.setPen(QPen(Qt::NoPen));
    viewBox.addItem(&dataRect);
    viewBox.setRange(QRectF(0.0, 0.0, 1.0, 1.0), 0.0);
    ViewBoxMenu* menu = viewBox.menu();
    CHECK(menu != nullptr);
    QAction* viewAllAction = findAction(menu, QStringLiteral("View All"));
    CHECK(viewAllAction != nullptr);
    viewAllAction->trigger();
    CHECK(rangeNearly(viewBox.viewRange()[ViewBox::XAxis], ViewBox::AxisRange{2.0, 8.0}));
    CHECK(rangeNearly(viewBox.viewRange()[ViewBox::YAxis], ViewBox::AxisRange{3.0, 7.0}));

    QAction* xAxisMenuAction = findAction(menu, QStringLiteral("X axis"));
    QAction* yAxisMenuAction = findAction(menu, QStringLiteral("Y axis"));
    CHECK(xAxisMenuAction != nullptr);
    CHECK(yAxisMenuAction != nullptr);
    CHECK(findSubMenu(menu, QStringLiteral("X axis")) != nullptr);
    CHECK(findSubMenu(menu, QStringLiteral("Y axis")) != nullptr);

    QAction* panModeAction = findSubMenuAction(menu, QStringLiteral("Mouse Mode"), QStringLiteral("3 button"));
    QAction* rectModeAction = findSubMenuAction(menu, QStringLiteral("Mouse Mode"), QStringLiteral("1 button"));
    CHECK(panModeAction != nullptr);
    CHECK(rectModeAction != nullptr);
    CHECK(viewBox.mouseMode() == ViewBox::PanMode);
    rectModeAction->trigger();
    CHECK(viewBox.mouseMode() == ViewBox::RectMode);
    panModeAction->trigger();
    CHECK(viewBox.mouseMode() == ViewBox::PanMode);

    ScriptableViewBox rectViewBox;
    scene.addItem(&rectViewBox);
    rectViewBox.resize(200.0, 100.0);
    rectViewBox.setDefaultPadding(0.0);
    rectViewBox.setMouseMode(ViewBox::RectMode);
    rectViewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    const auto beforeRectDrag = rectViewBox.viewRange();
    const QPoint rectPressScreen(50, 50);
    const QPoint rectMoveScreen(150, 90);
    auto rectPress = mouseEvent(QEvent::GraphicsSceneMousePress,
                                QPointF(50.0, 50.0),
                                QPointF(50.0, 50.0),
                                Qt::LeftButton,
                                Qt::LeftButton,
                                QPointF(50.0, 50.0),
                                rectPressScreen,
                                rectPressScreen,
                                rectPressScreen);
    auto rectMove = mouseEvent(QEvent::GraphicsSceneMouseMove,
                               QPointF(150.0, 90.0),
                               QPointF(50.0, 50.0),
                               Qt::LeftButton,
                               Qt::LeftButton,
                               QPointF(50.0, 50.0),
                               rectMoveScreen,
                               rectPressScreen,
                               rectPressScreen);
    auto rectRelease = mouseEvent(QEvent::GraphicsSceneMouseRelease,
                                  QPointF(150.0, 90.0),
                                  QPointF(150.0, 90.0),
                                  Qt::LeftButton,
                                  Qt::NoButton,
                                  QPointF(50.0, 50.0),
                                  rectMoveScreen,
                                  rectMoveScreen,
                                  rectPressScreen);
    rectViewBox.mousePressEvent(rectPress.get());
    rectViewBox.mouseMoveEvent(rectMove.get());
    rectViewBox.mouseReleaseEvent(rectRelease.get());
    const auto afterRectDrag = rectViewBox.viewRange();
    CHECK(span(afterRectDrag[ViewBox::XAxis]) < span(beforeRectDrag[ViewBox::XAxis]));
    CHECK(span(afterRectDrag[ViewBox::YAxis]) < span(beforeRectDrag[ViewBox::YAxis]));

    ScriptableViewBox autoViewBox;
    scene.addItem(&autoViewBox);
    autoViewBox.resize(200.0, 100.0);
    autoViewBox.setDefaultPadding(0.0);
    autoViewBox.disableAutoRange(ViewBox::XYAxes);
    autoViewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    autoViewBox.raiseContextMenu(QPoint(0, 0));
    ViewBoxMenu* autoMenu = autoViewBox.menu();
    CHECK(autoMenu != nullptr);
    QRadioButton* xAutoRadio = findAxisAutoRadio(autoMenu, QStringLiteral("X axis"));
    QRadioButton* yAutoRadio = findAxisAutoRadio(autoMenu, QStringLiteral("Y axis"));
    CHECK(xAutoRadio != nullptr);
    CHECK(yAutoRadio != nullptr);
    xAutoRadio->click();
    CHECK(autoViewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(!autoViewBox.autoRangeEnabled()[ViewBox::YAxis]);
    yAutoRadio->click();
    CHECK(autoViewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(autoViewBox.autoRangeEnabled()[ViewBox::YAxis]);

    return 0;
}
