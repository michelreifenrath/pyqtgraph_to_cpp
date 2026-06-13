#define CPPQTGRAPH_PLOTTING_NO_MAIN
#include "../../examples/Plotting.cpp"

#include <cppqtgraph/graphicsItems/AxisItem.hpp>
#include <cppqtgraph/graphicsItems/ButtonItem.hpp>
#include <cppqtgraph/graphicsItems/ScatterPlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <array>
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

bool nearlyEqual(double lhs, double rhs, double tolerance = 1.0e-9)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool testPlottingStructure(cppqtgraph::examples::PlottingExample& example)
{
    CHECK(example.widget != nullptr);
    CHECK(example.state != nullptr);
    CHECK(example.widget->windowTitle() == QStringLiteral("pyqtgraph example: Plotting"));
    CHECK(example.widget->size() == QSize(1000, 600));
    CHECK(example.plots.size() == cppqtgraph::examples::plottingPlotCount());
    CHECK(example.timer != nullptr);
    CHECK(example.timer->interval() == cppqtgraph::examples::plottingTimerIntervalMs());
    CHECK(example.region != nullptr);
    CHECK(example.p1Curve != nullptr);
    CHECK(example.p5Scatter != nullptr);
    CHECK(example.p6Curve != nullptr);
    CHECK(example.p7Curve != nullptr);
    CHECK(example.p8Curve != nullptr);
    CHECK(example.p9Curve != nullptr);

    for (std::size_t index = 0; index < example.plots.size(); ++index) {
        CHECK(example.plots[index] != nullptr);
    }

    return true;
}

bool testPlottingData(cppqtgraph::examples::PlottingExample& example)
{
    CHECK(example.p1Curve->yData().size() == 100);
    CHECK(nearlyEqual(example.p1Curve->yData()[0], 0.51725299714285011));
    CHECK(nearlyEqual(example.p1Curve->yData()[99], -0.44365542819507126));

    CHECK(example.p2RedCurve->yData().size() == 100);
    CHECK(example.p2GreenCurve->yData().size() == 110);
    CHECK(example.p2BlueCurve->yData().size() == 120);
    CHECK(nearlyEqual(example.p2GreenCurve->yData()[0], 6.6289257928958225));
    CHECK(example.p2RedCurve->pen().color() == QColor(255, 0, 0));
    CHECK(example.p2GreenCurve->pen().color() == QColor(0, 255, 0));
    CHECK(example.p2BlueCurve->pen().color() == QColor(0, 0, 255));
    CHECK(example.plots[1]->legend() == nullptr);

    CHECK(example.p3Curve->pen().color() == QColor(200, 200, 200));
    CHECK(example.p3Curve->symbolsVisible());
    CHECK(example.p3Curve->symbol() == QStringLiteral("o"));
    CHECK(example.p3Curve->symbolBrush().color() == QColor(255, 0, 0));
    CHECK(example.p3Curve->symbolPen().color() == QColor(255, 255, 255));

    CHECK(example.plots[3]->gridState().x);
    CHECK(example.plots[3]->gridState().y);

    CHECK(example.p5Scatter->xData().size() == example.p5Scatter->yData().size());
    CHECK(example.p5Scatter->xData().size() > 400);
    CHECK(!example.p5Scatter->lineVisible());
    CHECK(example.p5Scatter->symbolsVisible());
    CHECK(example.p5Scatter->symbol() == QStringLiteral("t"));
    CHECK(example.p5Scatter->symbolSize() == 10.0);

    const auto logMode = example.plots[4]->logMode();
    CHECK(logMode[0]);
    CHECK(!logMode[1]);

    auto* leftAxis = example.plots[4]->getAxis(QStringLiteral("left"));
    auto* bottomAxis = example.plots[4]->getAxis(QStringLiteral("bottom"));
    CHECK(leftAxis != nullptr);
    CHECK(bottomAxis != nullptr);
    CHECK(leftAxis->labelText() == QStringLiteral("Y Axis"));
    CHECK(leftAxis->labelUnits() == QStringLiteral("A"));
    CHECK(bottomAxis->labelText() == QStringLiteral("Y Axis"));
    CHECK(bottomAxis->labelUnits() == QStringLiteral("s"));

    CHECK(example.p7Curve->fillLevel().has_value());
    CHECK(nearlyEqual(example.p7Curve->fillLevel().value(), -0.3));
    CHECK(example.p7Curve->fillBrush().color() == QColor(50, 50, 200, 100));
    CHECK(!example.plots[6]->getAxis(QStringLiteral("bottom"))->isVisible());

    CHECK(example.state->sincData.size() == cppqtgraph::examples::plottingSincPointCount());
    CHECK(nearlyEqual(example.state->sincData[400], 0.044045117356285558, 1.0e-12));
    CHECK(nearlyEqual(example.state->sincData[700], 0.016059964565486425, 1.0e-12));
    CHECK(example.p8Curve->pen().color() == QColor(255, 255, 255, 200));

    const auto [regionMinimum, regionMaximum] = example.region->getRegion();
    CHECK(nearlyEqual(regionMinimum, 400.0));
    CHECK(nearlyEqual(regionMaximum, 700.0));

    return true;
}

