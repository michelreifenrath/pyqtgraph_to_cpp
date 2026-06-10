#include <cppqtgraph/core/ArrayView.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

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
    cppqtgraph::core::ArrayView<double> view;

    CHECK(view.data() == nullptr);
    CHECK(view.size() == 0);
    CHECK(view.empty());
    CHECK(view.shape().size() == 1);
    CHECK(view.shape()[0] == 0);
    CHECK(view.strides().size() == 1);
    CHECK(view.strides()[0] == 1);

    return true;
}

bool testPointerAndShapeView()
{
    std::array<double, 3> values{1.0, 2.0, 3.0};
    cppqtgraph::core::ArrayView<double> view(values.data(), std::array<std::size_t, 1>{values.size()});

    CHECK(view.data() == values.data());
    CHECK(view.size() == values.size());
    CHECK(!view.empty());
    CHECK(view.shape().size() == 1);
    CHECK(view.shape()[0] == values.size());
    CHECK(view.strides()[0] == 1);
    CHECK(view[0] == 1.0);
    CHECK(view[1] == 2.0);
    CHECK(view[2] == 3.0);

    values[1] = 4.0;
    CHECK(view[1] == 4.0);

    return true;
}

bool testRankTwoContiguousView()
{
    std::array<int, 6> values{0, 1, 2, 3, 4, 5};
    cppqtgraph::core::ArrayView<int, 2> view(values.data(), std::array<std::size_t, 2>{2, 3});

    CHECK(view.data() == values.data());
    CHECK(view.size() == values.size());
    CHECK(!view.empty());
    CHECK(view.shape()[0] == 2);
    CHECK(view.shape()[1] == 3);
    CHECK(view.strides()[0] == 3);
    CHECK(view.strides()[1] == 1);
    CHECK(view(0, 0) == 0);
    CHECK(view(0, 2) == 2);
    CHECK(view(1, 0) == 3);
    CHECK(view.at(std::array<std::size_t, 2>{1, 2}) == 5);

    view(1, 1) = 40;
    CHECK(values[4] == 40);

    values[2] = 20;
    CHECK(view(0, 2) == 20);

    return true;
}

bool testRankTwoStridedView()
{
    std::array<int, 12> values{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    cppqtgraph::core::ArrayView<int, 2> view(
        values.data() + 1,
        std::array<std::size_t, 2>{2, 3},
        std::array<std::ptrdiff_t, 2>{4, 1});

    CHECK(view.data() == values.data() + 1);
    CHECK(view.size() == 6);
    CHECK(view.strides()[0] == 4);
    CHECK(view.strides()[1] == 1);
    CHECK(view(0, 0) == 1);
    CHECK(view(0, 2) == 3);
    CHECK(view(1, 0) == 5);
    CHECK(view(1, 2) == 7);

    view(1, 1) = 60;
    CHECK(values[6] == 60);

    return true;
}

bool testZeroExtentIsEmpty()
{
    std::array<int, 3> values{1, 2, 3};
    cppqtgraph::core::ArrayView<int, 2> view(values.data(), std::array<std::size_t, 2>{3, 0});

    CHECK(view.size() == 0);
    CHECK(view.empty());
    CHECK(view.shape()[0] == 3);
    CHECK(view.shape()[1] == 0);
    CHECK(view.strides()[0] == 0);
    CHECK(view.strides()[1] == 1);

    return true;
}

bool testRankOneStridedIndexing()
{
    std::array<int, 6> values{0, 1, 2, 3, 4, 5};
    cppqtgraph::core::ArrayView<int> view(
        values.data() + 1,
        std::array<std::size_t, 1>{3},
        std::array<std::ptrdiff_t, 1>{2});

    CHECK(view.data() == values.data() + 1);
    CHECK(view.size() == 3);
    CHECK(view.strides()[0] == 2);
    CHECK(view[0] == 1);
    CHECK(view[1] == 3);
    CHECK(view[2] == 5);

    view[1] = 30;
    CHECK(values[3] == 30);

    return true;
}

bool testSlicing()
{
    std::array<int, 12> values{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    cppqtgraph::core::ArrayView<int, 2> view(values.data(), std::array<std::size_t, 2>{3, 4});

    const auto columns = view.slice(1, 1, 4, 2);
    CHECK(columns.data() == values.data() + 1);
    CHECK(columns.shape()[0] == 3);
    CHECK(columns.shape()[1] == 2);
    CHECK(columns.strides()[0] == 4);
    CHECK(columns.strides()[1] == 2);
    CHECK(columns(0, 0) == 1);
    CHECK(columns(0, 1) == 3);
    CHECK(columns(2, 0) == 9);
    CHECK(columns(2, 1) == 11);

    columns(1, 1) = 70;
    CHECK(values[7] == 70);

    const auto rows = view.slice(0, 1, 3);
    CHECK(rows.data() == values.data() + 4);
    CHECK(rows.shape()[0] == 2);
    CHECK(rows.shape()[1] == 4);
    CHECK(rows.strides()[0] == 4);
    CHECK(rows.strides()[1] == 1);
    CHECK(rows(0, 0) == 4);
    CHECK(rows(1, 3) == 11);

    cppqtgraph::core::ArrayView<int> vector(values.data(), std::array<std::size_t, 1>{6});
    const auto everyOther = vector.slice(0, 1, 6, 2);
    CHECK(everyOther.data() == values.data() + 1);
    CHECK(everyOther.shape()[0] == 3);
    CHECK(everyOther.strides()[0] == 2);
    CHECK(everyOther[0] == 1);
    CHECK(everyOther[1] == 3);
    CHECK(everyOther[2] == 5);

    return true;
}

bool testShapeOnlySlicePreservesNullData()
{
    cppqtgraph::core::ArrayView<int> view(nullptr, std::array<std::size_t, 1>{4});

    const auto sliced = view.slice(0, 1, 3);
    CHECK(sliced.data() == nullptr);
    CHECK(sliced.size() == 2);
    CHECK(sliced.shape()[0] == 2);
    CHECK(sliced.strides()[0] == 1);

    return true;
}

bool testInvalidSlices()
{
    std::array<int, 6> values{0, 1, 2, 3, 4, 5};
    cppqtgraph::core::ArrayView<int, 2> view(values.data(), std::array<std::size_t, 2>{2, 3});

    bool threw = false;
    try {
        (void)view.slice(2, 0, 1);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)view.slice(0, 0, 1, 0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)view.slice(0, 2, 1);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)view.slice(1, 0, 4);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    return true;
}

bool testConstView()
{
    const std::array<double, 2> values{5.0, 6.0};
    cppqtgraph::core::ArrayView<const double> view(values.data(), std::array<std::size_t, 1>{values.size()});

    CHECK(view.data() == values.data());
    CHECK(view.size() == values.size());
    CHECK(view.shape()[0] == values.size());
    CHECK(view.strides()[0] == 1);
    CHECK(std::is_const_v<std::remove_reference_t<decltype(view[0])>>);
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
    success = testRankTwoContiguousView() && success;
    success = testRankTwoStridedView() && success;
    success = testZeroExtentIsEmpty() && success;
    success = testRankOneStridedIndexing() && success;
    success = testSlicing() && success;
    success = testShapeOnlySlicePreservesNullData() && success;
    success = testInvalidSlices() && success;
    success = testConstView() && success;

    return success ? 0 : 1;
}
