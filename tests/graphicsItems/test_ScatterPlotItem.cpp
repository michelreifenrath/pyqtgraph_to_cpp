#include <cppqtgraph/graphicsItems/GraphicsObject.hpp>
#include <cppqtgraph/graphicsItems/ScatterPlotItem.hpp>

#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <limits>
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

bool testPxModeDataBoundsExcludeSymbolPadding()
{
    cppqtgraph::graphicsItems::ScatterPlotItem item;
    const std::vector<double> x{0.0, 10.0};
    const std::vector<double> y{-2.0, 3.0};

    item.setPxMode(true);
    item.setSize(12.0);
    item.setPen(QPen(Qt::white, 2.0));
    item.setData(x, y);

    const auto [yMin, yMax] = item.dataBounds(1);
    CHECK(nearlyEqual(yMin, -2.0));
    CHECK(nearlyEqual(yMax, 3.0));

    const auto autoBounds = item.autoRangeBoundsRect();
    CHECK(autoBounds.has_value());
    CHECK(nearlyEqual(autoBounds->top(), -2.0));
    CHECK(nearlyEqual(autoBounds->bottom(), 3.0));
    CHECK(nearlyEqual(autoBounds->height(), 5.0));

    const QRectF paintBounds = item.boundingRect();
    CHECK(nearlyEqual(paintBounds.top(), -2.0));
    CHECK(nearlyEqual(paintBounds.bottom(), 3.0));
    CHECK(nearlyEqual(paintBounds.height(), 5.0));

    return true;
}

bool testRenderRedCircleWhiteOutline()
{
    const QBrush brush(QColor(255, 0, 0));
    const QImage fillImage = cppqtgraph::graphicsItems::renderSymbol(
        QStringLiteral("o"), 10.0, QPen(Qt::NoPen), brush, 1.0);
    CHECK(!fillImage.isNull());

    bool foundRedFill = false;
    for (int row = 0; row < fillImage.height(); ++row) {
        for (int column = 0; column < fillImage.width(); ++column) {
            const QColor pixel = QColor::fromRgba(fillImage.pixel(column, row));
            if (pixel.alpha() > 0 && pixel.red() > pixel.green() && pixel.red() > pixel.blue()) {
                foundRedFill = true;
                break;
            }
        }
        if (foundRedFill) {
            break;
        }
    }
    CHECK(foundRedFill);

    const QPen outlinePen(Qt::white, 1.0);
    const QImage outlinedImage = cppqtgraph::graphicsItems::renderSymbol(
        QStringLiteral("o"), 10.0, outlinePen, brush, 1.0);
    CHECK(!outlinedImage.isNull());

    bool foundWhiteOutline = false;
    bool foundRenderedPixel = false;
    for (int row = 0; row < outlinedImage.height(); ++row) {
        for (int column = 0; column < outlinedImage.width(); ++column) {
            const QColor pixel = QColor::fromRgba(outlinedImage.pixel(column, row));
            if (pixel.alpha() == 0) {
                continue;
            }
            foundRenderedPixel = true;
            if (pixel.red() > 200 && pixel.green() > 200 && pixel.blue() > 200) {
                foundWhiteOutline = true;
            }
        }
    }
    CHECK(foundRenderedPixel);
    CHECK(foundWhiteOutline);

    return true;
}

bool testAutoRangeBoundsRectIgnoresNonFinitePoints()
{
    cppqtgraph::graphicsItems::ScatterPlotItem item;
    std::vector<double> x{1.0, 2.0, 3.0};
    std::vector<double> y{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), 4.0};
    item.setData(x, y);

    const auto bounds = item.autoRangeBoundsRect();
    CHECK(bounds.has_value());
    CHECK(nearlyEqual(bounds->center().x(), 3.0));
    CHECK(nearlyEqual(bounds->center().y(), 4.0));
    CHECK(bounds->width() > 0.0);
    CHECK(bounds->height() > 0.0);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testPxModeDataBoundsExcludeSymbolPadding()) {
        return 1;
    }
    if (!testRenderRedCircleWhiteOutline()) {
        return 1;
    }
    if (!testAutoRangeBoundsRectIgnoresNonFinitePoints()) {
        return 1;
    }

    return 0;
}
