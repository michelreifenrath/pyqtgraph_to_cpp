// Source note: translated/adapted from PyQtGraph pyqtgraph/Transform3D.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include "cppqtgraph/Vector.hpp"

#include <QtCore/qpoint.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qvector3d.h>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph {
namespace detail {

inline void assignTransform3DRowMajor(QMatrix4x4& matrix, const double* values, qsizetype count)
{
    if (count != 16) {
        throw std::invalid_argument("Single argument to Transform3D must have 16 elements.");
    }
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            matrix(row, column) = static_cast<float>(values[row * 4 + column]);
        }
    }
}

} // namespace detail

class Transform3D : public QMatrix4x4 {
public:
    Transform3D() = default;

    explicit Transform3D(const QMatrix4x4& matrix)
        : QMatrix4x4(matrix)
    {
    }

    explicit Transform3D(const std::array<double, 16>& values)
    {
        detail::assignTransform3DRowMajor(*this, values.data(), static_cast<qsizetype>(values.size()));
    }

    explicit Transform3D(const std::vector<double>& values)
    {
        detail::assignTransform3DRowMajor(*this, values.data(), static_cast<qsizetype>(values.size()));
    }

    Transform3D(std::initializer_list<double> values)
    {
        detail::assignTransform3DRowMajor(*this, values.begin(), static_cast<qsizetype>(values.size()));
    }

    using QMatrix4x4::map;

    [[nodiscard]] std::array<double, 16> matrix3D() const
    {
        std::array<float, 16> copied{};
        copyDataTo(copied.data());
        std::array<double, 16> result{};
        for (std::size_t index = 0; index < result.size(); ++index) {
            result[index] = copied[index];
        }
        return result;
    }

    [[nodiscard]] std::array<double, 9> matrix2D() const
    {
        const std::array<double, 16> matrix = matrix3D();
        return {
            matrix[0], matrix[1], matrix[3],
            matrix[4], matrix[5], matrix[7],
            matrix[12], matrix[13], matrix[15],
        };
    }

    [[nodiscard]] std::vector<double> matrix(int nd = 3) const
    {
        if (nd == 3) {
            const std::array<double, 16> values = matrix3D();
            return {values.begin(), values.end()};
        }
        if (nd == 2) {
            const std::array<double, 9> values = matrix2D();
            return {values.begin(), values.end()};
        }
        throw std::invalid_argument("Argument 'nd' must be 2 or 3");
    }

    [[nodiscard]] Vector map(const Vector& vector) const
    {
        return Vector{QMatrix4x4::map(static_cast<const QVector3D&>(vector))};
    }

    [[nodiscard]] QVector3D map(const QVector3D& vector) const
    {
        return QMatrix4x4::map(vector);
    }

    [[nodiscard]] QPointF map(const QPointF& point) const
    {
        return QMatrix4x4::map(point);
    }

    [[nodiscard]] QPoint map(const QPoint& point) const
    {
        return QMatrix4x4::map(point);
    }

    [[nodiscard]] std::vector<double> map(const std::vector<double>& coordinates) const
    {
        if (coordinates.size() != 2 && coordinates.size() != 3) {
            throw std::invalid_argument("Transform3D::map vector coordinates must contain two or three values");
        }
        const QVector3D mapped = QMatrix4x4::map(QVector3D{
            static_cast<float>(coordinates[0]),
            static_cast<float>(coordinates[1]),
            coordinates.size() == 3 ? static_cast<float>(coordinates[2]) : 0.0F,
        });
        if (coordinates.size() == 2) {
            return {mapped.x(), mapped.y()};
        }
        return {mapped.x(), mapped.y(), mapped.z()};
    }

    template <std::size_t N>
    [[nodiscard]] std::array<double, N> map(const std::array<double, N>& coordinates) const
    {
        static_assert(N == 2 || N == 3, "Transform3D::map array coordinates must contain two or three values");
        const QVector3D mapped = QMatrix4x4::map(QVector3D{
            static_cast<float>(coordinates[0]),
            static_cast<float>(coordinates[1]),
            N == 3 ? static_cast<float>(coordinates[2]) : 0.0F,
        });
        if constexpr (N == 2) {
            return {mapped.x(), mapped.y()};
        } else {
            return {mapped.x(), mapped.y(), mapped.z()};
        }
    }

    [[nodiscard]] std::pair<Transform3D, bool> inverted() const
    {
        bool invertible = false;
        const QMatrix4x4 inverse = QMatrix4x4::inverted(&invertible);
        return {Transform3D{inverse}, invertible};
    }
};

} // namespace cppqtgraph
