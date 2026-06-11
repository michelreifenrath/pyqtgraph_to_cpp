#include <cppqtgraph/GraphicsScene/mouseEvents.hpp>
#include <cppqtgraph/graphicsItems/ButtonItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <iostream>
#include <memory>
#include <string_view>

namespace {

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

    return 0;
}
