// Source note: translated/adapted from PyQtGraph pyqtgraph/SRTTransform.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include "cppqtgraph/Point.hpp"

#include <QtCore/qstringview.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qtransform.h>

#include <array>
#include <cmath>
#include <stdexcept>

namespace cppqtgraph {
namespace detail {

inline constexpr double kSrtTransformTolerance = 1.0e-6;

inline double srtRadiansToDegrees(double radians)
{
    return radians * 180.0 / std::acos(-1.0);
}

} // namespace detail

class SRTTransform : public QTransform {
public:
    struct State {
        Point pos{0.0, 0.0};
        Point scale{1.0, 1.0};
        double angle = 0.0;
    };

    SRTTransform()
    {
        reset();
    }

    explicit SRTTransform(const State& state)
    {
        restoreState(state);
    }

    explicit SRTTransform(const QTransform& transform)
    {
        reset();
        setFromQTransform(transform);
    }

    explicit SRTTransform(const QMatrix4x4& matrix)
    {
        reset();
        setFromMatrix4x4(matrix);
    }

    using QTransform::map;

    [[nodiscard]] Point getScale() const { return state_.scale; }
    [[nodiscard]] double getRotation() const { return state_.angle; }
    [[nodiscard]] Point getTranslation() const { return state_.pos; }

    void reset()
    {
        state_ = State{};
        update();
    }

    void setFromQTransform(const QTransform& transform)
    {
        const Point p1{transform.map(QPointF{0.0, 0.0})};
        const Point p2{transform.map(QPointF{1.0, 0.0})};
        const Point p3{transform.map(QPointF{0.0, 1.0})};
        const Point dp2 = p2 - p1;
        const Point dp3 = p3 - p1;
        const double determinant = dp2.x() * dp3.y() - dp2.y() * dp3.x();
        const double sy = determinant < 0.0 ? -1.0 : 1.0;
        state_ = State{
            p1,
            Point{dp2.length(), dp3.length() * sy},
            detail::srtRadiansToDegrees(std::atan2(dp2.y(), dp2.x())),
        };
        update();
    }

    void setFromMatrix4x4(const QMatrix4x4& matrix)
    {
        const double zCoupling = std::abs(matrix(0, 2)) + std::abs(matrix(1, 2)) + std::abs(matrix(2, 0)) + std::abs(matrix(2, 1));
        if (zCoupling > detail::kSrtTransformTolerance) {
            throw std::invalid_argument("Can only convert 4x4 matrix to 3x3 if rotation is around Z-axis.");
        }

        const double sx = std::sqrt(
            static_cast<double>(matrix(0, 0)) * matrix(0, 0) + static_cast<double>(matrix(1, 0)) * matrix(1, 0));
        double sy = std::sqrt(
            static_cast<double>(matrix(0, 1)) * matrix(0, 1) + static_cast<double>(matrix(1, 1)) * matrix(1, 1));
        const double determinant = static_cast<double>(matrix(0, 0)) * matrix(1, 1) - static_cast<double>(matrix(0, 1)) * matrix(1, 0);
        if (determinant < 0.0) {
            sy *= -1.0;
        }
        state_ = State{
            Point{matrix(0, 3), matrix(1, 3)},
            Point{sx, sy},
            detail::srtRadiansToDegrees(std::atan2(matrix(1, 0), matrix(0, 0))),
        };
        update();
    }

    void translate(double x, double y) { setTranslate(state_.pos + QPointF{x, y}); }
    void translate(const QPointF& point) { setTranslate(state_.pos + point); }
    void setTranslate(double x, double y) { setTranslate(QPointF{x, y}); }

    void setTranslate(const QPointF& point)
    {
        state_.pos = Point{point};
        update();
    }

    void scale(double x, double y) { setScale(state_.scale * QPointF{x, y}); }
    void scale(const QPointF& point) { setScale(state_.scale * point); }
    void setScale(double x, double y) { setScale(QPointF{x, y}); }

    void setScale(const QPointF& point)
    {
        state_.scale = Point{point};
        update();
    }

    void rotate(double angle) { setRotate(state_.angle + angle); }

    void setRotate(double angle)
    {
        state_.angle = angle;
        update();
    }

    [[nodiscard]] State saveState() const { return state_; }

    void restoreState(const State& state)
    {
        state_ = state;
        update();
    }

    void update()
    {
        QTransform::reset();
        QTransform::translate(state_.pos.x(), state_.pos.y());
        QTransform::rotate(state_.angle);
        QTransform::scale(state_.scale.x(), state_.scale.y());
    }

    [[nodiscard]] std::array<double, 9> matrix() const
    {
        return {
            m11(), m12(), m13(),
            m21(), m22(), m23(),
            m31(), m32(), m33(),
        };
    }

    [[nodiscard]] SRTTransform dividedBy(const SRTTransform& transform) const
    {
        bool invertible = false;
        const QTransform inverse = transform.inverted(&invertible);
        if (!invertible) {
            throw std::invalid_argument("Cannot divide by a non-invertible SRTTransform");
        }
        return SRTTransform{inverse * static_cast<const QTransform&>(*this)};
    }

    [[nodiscard]] SRTTransform operator*(const SRTTransform& transform) const
    {
        return SRTTransform{static_cast<const QTransform&>(*this) * static_cast<const QTransform&>(transform)};
    }

private:
    State state_;
};

} // namespace cppqtgraph