bool testPlottingTimer(cppqtgraph::examples::PlottingExample& example)
{
    const std::size_t initialPtr = example.state->updatePtr;
    const auto initialY = example.p6Curve->yData().empty() ? 0.0 : example.p6Curve->yData()[0];

    cppqtgraph::examples::advancePlottingTimer(example, 1);
    CHECK(example.state->updatePtr == (initialPtr + 1) % cppqtgraph::examples::plottingUpdateRowCount());
    CHECK(example.p6Curve->yData().size() == cppqtgraph::examples::plottingUpdateColumnCount());
    if (!example.p6Curve->yData().empty()) {
        CHECK(!nearlyEqual(example.p6Curve->yData()[0], initialY) || initialPtr % 10 == 9);
    }
    CHECK(example.state->updateAutoRangeDisabled);
    CHECK(!example.plots[5]->getViewBox()->autoRangeEnabled()[0]);
    CHECK(!example.plots[5]->getViewBox()->autoRangeEnabled()[1]);

    return true;
}

bool testPlottingP8AutorangeRange(cppqtgraph::examples::PlottingExample& example)
{
    example.widget->show();
    QApplication::processEvents();

    const auto* p8 = example.plots[7];
    const auto viewRange = p8->viewRange();
    const auto xRange = viewRange[cppqtgraph::graphicsItems::ViewBox::XAxis];
    const auto yRange = viewRange[cppqtgraph::graphicsItems::ViewBox::YAxis];

    CHECK(std::isfinite(xRange[0]));
    CHECK(std::isfinite(xRange[1]));
    CHECK(std::isfinite(yRange[0]));
    CHECK(std::isfinite(yRange[1]));
    CHECK(xRange[1] > xRange[0]);
    CHECK(yRange[1] > yRange[0]);
    CHECK(yRange[1] - yRange[0] < 5.0);

    const QRectF curveBounds = example.p8Curve->boundingRect();
    const auto childrenBounds = p8->getViewBox()->childrenBounds();
    CHECK(childrenBounds[cppqtgraph::graphicsItems::ViewBox::XAxis].has_value());
    CHECK(childrenBounds[cppqtgraph::graphicsItems::ViewBox::YAxis].has_value());
    CHECK(nearlyEqual((*childrenBounds[cppqtgraph::graphicsItems::ViewBox::XAxis])[0], curveBounds.left(), 1.0e-6));
    CHECK(nearlyEqual((*childrenBounds[cppqtgraph::graphicsItems::ViewBox::XAxis])[1], curveBounds.right(), 1.0e-6));
    CHECK(nearlyEqual((*childrenBounds[cppqtgraph::graphicsItems::ViewBox::YAxis])[0], curveBounds.top(), 1.0e-6));
    CHECK(nearlyEqual((*childrenBounds[cppqtgraph::graphicsItems::ViewBox::YAxis])[1], curveBounds.bottom(), 1.0e-6));

    int rangeChanges = 0;
    const auto connection = QObject::connect(p8->getViewBox(),
        &cppqtgraph::graphicsItems::ViewBox::sigRangeChanged,
        p8->getViewBox(),
        [&rangeChanges](cppqtgraph::graphicsItems::ViewBox*, cppqtgraph::graphicsItems::ViewBox::Range2D, std::array<bool, 2>) {
            ++rangeChanges;
        });
    QApplication::processEvents();
    const int changesAfterSettle = rangeChanges;
    QApplication::processEvents();
    CHECK(rangeChanges == changesAfterSettle);
    QObject::disconnect(connection);

    return true;
}

