#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QKeyEvent>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsView>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#ifndef CPPQTGRAPH_INTERACTION_FIXTURE_DIR
#define CPPQTGRAPH_INTERACTION_FIXTURE_DIR "oracle/fixtures/interactions"
#endif

using ViewBox = cppqtgraph::graphicsItems::ViewBox;

namespace {

class ScriptableViewBox : public ViewBox {
public:
    using ViewBox::keyPressEvent;
    using ViewBox::mouseMoveEvent;
    using ViewBox::mousePressEvent;
    using ViewBox::mouseReleaseEvent;
};

bool nearly(qreal actual, qreal expected, qreal tolerance = 1.0e-6)
{
    return std::abs(actual - expected) <= tolerance;
}

bool rangeNearly(const ViewBox::AxisRange& actual, const ViewBox::AxisRange& expected, qreal tolerance = 1.0e-6)
{
    return nearly(actual[0], expected[0], tolerance) && nearly(actual[1], expected[1], tolerance);
}

ViewBox::AxisRange readAxisRange(const QJsonObject& object, const char* key)
{
    const auto values = object.value(QLatin1String(key)).toArray();
    if (values.size() != 2) {
        throw std::runtime_error("fixture axis range must contain two numbers");
    }
    return ViewBox::AxisRange{values.at(0).toDouble(), values.at(1).toDouble()};
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

void rectZoom(ScriptableViewBox& viewBox, const QPointF& start, const QPointF& end)
{
    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, start, start, Qt::LeftButton, Qt::LeftButton, start);
    auto move = mouseEvent(QEvent::GraphicsSceneMouseMove, end, start, Qt::LeftButton, Qt::LeftButton, start);
    auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, end, end, Qt::LeftButton, Qt::NoButton, start);
    viewBox.mousePressEvent(press.get());
    viewBox.mouseMoveEvent(move.get());
    viewBox.mouseReleaseEvent(release.get());
}

std::unique_ptr<QKeyEvent> keyEvent(const QString& text, Qt::Key key = Qt::Key_unknown)
{
    return std::make_unique<QKeyEvent>(QEvent::KeyPress, key, Qt::NoModifier, text);
}

bool checkStepRange(const ViewBox& viewBox, const QJsonObject& step, const char* label)
{
    const auto rangeObject = step.value(QStringLiteral("range")).toObject();
    const auto actual = viewBox.viewRange();
    const auto expectedX = readAxisRange(rangeObject, "x");
    const auto expectedY = readAxisRange(rangeObject, "y");
    if (!rangeNearly(actual[ViewBox::XAxis], expectedX) || !rangeNearly(actual[ViewBox::YAxis], expectedY)) {
        std::cerr << label << ": range mismatch for action " << step.value(QStringLiteral("action")).toString().toStdString()
                  << " expected x=[" << expectedX[0] << ',' << expectedX[1] << "] y=[" << expectedY[0] << ','
                  << expectedY[1] << "] actual x=[" << actual[ViewBox::XAxis][0] << ',' << actual[ViewBox::XAxis][1]
                  << "] y=[" << actual[ViewBox::YAxis][0] << ',' << actual[ViewBox::YAxis][1] << "]\n";
        return false;
    }
    return true;
}

