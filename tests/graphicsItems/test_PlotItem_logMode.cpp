#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/graphicsItems/PlotDataItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ScatterPlotItem.hpp>

#include <QtCore/QString>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

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

bool nearlyEqual(double lhs, double rhs, double tolerance = 1.0e-9)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool testPlotItemPropagatesLogModeToExistingPlotDataItem()
{
    cppqtgraph::graphicsItems::PlotItem plot;
    auto data = std::make_unique<cppqtgraph::graphicsItems::PlotDataItem>(
        std::vector<double>{1.0, 10.0},
        std::vector<double>{2.0, 3.0});
    plot.addItem(data.get());
    plot.setLogMode(true, false);

    CHECK((data->logMode() == std::array<bool, 2>{true, false}));
    CHECK(nearlyEqual(data->curve()->xData()[0], 0.0));
    CHECK(nearlyEqual(data->curve()->xData()[1], 1.0));

    plot.removeItem(data.get());
    return true;
}

bool testPlotItemPropagatesLogModeToLaterAddedPlotDataItem()
{
    cppqtgraph::graphicsItems::PlotItem plot;
    plot.setLogMode(true, false);

    auto data = std::make_unique<cppqtgraph::graphicsItems::PlotDataItem>(
        std::vector<double>{1.0e-5, 1.0e-4},
        std::vector<double>{1.1, 1.2});
    data->setPen(nullptr);
    data->setSymbol(QStringLiteral("t"));
    plot.addItem(data.get());

    CHECK((data->logMode() == std::array<bool, 2>{true, false}));
    CHECK(nearlyEqual(data->scatter()->xData()[0], std::log10(1.0e-5)));
    CHECK(nearlyEqual(data->scatter()->xData()[1], std::log10(1.0e-4)));

    plot.removeItem(data.get());
    return true;
}

bool testPlotItemAutorangesLaterAddedItemWithMappedLogBounds()
{
    cppqtgraph::graphicsItems::PlotItem plot;
    plot.setLogMode(true, false);

    auto data = std::make_unique<cppqtgraph::graphicsItems::PlotDataItem>(
        std::vector<double>{1.0e-5, 1.0e-4},
        std::vector<double>{1.1, 1.2});
    data->setPen(nullptr);
    data->setSymbol(QStringLiteral("t"));
    plot.addItem(data.get());

    const auto viewRange = plot.viewRange();
    const auto xRange = viewRange[cppqtgraph::graphicsItems::ViewBox::XAxis];
    CHECK(xRange[1] - xRange[0] > 0.5);
    CHECK(xRange[0] < std::log10(1.0e-5));
    CHECK(xRange[1] > std::log10(1.0e-4));
    CHECK(xRange[0] < -1.0);

    plot.removeItem(data.get());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testPlotItemPropagatesLogModeToExistingPlotDataItem()) {
        return 1;
    }
    if (!testPlotItemPropagatesLogModeToLaterAddedPlotDataItem()) {
        return 1;
    }
    if (!testPlotItemAutorangesLaterAddedItemWithMappedLogBounds()) {
        return 1;
    }

    return 0;
}
