#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp>

#include <QtCore/QRectF>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
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

bool rectNearlyEqual(const QRectF& lhs, const QRectF& rhs)
{
    return nearlyEqual(lhs.x(), rhs.x()) && nearlyEqual(lhs.y(), rhs.y()) && nearlyEqual(lhs.width(), rhs.width())
        && nearlyEqual(lhs.height(), rhs.height());
}

bool testFlatCurveBoundsIncludePenMargin()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> x{0.0, 1.0, 2.0};
    const std::vector<double> y{5.0, 5.0, 5.0};

    curve.setData(std::span<const double>(x), std::span<const double>(y));

    const QRectF bounds = curve.boundingRect();
    CHECK(rectNearlyEqual(bounds, QRectF(-0.5, 4.5, 3.0, 1.0)));
    CHECK(bounds.width() > 0.0);
    CHECK(bounds.height() > 0.0);

    return true;
}

bool testCurveBoundsIncludeEdgePenMargin()
{
    pyqtgraph::graphicsItems::PlotCurveItem curve;
    const std::vector<double> x{-2.0, 3.0};
    const std::vector<double> y{-4.0, 6.0};

    curve.setData(std::span<const double>(x), std::span<const double>(y));

    CHECK(rectNearlyEqual(curve.boundingRect(), QRectF(-2.5, -4.5, 6.0, 11.0)));

    return true;
}

bool testOffscreenDataIsTransformedBeforePaint()
{
    pyqtgraph::graphicsItems::PlotItem plot;
    plot.setGeometry(QRectF(0.0, 0.0, 800.0, 600.0));
    pyqtgraph::graphicsItems::PlotCurveItem curve(&plot);
    const std::vector<double> x{10000.0, 10010.0};
    const std::vector<double> y{50000.0, 50010.0};

    curve.setData(std::span<const double>(x), std::span<const double>(y));

    CHECK(!curve.transform().isIdentity());
    CHECK(curve.sceneBoundingRect().intersects(QRectF(0.0, 0.0, 800.0, 600.0)));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testFlatCurveBoundsIncludePenMargin()) {
        return 1;
    }
    if (!testCurveBoundsIncludeEdgePenMargin()) {
        return 1;
    }
    if (!testOffscreenDataIsTransformedBeforePaint()) {
        return 1;
    }

    return 0;
}
