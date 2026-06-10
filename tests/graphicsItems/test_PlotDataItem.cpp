#include <cppqtgraph/graphicsItems/PlotDataItem.hpp>
#include <cppqtgraph/graphicsItems/GraphicsObject.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <QtCore/QPointer>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsObject>

#include <cmath>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
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

bool spanEquals(std::span<const double> values, const std::vector<double>& expected)
{
    if (values.size() != expected.size()) {
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (!nearlyEqual(values[i], expected[i])) {
            return false;
        }
    }
    return true;
}

bool testTypeShapeAndDefaultState()
{
    using cppqtgraph::graphicsItems::GraphicsItem;
    using cppqtgraph::graphicsItems::GraphicsObject;
    using cppqtgraph::graphicsItems::PlotDataItem;

    static_assert(std::is_constructible_v<PlotDataItem>);
    static_assert(std::is_constructible_v<PlotDataItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<PlotDataItem>);
    static_assert(!std::is_copy_constructible_v<PlotDataItem>);
    static_assert(!std::is_copy_assignable_v<PlotDataItem>);
    static_assert(!std::is_move_constructible_v<PlotDataItem>);
    static_assert(!std::is_move_assignable_v<PlotDataItem>);
    static_assert(std::is_base_of_v<GraphicsObject, PlotDataItem>);
    static_assert(std::is_base_of_v<GraphicsItem, PlotDataItem>);
    static_assert(std::is_base_of_v<QGraphicsObject, PlotDataItem>);
    static_assert(std::is_base_of_v<QObject, PlotDataItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, PlotDataItem>);

    PlotDataItem item;

    CHECK(item.toGraphicsObject() == &item);
    CHECK(item.flags().testFlag(QGraphicsItem::ItemHasNoContents));
    CHECK(item.curve() != nullptr);
    CHECK(item.curve()->parentItem() == &item);
    CHECK(item.hasData() == false);
    CHECK(item.xData().empty());
    CHECK(item.yData().empty());
    CHECK(item.curve()->xData().empty());
    CHECK(item.curve()->yData().empty());
    CHECK(item.lineVisible());
    CHECK(item.pen().color() == QColor(200, 200, 200));
    CHECK(item.pen().isCosmetic());

    return true;
}

bool testYOnlyDataNormalizesAndForwardsToCurve()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    const std::vector<double> y{2.0, -1.0, 4.0};

    item.setData(y);

    CHECK(item.hasData());
    CHECK(spanEquals(item.xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(item.yData(), y));
    CHECK(spanEquals(item.curve()->xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(item.curve()->yData(), y));
    CHECK(item.curve()->isVisible());

    return true;
}

bool testXYDataIsCopiedAndMismatchPreservesPreviousData()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    std::vector<double> x{-2.0, 1.0, 5.0};
    std::vector<double> y{10.0, -4.0, 6.0};

    item.setData(x, y);
    x[0] = 100.0;
    y[1] = 200.0;

    CHECK(spanEquals(item.xData(), {-2.0, 1.0, 5.0}));
    CHECK(spanEquals(item.yData(), {10.0, -4.0, 6.0}));
    CHECK(spanEquals(item.curve()->xData(), {-2.0, 1.0, 5.0}));
    CHECK(spanEquals(item.curve()->yData(), {10.0, -4.0, 6.0}));

    bool threw = false;
    try {
        const std::vector<double> mismatchedX{1.0, 2.0};
        const std::vector<double> mismatchedY{3.0};
        item.setData(mismatchedX, mismatchedY);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(spanEquals(item.xData(), {-2.0, 1.0, 5.0}));
    CHECK(spanEquals(item.yData(), {10.0, -4.0, 6.0}));
    CHECK(spanEquals(item.curve()->xData(), {-2.0, 1.0, 5.0}));
    CHECK(spanEquals(item.curve()->yData(), {10.0, -4.0, 6.0}));

    return true;
}

bool testClearAndEmptyInputsResetWrapperAndCurve()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    const std::vector<double> x{1.0, 2.0, 3.0};
    const std::vector<double> y{4.0, 5.0, 6.0};
    const std::vector<double> empty;

    item.setData(x, y);
    item.setData();
    CHECK(!item.hasData());
    CHECK(item.xData().empty());
    CHECK(item.yData().empty());
    CHECK(item.curve()->xData().empty());
    CHECK(item.curve()->yData().empty());
    CHECK(!item.curve()->isVisible());

    item.setData(x, y);
    item.setData(empty, empty);
    CHECK(!item.hasData());
    CHECK(item.xData().empty());
    CHECK(item.yData().empty());
    CHECK(item.curve()->xData().empty());
    CHECK(item.curve()->yData().empty());
    CHECK(!item.curve()->isVisible());

    return true;
}

bool testPenStateControlsLineVisibilityWithoutDroppingData()
{
    cppqtgraph::graphicsItems::PlotDataItem item;
    const std::vector<double> y{2.0, -1.0, 4.0};
    const QPen redPen(QColor(255, 0, 0));

    item.setData(y);
    item.setPen(redPen);

    CHECK(item.lineVisible());
    CHECK(item.pen().color() == QColor(255, 0, 0));
    CHECK(item.curve()->isVisible());
    CHECK(spanEquals(item.curve()->yData(), y));

    item.setPen(nullptr);
    CHECK(!item.lineVisible());
    CHECK(!item.curve()->isVisible());
    CHECK(item.hasData());
    CHECK(spanEquals(item.xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(item.yData(), y));

    item.setPen(redPen);
    CHECK(item.lineVisible());
    CHECK(item.curve()->isVisible());
    CHECK(spanEquals(item.curve()->xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(item.curve()->yData(), y));

    return true;
}

bool testCurveLifetimeIsOwnedByPlotDataItem()
{
    QPointer<cppqtgraph::graphicsItems::PlotCurveItem> curve;

    {
        cppqtgraph::graphicsItems::PlotDataItem item;
        curve = item.curve();
        CHECK(curve != nullptr);
        CHECK(curve->parentItem() == &item);

        const auto* firstCurve = item.curve();
        const std::vector<double> y{1.0, 2.0, 3.0};
        item.setData(y);
        item.setData(y);
        CHECK(item.curve() == firstCurve);
    }

    CHECK(curve.isNull());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testTypeShapeAndDefaultState()) {
        return 1;
    }
    if (!testYOnlyDataNormalizesAndForwardsToCurve()) {
        return 1;
    }
    if (!testXYDataIsCopiedAndMismatchPreservesPreviousData()) {
        return 1;
    }
    if (!testClearAndEmptyInputsResetWrapperAndCurve()) {
        return 1;
    }
    if (!testPenStateControlsLineVisibilityWithoutDroppingData()) {
        return 1;
    }
    if (!testCurveLifetimeIsOwnedByPlotDataItem()) {
        return 1;
    }

    return 0;
}
