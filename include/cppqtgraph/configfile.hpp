// Source note: translated/adapted from PyQtGraph pyqtgraph/configfile.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace cppqtgraph {

using ConfigOptionValue = std::variant<std::monostate, bool, int, double, std::string>;
using ConfigOptionMap = std::map<std::string, ConfigOptionValue>;

namespace detail {

inline ConfigOptionMap defaultConfigOptions()
{
    return {
        {"useOpenGL", false},
        {"leftButtonPan", true},
        {"foreground", std::string{"d"}},
        {"background", std::string{"k"}},
        {"antialias", false},
        {"editorCommand", std::monostate{}},
        {"exitCleanup", true},
        {"enableExperimental", false},
        {"crashWarning", false},
        {"mouseRateLimit", 100},
        {"imageAxisOrder", std::string{"col-major"}},
        {"useCupy", false},
        {"useNumba", false},
        {"segmentedLineMode", std::string{"auto"}},
    };
}

inline ConfigOptionMap& mutableConfigOptions()
{
    static ConfigOptionMap options = defaultConfigOptions();
    return options;
}

inline std::string requireConfigString(std::string_view opt, const ConfigOptionValue& value)
{
    if (const auto* stringValue = std::get_if<std::string>(&value)) {
        return *stringValue;
    }
    throw std::invalid_argument(std::string{opt} + " must be a string option value");
}

inline void validateConfigOption(std::string_view opt, const ConfigOptionValue& value)
{
    if (opt == "imageAxisOrder") {
        const std::string stringValue = requireConfigString(opt, value);
        if (stringValue != "row-major" && stringValue != "col-major") {
            throw std::invalid_argument("imageAxisOrder must be either \"row-major\" or \"col-major\"");
        }
    }
    if (opt == "segmentedLineMode") {
        const std::string stringValue = requireConfigString(opt, value);
        if (stringValue != "auto" && stringValue != "on" && stringValue != "off") {
            throw std::invalid_argument("segmentedLineMode must be \"auto\", \"on\" or \"off\"");
        }
    }
}

} // namespace detail

// C++ equivalent of pyqtgraph.CONFIG_OPTIONS from pyqtgraph/__init__.py.  The
// Python module exposes a mutable dict; this port exposes controlled mutation
// so PyQtGraph 0.14.0 enum validation is always applied.  Python None is
// represented by std::monostate.
[[nodiscard]] inline const ConfigOptionMap& CONFIG_OPTIONS()
{
    return detail::mutableConfigOptions();
}

inline void resetConfigOptions()
{
    detail::mutableConfigOptions() = detail::defaultConfigOptions();
}

inline void setConfigOption(std::string_view opt, ConfigOptionValue value)
{
    auto& options = detail::mutableConfigOptions();
    const auto found = options.find(std::string{opt});
    if (found == options.end()) {
        throw std::out_of_range("Unknown configuration option \"" + std::string{opt} + "\"");
    }
    detail::validateConfigOption(opt, value);
    found->second = std::move(value);
}

inline void setConfigOption(std::string_view opt, const char* value)
{
    setConfigOption(opt, ConfigOptionValue{std::string{value == nullptr ? "" : value}});
}

inline void setConfigOptions(std::initializer_list<std::pair<std::string, ConfigOptionValue>> opts)
{
    for (const auto& [opt, value] : opts) {
        setConfigOption(opt, value);
    }
}

[[nodiscard]] inline const ConfigOptionValue& getConfigOption(std::string_view opt)
{
    const auto& options = detail::mutableConfigOptions();
    const auto found = options.find(std::string{opt});
    if (found == options.end()) {
        throw std::out_of_range("Unknown configuration option \"" + std::string{opt} + "\"");
    }
    return found->second;
}

} // namespace cppqtgraph

namespace cppqtgraph::configfile {

class ConfigValue;
using ConfigList = std::vector<ConfigValue>;
using ConfigMap = std::map<std::string, ConfigValue>;

class ConfigValue final {
public:
    enum class Type { Null, Bool, Integer, Floating, String, List, Map };

