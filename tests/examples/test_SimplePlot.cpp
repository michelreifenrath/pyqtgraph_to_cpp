#define PYQTGRAPH_CPP_SIMPLEPLOT_NO_MAIN
#include "../../examples/SimplePlot.cpp"

#include <QtCore/QSize>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <cmath>
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

bool nearlyEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= 1.0e-12;
}

bool testSimplePlotFactory()
{
    auto example = pyqtgraph::examples::createSimplePlotExample();

    CHECK(example.widget != nullptr);
    CHECK(example.curve != nullptr);
    CHECK(example.widget->windowTitle() == QStringLiteral("Simplest possible plotting example"));
    CHECK(example.widget->size() == QSize(800, 600));
    CHECK(example.widget->getPlotItem() != nullptr);
    CHECK(example.curve->parentItem() == example.widget->getPlotItem());

    const auto x = example.curve->xData();
    const auto y = example.curve->yData();
    CHECK(x.size() == 100);
    CHECK(y.size() == 100);
    CHECK(nearlyEqual(x.front(), 0.0));
    CHECK(nearlyEqual(x.back(), 99.0));
    CHECK(nearlyEqual(y[0], 1.764052345967664));
    CHECK(nearlyEqual(y[1], 0.40015720836722329));
    CHECK(nearlyEqual(y[2], 0.9787379841057392));
    CHECK(nearlyEqual(y[20], -2.5529898158340787));
    CHECK(nearlyEqual(y[99], 0.40198936344470165));

    example.widget->show();
    const QPixmap pixmap = example.widget->grab();
    CHECK(!pixmap.isNull());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testSimplePlotFactory()) {
        return 1;
    }

    return 0;
}
