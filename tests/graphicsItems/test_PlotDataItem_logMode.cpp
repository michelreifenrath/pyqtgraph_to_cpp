#include <cppqtgraph/graphicsItems/PlotDataItem.hpp>
#include <cppqtgraph/graphicsItems/ScatterPlotItem.hpp>

#include <QtCore/QString>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
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

bool nearlyEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= 1.0e-12;
}

bool isNaN(double value)
{
    return std::isnan(value);
}

bool spanEquals(std::span<const double> values, const std::vector<double>& expected)
{
    if (values.size() != expected.size()) {
        return false;
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (!nearlyEqual(values[index], expected[index])) {
            return false;
        }
    }
    return true;
}

bool testLogModeMapsXToChildrenAndPreservesRawData()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    const std::vector<double> x{0.1, 1.0, 10.0, -1.0, 0.0};
    const std::vector<double> y{1.0, 2.0, 3.0, 4.0, 5.0};

    item.setData(x, y);
    item.setLogMode(true, false);

    CHECK((item.logMode() == std::array<bool, 2>{true, false}));
    CHECK(spanEquals(item.xData(), x));
    CHECK(spanEquals(item.yData(), y));

    const auto curveX = item.curve()->xData();
    CHECK(curveX.size() == x.size());
    CHECK(nearlyEqual(curveX[0], std::log10(0.1)));
    CHECK(nearlyEqual(curveX[1], std::log10(1.0)));
    CHECK(nearlyEqual(curveX[2], std::log10(10.0)));
    CHECK(isNaN(curveX[3]));
    CHECK(isNaN(curveX[4]));
    CHECK(spanEquals(item.curve()->yData(), y));

    item.setSymbol(QStringLiteral("o"));
    const auto scatterX = item.scatter()->xData();
    CHECK(scatterX.size() == x.size());
    CHECK(nearlyEqual(scatterX[0], std::log10(0.1)));
    CHECK(isNaN(scatterX[3]));

    return true;
}

bool testLogModeMapsYWhenEnabled()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    const std::vector<double> x{1.0, 10.0, 100.0};
    const std::vector<double> y{0.01, 1.0, -2.0};

    item.setData(x, y);
    item.setLogMode(false, true);

    CHECK(spanEquals(item.curve()->xData(), x));
    const auto curveY = item.curve()->yData();
    CHECK(nearlyEqual(curveY[0], std::log10(0.01)));
    CHECK(nearlyEqual(curveY[1], std::log10(1.0)));
    CHECK(isNaN(curveY[2]));

    return true;
}

bool testLogModeBoundsUseMappedFiniteValues()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    const std::vector<double> x{1.0e-7, 1.0e-4};
    const std::vector<double> y{1.03, 1.06};

    item.setData(x, y);
    item.setPen(nullptr);
    item.setSymbol(QStringLiteral("t"));

    const auto rawBounds = item.autoRangeBoundsRect();
    CHECK(rawBounds.has_value());
    CHECK(rawBounds->left() < 1.0e-6);
    CHECK(rawBounds->right() > 1.0e-5);

    item.setLogMode(true, false);
    const auto mappedBounds = item.autoRangeBoundsRect();
    CHECK(mappedBounds.has_value());
    CHECK(mappedBounds->left() < std::log10(1.0e-6));
    CHECK(mappedBounds->right() > std::log10(1.0e-5));

    return true;
}

bool testSetDataAfterLogModeRecomputesMappedDisplay()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    item.setLogMode(true, false);
    item.setData(std::vector<double>{1.0, 10.0}, std::vector<double>{2.0, 3.0});

    CHECK(nearlyEqual(item.curve()->xData()[0], 0.0));
    CHECK(nearlyEqual(item.curve()->xData()[1], 1.0));

    item.setLogMode(false, false);
    CHECK(nearlyEqual(item.curve()->xData()[0], 1.0));
    CHECK(nearlyEqual(item.curve()->xData()[1], 10.0));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testLogModeMapsXToChildrenAndPreservesRawData()) {
        return 1;
    }
    if (!testLogModeMapsYWhenEnabled()) {
        return 1;
    }
    if (!testLogModeBoundsUseMappedFiniteValues()) {
        return 1;
    }
    if (!testSetDataAfterLogModeRecomputesMappedDisplay()) {
        return 1;
    }

    return 0;
}
