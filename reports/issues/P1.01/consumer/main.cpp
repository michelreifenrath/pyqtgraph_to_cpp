#include <pyqtgraph/PlotData.hpp>
#include <pyqtgraph/core/ArrayView.hpp>

#include <cassert>
#include <cmath>
#include <vector>

int main()
{
    std::vector<double> samples{1.0, 2.5, -3.0};
    pyqtgraph::core::ArrayView<double> view(samples.data(), {samples.size()});
    assert(view.size() == samples.size());
    assert(view[1] == 2.5);

    pyqtgraph::PlotData plotData;
    plotData.set("y", samples);
    assert(plotData.hasField("y"));
    assert(plotData["y"].size() == samples.size());
    assert(std::abs(plotData.min("y") - -3.0) < 1e-12);
    assert(std::abs(plotData.max("y") - 2.5) < 1e-12);

    return 0;
}
