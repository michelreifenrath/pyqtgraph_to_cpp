#pragma once

// Original implementation; no PyQtGraph source translation.

#include <array>
#include <cstddef>
#include <stdexcept>

namespace pyqtgraph::core {

template <typename T, std::size_t Rank = 1>
class ArrayView {
    static_assert(Rank > 0, "ArrayView rank must be greater than zero");

public:
    using Shape = std::array<std::size_t, Rank>;
    using Strides = std::array<std::ptrdiff_t, Rank>;

    constexpr ArrayView() noexcept = default;

    constexpr ArrayView(T* data, Shape shape) noexcept
        : data_(data)
        , shape_(shape)
        , strides_(contiguousStrides(shape))
    {
    }

    constexpr ArrayView(T* data, Shape shape, Strides strides) noexcept
        : data_(data)
        , shape_(shape)
        , strides_(strides)
    {
    }

    [[nodiscard]] constexpr T* data() const noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        std::size_t product = 1;
        for (std::size_t extent : shape_) {
            product *= extent;
        }
        return product;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]] constexpr const Shape& shape() const noexcept
    {
        return shape_;
    }

    [[nodiscard]] constexpr const Strides& strides() const noexcept
    {
        return strides_;
    }

    constexpr T& operator[](std::size_t index) const noexcept
    {
        static_assert(Rank == 1, "operator[] is only available for rank-1 ArrayView");
        return data_[static_cast<std::ptrdiff_t>(index) * strides_[0]];
    }

    constexpr T& at(const Shape& indices) const noexcept
    {
        return data_[offset(indices)];
    }

    template <typename... Indices>
    constexpr T& operator()(Indices... indices) const noexcept
    {
        static_assert(sizeof...(Indices) == Rank, "operator() index count must match ArrayView rank");
        return at(Shape{static_cast<std::size_t>(indices)...});
    }

    [[nodiscard]] constexpr ArrayView slice(std::size_t axis, std::size_t begin, std::size_t end, std::size_t step = 1) const
    {
        if (axis >= Rank) {
            throw std::out_of_range("ArrayView slice axis is out of range");
        }
        if (step == 0) {
            throw std::invalid_argument("ArrayView slice step must be greater than zero");
        }
        if (begin > end) {
            throw std::out_of_range("ArrayView slice begin must not exceed end");
        }
        if (end > shape_[axis]) {
            throw std::out_of_range("ArrayView slice end is out of range");
        }

        Shape slicedShape = shape_;
        slicedShape[axis] = (end - begin + step - 1) / step;

        Strides slicedStrides = strides_;
        slicedStrides[axis] *= static_cast<std::ptrdiff_t>(step);

        T* slicedData = data_;
        const std::ptrdiff_t dataOffset = static_cast<std::ptrdiff_t>(begin) * strides_[axis];
        if (slicedData != nullptr || dataOffset != 0) {
            slicedData += dataOffset;
        }

        return ArrayView(slicedData, slicedShape, slicedStrides);
    }

private:
    [[nodiscard]] static constexpr Strides contiguousStrides(const Shape& shape) noexcept
    {
        Strides strides{};
        std::ptrdiff_t stride = 1;
        for (std::size_t i = Rank; i > 0; --i) {
            const std::size_t axis = i - 1;
            strides[axis] = stride;
            stride *= static_cast<std::ptrdiff_t>(shape[axis]);
        }
        return strides;
    }

    [[nodiscard]] constexpr std::ptrdiff_t offset(const Shape& indices) const noexcept
    {
        std::ptrdiff_t result = 0;
        for (std::size_t axis = 0; axis < Rank; ++axis) {
            result += static_cast<std::ptrdiff_t>(indices[axis]) * strides_[axis];
        }
        return result;
    }

    T* data_ = nullptr;
    Shape shape_{};
    Strides strides_ = contiguousStrides(shape_);
};

} // namespace pyqtgraph::core
