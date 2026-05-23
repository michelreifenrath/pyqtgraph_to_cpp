#include <pyqtgraph/core/ArrayView.hpp>

#include <array>
#include <cassert>
#include <cstddef>

namespace {

void testDefaultView()
{
    pyqtgraph::core::ArrayView<double> view;

    assert(view.data() == nullptr);
    assert(view.size() == 0);
    assert(view.empty());
    assert(view.shape().size() == 1);
    assert(view.shape()[0] == 0);
}

void testPointerAndShapeView()
{
    std::array<double, 3> values{1.0, 2.0, 3.0};
    pyqtgraph::core::ArrayView<double> view(values.data(), std::array<std::size_t, 1>{values.size()});

    assert(view.data() == values.data());
    assert(view.size() == values.size());
    assert(!view.empty());
    assert(view.shape().size() == 1);
    assert(view.shape()[0] == values.size());
    assert(view[0] == 1.0);
    assert(view[1] == 2.0);
    assert(view[2] == 3.0);

    values[1] = 4.0;
    assert(view[1] == 4.0);
}

void testConstView()
{
    const std::array<double, 2> values{5.0, 6.0};
    pyqtgraph::core::ArrayView<const double> view(values.data(), std::array<std::size_t, 1>{values.size()});

    assert(view.data() == values.data());
    assert(view.size() == values.size());
    assert(view.shape()[0] == values.size());
    assert(view[0] == 5.0);
    assert(view[1] == 6.0);
}

} // namespace

int main()
{
    testDefaultView();
    testPointerAndShapeView();
    testConstView();

    return 0;
}