    [[nodiscard]] static ConfigValue null() { return {}; }
    [[nodiscard]] static ConfigValue boolean(bool value)
    {
        ConfigValue result;
        result.type_ = Type::Bool;
        result.boolValue_ = value;
        return result;
    }
    [[nodiscard]] static ConfigValue integer(std::int64_t value)
    {
        ConfigValue result;
        result.type_ = Type::Integer;
        result.integerValue_ = value;
        return result;
    }
    [[nodiscard]] static ConfigValue floating(double value)
    {
        ConfigValue result;
        result.type_ = Type::Floating;
        result.floatingValue_ = value;
        return result;
    }
    [[nodiscard]] static ConfigValue string(std::string value)
    {
        ConfigValue result;
        result.type_ = Type::String;
        result.stringValue_ = std::move(value);
        return result;
    }
    [[nodiscard]] static ConfigValue list(ConfigList value)
    {
        ConfigValue result;
        result.type_ = Type::List;
        result.listValue_ = std::move(value);
        return result;
    }
    [[nodiscard]] static ConfigValue map(ConfigMap value)
    {
        ConfigValue result;
        result.type_ = Type::Map;
        result.mapValue_ = std::move(value);
        return result;
    }

    [[nodiscard]] Type type() const noexcept { return type_; }
    [[nodiscard]] bool asBool() const
    {
        require(Type::Bool, "bool");
        return boolValue_;
    }
    [[nodiscard]] std::int64_t asInteger() const
    {
        require(Type::Integer, "integer");
        return integerValue_;
    }
    [[nodiscard]] double asFloating() const
    {
        require(Type::Floating, "floating");
        return floatingValue_;
    }
    [[nodiscard]] const std::string& asString() const
    {
        require(Type::String, "string");
        return stringValue_;
    }
    [[nodiscard]] const ConfigList& asList() const
    {
        require(Type::List, "list");
        return listValue_;
    }
    [[nodiscard]] const ConfigMap& asMap() const
    {
        require(Type::Map, "map");
        return mapValue_;
    }

    friend bool operator==(const ConfigValue& lhs, const ConfigValue& rhs)
    {
        if (lhs.type_ != rhs.type_) {
            return false;
        }
        switch (lhs.type_) {
        case Type::Null:
            return true;
        case Type::Bool:
            return lhs.boolValue_ == rhs.boolValue_;
        case Type::Integer:
            return lhs.integerValue_ == rhs.integerValue_;
        case Type::Floating:
            return lhs.floatingValue_ == rhs.floatingValue_;
        case Type::String:
            return lhs.stringValue_ == rhs.stringValue_;
        case Type::List:
            return lhs.listValue_ == rhs.listValue_;
        case Type::Map:
            return lhs.mapValue_ == rhs.mapValue_;
        }
        return false;
    }
    friend bool operator!=(const ConfigValue& lhs, const ConfigValue& rhs) { return !(lhs == rhs); }

private:
    void require(Type type, std::string_view name) const
    {
        if (type_ != type) {
            throw std::logic_error("ConfigValue is not a " + std::string{name});
        }
    }

    Type type_{Type::Null};
    bool boolValue_{false};
    std::int64_t integerValue_{0};
    double floatingValue_{0.0};
    std::string stringValue_;
    ConfigList listValue_;
    ConfigMap mapValue_;
};

class ParseError : public std::runtime_error {
public:
    ParseError(std::string message, std::size_t lineNum, std::string line, std::string fileName = {})
        : std::runtime_error(format(message, lineNum, line, fileName))
        , message_(std::move(message))
        , lineNum_(lineNum)
        , line_(std::move(line))
        , fileName_(std::move(fileName))
    {
    }

