#include "../../include/cppqtgraph/functions.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <span>
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

bool testFiniteValues()
{
    const std::array<double, 3> values{4.0, -2.5, 9.25};

    CHECK(cppqtgraph::nanmin(std::span<const double>(values.data(), values.size())) == -2.5);
    CHECK(cppqtgraph::nanmax(std::span<const double>(values.data(), values.size())) == 9.25);
    return true;
}

bool testMixedNaNs()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::array<double, 5> values{nan, 3.0, nan, -7.0, 2.0};

    CHECK(cppqtgraph::nanmin(std::span<const double>(values.data(), values.size())) == -7.0);
    CHECK(cppqtgraph::nanmax(std::span<const double>(values.data(), values.size())) == 3.0);
    return true;
}

bool testInfinitiesAreValues()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::array<double, 4> values{nan, inf, -inf, 5.0};

    CHECK(cppqtgraph::nanmin(std::span<const double>(values.data(), values.size())) == -inf);
    CHECK(cppqtgraph::nanmax(std::span<const double>(values.data(), values.size())) == inf);
    return true;
}

bool testAllNaNAndEmpty()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::array<double, 2> allNan{nan, nan};
    const std::span<const double> empty;

    CHECK(std::isnan(cppqtgraph::nanmin(std::span<const double>(allNan.data(), allNan.size()))));
    CHECK(std::isnan(cppqtgraph::nanmax(std::span<const double>(allNan.data(), allNan.size()))));

    bool nanminThrows = false;
    try {
        (void)cppqtgraph::nanmin(empty);
    } catch (const std::invalid_argument&) {
        nanminThrows = true;
    }
    CHECK(nanminThrows);

    bool nanmaxThrows = false;
    try {
        (void)cppqtgraph::nanmax(empty);
    } catch (const std::invalid_argument&) {
        nanmaxThrows = true;
    }
    CHECK(nanmaxThrows);
    return true;
}

bool testFloatAndInitializerListConvenience()
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const std::array<float, 4> values{nan, 1.5F, -4.25F, nan};
    auto minValue = cppqtgraph::nanmin(std::span<const float>(values.data(), values.size()));
    auto maxValue = cppqtgraph::nanmax(std::span<const float>(values.data(), values.size()));

    CHECK((std::is_same_v<decltype(minValue), float>));
    CHECK((std::is_same_v<decltype(maxValue), float>));
    CHECK(minValue == -4.25F);
    CHECK(maxValue == 1.5F);
    CHECK(cppqtgraph::nanmin<double>({std::numeric_limits<double>::quiet_NaN(), 2.0, 1.0}) == 1.0);
    CHECK(cppqtgraph::nanmax<double>({std::numeric_limits<double>::quiet_NaN(), 2.0, 1.0}) == 2.0);
    return true;
}

bool testLongDoubleOverload()
{
    const long double nan = std::numeric_limits<long double>::quiet_NaN();
    const std::array<long double, 4> values{nan, 9.0L, -1.25L, nan};
    auto minValue = cppqtgraph::nanmin(std::span<const long double>(values.data(), values.size()));
    auto maxValue = cppqtgraph::nanmax(std::span<const long double>(values.data(), values.size()));

    CHECK((std::is_same_v<decltype(minValue), long double>));
    CHECK((std::is_same_v<decltype(maxValue), long double>));
    CHECK(minValue == -1.25L);
    CHECK(maxValue == 9.0L);
    CHECK(cppqtgraph::nanmin<long double>({nan, 4.0L, -3.0L}) == -3.0L);
    CHECK(cppqtgraph::nanmax<long double>({nan, 4.0L, -3.0L}) == 4.0L);
    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testFiniteValues() && success;
    success = testMixedNaNs() && success;
    success = testInfinitiesAreValues() && success;
    success = testAllNaNAndEmpty() && success;
    success = testFloatAndInitializerListConvenience() && success;
    success = testLongDoubleOverload() && success;

    return success ? 0 : 1;
}
