#include "../../include/pyqtgraph/PlotData.hpp"

#include <cmath>
#include <iostream>
#include <limits>
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

} // namespace

int main()
{
    bool success = true;
    success = testFieldLifecycleAndLookup() && success;
    success = testAssignmentReplacementAndCachedExtrema() && success;
    success = testErrorsAndNanExtrema() && success;
    success = testConstLookupAndMutableIndexing() && success;

    return success ? 0 : 1;
}
