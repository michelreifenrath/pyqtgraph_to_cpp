#include <pyqtgraph/core/ArrayView.hpp>

#include <array>
#include <cstddef>
#include <iostream>
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

bool testDefaultView()
{
    pyqtgraph::core::ArrayView<double> view;

    CHECK(view.data() == nullptr);
    CHECK(view.size() == 0);
    CHECK(view.empty());
    CHECK(view.shape().size() == 1);
    CHECK(view.shape()[0] == 0);

    return true;
}

bool testPointerAndShapeView()
{
    std::array<double, 3> values{1.0, 2.0, 3.0};
    pyqtgraph::core::ArrayView<double> view(values.data(), std::array<std::size_t, 1>{values.size()});

    CHECK(view.data() == values.data());
    CHECK(view.size() == values.size());
    CHECK(!view.empty());
    CHECK(view.shape().size() == 1);
    CHECK(view.shape()[0] == values.size());
    CHECK(view[0] == 1.0);
    CHECK(view[1] == 2.0);
    CHECK(view[2] == 3.0);

    values[1] = 4.0;
    CHECK(view[1] == 4.0);

    return true;
}

bool testConstView()
{
    const std::array<double, 2> values{5.0, 6.0};
    pyqtgraph::core::ArrayView<const double> view(values.data(), std::array<std::size_t, 1>{values.size()});

    CHECK(view.data() == values.data());
    CHECK(view.size() == values.size());
    CHECK(view.shape()[0] == values.size());
    CHECK(view[0] == 5.0);
    CHECK(view[1] == 6.0);

    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testDefaultView() && success;
    success = testPointerAndShapeView() && success;
    success = testConstView() && success;

    return success ? 0 : 1;
}
