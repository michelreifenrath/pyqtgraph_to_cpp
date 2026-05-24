#include <pyqtgraph/graphicsItems/GraphicsItem.hpp>

#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>

#include <iostream>
#include <memory>
#include <string_view>

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

bool testNoViewReturnsNull()
{
    QGraphicsRectItem host(QRectF(0.0, 0.0, 1.0, 1.0));
    pyqtgraph::graphicsItems::GraphicsItem item(&host);

    CHECK(item.getViewWidget() == nullptr);

    QGraphicsScene scene;
    scene.addItem(&host);
    CHECK(item.getViewWidget() == nullptr);

    scene.removeItem(&host);
    return true;
}

bool testViewWidgetCachingAndForget()
{
    QGraphicsScene scene;
    QGraphicsRectItem host(QRectF(0.0, 0.0, 1.0, 1.0));
    scene.addItem(&host);

    QGraphicsView firstView(&scene);
    pyqtgraph::graphicsItems::GraphicsItem item(&host);

    QGraphicsView* cached = item.getViewWidget();
    CHECK(cached == &firstView);
    CHECK(item.getViewWidget() == cached);

    QGraphicsView secondView(&scene);
    CHECK(item.getViewWidget() == cached);

    scene.removeItem(&host);
    item.forgetViewWidget();
    CHECK(item.getViewWidget() == nullptr);

    scene.addItem(&host);
    CHECK(item.getViewWidget() != nullptr);

    scene.removeItem(&host);
    return true;
}

bool testDeletedViewClearsCachedPointer()
{
    QGraphicsScene scene;
    QGraphicsRectItem host(QRectF(0.0, 0.0, 1.0, 1.0));
    scene.addItem(&host);
    pyqtgraph::graphicsItems::GraphicsItem item(&host);

    auto view = std::make_unique<QGraphicsView>(&scene);
    QPointer<QGraphicsView> viewPointer(view.get());

    CHECK(item.getViewWidget() == view.get());
    CHECK(item.getViewWidget() == viewPointer.data());

    view.reset();
    CHECK(viewPointer.isNull());
    CHECK(item.getViewWidget() == nullptr);

    scene.removeItem(&host);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testNoViewReturnsNull()) {
        return 1;
    }
    if (!testViewWidgetCachingAndForget()) {
        return 1;
    }
    if (!testDeletedViewClearsCachedPointer()) {
        return 1;
    }

    return 0;
}
