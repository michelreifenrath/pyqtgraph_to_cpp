#include "../../include/cppqtgraph/configfile.hpp"
#include "../../include/cppqtgraph/debug.hpp"
#include "../../include/cppqtgraph/exceptionHandling.hpp"
#include "../../include/cppqtgraph/units.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifndef CPPQTGRAPH_P2_07_FIXTURE
#define CPPQTGRAPH_P2_07_FIXTURE "oracle/fixtures/P2_07/core_helpers_oracle.json"
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

std::string readOracleFixture()
{
    std::ifstream input(std::filesystem::path{CPPQTGRAPH_P2_07_FIXTURE});
    if (!input.good()) {
        std::cerr << "missing P2.07 oracle fixture: " << CPPQTGRAPH_P2_07_FIXTURE << '\n';
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

template <typename Exception, typename Fn>
bool throwsContaining(Fn&& fn, std::string_view message)
{
    try {
        fn();
    } catch (const Exception& exc) {
        return contains(exc.what(), message);
    }
    return false;
}

template <typename T>
const T& as(const cppqtgraph::ConfigOptionValue& value)
{
    return std::get<T>(value);
}

bool requireP207OracleFixture()
{
    const std::string fixture = readOracleFixture();
    CHECK(contains(fixture, "\"issue\": \"P2.07\""));
    CHECK(contains(fixture, "\"commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\""));
    CHECK(contains(fixture, "pyqtgraph/__init__.py"));
    CHECK(contains(fixture, "pyqtgraph/configfile.py"));
    CHECK(contains(fixture, "pyqtgraph/debug.py"));
    CHECK(contains(fixture, "pyqtgraph/exceptionHandling.py"));
    CHECK(contains(fixture, "pyqtgraph/units.py"));
    CHECK(contains(fixture, "tests/test_configparser.py"));
    CHECK(contains(fixture, "\"unit_scale_absolute\": 1e-15"));
    CHECK(contains(fixture, "arbitrary Python eval"));
    CHECK(contains(fixture, "std::nullopt"));
    return true;
}

bool testConfigOptions()
{
    cppqtgraph::resetConfigOptions();
    CHECK(cppqtgraph::CONFIG_OPTIONS().size() == 14U);
    CHECK(as<bool>(cppqtgraph::getConfigOption("useOpenGL")) == false);
    CHECK(as<bool>(cppqtgraph::getConfigOption("leftButtonPan")) == true);
    CHECK(as<std::string>(cppqtgraph::getConfigOption("foreground")) == "d");
    CHECK(as<std::string>(cppqtgraph::getConfigOption("background")) == "k");
    CHECK(as<bool>(cppqtgraph::getConfigOption("antialias")) == false);
    CHECK(std::holds_alternative<std::monostate>(cppqtgraph::getConfigOption("editorCommand")));
    CHECK(as<bool>(cppqtgraph::getConfigOption("exitCleanup")) == true);
    CHECK(as<bool>(cppqtgraph::getConfigOption("enableExperimental")) == false);
    CHECK(as<bool>(cppqtgraph::getConfigOption("crashWarning")) == false);
    CHECK(as<int>(cppqtgraph::getConfigOption("mouseRateLimit")) == 100);
    CHECK(as<std::string>(cppqtgraph::getConfigOption("imageAxisOrder")) == "col-major");
    CHECK(as<bool>(cppqtgraph::getConfigOption("useCupy")) == false);
    CHECK(as<bool>(cppqtgraph::getConfigOption("useNumba")) == false);
    CHECK(as<std::string>(cppqtgraph::getConfigOption("segmentedLineMode")) == "auto");

    cppqtgraph::setConfigOption("imageAxisOrder", std::string{"row-major"});
    CHECK(as<std::string>(cppqtgraph::getConfigOption("imageAxisOrder")) == "row-major");
    cppqtgraph::setConfigOption("segmentedLineMode", std::string{"on"});
    CHECK(as<std::string>(cppqtgraph::getConfigOption("segmentedLineMode")) == "on");
    cppqtgraph::setConfigOptions({{"antialias", true}, {"mouseRateLimit", 25}, {"segmentedLineMode", std::string{"off"}}});
    CHECK(as<bool>(cppqtgraph::getConfigOption("antialias")) == true);
    CHECK(as<int>(cppqtgraph::getConfigOption("mouseRateLimit")) == 25);
    CHECK(as<std::string>(cppqtgraph::getConfigOption("segmentedLineMode")) == "off");

    CHECK(throwsContaining<std::out_of_range>([] { (void)cppqtgraph::getConfigOption("missing"); }, "Unknown configuration option"));
    CHECK(throwsContaining<std::out_of_range>([] { cppqtgraph::setConfigOption("missing", true); }, "Unknown configuration option"));
    CHECK(throwsContaining<std::invalid_argument>([] { cppqtgraph::setConfigOption("imageAxisOrder", std::string{"bad"}); }, "row-major"));
    CHECK(throwsContaining<std::invalid_argument>([] { cppqtgraph::setConfigOption("segmentedLineMode", true); }, "segmentedLineMode"));
    CHECK(throwsContaining<std::invalid_argument>([] { cppqtgraph::setConfigOption("segmentedLineMode", std::string{"bad"}); }, "auto"));
    return true;
}

bool nearlyEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= 1.0e-15;
}

bool testUnits()
{
    const auto& prefixes = cppqtgraph::units::SI_PREFIXES();
    CHECK(prefixes.size() == 17U);
    CHECK(prefixes.at(8) == " ");
    const auto& allUnits = cppqtgraph::units::allUnits();
    CHECK(nearlyEqual(allUnits.at("m"), 1.0));
    CHECK(nearlyEqual(allUnits.at("mm"), 1.0e-3));
    CHECK(nearlyEqual(allUnits.at("µm"), 1.0e-6));
    CHECK(nearlyEqual(allUnits.at("μm"), 1.0e-6));
    CHECK(nearlyEqual(allUnits.at("um"), 1.0e-6));
    CHECK(nearlyEqual(allUnits.at("kHz"), 1.0e3));
    CHECK(nearlyEqual(allUnits.at("MHz"), 1.0e6));
    CHECK(nearlyEqual(allUnits.at("Ohm"), 1.0));
    CHECK(nearlyEqual(allUnits.at("Ω"), 1.0));
    CHECK(nearlyEqual(allUnits.at("mV"), 1.0e-3));
    CHECK(nearlyEqual(allUnits.at("daV"), 10.0));
    CHECK(nearlyEqual(allUnits.at("hPa"), 100.0));
    CHECK(nearlyEqual(allUnits.at("dB"), 0.1));
    CHECK(nearlyEqual(allUnits.at("cA"), 0.01));
    CHECK(!cppqtgraph::units::evalUnits("N m/s^2").has_value());
    CHECK(!cppqtgraph::units::formatUnits("unused").has_value());
    CHECK(!cppqtgraph::units::simplify("unused").has_value());
    return true;
}

bool testConfigFile()
{
    using cppqtgraph::configfile::ConfigMap;
    using cppqtgraph::configfile::ConfigValue;
    using cppqtgraph::configfile::ParseError;

    ConfigMap input{
        {"par1", ConfigValue::list({ConfigValue::integer(1), ConfigValue::integer(2), ConfigValue::integer(3)})},
        {"par2", ConfigValue::string("Test")},
        {"par3", ConfigValue::map({{"a", ConfigValue::integer(3)}, {"b", ConfigValue::string("c")}})},
    };
    const std::string text = cppqtgraph::configfile::genString(input);
    CHECK(contains(text, "par1: [1, 2, 3]\n"));
    CHECK(contains(text, "par2: 'Test'\n"));
    CHECK(contains(text, "par3:\n"));
    const auto parsed = cppqtgraph::configfile::parseString(text);
    CHECK(parsed.data == input);

    CHECK(throwsContaining<ParseError>([] { (void)cppqtgraph::configfile::parseString("a: 1\na: 2\n"); }, "Duplicate key"));
    try {
        (void)cppqtgraph::configfile::parseString("a: 1\n\n# comment\na: 2\n");
        CHECK(false);
    } catch (const ParseError& exc) {
        CHECK(exc.lineNum() == 4);
        CHECK(contains(exc.what(), "at line 4"));
    }

    const auto nested = cppqtgraph::configfile::parseString("a:\n        # comment\n    b:\n# more comments\n        c: 2\n");
    CHECK(nested.data.at("a").asMap().at("b").asMap().at("c") == ConfigValue::integer(2));
    ConfigMap listWithMap{
        {"items", ConfigValue::list({ConfigValue::map({{"inner", ConfigValue::string("v")}, {"nums", ConfigValue::list({ConfigValue::integer(1), ConfigValue::integer(2)})}})})},
    };
    const std::string listWithMapText = cppqtgraph::configfile::genString(listWithMap);
    CHECK(contains(listWithMapText, "items: [{'inner': 'v', 'nums': [1, 2]}]\n"));
    CHECK(cppqtgraph::configfile::parseString(listWithMapText).data == listWithMap);
    ConfigMap floats{{"one", ConfigValue::floating(1.0)}, {"negzero", ConfigValue::floating(-0.0)}};
    const std::string floatsText = cppqtgraph::configfile::genString(floats);
    CHECK(contains(floatsText, "one: 1.0\n"));
    CHECK(contains(floatsText, "negzero: -0.0\n"));
    CHECK(cppqtgraph::configfile::parseString(floatsText).data == floats);
    const auto comments = cppqtgraph::configfile::parseString("a: 1 # keep default\ns: '# not comment' # trailing\nlist: [1, 2] # trailing list\n");
    CHECK(comments.data.at("a") == ConfigValue::integer(1));
    CHECK(comments.data.at("s") == ConfigValue::string("# not comment"));
    CHECK(comments.data.at("list").asList().at(1) == ConfigValue::integer(2));
    const auto parsedListDicts = cppqtgraph::configfile::parseString("items: [{'inner': 'v'}, {'n': 2}]\n");
    CHECK(parsedListDicts.data.at("items").asList().at(0).asMap().at("inner") == ConfigValue::string("v"));
    CHECK(parsedListDicts.data.at("items").asList().at(1).asMap().at("n") == ConfigValue::integer(2));

    const auto parsedEscapes = cppqtgraph::configfile::parseString("s: '\\t\\r\\x41\\u00b5\\U0001f600\\141'\n");
    const std::string expectedEscapes = std::string{"\t\rA"} + "\xc2\xb5" + "\xf0\x9f\x98\x80" + "a";
    CHECK(parsedEscapes.data.at("s") == ConfigValue::string(expectedEscapes));
    CHECK(contains(cppqtgraph::configfile::genString({{"s", ConfigValue::string("\t\r\n")}}), "s: '\\t\\r\\n'\n"));
    CHECK(throwsContaining<ParseError>([] { (void)cppqtgraph::configfile::parseString("s: '\\q'\n"); }, "unsupported escape sequence"));

    CHECK(throwsContaining<std::invalid_argument>([] { (void)cppqtgraph::configfile::genString({{"bad:key", ConfigValue::integer(1)}}); }, "must not contain"));
    const auto tempPath = std::filesystem::temp_directory_path() / "cppqtgraph_P2_07_existing_config.cfg";
    {
        std::ofstream output{tempPath};
        output << "still: 'here'\n";
    }
    CHECK(throwsContaining<std::invalid_argument>([&] { cppqtgraph::configfile::writeConfigFile({{"bad:key", ConfigValue::integer(1)}}, tempPath.string()); }, "must not contain"));
    {
        std::ifstream input{tempPath};
        std::ostringstream buffer;
        buffer << input.rdbuf();
        CHECK(buffer.str() == "still: 'here'\n");
    }
    std::filesystem::remove(tempPath);

    CHECK(throwsContaining<ParseError>([] { (void)cppqtgraph::configfile::parseString("unsupported: object()\n"); }, "Unsupported expression"));
    return true;
}

bool testExceptionHandling()
{
    namespace eh = cppqtgraph::exceptionHandling;
    eh::clearCallbacks();
    std::vector<std::string> calls;
    const auto first = eh::registerCallback([&](const eh::ExceptionArgs& args) {
        calls.push_back("first:" + args.message);
    });
    const auto throwing = eh::registerCallback([](const eh::ExceptionArgs&) { throw std::runtime_error("callback failed"); });
    const auto second = eh::registerCallback([&](const eh::ExceptionArgs& args) {
        calls.push_back(args.thread == std::thread::id{} ? "second-main" : "second-thread");
    });
    const auto result = eh::notifyUnhandledException(std::make_exception_ptr(std::runtime_error("boom")));
    CHECK(result.callbackCount == 3U);
    CHECK(result.callbackFailureCount == 1U);
    CHECK((calls == std::vector<std::string>{"first:boom", "second-main"}));

    eh::unregisterCallback(throwing);
    eh::unregisterCallback(first);
    calls.clear();
    const auto old = eh::registerCallbackDeprecated([&](std::exception_ptr, std::string_view typeName, std::string_view message) {
        calls.push_back(std::string{typeName} + ":" + std::string{message});
    });
    const auto result2 = eh::ExceptionHandler{}.handle(std::make_exception_ptr(std::logic_error("old")));
    CHECK(result2.callbackCount == 1U);
    CHECK(result2.oldCallbackCount == 1U);
    CHECK(calls.size() == 2U);
    CHECK(calls.front() == "second-main");
    CHECK(contains(calls.back(), "logic_error:old"));
    eh::unregisterCallback(second);
    eh::unregisterDeprecated(old);
    eh::setTracebackClearing(true);
    CHECK(eh::tracebackClearing());
    eh::setTracebackClearing(false);
    CHECK(!eh::tracebackClearing());
    eh::clearCallbacks();
    return true;
}

bool testDebug()
{
    const auto formatted = cppqtgraph::debug::formatException(std::runtime_error("boom"));
    CHECK(contains(formatted, "runtime_error"));
    CHECK(contains(formatted, "boom"));
    bool ran = false;
    CHECK(!cppqtgraph::debug::warnOnException([&] {
        ran = true;
        throw std::runtime_error("ignored");
    }, "ignored"));
    CHECK(ran);
    CHECK(cppqtgraph::debug::warnOnException([] {}, "ok"));
    CHECK(contains(cppqtgraph::debug::backtrace(), "native backtrace"));
    const cppqtgraph::debug::Profiler profiler{"P2.07", false};
    CHECK(profiler.name() == "P2.07");
    CHECK(profiler.elapsedSeconds() >= 0.0);
    const auto unsupported = cppqtgraph::debug::unsupportedPythonHelpers();
    CHECK(std::find(unsupported.begin(), unsupported.end(), "listObjs") != unsupported.end());
    return true;
}

} // namespace

int main()
{
    if (!requireP207OracleFixture() || !testConfigOptions() || !testUnits() || !testConfigFile() ||
        !testExceptionHandling() || !testDebug()) {
        return 1;
    }
    return 0;
}
