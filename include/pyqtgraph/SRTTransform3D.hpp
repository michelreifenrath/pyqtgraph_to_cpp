// Source note: translated/adapted from PyQtGraph pyqtgraph/SRTTransform3D.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include "pyqtgraph/SRTTransform.hpp"
#include "pyqtgraph/Transform3D.hpp"
#include "pyqtgraph/Vector.hpp"

#include <QtGui/qmatrix4x4.h>
#include <QtGui/qtransform.h>
#include <QtGui/qvector3d.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace pyqtgraph {
namespace detail {

inline constexpr double kSrtTransform3DTolerance = 1.0e-6;

inline bool srt3DNearlyEqual(double left, double right, double tolerance = kSrtTransform3DTolerance)
{
    return std::abs(left - right) <= tolerance;
}

inline bool srt3DSameAxis(const QVector3D& left, const QVector3D& right)
{
    return srt3DNearlyEqual(left.x(), right.x()) && srt3DNearlyEqual(left.y(), right.y()) && srt3DNearlyEqual(left.z(), right.z());
}

inline Vector srt3DNormalizedOrZ(const QVector3D& axis)
{
    if (axis.length() == 0.0F) {
        return Vector{0.0, 0.0, 1.0};
    }
    return Vector{axis.normalized()};
}

inline std::array<double, 3> srt3DCross(const std::array<double, 3>& a, const std::array<double, 3>& b)
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}

inline double srt3DDot(const std::array<double, 3>& a, const std::array<double, 3>& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline double srt3DLength(const std::array<double, 3>& values)
{
    return std::sqrt(srt3DDot(values, values));
}

} // namespace detail

class SRTTransform3D : public Transform3D {
public:
    struct Rotation {
        double angle = 0.0;
        Vector axis{0.0, 0.0, 1.0};
    };

    struct State {
        Vector pos{0.0, 0.0, 0.0};
        Vector scale{1.0, 1.0, 1.0};
        double angle = 0.0;
        Vector axis{0.0, 0.0, 1.0};
    };

    SRTTransform3D()
    {
        reset();
    }

    explicit SRTTransform3D(const State& state)
    {
        restoreState(state);
    }

    explicit SRTTransform3D(const SRTTransform& transform)
    {
        reset();
        const SRTTransform::State state = transform.saveState();
        state_ = State{
            Vector{state.pos.x(), state.pos.y(), 0.0},
            Vector{state.scale.x(), state.scale.y(), 1.0},
            state.angle,
            Vector{0.0, 0.0, 1.0},
        };
        update();
    }

    explicit SRTTransform3D(const QTransform& transform)
        : SRTTransform3D(SRTTransform{transform})
    {
    }

    explicit SRTTransform3D(const QMatrix4x4& matrix)
    {
        reset();
        setFromMatrix(matrix);
    }

    [[nodiscard]] Vector getScale() const { return state_.scale; }
    [[nodiscard]] Rotation getRotation() const { return Rotation{state_.angle, state_.axis}; }
    [[nodiscard]] Vector getTranslation() const { return state_.pos; }

    void reset()
    {
        state_ = State{};
        update();
    }

    void translate(double x, double y, double z = 0.0) { setTranslate(state_.pos.x() + x, state_.pos.y() + y, state_.pos.z() + z); }
    void translate(const QVector3D& vector) { translate(vector.x(), vector.y(), vector.z()); }

    void setTranslate(double x, double y, double z = 0.0)
    {
        state_.pos = Vector{x, y, z};
        update();
    }

    void setTranslate(const QVector3D& vector) { setTranslate(vector.x(), vector.y(), vector.z()); }
    void scale(double x, double y) { scale(x, y, 1.0); }
    void scale(double x, double y, double z) { setScale(state_.scale.x() * x, state_.scale.y() * y, state_.scale.z() * z); }
    void scale(const QVector3D& vector) { scale(vector.x(), vector.y(), vector.z()); }
    void setScale(double x, double y) { setScale(x, y, 1.0); }

    void setScale(double x, double y, double z)
    {
        state_.scale = Vector{x, y, z};
        update();
    }

    void setScale(const QVector3D& vector) { setScale(vector.x(), vector.y(), vector.z()); }

    void rotate(double angle, const QVector3D& axis = QVector3D{0.0F, 0.0F, 1.0F})
    {
        if (detail::srt3DSameAxis(axis, state_.axis)) {
            setRotate(state_.angle + angle, state_.axis);
            return;
        }
        QMatrix4x4 matrix;
        matrix.translate(state_.pos.x(), state_.pos.y(), state_.pos.z());
        matrix.rotate(static_cast<float>(state_.angle), state_.axis);
        matrix.rotate(static_cast<float>(angle), axis);
        matrix.scale(state_.scale.x(), state_.scale.y(), state_.scale.z());
        setFromMatrix(matrix);
    }

