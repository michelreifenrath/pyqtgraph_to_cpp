#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef CPPQTGRAPH_P0_06_FIXTURE
#define CPPQTGRAPH_P0_06_FIXTURE "oracle/fixtures/P0_06/probe_contract.json"
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

std::size_t find_matching_bracket(
    const std::string& text,
    const std::size_t open_index,
    const char open_char,
    const char close_char
) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = open_index; index < text.size(); ++index) {
        const char character = text[index];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if (character == '"') {
                in_string = false;
            }
            continue;
        }
        if (character == '"') {
            in_string = true;
        } else if (character == open_char) {
            ++depth;
        } else if (character == close_char) {
            --depth;
            if (depth == 0) {
                return index;
            }
        }
    }
    throw std::runtime_error("unclosed JSON bracket");
}

std::string expected_object(const std::string& fixture) {
    std::smatch match;
    const std::regex object_start_pattern("\\\"expected\\\"\\s*:\\s*\\{");
    if (!std::regex_search(fixture, match, object_start_pattern)) {
        throw std::runtime_error("missing $.expected object");
    }
    const auto object_start = static_cast<std::size_t>(match.position(0) + match.length(0) - 1);
    const auto object_end = find_matching_bracket(fixture, object_start, '{', '}');
    return fixture.substr(object_start, object_end - object_start + 1);
}

std::string trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::vector<double> expected_scaled_values(const std::string& fixture) {
    const auto expected = expected_object(fixture);
    std::smatch match;
    const std::regex array_start_pattern("\\\"scaled_values\\\"\\s*:\\s*\\[");
    if (!std::regex_search(expected, match, array_start_pattern)) {
        throw std::runtime_error("missing $.expected.scaled_values array");
    }
    const auto array_start = static_cast<std::size_t>(match.position(0) + match.length(0) - 1);
    const auto array_end = find_matching_bracket(expected, array_start, '[', ']');
    std::stringstream input(expected.substr(array_start + 1, array_end - array_start - 1));
    std::vector<double> values;
    std::string item;
    while (std::getline(input, item, ',')) {
        item = trim(item);
        if (item.empty()) {
            throw std::runtime_error("empty $.expected.scaled_values entry");
        }
        std::size_t parsed = 0;
        const double value = std::stod(item, &parsed);
        if (parsed != item.size()) {
            throw std::runtime_error("non-numeric $.expected.scaled_values entry: " + item);
        }
        values.push_back(value);
    }
    return values;
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

    const auto fixture_path = std::filesystem::path(CPPQTGRAPH_P0_06_FIXTURE);
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
    std::vector<double> fixture_scaled_values;
    try {
        fixture_scaled_values = expected_scaled_values(fixture);
    } catch (const std::exception& exception) {
        return report_mismatch("$.expected.scaled_values", "JSON numeric array", exception.what());
    }
    if (fixture_scaled_values.size() != scaled_values.size()) {
        return report_mismatch(
            "$.expected.scaled_values.size",
            std::to_string(fixture_scaled_values.size()),
            std::to_string(scaled_values.size())
        );
    }
    for (std::size_t index = 0; index < scaled_values.size(); ++index) {
        if (std::abs(fixture_scaled_values[index] - scaled_values[index]) > 0.0) {
            return report_mismatch(
                "$.expected.scaled_values[" + std::to_string(index) + "]",
                std::to_string(fixture_scaled_values[index]),
                std::to_string(scaled_values[index])
            );
        }
    }

    return 0;
}