    [[nodiscard]] std::size_t lineNum() const noexcept { return lineNum_; }
    [[nodiscard]] const std::string& line() const noexcept { return line_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const std::string& fileName() const noexcept { return fileName_; }

private:
    static std::string format(std::string_view message, std::size_t lineNum, std::string_view line, std::string_view fileName)
    {
        std::ostringstream stream;
        if (fileName.empty()) {
            stream << "Error parsing string at line " << lineNum << ":\n";
        } else {
            stream << "Error parsing config file '" << fileName << "' at line " << lineNum << ":\n";
        }
        stream << line << '\n' << message;
        return stream.str();
    }

    std::string message_;
    std::size_t lineNum_{0};
    std::string line_;
    std::string fileName_;
};

struct ParseResult final {
    std::size_t line{0};
    ConfigMap data;
};

[[nodiscard]] inline int measureIndent(std::string_view line)
{
    int count = 0;
    while (static_cast<std::size_t>(count) < line.size() && line[static_cast<std::size_t>(count)] == ' ') {
        ++count;
    }
    return count;
}

namespace detail {

[[nodiscard]] inline std::string trim(std::string_view text)
{
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first])) != 0) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) {
        --last;
    }
    return std::string{text.substr(first, last - first)};
}

[[nodiscard]] inline bool lineIsReal(std::string_view line)
{
    for (const char ch : line) {
        if (std::isspace(static_cast<unsigned char>(ch)) == 0) {
            return ch != '#';
        }
    }
    return false;
}

[[nodiscard]] inline std::vector<std::string> splitLines(std::string_view input)
{
    std::string normalized;
    normalized.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r') {
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                ++i;
            }
            normalized.push_back('\n');
        } else {
            normalized.push_back(input[i]);
        }
    }
    std::vector<std::string> lines;
    std::stringstream stream{normalized};
    std::string line;
    while (std::getline(stream, line, '\n')) {
        lines.push_back(line);
    }
    if (!normalized.empty() && normalized.back() == '\n') {
        lines.emplace_back();
    }
    return lines;
}

