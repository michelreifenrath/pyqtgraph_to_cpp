// Source note: translated/adapted from PyQtGraph pyqtgraph/PlotData.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include <cstddef>
#include <initializer_list>
#if __has_include(<span>)
#include <span>
#endif
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if !defined(__cpp_lib_span)
namespace std {
template <typename T>
class span {
public:
    using element_type = T;
    using pointer = T*;
    using iterator = pointer;

    constexpr span() noexcept = default;
    constexpr span(pointer data, std::size_t size) noexcept
        : data_(data)
        , size_(size)
    {
    }

    [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
    [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }
    [[nodiscard]] constexpr pointer data() const noexcept { return data_; }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

private:
    pointer data_ = nullptr;
    std::size_t size_ = 0;
};
} // namespace std
#endif

namespace pyqtgraph {

class PlotData {
public:
    using Values = std::vector<double>;

    // C++ port restriction: upstream PlotData stores arbitrary Python values and
    // uses None for fields added without data. This native value model stores
    // owning one-dimensional double arrays; default-added fields are empty.
    void addFields(std::initializer_list<std::string> fields);

    [[nodiscard]] bool hasField(std::string_view field) const;

    [[nodiscard]] const Values& operator[](std::string_view field) const;
    [[nodiscard]] Values& operator[](std::string_view field);

    void set(std::string field, std::span<const double> values);
    void set(std::string field, const Values& values);
    void set(std::string field, std::initializer_list<double> values);

    [[nodiscard]] double min(std::string_view field) const;
    [[nodiscard]] double max(std::string_view field) const;

private:
    [[nodiscard]] const Values& valuesFor(std::string_view field) const;
    [[nodiscard]] Values& valuesFor(std::string_view field);

    std::unordered_map<std::string, Values> fields_;
    mutable std::unordered_map<std::string, double> minValues_;
    mutable std::unordered_map<std::string, double> maxValues_;
};

} // namespace pyqtgraph
