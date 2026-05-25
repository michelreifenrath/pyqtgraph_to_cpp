#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifndef PYQTGRAPH_CPP_P0_06_FIXTURE
#define PYQTGRAPH_CPP_P0_06_FIXTURE "oracle/fixtures/P0_06/probe_contract.json"
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open oracle fixture: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

std::string number_pattern(const std::string& key) {
    return "\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)";
}

double expected_number(const std::string& fixture, const std::string& key) {
    std::smatch match;
    if (!std::regex_search(fixture, match, std::regex(number_pattern(key)))) {
        throw std::runtime_error("missing numeric fixture key: " + key);
    }
    return std::stod(match[1].str());
}

int report_mismatch(const std::string& path, const std::string& expected, const std::string& actual) {
    std::cerr << "oracle fixture mismatch\n"
              << "fixture: oracle/fixtures/P0_06/probe_contract.json\n"
              << "path: " << path << "\n"
              << "expected fixture value: " << expected << "\n"
              << "actual C++ value: " << actual << "\n"
              << "tolerance absolute=0.0 relative=0.0\n";
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    bool mismatch_mode = false;
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--mismatch") {
            mismatch_mode = true;
        }
    }

    const auto fixture_path = std::filesystem::path(PYQTGRAPH_CPP_P0_06_FIXTURE);
    const auto fixture = read_file(fixture_path);

    const std::vector<double> values{1.25, -2.5, 3.75};
    const double scale = 2.0;
    const double offset = 0.5;
    std::vector<double> scaled_values;
    scaled_values.reserve(values.size());
    double sum = 0.0;
    for (double value : values) {
        const double scaled = value * scale + offset;
        scaled_values.push_back(scaled);
        sum += scaled;
    }
    if (mismatch_mode) {
        scaled_values.front() = -999.0;
    }

    if (!contains(fixture, "\"schema_version\": 1")) {
        return report_mismatch("$.schema_version", "1", "<missing>");
    }
    if (!contains(fixture, "\"issue\": \"P0.06\"")) {
        return report_mismatch("$.issue", "'P0.06'", "<missing>");
    }
    if (!contains(fixture, "\"absolute\": 0.0") || !contains(fixture, "\"relative\": 0.0")) {
        return report_mismatch("$.tolerance", "absolute=0.0 relative=0.0", "<missing>");
    }
    const auto expected_count = static_cast<std::size_t>(expected_number(fixture, "count"));
    if (expected_count != values.size()) {
        return report_mismatch("$.expected.count", std::to_string(expected_count), std::to_string(values.size()));
    }
    const double expected_sum = expected_number(fixture, "sum");
    if (std::abs(expected_sum - sum) > 0.0) {
        return report_mismatch("$.expected.sum", std::to_string(expected_sum), std::to_string(sum));
    }
    if (!contains(fixture, "      3.0,") || !contains(fixture, "      -4.5,") || !contains(fixture, "      8.0")) {
        return report_mismatch("$.expected.scaled_values", "[3.0, -4.5, 8.0]", "fixture text differed");
    }
    if (std::abs(scaled_values[0] - 3.0) > 0.0) {
        return report_mismatch("$.expected.scaled_values[0]", "3.0", std::to_string(scaled_values[0]));
    }

    return 0;
}
