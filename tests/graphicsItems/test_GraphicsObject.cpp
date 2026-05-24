#include <pyqtgraph/graphicsItems/GraphicsObject.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

class QPainter;

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

class ConcreteGraphicsObject final : public pyqtgraph::graphicsItems::GraphicsObject {
public:
    using pyqtgraph::graphicsItems::GraphicsObject::GraphicsObject;

    [[nodiscard]] QRectF boundingRect() const override
    {
        return QRectF(0.0, 0.0, 1.0, 1.0);
    }

    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}
};

bool testGraphicsObjectTypeShape()
{
    using pyqtgraph::graphicsItems::GraphicsItem;
    using pyqtgraph::graphicsItems::GraphicsObject;

    static_assert(std::is_base_of_v<QGraphicsObject, GraphicsObject>);
    static_assert(std::is_base_of_v<GraphicsItem, GraphicsObject>);
    static_assert(std::is_base_of_v<QObject, GraphicsObject>);
    static_assert(std::is_base_of_v<QGraphicsItem, GraphicsObject>);
    static_assert(std::is_abstract_v<GraphicsObject>);
    static_assert(!std::is_copy_constructible_v<GraphicsObject>);
    static_assert(!std::is_copy_assignable_v<GraphicsObject>);
    static_assert(!std::is_move_constructible_v<GraphicsObject>);
    static_assert(!std::is_move_assignable_v<GraphicsObject>);
    static_assert(std::is_destructible_v<GraphicsObject>);

    return true;
}

bool testGraphicsObjectBindsItselfAsGraphicsItem()
{
    ConcreteGraphicsObject object;

    CHECK(object.graphicsItem() == static_cast<QGraphicsItem*>(&object));
    CHECK(object.flags().testFlag(QGraphicsItem::ItemSendsGeometryChanges));
    CHECK(object.parentItem() == nullptr);

    ConcreteGraphicsObject parent;
    ConcreteGraphicsObject child(&parent);
    CHECK(child.parentItem() == &parent);
    CHECK(child.graphicsItem() == static_cast<QGraphicsItem*>(&child));

    return true;
}

bool testInheritedViewWidgetCacheInvalidatesAcrossScenes()
{
    QGraphicsScene firstScene;
    QGraphicsScene secondScene;
    ConcreteGraphicsObject object;
    firstScene.addItem(&object);

    QGraphicsView firstView(&firstScene);
    QGraphicsView secondView(&secondScene);
    CHECK(object.getViewWidget() == &firstView);
    CHECK(object.getViewWidget() == &firstView);

    firstScene.removeItem(&object);
    CHECK(object.getViewWidget() == nullptr);

    secondScene.addItem(&object);
    CHECK(object.getViewWidget() == &secondView);

    secondScene.removeItem(&object);
    CHECK(object.getViewWidget() == nullptr);

    return true;
}

bool testInheritedViewWidgetCacheInvalidatesAcrossParents()
{
    QGraphicsScene scene;
    ConcreteGraphicsObject sceneParent;
    ConcreteGraphicsObject detachedParent;
    ConcreteGraphicsObject child;
    scene.addItem(&sceneParent);

    QGraphicsView view(&scene);
    child.setParentItem(&sceneParent);
    CHECK(child.getViewWidget() == &view);
    CHECK(child.getViewWidget() == &view);

    child.setParentItem(&detachedParent);
    CHECK(child.getViewWidget() == nullptr);

    child.setParentItem(nullptr);
    scene.removeItem(&sceneParent);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testGraphicsObjectTypeShape()) {
        return 1;
    }
    if (!testGraphicsObjectBindsItselfAsGraphicsItem()) {
        return 1;
    }
    if (!testInheritedViewWidgetCacheInvalidatesAcrossScenes()) {
        return 1;
    }
    if (!testInheritedViewWidgetCacheInvalidatesAcrossParents()) {
        return 1;
    }

    return 0;
}
