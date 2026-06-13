#define CPPQTGRAPH_PARAMETERTREE_NO_MAIN
#include "../../examples/parametertree.cpp"

#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

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

bool testParametertreeFactory()
{
    auto example = cppqtgraph::examples::createParametertreeExample();

    CHECK(example.window != nullptr);
    CHECK(example.root != nullptr);
    CHECK(example.tree1 != nullptr);
    CHECK(example.tree2 != nullptr);
    CHECK(example.window->windowTitle() == QStringLiteral("pyqtgraph example: Parameter Tree"));
    CHECK(example.tree1->parameters() == example.root.get());
    CHECK(example.tree2->parameters() == example.root.get());

    const auto labels = example.window->findChildren<QLabel*>();
    CHECK(!labels.isEmpty());
    CHECK(labels.front()->text().contains(QStringLiteral("two views of the same data")));

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

    if (!testParametertreeFactory()) {
        return 1;
    }

    return 0;
}
