#include "../../include/pyqtgraph/PlotData.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P2_03_FIXTURE
#define PYQTGRAPH_CPP_P2_03_FIXTURE "oracle/fixtures/P2_03/plotdata_oracle.json"
#endif

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

#define CHECK_THROWS(expression, exceptionType) \
    do { \
        bool threwExpectedException = false; \
        try { \
            (void)(expression); \
        } catch (const exceptionType&) { \
            threwExpectedException = true; \
        } \
        CHECK(threwExpectedException); \
    } while (false)

std::string readOracleFixture()
{
    std::ifstream input(std::filesystem::path{PYQTGRAPH_CPP_P2_03_FIXTURE});
    if (!input.good()) {
        std::cerr << "missing P2.03 oracle fixture: " << PYQTGRAPH_CPP_P2_03_FIXTURE << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(std::string_view text, std::string_view needle)
{
    return text.find(needle) != std::string_view::npos;
}

bool requireP203OracleFixture()
{
    const std::string fixture = readOracleFixture();
    CHECK(contains(fixture, "\"issue\": \"P2.03\""));
    CHECK(contains(fixture, "\"commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\""));
    CHECK(contains(fixture, "\"pyqtgraph/PlotData.py\""));
    CHECK(contains(fixture, "\"tests/graphicsItems/test_PlotDataItem.py\""));
    CHECK(contains(fixture, "\"range_container\""));
    CHECK(contains(fixture, "\"masked_partial\""));
    CHECK(contains(fixture, "\"empty_exception\": \"ValueError\""));
    CHECK(contains(fixture, "C++ all-masked extrema throw std::invalid_argument"));
    return true;
}

bool testFieldLifecycleAndLookup()
{
    pyqtgraph::PlotData data;

    CHECK(!data.hasField("x"));
    CHECK(!data.hasField("y"));

    data.addFields({"x", "y"});
    CHECK(data.hasField("x"));
    CHECK(data.hasField("y"));
    CHECK(data["x"].empty());
    CHECK(data["y"].empty());

    data.set("x", {1.0, 2.0});
    data.addFields({"x", "z"});
    CHECK(data.hasField("z"));
    CHECK(data["x"].size() == 2);
    CHECK(data["x"][0] == 1.0);
    CHECK(data["x"][1] == 2.0);
    CHECK(data["z"].empty());

    CHECK_THROWS(data["missing"], std::out_of_range);

    return true;
}

bool testAssignmentReplacementAndCachedExtrema()
{
    pyqtgraph::PlotData data;

    data.set("x", {3.0, -2.0, 5.0});
    CHECK(data.hasField("x"));
    CHECK(data["x"].size() == 3);
    CHECK(data.min("x") == -2.0);
    CHECK(data.max("x") == 5.0);

    data.set("x", {10.0, 12.0});
    CHECK(data["x"].size() == 2);
    CHECK(data["x"][0] == 10.0);
    CHECK(data["x"][1] == 12.0);

    // Upstream PlotData.py does not invalidate cached extrema after assignment;
    // preserve that stale-cache behavior for compatibility.
    CHECK(data.min("x") == -2.0);
    CHECK(data.max("x") == 5.0);

    return true;
}

bool testErrorsAndNanExtrema()
{
    pyqtgraph::PlotData data;

    CHECK_THROWS(data.min("missing"), std::out_of_range);
    CHECK_THROWS(data.max("missing"), std::out_of_range);

    data.addFields({"empty"});
    CHECK_THROWS(data.min("empty"), std::invalid_argument);
    CHECK_THROWS(data.max("empty"), std::invalid_argument);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    data.set("nan", {1.0, nan, -3.0});
    CHECK(std::isnan(data.min("nan")));
    CHECK(std::isnan(data.max("nan")));

    return true;
}

bool testConstLookupAndMutableIndexing()
{
    pyqtgraph::PlotData data;
    data.addFields({"x"});
    data["x"].push_back(4.0);
    data["x"].push_back(-1.0);

    const pyqtgraph::PlotData& constData = data;
    CHECK((std::is_same_v<decltype(constData["x"]), const pyqtgraph::PlotData::Values&>));
    CHECK(constData["x"].size() == 2);
    CHECK(constData.min("x") == -1.0);
    CHECK(constData.max("x") == 4.0);

    return true;
}

bool testP203NormalizedRangeAndContainerInputs()
{
    pyqtgraph::PlotData data;

    const std::array<int, 4> integerArray{1, 2, 3, 4};
    data.set("range", integerArray);
    CHECK(data["range"].size() == 4);
    CHECK(data.min("range") == 1.0);
    CHECK(data.max("range") == 4.0);

    const std::list<float> tupleLikeContainer{8.0F, -4.0F, 6.0F};
    data.set("tuple", tupleLikeContainer);
    CHECK(data["tuple"].size() == 3);
    CHECK(data.min("tuple") == -4.0);
    CHECK(data.max("tuple") == 8.0);

    auto iotaView = std::views::iota(1, 5);
    data.set("iota_view", iotaView);
    CHECK(data["iota_view"].size() == 4);
    CHECK(data.min("iota_view") == 1.0);
    CHECK(data.max("iota_view") == 4.0);

    const std::vector<double> empty;
    data.set("empty_container", empty);
    CHECK(data["empty_container"].empty());
    CHECK_THROWS(data.min("empty_container"), std::invalid_argument);
    CHECK_THROWS(data.max("empty_container"), std::invalid_argument);

    return true;
}

bool testP203MaskedExtrema()
{
    pyqtgraph::PlotData data;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    data.setMasked("masked", {7.0, -5.0, 2.0, nan}, {false, true, false, true});
    CHECK(data["masked"].size() == 4);
    CHECK(data.min("masked") == 2.0);
    CHECK(data.max("masked") == 7.0);

    data.setMasked("masked_nan", {7.0, nan, 2.0}, {false, false, true});
    CHECK(std::isnan(data.min("masked_nan")));
    CHECK(std::isnan(data.max("masked_nan")));

    data.setMasked("all_masked", {1.0, 2.0}, {true, true});
    CHECK_THROWS(data.min("all_masked"), std::invalid_argument);
    CHECK_THROWS(data.max("all_masked"), std::invalid_argument);

    CHECK_THROWS(data.setMasked("bad_mask", {1.0, 2.0}, {false}), std::invalid_argument);

    data.setMasked("erase_mask", {100.0, -1.0}, {true, false});
    data.set("erase_mask", {1.0, 2.0});
    CHECK(data.min("erase_mask") == 1.0);
    CHECK(data.max("erase_mask") == 2.0);

    return true;
}

} // namespace

int main()
{
    static_assert(std::ranges::input_range<std::array<int, 4>>);

    bool success = true;
    success = requireP203OracleFixture() && success;
    success = testFieldLifecycleAndLookup() && success;
    success = testAssignmentReplacementAndCachedExtrema() && success;
    success = testErrorsAndNanExtrema() && success;
    success = testConstLookupAndMutableIndexing() && success;
    success = testP203NormalizedRangeAndContainerInputs() && success;
    success = testP203MaskedExtrema() && success;

    return success ? 0 : 1;
}
