#include <cppqtgraph/graphicsItems/PlotDataItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ScatterPlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QRectF>
#include <QtCore/QtGlobal>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>

#include <cmath>
#include <iostream>
#include <memory>
#include <span>
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

bool testConstructionAndHierarchy()
{
    using cppqtgraph::graphicsItems::GraphicsItem;
    using cppqtgraph::graphicsItems::GraphicsWidget;
    using cppqtgraph::graphicsItems::PlotItem;

    static_assert(std::is_constructible_v<PlotItem>);
    static_assert(std::is_constructible_v<PlotItem, QGraphicsItem*>);
    static_assert(std::is_destructible_v<PlotItem>);
    static_assert(!std::is_copy_constructible_v<PlotItem>);
    static_assert(!std::is_copy_assignable_v<PlotItem>);
    static_assert(!std::is_move_constructible_v<PlotItem>);
    static_assert(!std::is_move_assignable_v<PlotItem>);
    static_assert(std::is_base_of_v<GraphicsWidget, PlotItem>);
    static_assert(std::is_base_of_v<GraphicsItem, PlotItem>);
    static_assert(std::is_base_of_v<QGraphicsWidget, PlotItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, PlotItem>);
    static_assert(!std::is_final_v<PlotItem>);

    PlotItem plot;
    CHECK(plot.graphicsItem() == static_cast<QGraphicsItem*>(&plot));
    CHECK(plot.getViewWidget() == nullptr);

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    cppqtgraph::graphicsItems::PlotItem plot;
    QGraphicsScene firstScene;
    firstScene.addItem(&plot);

    CHECK(plot.getViewWidget() == nullptr);

    QGraphicsView firstView(&firstScene);
    CHECK(plot.getViewWidget() == &firstView);
    CHECK(plot.getViewWidget() == &firstView);

    firstScene.removeItem(&plot);
    CHECK(plot.getViewWidget() == nullptr);

    QGraphicsScene secondScene;
    QGraphicsView secondView(&secondScene);
    secondScene.addItem(&plot);
    CHECK(plot.getViewWidget() == &secondView);

    secondScene.removeItem(&plot);
    CHECK(plot.getViewWidget() == nullptr);

    return true;
}

bool testParentConstruction()
{
    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    cppqtgraph::graphicsItems::PlotItem plot(&parent);

    CHECK(plot.parentItem() == &parent);
    CHECK(plot.graphicsItem() == static_cast<QGraphicsItem*>(&plot));
    CHECK(plot.getViewWidget() == nullptr);

    return true;
}

