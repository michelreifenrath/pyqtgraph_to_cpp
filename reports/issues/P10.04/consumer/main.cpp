#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtWidgets/QApplication>

#include <cmath>
#include <iostream>
#include <span>
#include <vector>

namespace {

bool closeEnough(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1e-12;
}

int fail(const char* message)
{
    std::cerr << "P10.04 package consumer failed: " << message << '\n';
    return 1;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    pyqtgraph::widgets::PlotWidget widget;
    widget.setWindowTitle(QStringLiteral("P10.04 package consumer SimplePlot"));
    widget.resize(320, 240);

    pyqtgraph::graphicsItems::PlotItem* plotItem = widget.getPlotItem();
    if (plotItem == nullptr) {
        return fail("PlotWidget did not expose a PlotItem");
    }

    const std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0};
    const std::vector<double> y{0.0, 1.0, 0.25, 2.0, 1.5};
    pyqtgraph::graphicsItems::PlotCurveItem* curve = plotItem->plot(std::span<const double>(x), std::span<const double>(y), QStringLiteral("consumer curve"));
    if (curve == nullptr) {
        return fail("PlotItem::plot did not return a PlotCurveItem");
    }

    const auto curveX = curve->xData();
    const auto curveY = curve->yData();
    if (curveX.size() != x.size() || curveY.size() != y.size()) {
        return fail("curve data size mismatch");
    }
    for (std::size_t index = 0; index < x.size(); ++index) {
        if (!closeEnough(curveX[index], x[index]) || !closeEnough(curveY[index], y[index])) {
            return fail("curve data value mismatch");
        }
    }

    widget.show();
    app.processEvents();

    const QPixmap pixmap = widget.grab();
    if (pixmap.isNull() || pixmap.width() <= 0 || pixmap.height() <= 0) {
        return fail("offscreen widget grab did not produce a pixmap");
    }

    std::cout << "P10.04 package consumer exercised installed PlotWidget/PlotItem/PlotCurveItem" << '\n';
    return 0;
}