bool testPlottingRegionZoomLinkage(cppqtgraph::examples::PlottingExample& example)
{
    const auto p9Range = example.plots[8]->viewRange()[cppqtgraph::graphicsItems::ViewBox::XAxis];
    CHECK(nearlyEqual(p9Range[0], 400.0, 1.0e-6));
    CHECK(nearlyEqual(p9Range[1], 700.0, 1.0e-6));

    example.region->setRegion(std::make_pair(420.0, 680.0));
    QApplication::processEvents();
    const auto updatedFromRegion = example.plots[8]->viewRange()[cppqtgraph::graphicsItems::ViewBox::XAxis];
    CHECK(nearlyEqual(updatedFromRegion[0], 420.0, 1.0e-6));
    CHECK(nearlyEqual(updatedFromRegion[1], 680.0, 1.0e-6));

    example.plots[8]->setXRange(450.0, 650.0, 0.0);
    QApplication::processEvents();
    const auto [regionMinimum, regionMaximum] = example.region->getRegion();
    CHECK(nearlyEqual(regionMinimum, 450.0, 1.0e-6));
    CHECK(nearlyEqual(regionMaximum, 650.0, 1.0e-6));

    return true;
}

bool testPlottingDeterminism()
{
    auto first = cppqtgraph::examples::createPlottingExample();
    auto second = cppqtgraph::examples::createPlottingExample();

    CHECK(first.p1Curve->yData().size() == second.p1Curve->yData().size());
    CHECK(nearlyEqual(first.p1Curve->yData()[0], second.p1Curve->yData()[0]));
    CHECK(nearlyEqual(first.state->sincData[400], second.state->sincData[400]));
    CHECK(first.p5Scatter->xData().size() == second.p5Scatter->xData().size());

    return true;
}

bool testPlottingP5LogMappedAutorangeAndRender(cppqtgraph::examples::PlottingExample& example)
{
    constexpr double kP5LogXMin = -7.388271382801615;
    constexpr double kP5LogXMax = -4.509981088978167;
    constexpr double kP5LinearYMin = 1.02795;
    constexpr double kP5LinearYMax = 1.0679;

    example.widget->show();
    QApplication::processEvents();

    auto* p5 = example.plots[4];
    const auto viewRange = p5->viewRange();
    const auto xRange = viewRange[cppqtgraph::graphicsItems::ViewBox::XAxis];
    const auto yRange = viewRange[cppqtgraph::graphicsItems::ViewBox::YAxis];

    CHECK(std::isfinite(xRange[0]));
    CHECK(std::isfinite(xRange[1]));
    CHECK(std::isfinite(yRange[0]));
    CHECK(std::isfinite(yRange[1]));
    CHECK(nearlyEqual(xRange[0], kP5LogXMin, 0.15));
    CHECK(nearlyEqual(xRange[1], kP5LogXMax, 0.15));
    CHECK(nearlyEqual(yRange[0], kP5LinearYMin, 0.05));
    CHECK(nearlyEqual(yRange[1], kP5LinearYMax, 0.05));

    const auto scatterX = example.p5Scatter->scatter()->xData();
    CHECK(!scatterX.empty());
    for (double value : scatterX) {
        CHECK(std::isfinite(value));
        CHECK(value >= kP5LogXMin - 0.2);
        CHECK(value <= kP5LogXMax + 0.2);
    }

    auto* bottomAxis = p5->getAxis(QStringLiteral("bottom"));
    CHECK(bottomAxis != nullptr);
    CHECK(bottomAxis->labelUnitPrefix() == QString::fromUtf8("µ"));
    CHECK(nearlyEqual(bottomAxis->autoSIPrefixScale(), 1.0e6, 1.0e-6));

    const auto childrenBounds = p5->getViewBox()->childrenBounds();
    CHECK(childrenBounds[cppqtgraph::graphicsItems::ViewBox::XAxis].has_value());
    CHECK(childrenBounds[cppqtgraph::graphicsItems::ViewBox::YAxis].has_value());
    CHECK(example.p5Scatter->scatter()->isVisible());

    auto* p5Scatter = example.p5Scatter->scatter();
    CHECK(p5Scatter->symbol() == QStringLiteral("t"));
    CHECK(p5Scatter->pen().style() == Qt::NoPen);
    CHECK(p5Scatter->brush().color().alpha() < 255);

    const QImage triangleImage = cppqtgraph::graphicsItems::renderSymbol(
        QStringLiteral("t"), p5Scatter->size(), p5Scatter->pen(), p5Scatter->brush(), 1.0);
    CHECK(!triangleImage.isNull());
    bool foundBlueFill = false;
    bool foundWhiteOutline = false;
    for (int row = 0; row < triangleImage.height(); ++row) {
        for (int column = 0; column < triangleImage.width(); ++column) {
            const QColor pixel = QColor::fromRgba(triangleImage.pixel(column, row));
            if (pixel.alpha() == 0) {
                continue;
            }
            if (pixel.blue() > pixel.red() && pixel.blue() > pixel.green()) {
                foundBlueFill = true;
            }
            if (pixel.red() > 200 && pixel.green() > 200 && pixel.blue() > 200) {
                foundWhiteOutline = true;
            }
        }
    }
    CHECK(foundBlueFill);
    CHECK(!foundWhiteOutline);

    const auto mappedBounds = example.p5Scatter->autoRangeBoundsRect();
    CHECK(mappedBounds.has_value());
    CHECK(mappedBounds->width() > 0.0);
    CHECK(mappedBounds->height() > 0.0);
    CHECK(!example.p5Scatter->scatter()->boundingRect().isNull());

    const QPixmap pixmap = example.widget->grab();
    CHECK(!pixmap.isNull());
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int nonBackgroundPixels = 0;
    for (int row = 0; row < image.height(); ++row) {
        for (int column = 0; column < image.width(); ++column) {
            const QColor color(image.pixel(column, row));
            if (color.alpha() > 0 && color != QColor(0, 0, 0)) {
                ++nonBackgroundPixels;
            }
        }
    }
    CHECK(nonBackgroundPixels > 1000);

    return true;
}

