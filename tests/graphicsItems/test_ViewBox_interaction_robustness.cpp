#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QPointF>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneWheelEvent>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <vector>

using ViewBox = cppqtgraph::graphicsItems::ViewBox;

namespace {

class ScriptableViewBox : public ViewBox {
public:
    using ViewBox::mouseMoveEvent;
    using ViewBox::mousePressEvent;
    using ViewBox::mouseReleaseEvent;
    using ViewBox::wheelEvent;
};

bool check(bool condition, const char* expression, const char* file, int line)
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

bool rangeIsFiniteIncreasing(const ViewBox::AxisRange& axis)
{
    return std::isfinite(axis[0]) && std::isfinite(axis[1]) && axis[0] < axis[1];
}

bool range2DIsFiniteIncreasing(const ViewBox::Range2D& range)
{
    return rangeIsFiniteIncreasing(range[ViewBox::XAxis]) && rangeIsFiniteIncreasing(range[ViewBox::YAxis]);
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

void setupViewBox(ScriptableViewBox& viewBox, QGraphicsScene& scene)
{
    scene.addItem(&viewBox);
    viewBox.resize(200.0, 100.0);
    viewBox.setDefaultPadding(0.0);
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
}

bool testZoomOutOverflowDoesNotAbort()
{
    QGraphicsScene scene;
    ScriptableViewBox viewBox;
    setupViewBox(viewBox, scene);
    viewBox.setRange(QRectF(-1.0e200, -1.0e200, 2.0e200, 2.0e200), 0.0);
    CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));

    const QPointF center(100.0, 50.0);
    for (int i = 0; i < 500; ++i) {
        auto wheel = wheelEvent(center, -120);
        viewBox.wheelEvent(wheel.get());
        CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
        CHECK(range2DIsFiniteIncreasing(viewBox.targetRange()));
    }
    return true;
}

bool testZoomInCollapseDoesNotAbort()
{
    QGraphicsScene scene;
    ScriptableViewBox viewBox;
    setupViewBox(viewBox, scene);
    viewBox.setRange(QRectF(0.0, 0.0, 1.0e-12, 1.0e-12), 0.0);
    CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));

    const QPointF center(100.0, 50.0);
    for (int i = 0; i < 200; ++i) {
        auto wheel = wheelEvent(center, 120);
        viewBox.wheelEvent(wheel.get());
        CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
    }

    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, center, center, Qt::RightButton, Qt::RightButton, center);
    viewBox.mousePressEvent(press.get());
    for (int i = 0; i < 100; ++i) {
        const QPointF pos(center.x() + static_cast<qreal>(i), center.y() + static_cast<qreal>(i));
        auto move = mouseEvent(QEvent::GraphicsSceneMouseMove, pos, center, Qt::RightButton, Qt::RightButton, center);
        viewBox.mouseMoveEvent(move.get());
        CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
    }
    auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(199.0, 149.0), QPointF(199.0, 149.0),
                              Qt::RightButton, Qt::NoButton, center);
    viewBox.mouseReleaseEvent(release.get());
    CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
    return true;
}

bool testRectModeZeroAreaReleaseDoesNotAbort()
{
    QGraphicsScene scene;
    ScriptableViewBox viewBox;
    setupViewBox(viewBox, scene);
    const auto before = viewBox.viewRange();

    viewBox.setMouseMode(ViewBox::RectMode);
    const QPointF pos(100.0, 50.0);
    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, pos, pos, Qt::LeftButton, Qt::LeftButton, pos);
    auto move = mouseEvent(QEvent::GraphicsSceneMouseMove, pos, pos, Qt::LeftButton, Qt::LeftButton, pos);
    auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, pos, pos, Qt::LeftButton, Qt::NoButton, pos);
    viewBox.mousePressEvent(press.get());
    viewBox.mouseMoveEvent(move.get());
    viewBox.mouseReleaseEvent(release.get());

    CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
    CHECK(viewBox.viewRange() == before);
    return true;
}

