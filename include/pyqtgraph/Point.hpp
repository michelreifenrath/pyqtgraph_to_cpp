// Source note: translated/adapted from PyQtGraph pyqtgraph/Point.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qstringview.h>

#include <initializer_list>

namespace pyqtgraph {

class Point : public QPointF {
public:
    Point();
    Point(double x, double y);
    explicit Point(double value);
    explicit Point(const QPointF& point);
    explicit Point(const QPoint& point);
    explicit Point(const QSizeF& size);
    explicit Point(const QSize& size);
    Point(std::initializer_list<double> values);

    static constexpr qsizetype coordinateCount() noexcept { return 2; }

    [[nodiscard]] double at(qsizetype index) const;
    void set(qsizetype index, double value);

    [[nodiscard]] Point operator+(const QPointF& other) const;
    [[nodiscard]] Point operator-(const QPointF& other) const;
    [[nodiscard]] Point operator*(const QPointF& other) const;
    [[nodiscard]] Point operator/(const QPointF& other) const;

    [[nodiscard]] Point operator+(double value) const;
    [[nodiscard]] Point operator-(double value) const;
    [[nodiscard]] Point operator*(double value) const;
    [[nodiscard]] Point operator/(double value) const;
    [[nodiscard]] Point pow(const QPointF& other) const;
    [[nodiscard]] Point pow(double value) const;

    Point& operator+=(const QPointF& other);
    Point& operator-=(const QPointF& other);
    Point& operator*=(const QPointF& other);
    Point& operator/=(const QPointF& other);

    Point& operator+=(double value);
    Point& operator-=(double value);
    Point& operator*=(double value);
    Point& operator/=(double value);

    [[nodiscard]] double length() const;
    [[nodiscard]] Point norm() const;
    [[nodiscard]] double angle(const QPointF& other, QStringView units = QStringView{u"degrees"}) const;
    [[nodiscard]] double dot(const QPointF& other) const;
    [[nodiscard]] double cross(const QPointF& other) const;
    [[nodiscard]] Point proj(const QPointF& other) const;
    [[nodiscard]] double min() const;
    [[nodiscard]] double max() const;
    [[nodiscard]] Point copy() const;
    [[nodiscard]] QPoint toQPoint() const;
};

[[nodiscard]] Point operator+(double value, const Point& point);
[[nodiscard]] Point operator-(double value, const Point& point);
[[nodiscard]] Point operator*(double value, const Point& point);
[[nodiscard]] Point operator/(double value, const Point& point);
[[nodiscard]] Point pow(double value, const Point& point);

} // namespace pyqtgraph