    void setRotate(double angle, const QVector3D& axis = QVector3D{0.0F, 0.0F, 1.0F})
    {
        state_.angle = angle;
        state_.axis = detail::srt3DNormalizedOrZ(axis);
        update();
    }

    void setFromMatrix(const QMatrix4x4& matrix)
    {
        double m[3][3]{};
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                m[row][column] = matrix(row, column);
            }
        }
        state_.pos = Vector{matrix(0, 3), matrix(1, 3), matrix(2, 3)};

        std::array<double, 3> scale{
            std::sqrt(m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0]),
            std::sqrt(m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1]),
            std::sqrt(m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2]),
        };
        const std::array<double, 3> row0{m[0][0], m[0][1], m[0][2]};
        const std::array<double, 3> row1{m[1][0], m[1][1], m[1][2]};
        const std::array<double, 3> row2{m[2][0], m[2][1], m[2][2]};
        if (detail::srt3DDot(detail::srt3DCross(row0, row1), row2) < 0.0) {
            scale[1] *= -1.0;
        }
        state_.scale = Vector{scale[0], scale[1], scale[2]};

        double r[3][3]{};
        for (int column = 0; column < 3; ++column) {
            if (std::abs(scale[column]) <= detail::kSrtTransform3DTolerance) {
                throw std::invalid_argument("Cannot decompose SRTTransform3D matrix with zero scale");
            }
            for (int row = 0; row < 3; ++row) {
                r[row][column] = m[row][column] / scale[column];
            }
        }

        const double cosAngle = std::clamp((r[0][0] + r[1][1] + r[2][2] - 1.0) * 0.5, -1.0, 1.0);
        const std::array<double, 3> skew{r[2][1] - r[1][2], r[0][2] - r[2][0], r[1][0] - r[0][1]};
        const double sinMagnitude = 0.5 * detail::srt3DLength(skew);
        const double angle = detail::srtRadiansToDegrees(std::atan2(sinMagnitude, cosAngle));
        if (std::abs(angle) <= detail::kSrtTransform3DTolerance) {
            state_.angle = 0.0;
            state_.axis = Vector{0.0, 0.0, 1.0};
            update();
            return;
        }

        if (sinMagnitude > detail::kSrtTransform3DTolerance) {
            state_.axis = Vector{skew[0] / (2.0 * sinMagnitude), skew[1] / (2.0 * sinMagnitude), skew[2] / (2.0 * sinMagnitude)};
        } else {
            state_.axis = detail::srt3DNormalizedOrZ(QVector3D{
                static_cast<float>(std::sqrt(std::max(0.0, (r[0][0] + 1.0) * 0.5))),
                static_cast<float>(std::sqrt(std::max(0.0, (r[1][1] + 1.0) * 0.5))),
                static_cast<float>(std::sqrt(std::max(0.0, (r[2][2] + 1.0) * 0.5))),
            });
        }
        state_.angle = angle;
        update();
    }

    [[nodiscard]] SRTTransform as2D() const
    {
        if (std::abs(state_.angle) > detail::kSrtTransform3DTolerance
            && (!detail::srt3DNearlyEqual(state_.axis.x(), 0.0) || !detail::srt3DNearlyEqual(state_.axis.y(), 0.0)
                || !detail::srt3DNearlyEqual(state_.axis.z(), 1.0))) {
            throw std::invalid_argument("Can only convert 4x4 matrix to 3x3 if rotation is around Z-axis.");
        }
        return SRTTransform{SRTTransform::State{Point{state_.pos.x(), state_.pos.y()}, Point{state_.scale.x(), state_.scale.y()}, state_.angle}};
    }

    [[nodiscard]] State saveState() const { return state_; }

    void restoreState(const State& state)
    {
        state_ = state;
        if (state_.scale.z() == 0.0F) {
            state_.scale.setZ(1.0F);
        }
        if (state_.axis.length() == 0.0F) {
            state_.axis = Vector{0.0, 0.0, 1.0};
        }
        update();
    }

    void update()
    {
        QMatrix4x4::setToIdentity();
        QMatrix4x4::translate(state_.pos.x(), state_.pos.y(), state_.pos.z());
        QMatrix4x4::rotate(static_cast<float>(state_.angle), state_.axis);
        QMatrix4x4::scale(state_.scale.x(), state_.scale.y(), state_.scale.z());
    }

private:
    State state_;
};

} // namespace pyqtgraph