bool testPlottingP3AutorangeAndSymbolRender(cppqtgraph::examples::PlottingExample& example)
{
    constexpr double kP3DataYMin = -2.1174059170159678;
    constexpr double kP3DataYMax = 1.7818839591633955;
    constexpr double kP3DataYSpan = kP3DataYMax - kP3DataYMin;

    example.widget->show();
    QApplication::processEvents();

    auto* p3 = example.plots[2];
    const auto viewRange = p3->viewRange();
    const auto yRange = viewRange[cppqtgraph::graphicsItems::ViewBox::YAxis];
    CHECK(std::isfinite(yRange[0]));
    CHECK(std::isfinite(yRange[1]));
    CHECK(yRange[1] > yRange[0]);
    CHECK((yRange[1] - yRange[0]) < 8.0);

    const auto mappedBounds = example.p3Curve->autoRangeBoundsRect();
    CHECK(mappedBounds.has_value());
    CHECK(nearlyEqual(mappedBounds->top(), kP3DataYMin, 1.0e-6));
    CHECK(nearlyEqual(mappedBounds->bottom(), kP3DataYMax, 1.0e-6));
    CHECK(nearlyEqual(mappedBounds->height(), kP3DataYSpan, 1.0e-6));

    auto* scatter = example.p3Curve->scatter();
    CHECK(scatter->symbol() == QStringLiteral("o"));
    CHECK(scatter->pen().isCosmetic());
    CHECK(scatter->pen().color() == QColor(255, 255, 255));
    CHECK(scatter->brush().color() == QColor(255, 0, 0));

    const auto scatterAutoBounds = scatter->autoRangeBoundsRect();
    CHECK(scatterAutoBounds.has_value());
    CHECK(nearlyEqual(scatterAutoBounds->height(), kP3DataYSpan, 1.0e-6));
    CHECK(nearlyEqual(scatterAutoBounds->height(), scatter->boundingRect().height(), 1.0e-6));

    const QImage symbolImage = cppqtgraph::graphicsItems::renderSymbol(
        QStringLiteral("o"), scatter->size(), scatter->pen(), scatter->brush(), 1.0);
    CHECK(!symbolImage.isNull());

    const QColor centerPixel = QColor::fromRgba(symbolImage.pixel(symbolImage.width() / 2, symbolImage.height() / 2));
    CHECK(centerPixel.alpha() > 0);
    CHECK(centerPixel.red() > centerPixel.green());
    CHECK(centerPixel.red() > centerPixel.blue());
    CHECK(QColor::fromRgba(symbolImage.pixel(0, 0)).alpha() == 0);

    bool foundWhiteOutline = false;
    for (int row = 0; row < symbolImage.height(); ++row) {
        for (int column = 0; column < symbolImage.width(); ++column) {
            const QColor pixel = QColor::fromRgba(symbolImage.pixel(column, row));
            if (pixel.alpha() > 0 && pixel.red() > 200 && pixel.green() > 200 && pixel.blue() > 200) {
                foundWhiteOutline = true;
                break;
            }
        }
        if (foundWhiteOutline) {
            break;
        }
    }
    CHECK(foundWhiteOutline);

    return true;
}

