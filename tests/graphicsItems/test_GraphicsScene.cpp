#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>

#include <QtCore/QObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>

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

bool testGraphicsSceneTypeShape()
{
    using pyqtgraph::GraphicsScene::GraphicsScene;

    static_assert(std::is_base_of_v<QGraphicsScene, GraphicsScene>);
    static_assert(std::is_base_of_v<QObject, GraphicsScene>);
    static_assert(std::is_constructible_v<GraphicsScene>);
    static_assert(std::is_constructible_v<GraphicsScene, int>);
    static_assert(std::is_constructible_v<GraphicsScene, int, qreal>);
    static_assert(std::is_constructible_v<GraphicsScene, int, qreal, QObject*>);
    static_assert(!std::is_copy_constructible_v<GraphicsScene>);
    static_assert(!std::is_copy_assignable_v<GraphicsScene>);
    static_assert(!std::is_move_constructible_v<GraphicsScene>);
    static_assert(!std::is_move_assignable_v<GraphicsScene>);
    static_assert(std::is_destructible_v<GraphicsScene>);

    GraphicsScene scene;
    CHECK(scene.parent() == nullptr);
    CHECK(scene.getViewWidget() == nullptr);

    return true;
}

bool testDefaultsAndSetters()
{
    pyqtgraph::GraphicsScene::GraphicsScene scene;
    CHECK(scene.clickRadius() == 2);
    CHECK(scene.moveDistance() == 5.0);

    scene.setClickRadius(7);
    scene.setMoveDistance(11.5);
    CHECK(scene.clickRadius() == 7);
    CHECK(scene.moveDistance() == 11.5);

    pyqtgraph::GraphicsScene::GraphicsScene configured(4, 8.25);
    CHECK(configured.clickRadius() == 4);
    CHECK(configured.moveDistance() == 8.25);

    return true;
}

bool testPrepareForPaintSignal()
{
    pyqtgraph::GraphicsScene::GraphicsScene scene;
    int prepareCount = 0;
    QObject::connect(&scene, &pyqtgraph::GraphicsScene::GraphicsScene::sigPrepareForPaint, [&prepareCount]() {
        ++prepareCount;
    });

    scene.prepareForPaint();
    scene.prepareForPaint();
    CHECK(prepareCount == 2);

    return true;
}

bool testAddRemoveSignals()
{
    pyqtgraph::GraphicsScene::GraphicsScene scene;
    QGraphicsRectItem item(0.0, 0.0, 1.0, 1.0);

    int addedCount = 0;
    int removedCount = 0;
    QGraphicsItem* addedItem = nullptr;
    QGraphicsItem* removedItem = nullptr;

    QObject::connect(&scene, &pyqtgraph::GraphicsScene::GraphicsScene::sigItemAdded,
        [&addedCount, &addedItem](QGraphicsItem* item) {
            ++addedCount;
            addedItem = item;
        });
    QObject::connect(&scene, &pyqtgraph::GraphicsScene::GraphicsScene::sigItemRemoved,
        [&removedCount, &removedItem](QGraphicsItem* item) {
            ++removedCount;
            removedItem = item;
        });

    scene.addItem(&item);
    CHECK(addedCount == 1);
    CHECK(addedItem == &item);
    CHECK(item.scene() == &scene);

    scene.removeItem(&item);
    CHECK(removedCount == 1);
    CHECK(removedItem == &item);
    CHECK(item.scene() == nullptr);

    return true;
}

bool testViewWidgetDiscovery()
{
    pyqtgraph::GraphicsScene::GraphicsScene scene;
    CHECK(scene.getViewWidget() == nullptr);

    QGraphicsView view(&scene);
    CHECK(scene.getViewWidget() == &view);

    view.setScene(nullptr);
    CHECK(scene.getViewWidget() == nullptr);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testGraphicsSceneTypeShape()) {
        return 1;
    }
    if (!testDefaultsAndSetters()) {
        return 1;
    }
    if (!testPrepareForPaintSignal()) {
        return 1;
    }
    if (!testAddRemoveSignals()) {
        return 1;
    }
    if (!testViewWidgetDiscovery()) {
        return 1;
    }

    return 0;
}
