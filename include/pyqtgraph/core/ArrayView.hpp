#pragma once

// Original implementation; no PyQtGraph source translation.

#include <array>
#include <cstddef>

namespace pyqtgraph::core {

template <typename T>
class ArrayView {
public:
    constexpr ArrayView() noexcept = default;

    constexpr ArrayView(T* data, std::array<std::size_t, 1> shape) noexcept
        : data_(data)
        , shape_(shape)
    {
    }

    [[nodiscard]] constexpr T* data() const noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return shape_[0];
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]] constexpr const std::array<std::size_t, 1>& shape() const noexcept
    {
        return shape_;
    }

    constexpr T& operator[](std::size_t index) const noexcept
    {
        return data_[index];
    }

private:
    T* data_ = nullptr;
    std::array<std::size_t, 1> shape_{0};
};

} // namespace pyqtgraph::core
