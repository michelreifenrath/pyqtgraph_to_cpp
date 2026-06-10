// Source note: translated/adapted from PyQtGraph pyqtgraph/Point.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/Point.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cppqtgraph {
namespace {

constexpr double radiansToDegrees(double radians)
{
    return radians * 180.0 / std::acos(-1.0);
}

} // namespace

Point::Point() = default;

Point::Point(double x, double y)
    : QPointF(x, y)
{
}

Point::Point(double value)
    : QPointF(value, value)
{
}

Point::Point(const QPointF& point)
    : QPointF(point)
{
}

Point::Point(const QPoint& point)
    : QPointF(point)
{
}

Point::Point(const QSizeF& size)
    : QPointF(size.width(), size.height())
{
}

Point::Point(const QSize& size)
    : QPointF(size.width(), size.height())
{
}

Point::Point(std::initializer_list<double> values)
{
    auto it = values.begin();
    if (values.size() == 1) {
        setX(*it);
        setY(*it);
        return;
    }

    if (values.size() != coordinateCount()) {
        throw std::invalid_argument("Point initializer list must contain one or two values");
    }

    setX(*it);
    ++it;
    setY(*it);
}

double Point::at(qsizetype index) const
{
    switch (index) {
    case 0:
        return x();
    case 1:
        return y();
    default:
        throw std::out_of_range("Point coordinate index out of range");
    }
}

void Point::set(qsizetype index, double value)
{
    switch (index) {
    case 0:
        setX(value);
        return;
    case 1:
        setY(value);
        return;
    default:
        throw std::out_of_range("Point coordinate index out of range");
    }
}

Point Point::operator+(const QPointF& other) const
{
    return Point{x() + other.x(), y() + other.y()};
}

Point Point::operator-(const QPointF& other) const
{
    return Point{x() - other.x(), y() - other.y()};
}

Point Point::operator-() const
{
    return Point{-x(), -y()};
}

Point Point::operator*(const QPointF& other) const
{
    return Point{x() * other.x(), y() * other.y()};
}

Point Point::operator/(const QPointF& other) const
{
    return Point{x() / other.x(), y() / other.y()};
}

Point Point::operator+(double value) const
{
    return *this + Point(value);
}

Point Point::operator-(double value) const
{
    return *this - Point(value);
}

Point Point::operator*(double value) const
{
    return *this * Point(value);
}

Point Point::operator/(double value) const
{
    return *this / Point(value);
}

Point& Point::operator+=(const QPointF& other)
{
    setX(x() + other.x());
    setY(y() + other.y());
    return *this;
}

Point& Point::operator-=(const QPointF& other)
{
    setX(x() - other.x());
    setY(y() - other.y());
    return *this;
}

Point& Point::operator*=(const QPointF& other)
{
    setX(x() * other.x());
    setY(y() * other.y());
    return *this;
}

Point& Point::operator/=(const QPointF& other)
{
    setX(x() / other.x());
    setY(y() / other.y());
    return *this;
}

Point& Point::operator+=(double value)
{
    return *this += Point(value);
}

Point& Point::operator-=(double value)
{
    return *this -= Point(value);
}

Point& Point::operator*=(double value)
{
    return *this *= Point(value);
}

Point& Point::operator/=(double value)
{
    return *this /= Point(value);
}

double Point::length() const
{
    return std::hypot(x(), y());
}

Point Point::norm() const
{
    return *this / length();
}

double Point::angle(const QPointF& other, QStringView units) const
{
    const double radians = std::atan2(y(), x()) - std::atan2(other.y(), other.x());
    if (units == QStringView{u"radians"}) {
        return radians;
    }
    return radiansToDegrees(radians);
}

double Point::dot(const QPointF& other) const
{
    return x() * other.x() + y() * other.y();
}

double Point::cross(const QPointF& other) const
{
    return x() * other.y() - y() * other.x();
}

Point Point::proj(const QPointF& other) const
{
    const Point basis = Point{other}.norm();
    return basis * dot(basis);
}

double Point::min() const
{
    return std::min(x(), y());
}

double Point::max() const
{
    return std::max(x(), y());
}

Point Point::copy() const
{
    return Point{*this};
}

QPoint Point::toQPoint() const
{
    return toPoint();
}

Point operator+(double value, const Point& point)
{
    return Point(value) + point;
}

Point operator-(double value, const Point& point)
{
    return Point(value) - point;
}

Point operator*(double value, const Point& point)
{
    return Point(value) * point;
}

Point operator/(double value, const Point& point)
{
    return Point(value) / point;
}

} // namespace cppqtgraph
