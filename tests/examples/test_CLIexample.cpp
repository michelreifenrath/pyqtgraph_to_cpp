#define CPPQTGRAPH_CLIEXAMPLE_NO_MAIN
#include "../../examples/CLIexample.cpp"

#include <QtCore/QSize>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <algorithm>
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
    return std::abs(lhs - rhs) <= 1.0e-6;
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 1.0e-5f;
}

bool testCLIexampleFactory()
{
    auto example = cppqtgraph::examples::createCLIexample();
    auto repeat = cppqtgraph::examples::createCLIexample();

    CHECK(example.plotWidget != nullptr);
    CHECK(example.imageWidget != nullptr);
    CHECK(example.plotCurve != nullptr);
    CHECK(example.imageView != nullptr);
    CHECK(example.state != nullptr);
    CHECK(example.plotWidget->windowTitle() == QStringLiteral("Simplest possible plotting example"));
    CHECK(example.imageWidget->windowTitle() == QStringLiteral("Simplest possible image example"));
    CHECK(example.plotWidget->size() == QSize(800, 600));
    CHECK(example.plotWidget->getPlotItem() != nullptr);
    CHECK(example.plotCurve->parentItem() == example.plotWidget->getPlotItem());

    const auto x = example.plotCurve->xData();
    const auto y = example.plotCurve->yData();
    CHECK(x.size() == cppqtgraph::examples::cliExamplePlotPointCount());
    CHECK(y.size() == cppqtgraph::examples::cliExamplePlotPointCount());
    CHECK(example.state->plotY.size() == cppqtgraph::examples::cliExamplePlotPointCount());
    CHECK(example.state->image.size()
          == cppqtgraph::examples::cliExampleImageWidth() * cppqtgraph::examples::cliExampleImageHeight());
    CHECK(nearlyEqual(x.front(), 0.0));
    CHECK(nearlyEqual(x.back(), static_cast<double>(cppqtgraph::examples::cliExamplePlotPointCount() - 1U)));
    CHECK(nearlyEqual(y[0], -0.78142694926954792));
    CHECK(nearlyEqual(y[1], -1.3016981227798621));
    CHECK(nearlyEqual(y[2], -0.74967537079454727));
    CHECK(nearlyEqual(y[3], -0.868094903818039));
    CHECK(nearlyEqual(y[4], 0.094398758380710088));
    CHECK(nearlyEqual(y[999], -1.0619949116689569));
    CHECK(nearlyEqual(example.state->plotY[999], y[999]));
    CHECK(nearlyEqual(repeat.plotCurve->yData()[999], y[999]));

    const auto [plotMin, plotMax] = std::minmax_element(y.begin(), y.end());
    CHECK(*plotMin >= -4.0);
    CHECK(*plotMax <= 4.0);

    auto* imageItem = example.imageView->getImageItem();
    CHECK(imageItem != nullptr);
    CHECK(example.imageView->hasImage());
    CHECK(imageItem->hasImage());
    CHECK(imageItem->width() == cppqtgraph::examples::cliExampleImageWidth());
    CHECK(imageItem->height() == cppqtgraph::examples::cliExampleImageHeight());
    CHECK(nearlyEqual(example.state->image.front(), -1.6874775f));
    CHECK(nearlyEqual(example.state->image[250U * cppqtgraph::examples::cliExampleImageWidth() + 250U], -0.50046998f));
    CHECK(nearlyEqual(repeat.state->image.front(), example.state->image.front()));

    const auto [imageMin, imageMax] = std::minmax_element(example.state->image.begin(), example.state->image.end());
    CHECK(*imageMin >= -5.0f);
    CHECK(*imageMax <= 5.0f);

    example.plotWidget->show();
    example.imageWidget->show();
    QApplication::processEvents();

    const QPixmap plotPixmap = example.plotWidget->grab();
    const QPixmap imagePixmap = example.imageWidget->grab();
    CHECK(!plotPixmap.isNull());
    CHECK(!imagePixmap.isNull());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testCLIexampleFactory()) {
        return 1;
    }

    return 0;
}