bool nearlyEqual(double lhs, double rhs, double tolerance = 1.0e-9)
{
    return std::abs(lhs - rhs) <= tolerance;
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

bool testPlotReturnsOwnedPlotDataItem()
{
    using cppqtgraph::graphicsItems::PlotDataItem;
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    const std::vector<double> y{1.0, 3.0, 2.0};
    PlotDataItem* data = plot.plot(y);
    CHECK(data != nullptr);
    CHECK(dynamic_cast<PlotDataItem*>(data) == data);
    CHECK(spanEquals(data->yData(), y));
    CHECK(data->curve() != nullptr);
    CHECK(spanEquals(data->curve()->yData(), y));

    const std::vector<double> x{0.0, 1.0, 2.0};
    const std::vector<double> xy{4.0, 5.0, 6.0};
    PlotDataItem* xyData = plot.plot(x, xy, QStringLiteral("series"));
    CHECK(xyData != nullptr);
    CHECK(spanEquals(xyData->xData(), x));
    CHECK(spanEquals(xyData->yData(), xy));
    CHECK(spanEquals(xyData->curve()->xData(), x));
    CHECK(spanEquals(xyData->curve()->yData(), xy));

    plot.removeItem(data);
    plot.clear();
    return true;
}

bool testPlotPropagatesPenSymbolAndSymbolBrush()
{
    using cppqtgraph::graphicsItems::PlotDataItem;
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    const std::vector<double> x{0.0, 1.0, 2.0};
    const std::vector<double> y{1.0, 2.0, 3.0};

    PlotItem::PlotOptions options;
    options.pen = QPen(QColor(10, 20, 30));
    options.symbol = QStringLiteral("o");
    options.symbolBrush = QBrush(QColor(255, 0, 0));
    options.name = QStringLiteral("styled");

    PlotDataItem* data = plot.plot(x, y, options);
    CHECK(data->pen().color() == QColor(10, 20, 30));
    CHECK(data->curve()->pen().color() == QColor(10, 20, 30));
    CHECK(data->symbol() == QStringLiteral("o"));
    CHECK(data->scatter()->symbol() == QStringLiteral("o"));
    CHECK(data->scatter()->brush().color() == QColor(255, 0, 0));

    return true;
}

bool testPlotDataParticipatesInAutorange()
{
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    QGraphicsScene scene;
    scene.addItem(&plot);
    plot.resize(400.0, 300.0);
    plot.setXRange(0.0, 1000.0, 0.0, true);
    plot.setYRange(0.0, 1000.0, 0.0, true);

    const std::vector<double> x{1.0, 2.0, 3.0};
    const std::vector<double> y{4.0, 8.0, 2.0};
    plot.plot(x, y);
    plot.autoRange();

    const auto range = plot.viewRange();
    CHECK(range[0][0] <= 1.0);
    CHECK(range[0][1] >= 3.0);
    CHECK(range[1][0] <= 2.0);
    CHECK(range[1][1] >= 8.0);

    plot.clear();
    scene.removeItem(&plot);
    return true;
}

bool isColoredPixel(const QColor& color)
{
    return color.alpha() > 0 && (color.red() < 250 || color.green() < 250 || color.blue() < 250);
}

bool testPlotItemOffscreenCropShowsLineAndMarkers()
{
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    QGraphicsScene scene;
    scene.addItem(&plot);
    plot.resize(420.0, 320.0);
    plot.setXRange(0.0, 3.0, 0.0, true);
    plot.setYRange(0.0, 3.0, 0.0, true);

    const std::vector<double> x{0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y{0.5, 2.5, 1.0, 2.0};

    PlotItem::PlotOptions options;
    options.pen = QPen(QColor(0, 0, 255), 2.0);
    options.symbol = QStringLiteral("o");
    options.symbolBrush = QBrush(QColor(255, 0, 0));
    options.name = QStringLiteral("line-markers");
    plot.plot(x, y, options);

    QImage image(420, 320, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    scene.render(&painter, QRectF(0.0, 0.0, 420.0, 320.0), QRectF(0.0, 0.0, 420.0, 320.0));
    painter.end();

    const QRectF viewSceneRect = plot.getViewBox()->sceneBoundingRect();
    const int cropLeft = static_cast<int>(std::lround(viewSceneRect.left() + viewSceneRect.width() * 0.1));
    const int cropRight = static_cast<int>(std::lround(viewSceneRect.left() + viewSceneRect.width() * 0.9));
    const int cropTop = static_cast<int>(std::lround(viewSceneRect.top() + viewSceneRect.height() * 0.1));
    const int cropBottom = static_cast<int>(std::lround(viewSceneRect.top() + viewSceneRect.height() * 0.9));

    int coloredPixels = 0;
    int redDominantPixels = 0;
    int blueDominantPixels = 0;
    for (int row = cropTop; row <= cropBottom; ++row) {
        for (int column = cropLeft; column <= cropRight; ++column) {
            const QColor color = image.pixelColor(column, row);
            if (!isColoredPixel(color)) {
                continue;
            }
            ++coloredPixels;
            if (color.red() > color.blue() && color.red() > color.green()) {
                ++redDominantPixels;
            }
            if (color.blue() > color.red() && color.blue() > color.green()) {
                ++blueDominantPixels;
            }
        }
    }

    CHECK(coloredPixels >= 24);
    CHECK(redDominantPixels >= 4);
    CHECK(blueDominantPixels >= 8);

    plot.clear();
    scene.removeItem(&plot);
    return true;
}

bool testPlotWidgetPlotReturnsPlotDataItem()
{
    using cppqtgraph::graphicsItems::PlotDataItem;
    using cppqtgraph::widgets::PlotWidget;

    cppqtgraph::widgets::PlotWidget plotWidget;
    const std::vector<double> y{1.0, 2.0, 3.0};
    PlotDataItem* data = plotWidget.plot(y);
    CHECK(data != nullptr);
    CHECK(data->scene() == plotWidget.scene());
    CHECK(spanEquals(data->yData(), y));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndHierarchy()) {
        return 1;
    }
    if (!testInheritedViewWidgetDiscovery()) {
        return 1;
    }
    if (!testParentConstruction()) {
        return 1;
    }
    if (!testPlotReturnsOwnedPlotDataItem()) {
        return 1;
    }
    if (!testPlotPropagatesPenSymbolAndSymbolBrush()) {
        return 1;
    }
    if (!testPlotDataParticipatesInAutorange()) {
        return 1;
    }
    if (!testPlotItemOffscreenCropShowsLineAndMarkers()) {
        return 1;
    }
    if (!testPlotWidgetPlotReturnsPlotDataItem()) {
        return 1;
    }

    return 0;
}
