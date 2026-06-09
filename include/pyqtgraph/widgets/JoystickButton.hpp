#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/JoystickButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QPoint>
#include <QtCore/QPointF>
#include <QtCore/QVector>
#include <QtWidgets/QPushButton>

class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;

namespace pyqtgraph::widgets {

class JoystickButton : public QPushButton {
    Q_OBJECT

public:
    explicit JoystickButton(QWidget* parent = nullptr);

    JoystickButton(const JoystickButton&) = delete;
    JoystickButton& operator=(const JoystickButton&) = delete;
    JoystickButton(JoystickButton&&) = delete;
    JoystickButton& operator=(JoystickButton&&) = delete;

    [[nodiscard]] QVector<double> getState() const;
    void setState(double x, double y);

    [[nodiscard]] QPoint spotPosition() const { return spotPos_; }
    [[nodiscard]] double radius() const { return radius_; }

signals:
    void sigStateChanged(JoystickButton* button, const QVector<double>& state);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    double radius_ = 200.0;
    QVector<double> state_ = {0.0, 0.0};
    QPointF pressPos_;
    QPoint spotPos_;
};

} // namespace pyqtgraph::widgets