bool testPlottingRegionBoundingRectAfterShow()
{
    auto example = cppqtgraph::examples::createPlottingExample();
    example.widget->show();
    QApplication::processEvents();
    const QRectF bounds = example.region->boundingRect();
    const auto [regionMinimum, regionMaximum] = example.region->getRegion();

    CHECK(nearlyEqual(regionMinimum, 400.0, 1.0e-6));
    CHECK(nearlyEqual(regionMaximum, 700.0, 1.0e-6));
    CHECK(nearlyEqual(bounds.left(), regionMinimum, 1.0e-6));
    CHECK(nearlyEqual(bounds.right(), regionMaximum, 1.0e-6));
    CHECK(bounds.height() > 1.0);

    return true;
}

bool testPlottingP8YRangeUnaffectedByRegion(cppqtgraph::examples::PlottingExample& example)
{
    example.widget->show();
    QApplication::processEvents();

    auto* p8 = example.plots[7];
    const auto yWithRegion = p8->viewRange()[cppqtgraph::graphicsItems::ViewBox::YAxis];

    p8->removeItem(example.region);
    example.region = nullptr;
    p8->getViewBox()->enableAutoRange(cppqtgraph::graphicsItems::ViewBox::YAxis, true);
    p8->getViewBox()->autoRange();
    QApplication::processEvents();

    const auto yWithoutRegion = p8->viewRange()[cppqtgraph::graphicsItems::ViewBox::YAxis];
    CHECK(nearlyEqual(yWithoutRegion[0], yWithRegion[0], 1.0e-6));
    CHECK(nearlyEqual(yWithoutRegion[1], yWithRegion[1], 1.0e-6));

    return true;
}

bool testPlottingGrab(cppqtgraph::examples::PlottingExample& example)
{
    example.widget->show();
    QApplication::processEvents();
    const QPixmap pixmap = example.widget->grab();
    CHECK(!pixmap.isNull());
    CHECK(pixmap.width() > 0);
    CHECK(pixmap.height() > 0);
    return true;
}

cppqtgraph::graphicsItems::ButtonItem* findAutoButton(
    const cppqtgraph::graphicsItems::PlotItem& plot)
{
    for (QGraphicsItem* child : plot.childItems()) {
        if (auto* button = dynamic_cast<cppqtgraph::graphicsItems::ButtonItem*>(child)) {
            return button;
        }
    }
    return nullptr;
}

bool testPlottingP6AutoButtonVisible(cppqtgraph::examples::PlottingExample& example)
{
    using cppqtgraph::graphicsItems::ButtonItem;
    using cppqtgraph::graphicsItems::PlotItem;
    using cppqtgraph::graphicsItems::ViewBox;

    example.widget->show();
    QApplication::processEvents();

    PlotItem* p6 = example.plots[5];
    CHECK(p6 != nullptr);
    const auto autoRange = p6->getViewBox()->autoRangeEnabled();
    CHECK(!autoRange[ViewBox::XAxis] || !autoRange[ViewBox::YAxis]);

    ButtonItem* autoButton = findAutoButton(*p6);
    CHECK(autoButton != nullptr);
    CHECK(autoButton->isVisible());
    CHECK(std::abs(autoButton->pos().x()) < 1.0e-6);
    const qreal expectedY = p6->height() - autoButton->boundingRect().height();
    CHECK(std::abs(autoButton->pos().y() - expectedY) < 1.0);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    auto example = cppqtgraph::examples::createPlottingExample();

    if (!testPlottingStructure(example)) {
        return 1;
    }
    if (!testPlottingData(example)) {
        return 1;
    }
    if (!testPlottingTimer(example)) {
        return 1;
    }
    if (!testPlottingP8AutorangeRange(example)) {
        return 1;
    }
    if (!testPlottingRegionBoundingRectAfterShow()) {
        return 1;
    }
    if (!testPlottingRegionZoomLinkage(example)) {
        return 1;
    }
    if (!testPlottingP8YRangeUnaffectedByRegion(example)) {
        return 1;
    }
    if (!testPlottingDeterminism()) {
        return 1;
    }
    if (!testPlottingP5LogMappedAutorangeAndRender(example)) {
        return 1;
    }
    if (!testPlottingP3AutorangeAndSymbolRender(example)) {
        return 1;
    }
    if (!testPlottingP6AutoButtonVisible(example)) {
        return 1;
    }
    if (!testPlottingGrab(example)) {
        return 1;
    }

    return 0;
}
