#include <cppqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <cppqtgraph/GraphicsScene/mouseEvents.hpp>
#include <cppqtgraph/graphicsItems/ButtonItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtGui/QImage>
#include <QtGui/QPen>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsView>

#include <cmath>
#include <iostream>
#include <memory>
#include <set>
#include <string_view>
#include <tuple>

namespace {

using cppqtgraph::GraphicsScene::GraphicsScene;
using cppqtgraph::GraphicsScene::MouseClickEvent;
using cppqtgraph::graphicsItems::ButtonItem;
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

ButtonItem* findAutoButton(const PlotItem& plot)
{
    for (QGraphicsItem* child : plot.childItems()) {
        if (auto* button = dynamic_cast<ButtonItem*>(child)) {
            return button;
        }
    }
    return nullptr;
}

MouseClickEvent makeLeftClickEvent()
{
    QGraphicsSceneMouseEvent qtEvent(QEvent::GraphicsSceneMouseRelease);
    qtEvent.setButton(Qt::LeftButton);
    qtEvent.setButtons(Qt::LeftButton);
    qtEvent.ignore();
    return MouseClickEvent(&qtEvent);
}

int countDistinctPixmapColors(const QPixmap& pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    std::set<std::tuple<int, int, int, int>> colors;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            colors.insert({qRed(pixel), qGreen(pixel), qBlue(pixel), qAlpha(pixel)});
        }
    }
    return static_cast<int>(colors.size());
}

void sendMouseClickToViewport(QGraphicsView& view, const QPointF& scenePos)
{
    const QPoint viewportPoint = view.mapFromScene(scenePos);
    const QPoint globalPoint = view.viewport()->mapToGlobal(viewportPoint);
    QMouseEvent pressEvent(QEvent::MouseButtonPress,
        QPointF(viewportPoint),
        QPointF(globalPoint),
        Qt::LeftButton,
        Qt::LeftButton,
        Qt::NoModifier);
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease,
        QPointF(viewportPoint),
        QPointF(globalPoint),
        Qt::LeftButton,
        Qt::NoButton,
        Qt::NoModifier);
    QCoreApplication::sendEvent(view.viewport(), &pressEvent);
    QCoreApplication::sendEvent(view.viewport(), &releaseEvent);
    QCoreApplication::processEvents();
}

bool testInitialHiddenWhileAutoRangeEnabled()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(plot.getViewBox()->autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(plot.getViewBox()->autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(!autoButton->isVisible());

    return true;
}

bool testPanShowsAutoButton()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(1.0, 2.0, 3.0, 4.0));
    item->setPen(QPen(Qt::NoPen));
    plot.getViewBox()->addItem(item.get());

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(!autoButton->isVisible());

    plot.getViewBox()->translateBy(1.0, 0.0);

    CHECK(!plot.getViewBox()->autoRangeEnabled()[ViewBox::XAxis]
           || !plot.getViewBox()->autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(autoButton->isVisible());

    return true;
}

bool testClickRestoresAutoRangeAndHidesButton()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(1.0, 2.0, 3.0, 4.0));
    item->setPen(QPen(Qt::NoPen));
    plot.getViewBox()->addItem(item.get());

    plot.getViewBox()->translateBy(1.0, 0.0);

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(autoButton->isVisible());

    MouseClickEvent clickEvent = makeLeftClickEvent();
    autoButton->mouseClickEvent(&clickEvent);
    CHECK(clickEvent.isAccepted());

    CHECK(plot.getViewBox()->autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(plot.getViewBox()->autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(!autoButton->isVisible());

    return true;
}

bool testProgrammaticDisableShowsAutoButton()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(1.0, 2.0, 3.0, 4.0));
    item->setPen(QPen(Qt::NoPen));
    plot.getViewBox()->addItem(item.get());

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(!autoButton->isVisible());

    plot.getViewBox()->enableAutoRange(ViewBox::XYAxes, false);

    CHECK(!plot.getViewBox()->autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(!plot.getViewBox()->autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(autoButton->isVisible());

    return true;
}

bool testAutoButtonAnchoredBottomLeftAfterResize()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(1.0, 2.0, 3.0, 4.0));
    item->setPen(QPen(Qt::NoPen));
    plot.getViewBox()->addItem(item.get());
    plot.getViewBox()->enableAutoRange(ViewBox::XYAxes, false);

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(autoButton->isVisible());

    const qreal plotHeight = plot.height();
    const qreal buttonHeight = autoButton->boundingRect().height();
    CHECK(std::abs(autoButton->pos().x()) < 1.0e-6);
    CHECK(std::abs(autoButton->pos().y() - (plotHeight - buttonHeight)) < 1.0);

    plot.resize(240.0, 160.0);
    CHECK(std::abs(autoButton->pos().x()) < 1.0e-6);
    CHECK(std::abs(autoButton->pos().y() - (plot.height() - buttonHeight)) < 1.0);

    return true;
}

bool testAutoButtonPixmapHasDistinctColors()
{
    QGraphicsScene scene;
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(countDistinctPixmapColors(autoButton->pixmap()) >= 6);

    return true;
}

bool testQTestClickRestoresAutoRangeAndHidesButton()
{
    GraphicsScene scene(4, 5.0);
    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(200.0, 100.0);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(1.0, 2.0, 3.0, 4.0));
    item->setPen(QPen(Qt::NoPen));
    plot.getViewBox()->addItem(item.get());
    plot.getViewBox()->enableAutoRange(ViewBox::XYAxes, false);

    QGraphicsView view(&scene);
    view.setSceneRect(plot.boundingRect());
    view.resize(240, 180);
    view.show();
    QTest::qWait(0);

    ButtonItem* autoButton = findAutoButton(plot);
    CHECK(autoButton != nullptr);
    CHECK(autoButton->isVisible());

    const QPointF clickPos = autoButton->mapToScene(autoButton->boundingRect().center());
    sendMouseClickToViewport(view, clickPos);
    QTest::qWait(0);

    CHECK(plot.getViewBox()->autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(plot.getViewBox()->autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(!autoButton->isVisible());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testInitialHiddenWhileAutoRangeEnabled()) {
        return 1;
    }
    if (!testPanShowsAutoButton()) {
        return 1;
    }
    if (!testClickRestoresAutoRangeAndHidesButton()) {
        return 1;
    }
    if (!testProgrammaticDisableShowsAutoButton()) {
        return 1;
    }
    if (!testAutoButtonAnchoredBottomLeftAfterResize()) {
        return 1;
    }
    if (!testAutoButtonPixmapHasDistinctColors()) {
        return 1;
    }
    if (!testQTestClickRestoresAutoRangeAndHidesButton()) {
        return 1;
    }

    return 0;
}