bool testLogModePlottingP5StyleInteractionDoesNotAbort()
{
    cppqtgraph::widgets::PlotWidget plotWidget;
    plotWidget.resize(320, 240);
    plotWidget.show();

    auto* plotItem = plotWidget.getPlotItem();
    CHECK(plotItem != nullptr);
    plotItem->setLogMode(true, false);

    std::vector<double> x;
    std::vector<double> y;
    x.reserve(500);
    y.reserve(500);
    for (int index = 0; index < 500; ++index) {
        const double value = static_cast<double>(index) / 50.0;
        if (value > 0.0) {
            x.push_back(value);
            y.push_back(std::sin(value * 10.0));
        }
    }
    plotWidget.plot(x, y);
    plotItem->autoRange();

    auto* viewBox = plotItem->getViewBox();
    CHECK(viewBox != nullptr);
    CHECK(range2DIsFiniteIncreasing(viewBox->viewRange()));

    const QPointF center(160.0, 120.0);
    for (int delta : {120, -120, 240, -240}) {
        auto wheel = wheelEvent(center, delta);
        QCoreApplication::sendEvent(viewBox, wheel.get());
        CHECK(range2DIsFiniteIncreasing(viewBox->viewRange()));
    }

    auto press = mouseEvent(QEvent::GraphicsSceneMousePress, center, center, Qt::LeftButton, Qt::LeftButton, center);
    auto move = mouseEvent(QEvent::GraphicsSceneMouseMove, QPointF(200.0, 120.0), center, Qt::LeftButton,
                           Qt::LeftButton, center);
    auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, QPointF(200.0, 120.0), QPointF(200.0, 120.0),
                              Qt::LeftButton, Qt::NoButton, center);
    QCoreApplication::sendEvent(viewBox, press.get());
    QCoreApplication::sendEvent(viewBox, move.get());
    QCoreApplication::sendEvent(viewBox, release.get());
    CHECK(range2DIsFiniteIncreasing(viewBox->viewRange()));
    return true;
}

bool testSeededInteractionFuzzDoesNotAbort()
{
    QGraphicsScene scene;
    ScriptableViewBox viewBox;
    setupViewBox(viewBox, scene);

    std::mt19937 rng(385);
    std::uniform_int_distribution<int> coordDist(0, 199);
    std::uniform_int_distribution<int> wheelDist(-480, 480);
    std::uniform_int_distribution<int> actionDist(0, 5);

    QPointF lastPos(static_cast<qreal>(coordDist(rng)), static_cast<qreal>(coordDist(rng)));
    bool dragging = false;
    Qt::MouseButton dragButton = Qt::NoButton;

    for (int step = 0; step < 2000; ++step) {
        const int action = actionDist(rng);
        if (action == 0 || action == 1) {
            auto wheel = wheelEvent(lastPos, wheelDist(rng));
            viewBox.wheelEvent(wheel.get());
        } else if (!dragging) {
            dragButton = (action % 2 == 0) ? Qt::LeftButton : Qt::RightButton;
            auto press = mouseEvent(QEvent::GraphicsSceneMousePress, lastPos, lastPos, dragButton, dragButton, lastPos);
            viewBox.mousePressEvent(press.get());
            dragging = true;
        } else if (action == 5) {
            auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, lastPos, lastPos, dragButton, Qt::NoButton,
                                       lastPos);
            viewBox.mouseReleaseEvent(release.get());
            dragging = false;
            dragButton = Qt::NoButton;
        } else {
            const QPointF nextPos(static_cast<qreal>(coordDist(rng)), static_cast<qreal>(coordDist(rng)));
            auto move = mouseEvent(QEvent::GraphicsSceneMouseMove, nextPos, lastPos, dragButton, dragButton, lastPos);
            viewBox.mouseMoveEvent(move.get());
            lastPos = nextPos;
        }

        CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
        CHECK(range2DIsFiniteIncreasing(viewBox.targetRange()));
    }

    if (dragging) {
        auto release = mouseEvent(QEvent::GraphicsSceneMouseRelease, lastPos, lastPos, dragButton, Qt::NoButton,
                                   lastPos);
        viewBox.mouseReleaseEvent(release.get());
    }

    CHECK(range2DIsFiniteIncreasing(viewBox.viewRange()));
    return true;
}

bool testProgrammaticInvalidInputStillThrows()
{
    ViewBox viewBox;
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 20.0), 0.0);
    const auto before = viewBox.viewRange();

    bool threw = false;
    try {
        viewBox.setXRange(0.0, std::numeric_limits<qreal>::infinity(), 0.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.viewRange() == before);

    threw = false;
    try {
        viewBox.scaleBy(std::numeric_limits<qreal>::quiet_NaN(), 1.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(viewBox.viewRange() == before);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard app(argc, argv);

    if (!testZoomOutOverflowDoesNotAbort()) {
        return 1;
    }
    if (!testZoomInCollapseDoesNotAbort()) {
        return 1;
    }
    if (!testRectModeZeroAreaReleaseDoesNotAbort()) {
        return 1;
    }
    if (!testLogModePlottingP5StyleInteractionDoesNotAbort()) {
        return 1;
    }
    if (!testSeededInteractionFuzzDoesNotAbort()) {
        return 1;
    }
    if (!testProgrammaticInvalidInputStillThrows()) {
        return 1;
    }

    return 0;
}
