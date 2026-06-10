// Source note: translated/adapted from PyQtGraph pyqtgraph/Vector.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtGui/qvector3d.h>

#include <initializer_list>
#include <optional>

namespace cppqtgraph {

class Vector : public QVector3D {
public:
    Vector();
    Vector(double x, double y);
    Vector(double x, double y, double z);
    explicit Vector(const QVector3D& vector);
    explicit Vector(const QPointF& point);
    explicit Vector(const QPoint& point);
    explicit Vector(const QSizeF& size);
    explicit Vector(const QSize& size);
    Vector(std::initializer_list<double> values);

    static constexpr qsizetype coordinateCount() noexcept { return 3; }

    [[nodiscard]] float operator[](int index) const;
    float& operator[](int index);
    [[nodiscard]] double at(qsizetype index) const;
    void set(qsizetype index, double value);

    [[nodiscard]] std::optional<double> angle(const QVector3D& other) const;
    [[nodiscard]] Vector abs() const;
    [[nodiscard]] Vector copy() const;
};

[[nodiscard]] Vector abs(const Vector& vector);

} // namespace cppqtgraph
