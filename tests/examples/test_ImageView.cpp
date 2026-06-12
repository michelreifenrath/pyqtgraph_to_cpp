#define CPPQTGRAPH_IMAGEVIEW_NO_MAIN
#include "../../examples/ImageView.cpp"

#include <QtCore/QSize>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

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

bool testImageViewFactory()
{
    auto example = cppqtgraph::examples::createImageViewExample();

    CHECK(example.window != nullptr);
    CHECK(example.imageView != nullptr);
    CHECK(example.window->windowTitle() == QStringLiteral("pyqtgraph example: ImageView"));
    CHECK(example.window->size() == QSize(800, 800));
    CHECK(example.window->centralWidget() == example.imageView);

    example.window->show();
    const QPixmap pixmap = example.window->grab();
    CHECK(!pixmap.isNull());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testImageViewFactory()) {
        return 1;
    }

    return 0;
}
