#include "../../include/cppqtgraph/colormap.hpp"

#include <QColor>
#include <QString>

#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
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

bool checkEqual(std::size_t actual, std::size_t expected, std::string_view label)
{
    if (actual != expected) {
        std::cerr << label << ": expected " << expected << ", got " << actual << '\n';
        return false;
    }
    return true;
}

#define CHECK_EQ(actual, expected) \
    do { \
        if (!checkEqual((actual), (expected), #actual)) { \
            return false; \
        } \
    } while (false)

template <typename Callable>
bool checkThrowsInvalidArgument(Callable callable, std::string_view label)
{
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << label << ": expected std::invalid_argument, got " << error.what() << '\n';
        return false;
    }

    std::cerr << label << ": expected std::invalid_argument\n";
    return false;
}

bool testConstructionStoresStopsAndName()
{
    const std::vector<double> positions{0.0, 0.5, 1.0};
    const std::vector<QColor> colors{QColor(0, 0, 0), QColor(128, 64, 32), QColor(255, 255, 255)};

    const cppqtgraph::ColorMap map(positions, colors, QStringLiteral("test-map"));

    CHECK_EQ(map.size(), positions.size());
    CHECK(!map.empty());
    CHECK(map.name() == QStringLiteral("test-map"));
    CHECK(map.positions() == positions);
    CHECK(map.colors() == colors);

    return true;
}

bool testDefaultNameIsEmpty()
{
    const cppqtgraph::ColorMap map({0.0, 1.0}, {QColor(1, 2, 3), QColor(4, 5, 6)});

    CHECK_EQ(map.size(), std::size_t{2});
    CHECK(map.name().isEmpty());

    return true;
}

bool testValidationErrors()
{
    CHECK(checkThrowsInvalidArgument([] { (void)cppqtgraph::ColorMap({}, {}); }, "empty stops"));
    CHECK(checkThrowsInvalidArgument(
        [] { (void)cppqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0)}); },
        "mismatched stop counts"));

    return true;
}

} // namespace

int main()
{
    bool success = true;
    success = testConstructionStoresStopsAndName() && success;
    success = testDefaultNameIsEmpty() && success;
    success = testValidationErrors() && success;

    return success ? 0 : 1;
}