bool runOracleScenario(ScriptableViewBox& viewBox, const QJsonObject& fixture)
{
    const auto size = fixture.value(QStringLiteral("viewbox_size")).toArray();
    if (size.size() != 2) {
        std::cerr << "fixture viewbox_size must contain two numbers\n";
        return false;
    }

    const auto initial = fixture.value(QStringLiteral("initial")).toObject();
    viewBox.resize(size.at(0).toDouble(), size.at(1).toDouble());
    viewBox.setDefaultPadding(0.0);
    viewBox.setRange(readAxisRange(initial, "x"), readAxisRange(initial, "y"), 0.0);
    viewBox.setMouseMode(ViewBox::RectMode);

    const auto steps = fixture.value(QStringLiteral("steps")).toArray();
    for (const auto& stepValue : steps) {
        const auto step = stepValue.toObject();
        const auto action = step.value(QStringLiteral("action")).toString();
        if (action == QLatin1String("initial")) {
            if (!checkStepRange(viewBox, step, "initial")) {
                return false;
            }
            continue;
        }
        if (action == QLatin1String("rect_zoom_1")) {
            rectZoom(viewBox, QPointF(25.0, 25.0), QPointF(175.0, 75.0));
        } else if (action == QLatin1String("rect_zoom_2")) {
            rectZoom(viewBox, QPointF(50.0, 30.0), QPointF(150.0, 70.0));
        } else if (action == QLatin1String("key_minus") || action == QLatin1String("key_minus_before_equals")
                   || action == QLatin1String("key_minus_before_backspace")
                   || action == QLatin1String("key_minus_to_history_head")) {
            viewBox.keyPressEvent(keyEvent(QStringLiteral("-")).get());
        } else if (action == QLatin1String("key_plus")) {
            viewBox.keyPressEvent(keyEvent(QStringLiteral("+")).get());
        } else if (action == QLatin1String("key_equals")) {
            viewBox.keyPressEvent(keyEvent(QStringLiteral("=")).get());
        } else if (action == QLatin1String("key_backspace")) {
            viewBox.keyPressEvent(keyEvent(QString(), Qt::Key_Backspace).get());
        } else {
            std::cerr << "unknown fixture action: " << action.toStdString() << '\n';
            return false;
        }

        if (!checkStepRange(viewBox, step, "step")) {
            return false;
        }
    }

    return true;
}

bool runFocusedViewportScenario(const QJsonObject& fixture)
{
    cppqtgraph::GraphicsScene::GraphicsScene scene;
    ScriptableViewBox viewBox;
    scene.addItem(&viewBox);

    const auto size = fixture.value(QStringLiteral("viewbox_size")).toArray();
    viewBox.resize(size.at(0).toDouble(), size.at(1).toDouble());
    viewBox.setDefaultPadding(0.0);
    const auto initial = fixture.value(QStringLiteral("initial")).toObject();
    viewBox.setRange(readAxisRange(initial, "x"), readAxisRange(initial, "y"), 0.0);
    viewBox.setMouseMode(ViewBox::RectMode);

    QGraphicsView view(&scene);
    view.resize(static_cast<int>(size.at(0).toDouble()), static_cast<int>(size.at(1).toDouble()));
    view.show();
    view.activateWindow();
    QTest::qWait(0);

    if (!viewBox.flags().testFlag(QGraphicsItem::ItemIsFocusable)) {
        std::cerr << "ViewBox must be focusable for keyboard zoom history\n";
        return false;
    }

    const QPoint center(static_cast<int>(size.at(0).toDouble() / 2.0), static_cast<int>(size.at(1).toDouble() / 2.0));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, center);
    QTest::qWait(0);
    if (!viewBox.hasFocus()) {
        viewBox.setFocus(Qt::MouseFocusReason);
    }
    view.setFocus();
    QTest::qWait(0);

    rectZoom(viewBox, QPointF(25.0, 25.0), QPointF(175.0, 75.0));
    rectZoom(viewBox, QPointF(50.0, 30.0), QPointF(150.0, 70.0));

    const auto minusStep = fixture.value(QStringLiteral("steps")).toArray().at(3).toObject();
    QTest::keyClick(view.viewport(), Qt::Key_Minus);
    QTest::qWait(0);

    if (!checkStepRange(viewBox, minusStep, "viewport key minus")) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    QFile fixtureFile(QStringLiteral(CPPQTGRAPH_INTERACTION_FIXTURE_DIR "/ViewBox_keyboard_history.json"));
    if (!fixtureFile.open(QIODevice::ReadOnly)) {
        std::cerr << "failed to open fixture: " << fixtureFile.fileName().toStdString() << '\n';
        return 1;
    }

    const auto fixture = QJsonDocument::fromJson(fixtureFile.readAll()).object();
    if (fixture.isEmpty()) {
        std::cerr << "fixture JSON is empty\n";
        return 1;
    }

    ScriptableViewBox directViewBox;
    if (!runOracleScenario(directViewBox, fixture)) {
        return 1;
    }

    if (!runFocusedViewportScenario(fixture)) {
        return 1;
    }

    return 0;
}
