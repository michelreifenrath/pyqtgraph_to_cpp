#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <QtCore/QRectF>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
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

bool rectNearlyEqual(const QRectF& lhs, const QRectF& rhs)
{
    return nearlyEqual(lhs.x(), rhs.x()) && nearlyEqual(lhs.y(), rhs.y()) && nearlyEqual(lhs.width(), rhs.width())
        && nearlyEqual(lhs.height(), rhs.height());
}

QRectF expandedForCurvePen(double x, double y, double width, double height)
{
    return QRectF(x - 0.5, y - 0.5, width + 1.0, height + 1.0);
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

bool testYOnlyGeneratesXData()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> y{2.0, -1.0, 4.0, 3.0};

    curve.setData(y);

    CHECK(spanEquals(curve.xData(), {0.0, 1.0, 2.0, 3.0}));
    CHECK(spanEquals(curve.yData(), y));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(0.0, -1.0, 3.0, 5.0)));

    return true;
}

bool testYOnlyNonFiniteValuesAreIgnoredForBounds()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::vector<double> y{nan, 2.0, inf, -1.0};

    curve.setData(y);

    CHECK(spanEquals(curve.xData(), {0.0, 1.0, 2.0, 3.0}));
    CHECK(curve.yData().size() == y.size());
    CHECK(std::isnan(curve.yData()[0]));
    CHECK(nearlyEqual(curve.yData()[1], 2.0));
    CHECK(std::isinf(curve.yData()[2]));
    CHECK(nearlyEqual(curve.yData()[3], -1.0));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(0.0, -1.0, 3.0, 3.0)));

    return true;
}

bool testXYDataIsCopied()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    std::vector<double> x{-2.0, 1.0, 5.0};
    std::vector<double> y{10.0, -4.0, 6.0};

    curve.setData(x, y);
    x[0] = 100.0;
    y[1] = 200.0;

    CHECK(spanEquals(curve.xData(), {-2.0, 1.0, 5.0}));
    CHECK(spanEquals(curve.yData(), {10.0, -4.0, 6.0}));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(-2.0, -4.0, 7.0, 14.0)));

    return true;
}

bool testFlatDataBoundsAreNonEmptyAndIncludePenMargin()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> x{0.0, 1.0, 2.0};
    const std::vector<double> y{5.0, 5.0, 5.0};

    curve.setData(x, y);

    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(0.0, 5.0, 2.0, 0.0)));
    CHECK(curve.boundingRect().width() > 0.0);
    CHECK(curve.boundingRect().height() > 0.0);

    return true;
}

bool testReturnedSpansCanBePassedBackToSetData()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> originalX{-2.0, 1.0, 5.0};
    const std::vector<double> originalY{10.0, -4.0, 6.0};
    const std::vector<double> replacementY{4.0, 5.0, 6.0};

    curve.setData(originalX, originalY);

    curve.setData(curve.xData(), replacementY);
    CHECK(spanEquals(curve.xData(), originalX));
    CHECK(spanEquals(curve.yData(), replacementY));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(-2.0, 4.0, 7.0, 2.0)));

    curve.setData(curve.yData());
    CHECK(spanEquals(curve.xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(curve.yData(), replacementY));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(0.0, 4.0, 2.0, 2.0)));

    curve.setData(curve.xData(), curve.yData());
    CHECK(spanEquals(curve.xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(curve.yData(), replacementY));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(0.0, 4.0, 2.0, 2.0)));

    curve.setData(curve.xData());
    CHECK(spanEquals(curve.xData(), {0.0, 1.0, 2.0}));
    CHECK(spanEquals(curve.yData(), {0.0, 1.0, 2.0}));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(0.0, 0.0, 2.0, 2.0)));

    return true;
}