[[nodiscard]] inline std::string quoteString(std::string_view value)
{
    std::string out{"'"};
    for (const unsigned char ch : value) {
        switch (ch) {
        case '\\':
            out += "\\\\";
            break;
        case '\'':
            out += "\\'";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\a':
            out += "\\a";
            break;
        case '\v':
            out += "\\v";
            break;
        default:
            if (ch < 0x20) {
                std::ostringstream stream;
                stream << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
                out += stream.str();
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    out.push_back('\'');
    return out;
}

[[nodiscard]] inline std::string valueRepr(const ConfigValue& value);

[[nodiscard]] inline std::string mapRepr(const ConfigMap& map)
{
    std::string out{"{"};
    std::size_t index = 0;
    for (const auto& [key, value] : map) {
        if (index++ > 0) {
            out += ", ";
        }
        out += quoteString(key);
        out += ": ";
        out += valueRepr(value);
    }
    out += "}";
    return out;
}

[[nodiscard]] inline std::string listRepr(const ConfigList& list)
{
    std::string out{"["};
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += valueRepr(list[i]);
    }
    out += "]";
    return out;
}

[[nodiscard]] inline std::string valueRepr(const ConfigValue& value)
{
    switch (value.type()) {
    case ConfigValue::Type::Null:
        return "None";
    case ConfigValue::Type::Bool:
        return value.asBool() ? "True" : "False";
    case ConfigValue::Type::Integer:
        return std::to_string(value.asInteger());
    case ConfigValue::Type::Floating: {
        std::ostringstream stream;
        stream << std::setprecision(17) << value.asFloating();
        std::string result = stream.str();
        if (result.find_first_of(".eE") == std::string::npos) {
            result += ".0";
        }
        return result;
    }
    case ConfigValue::Type::String:
        return quoteString(value.asString());
    case ConfigValue::Type::List:
        return listRepr(value.asList());
    case ConfigValue::Type::Map:
        return mapRepr(value.asMap());
    }
    return "None";
}

[[nodiscard]] inline bool isQuoted(std::string_view text)
{
    return text.size() >= 2 && ((text.front() == '\'' && text.back() == '\'') || (text.front() == '"' && text.back() == '"'));
}

[[nodiscard]] inline int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}

inline void appendUtf8(std::string& out, std::uint32_t codepoint)
{
    if (codepoint > 0x10ffffU || (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
        throw std::invalid_argument("invalid unicode escape");
    }
    if (codepoint <= 0x7fU) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7ffU) {
        out.push_back(static_cast<char>(0xc0U | (codepoint >> 6U)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    } else if (codepoint <= 0xffffU) {
        out.push_back(static_cast<char>(0xe0U | (codepoint >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    } else {
        out.push_back(static_cast<char>(0xf0U | (codepoint >> 18U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3fU)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }
}

[[nodiscard]] inline std::uint32_t parseHexDigits(std::string_view text, std::size_t& i, std::size_t count)
{
    std::uint32_t value = 0;
    for (std::size_t n = 0; n < count; ++n) {
        if (i + 1 >= text.size() - 1) {
            throw std::invalid_argument("truncated hex escape");
        }
        const int digit = hexValue(text[++i]);
        if (digit < 0) {
            throw std::invalid_argument("invalid hex escape");
        }
        value = (value << 4U) | static_cast<std::uint32_t>(digit);
    }
    return value;
}

[[nodiscard]] inline std::string parseQuoted(std::string_view text)
{
    std::string out;
    for (std::size_t i = 1; i + 1 < text.size(); ++i) {
        char ch = text[i];
        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }
        if (i + 1 >= text.size() - 1) {
            throw std::invalid_argument("truncated escape sequence");
        }
        const char next = text[++i];
        switch (next) {
        case '\n':
            break;
        case '\\':
            out.push_back('\\');
            break;
        case '\'':
            out.push_back('\'');
            break;
        case '"':
            out.push_back('"');
            break;
        case 'n':
            out.push_back('\n');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case 't':
            out.push_back('\t');
            break;
        case 'b':
            out.push_back('\b');
            break;
        case 'f':
            out.push_back('\f');
            break;
        case 'a':
            out.push_back('\a');
            break;
        case 'v':
            out.push_back('\v');
            break;
        case 'x':
            out.push_back(static_cast<char>(parseHexDigits(text, i, 2)));
            break;
        case 'u':
            appendUtf8(out, parseHexDigits(text, i, 4));
            break;
        case 'U':
            appendUtf8(out, parseHexDigits(text, i, 8));
            break;
        case 'N':
            throw std::invalid_argument("named unicode escapes are not supported by the C++ config reader");
        default:
            if (next >= '0' && next <= '7') {
                std::uint32_t value = static_cast<std::uint32_t>(next - '0');
                for (int n = 0; n < 2 && i + 1 < text.size() - 1 && text[i + 1] >= '0' && text[i + 1] <= '7'; ++n) {
                    value = (value << 3U) | static_cast<std::uint32_t>(text[++i] - '0');
                }
                out.push_back(static_cast<char>(value & 0xffU));
            } else {
                throw std::invalid_argument(std::string{"unsupported escape sequence \\"} + next + "'");
            }
            break;
        }
    }
    return out;
}

[[nodiscard]] inline std::vector<std::string> splitListItems(std::string_view body)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    int bracketDepth = 0;
    char quote = '\0';
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char ch = body[i];
        if (quote != '\0') {
            if (ch == '\\') {
                ++i;
            } else if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '\'' || ch == '"') {
            quote = ch;
        } else if (ch == '[' || ch == '{' || ch == '(') {
            ++bracketDepth;
        } else if (ch == ']' || ch == '}' || ch == ')') {
            --bracketDepth;
        } else if (ch == ',' && bracketDepth == 0) {
            parts.push_back(trim(body.substr(start, i - start)));
            start = i + 1;
        }
    }
    const auto tail = trim(body.substr(start));
    if (!tail.empty()) {
        parts.push_back(tail);
    }
    return parts;
}

[[nodiscard]] inline std::size_t findTopLevelColon(std::string_view body)
{
    int bracketDepth = 0;
    char quote = '\0';
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char ch = body[i];
        if (quote != '\0') {
            if (ch == '\\') {
                ++i;
            } else if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '\'' || ch == '"') {
            quote = ch;
        } else if (ch == '[' || ch == '{' || ch == '(') {
            ++bracketDepth;
        } else if (ch == ']' || ch == '}' || ch == ')') {
            --bracketDepth;
        } else if (ch == ':' && bracketDepth == 0) {
            return i;
        }
    }
    return std::string_view::npos;
}

[[nodiscard]] inline std::string stripInlineComment(std::string_view expression)
{
    char quote = '\0';
    for (std::size_t i = 0; i < expression.size(); ++i) {
        const char ch = expression[i];
        if (quote != '\0') {
            if (ch == '\\') {
                ++i;
            } else if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '\'' || ch == '"') {
            quote = ch;
        } else if (ch == '#') {
            return trim(expression.substr(0, i));
        }
    }
    return trim(expression);
}

[[nodiscard]] inline ConfigValue parseValue(const std::string& expression, std::size_t lineNum, const std::string& line)
{
    const std::string text = trim(expression);
    if (text == "None") {
        return ConfigValue::null();
    }
    if (text == "True") {
        return ConfigValue::boolean(true);
    }
    if (text == "False") {
        return ConfigValue::boolean(false);
    }
    if (isQuoted(text)) {
        try {
            return ConfigValue::string(parseQuoted(text));
        } catch (const std::invalid_argument& error) {
            throw ParseError(error.what(), lineNum, line);
        }
    }
    if (text.size() >= 2 && text.front() == '[' && text.back() == ']') {
        ConfigList values;
        for (const auto& part : splitListItems(std::string_view{text}.substr(1, text.size() - 2))) {
            values.push_back(parseValue(part, lineNum, line));
        }
        return ConfigValue::list(std::move(values));
    }
    if (text.size() >= 2 && text.front() == '{' && text.back() == '}') {
        ConfigMap values;
        const auto body = trim(std::string_view{text}.substr(1, text.size() - 2));
        if (!body.empty()) {
            for (const auto& part : splitListItems(body)) {
                const auto colon = findTopLevelColon(part);
                if (colon == std::string_view::npos) {
                    throw ParseError("Missing colon in dict literal", lineNum, line);
                }
                ConfigValue keyValue = parseValue(trim(std::string_view{part}.substr(0, colon)), lineNum, line);
                if (keyValue.type() != ConfigValue::Type::String) {
                    throw ParseError("Only string keys are supported in dict literals", lineNum, line);
                }
                auto inserted = values.emplace(keyValue.asString(), parseValue(trim(std::string_view{part}.substr(colon + 1)), lineNum, line));
                if (!inserted.second) {
                    throw ParseError("Duplicate key: " + keyValue.asString(), lineNum, line);
                }
            }
        }
        return ConfigValue::map(std::move(values));
    }
    try {
        std::size_t consumed = 0;
        const long long integer = std::stoll(text, &consumed, 0);
        if (consumed == text.size()) {
            return ConfigValue::integer(integer);
        }
    } catch (const std::exception&) {
    }
    try {
        std::size_t consumed = 0;
        const double floating = std::stod(text, &consumed);
        if (consumed == text.size()) {
            return ConfigValue::floating(floating);
        }
    } catch (const std::exception&) {
    }
    throw ParseError("Unsupported expression '" + text + "' (C++ configfile does not eval arbitrary Python)", lineNum, line);
}

[[nodiscard]] inline ParseResult parseLines(const std::vector<std::string>& lines, std::size_t start)
{
    ConfigMap data;
    int indent = -1;
    std::size_t ln = start;
    while (ln < lines.size()) {
        const auto& line = lines[ln];
        if (!lineIsReal(line)) {
            ++ln;
            continue;
        }
        const int lineIndent = measureIndent(line);
        if (indent < 0) {
            indent = lineIndent;
        }
        if (lineIndent < indent) {
            break;
        }
        if (lineIndent > indent) {
            throw ParseError("Indentation is incorrect. Expected " + std::to_string(indent) + ", got " + std::to_string(lineIndent), ln + 1, line);
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            throw ParseError("Missing colon", ln + 1, line);
        }
        std::string key = trim(std::string_view{line}.substr(0, colon));
        std::string expression = stripInlineComment(std::string_view{line}.substr(colon + 1));
        if (key.empty()) {
            throw ParseError("Missing name preceding colon", ln + 1, line);
        }
        ConfigValue value;
        if (!expression.empty() && lineIsReal(expression)) {
            value = parseValue(expression, ln + 1, line);
            ++ln;
        } else {
            std::size_t nextReal = ln + 1;
            while (nextReal < lines.size() && !lineIsReal(lines[nextReal])) {
                ++nextReal;
            }
            if (nextReal >= lines.size() || measureIndent(lines[nextReal]) <= indent) {
                value = ConfigValue::map({});
                ++ln;
            } else {
                auto nested = parseLines(lines, ln + 1);
                value = ConfigValue::map(std::move(nested.data));
                ln = nested.line;
            }
        }
        if (data.find(key) != data.end()) {
            throw ParseError("Duplicate key: " + key, ln, line);
        }
        data.emplace(std::move(key), std::move(value));
    }
    return {ln, std::move(data)};
}

} // namespace detail

[[nodiscard]] inline std::string genString(const ConfigMap& data, std::string indent = {})
{
    std::string out;
    for (const auto& [key, value] : data) {
        if (key.empty()) {
            throw std::invalid_argument("blank dict keys not allowed");
        }
        if (key.front() == ' ' || key.find(':') != std::string::npos) {
            throw std::invalid_argument("dict keys must not contain ':' or start with spaces [offending key is \"" + key + "\"]");
        }
        if (value.type() == ConfigValue::Type::Map) {
            out += indent + key + ":\n";
            out += genString(value.asMap(), indent + "    ");
        } else {
            out += indent + key + ": " + detail::valueRepr(value) + "\n";
        }
    }
    return out;
}

[[nodiscard]] inline ParseResult parseString(std::string_view lines, std::size_t start = 0)
{
    std::string joined{lines};
    std::size_t continuation = 0;
    while ((continuation = joined.find("\\\n", continuation)) != std::string::npos) {
        joined.erase(continuation, 2);
    }
    return detail::parseLines(detail::splitLines(joined), start);
}

[[nodiscard]] inline ConfigMap readConfigFile(const std::string& fileName)
{
    std::ifstream input{std::filesystem::path{fileName}};
    if (!input.good()) {
        throw std::runtime_error("Error while reading config file " + fileName);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    try {
        return parseString(buffer.str()).data;
    } catch (const ParseError& error) {
        throw ParseError(error.message(), error.lineNum(), error.line(), fileName);
    }
}

inline void writeConfigFile(const ConfigMap& data, const std::string& fileName)
{
    const std::string serialized = genString(data);
    std::ofstream output{std::filesystem::path{fileName}};
    if (!output.good()) {
        throw std::runtime_error("Error while writing config file " + fileName);
    }
    output << serialized;
    if (!output.good()) {
        throw std::runtime_error("Error while writing config file " + fileName);
    }
}

inline void appendConfigFile(const ConfigMap& data, const std::string& fileName)
{
    const std::string serialized = genString(data);
    std::ofstream output{std::filesystem::path{fileName}, std::ios::app};
    if (!output.good()) {
        throw std::runtime_error("Error while appending config file " + fileName);
    }
    output << serialized;
    if (!output.good()) {
        throw std::runtime_error("Error while appending config file " + fileName);
    }
}

} // namespace cppqtgraph::configfile
