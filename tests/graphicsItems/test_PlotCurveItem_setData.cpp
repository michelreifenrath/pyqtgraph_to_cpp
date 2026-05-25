#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>

#include <QtCore/QRectF>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
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
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> y{2.0, -1.0, 4.0, 3.0};

    curve.setData(y);

    CHECK(spanEquals(curve.xData(), {0.0, 1.0, 2.0, 3.0}));
    CHECK(spanEquals(curve.yData(), y));
    CHECK(rectNearlyEqual(curve.boundingRect(), QRectF(0.0, -1.0, 3.0, 5.0)));

    return true;
}

bool testXYDataIsCopied()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    std::vector<double> x{-2.0, 1.0, 5.0};
    std::vector<double> y{10.0, -4.0, 6.0};

    curve.setData(x, y);
    x[0] = 100.0;
    y[1] = 200.0;

    CHECK(spanEquals(curve.xData(), {-2.0, 1.0, 5.0}));
    CHECK(spanEquals(curve.yData(), {10.0, -4.0, 6.0}));
    CHECK(rectNearlyEqual(curve.boundingRect(), QRectF(-2.0, -4.0, 7.0, 14.0)));

    return true;
}

bool testRepeatedSetDataReplacesDataAndBounds()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> firstX{-10.0, -5.0, 0.0};
    const std::vector<double> firstY{1.0, 2.0, 3.0};
    const std::vector<double> secondX{4.0, 8.0, 10.0};
    const std::vector<double> secondY{-3.0, 7.0, 1.0};

    curve.setData(firstX, firstY);
    CHECK(rectNearlyEqual(curve.boundingRect(), QRectF(-10.0, 1.0, 10.0, 2.0)));

    curve.setData(secondX, secondY);

    CHECK(spanEquals(curve.xData(), secondX));
    CHECK(spanEquals(curve.yData(), secondY));
    CHECK(rectNearlyEqual(curve.boundingRect(), QRectF(4.0, -3.0, 6.0, 10.0)));

    return true;
}

bool testEmptyDataClearsDataAndBounds()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
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

bool testMismatchedLengthsThrowAndLeaveDataUnchanged()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
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
    if (!testXYDataIsCopied()) {
        return 1;
    }
    if (!testRepeatedSetDataReplacesDataAndBounds()) {
        return 1;
    }
    if (!testEmptyDataClearsDataAndBounds()) {
        return 1;
    }
    if (!testMismatchedLengthsThrowAndLeaveDataUnchanged()) {
        return 1;
    }

    return 0;
}