bool testRepeatedSetDataReplacesDataAndBounds()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> firstX{-10.0, -5.0, 0.0};
    const std::vector<double> firstY{1.0, 2.0, 3.0};
    const std::vector<double> secondX{4.0, 8.0, 10.0};
    const std::vector<double> secondY{-3.0, 7.0, 1.0};

    curve.setData(firstX, firstY);
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(-10.0, 1.0, 10.0, 2.0)));

    curve.setData(secondX, secondY);

    CHECK(spanEquals(curve.xData(), secondX));
    CHECK(spanEquals(curve.yData(), secondY));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(4.0, -3.0, 6.0, 10.0)));

    return true;
}

bool testEmptyDataClearsDataAndBounds()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> x{1.0, 2.0, 3.0};
    const std::vector<double> y{4.0, 5.0, 6.0};
    const std::vector<double> empty;

    curve.setData(x, y);
    curve.setData(empty);

    CHECK(curve.xData().empty());
    CHECK(curve.yData().empty());
    CHECK(curve.boundingRect().isNull());

    return true;
}

bool testNonFiniteValuesAreIgnoredForBounds()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::vector<double> x{-inf, 2.0, inf, 4.0};
    const std::vector<double> y{nan, -1.0, 7.0, inf};

    curve.setData(x, y);

    CHECK(curve.xData().size() == x.size());
    CHECK(std::isinf(curve.xData()[0]) && curve.xData()[0] < 0.0);
    CHECK(nearlyEqual(curve.xData()[1], 2.0));
    CHECK(std::isinf(curve.xData()[2]) && curve.xData()[2] > 0.0);
    CHECK(nearlyEqual(curve.xData()[3], 4.0));
    CHECK(curve.yData().size() == y.size());
    CHECK(std::isnan(curve.yData()[0]));
    CHECK(nearlyEqual(curve.yData()[1], -1.0));
    CHECK(nearlyEqual(curve.yData()[2], 7.0));
    CHECK(std::isinf(curve.yData()[3]));
    CHECK(rectNearlyEqual(curve.boundingRect(), expandedForCurvePen(2.0, -1.0, 2.0, 8.0)));

    return true;
}

bool testAllNonFiniteAxisClearsBounds()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    const std::vector<double> nonFiniteX{nan, inf};
    const std::vector<double> finiteY{1.0, 2.0};
    const std::vector<double> finiteX{1.0, 2.0};
    const std::vector<double> nonFiniteY{nan, -inf};

    curve.setData(nonFiniteX, finiteY);
    CHECK(curve.boundingRect().isNull());

    curve.setData(finiteX, nonFiniteY);
    CHECK(curve.boundingRect().isNull());

    return true;
}

bool testMismatchedLengthsThrowAndLeaveDataUnchanged()
{
    cppqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> x{1.0, 4.0, 9.0};
    const std::vector<double> y{-2.0, 6.0, 3.0};
    const std::vector<double> mismatchedX{100.0, 200.0};
    const std::vector<double> mismatchedY{1.0};

    curve.setData(x, y);
    const QRectF boundsBefore = curve.boundingRect();

    bool threw = false;
    try {
        curve.setData(mismatchedX, mismatchedY);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(spanEquals(curve.xData(), x));
    CHECK(spanEquals(curve.yData(), y));
    CHECK(rectNearlyEqual(curve.boundingRect(), boundsBefore));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testYOnlyGeneratesXData()) {
        return 1;
    }
    if (!testYOnlyNonFiniteValuesAreIgnoredForBounds()) {
        return 1;
    }
    if (!testXYDataIsCopied()) {
        return 1;
    }
    if (!testFlatDataBoundsAreNonEmptyAndIncludePenMargin()) {
        return 1;
    }
    if (!testReturnedSpansCanBePassedBackToSetData()) {
        return 1;
    }
    if (!testRepeatedSetDataReplacesDataAndBounds()) {
        return 1;
    }
    if (!testEmptyDataClearsDataAndBounds()) {
        return 1;
    }
    if (!testNonFiniteValuesAreIgnoredForBounds()) {
        return 1;
    }
    if (!testAllNonFiniteAxisClearsBounds()) {
        return 1;
    }
    if (!testMismatchedLengthsThrowAndLeaveDataUnchanged()) {
        return 1;
    }

    return 0;
}
