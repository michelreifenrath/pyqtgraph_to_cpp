// Source note: translated/adapted from PyQtGraph pyqtgraph/Vector.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/Vector.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pyqtgraph {
namespace {

constexpr float toFloat(double value)
{
    return static_cast<float>(value);
}

constexpr double radiansToDegrees(double radians)
{
    return radians * 180.0 / std::acos(-1.0);
}

int checkedCoordinateIndex(qsizetype index)
{
    switch (index) {
    case 0:
    case 1:
    case 2:
        return static_cast<int>(index);
    default:
        throw std::out_of_range("Vector coordinate index out of range");
    }
}

} // namespace

Vector::Vector() = default;

Vector::Vector(double x, double y)
    : QVector3D(toFloat(x), toFloat(y), 0.0F)
{
}

Vector::Vector(double x, double y, double z)
    : QVector3D(toFloat(x), toFloat(y), toFloat(z))
{
}

Vector::Vector(const QVector3D& vector)
    : QVector3D(vector)
{
}

Vector::Vector(const QPointF& point)
    : QVector3D(toFloat(point.x()), toFloat(point.y()), 0.0F)
{
}

Vector::Vector(const QPoint& point)
    : QVector3D(point.x(), point.y(), 0.0F)
{
}

Vector::Vector(const QSizeF& size)
    : QVector3D(toFloat(size.width()), toFloat(size.height()), 0.0F)
{
}

Vector::Vector(const QSize& size)
    : QVector3D(size.width(), size.height(), 0.0F)
{
}

Vector::Vector(std::initializer_list<double> values)
{
    auto it = values.begin();
    if (values.size() != 2 && values.size() != coordinateCount()) {
        throw std::invalid_argument("Vector initializer list must contain two or three values");
    }

    setX(toFloat(*it));
    ++it;
    setY(toFloat(*it));
    ++it;
    setZ(values.size() == coordinateCount() ? toFloat(*it) : 0.0F);
}

float Vector::operator[](int index) const
{
    return QVector3D::operator[](checkedCoordinateIndex(index));
}

float& Vector::operator[](int index)
{
    return QVector3D::operator[](checkedCoordinateIndex(index));
}

double Vector::at(qsizetype index) const
{
    return QVector3D::operator[](checkedCoordinateIndex(index));
}

void Vector::set(qsizetype index, double value)
{
    QVector3D::operator[](checkedCoordinateIndex(index)) = toFloat(value);
}

std::optional<double> Vector::angle(const QVector3D& other) const
{
    const double n1 = length();
    const double n2 = other.length();
    if (n1 == 0.0 || n2 == 0.0) {
        return std::nullopt;
    }

    const double ratio = QVector3D::dotProduct(*this, other) / (n1 * n2);
    const double radians = std::acos(std::clamp(ratio, -1.0, 1.0));
    return radiansToDegrees(radians);
}

Vector Vector::abs() const
{
    return Vector{
        std::abs(static_cast<double>(x())),
        std::abs(static_cast<double>(y())),
        std::abs(static_cast<double>(z())),
    };
}

Vector Vector::copy() const
{
    return Vector{*this};
}

Vector abs(const Vector& vector)
{
    return vector.abs();
}

} // namespace pyqtgraph
