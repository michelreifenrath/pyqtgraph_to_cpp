// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/JoystickButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/JoystickButton.hpp"

#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>

#include <cmath>

namespace cppqtgraph::widgets {

JoystickButton::JoystickButton(QWidget* parent)
    : QPushButton(parent)
{
    setCheckable(true);
    setFixedWidth(50);
    setFixedHeight(50);
    setState(0.0, 0.0);
}

QVector<double> JoystickButton::getState() const
{
    return state_;
}

void JoystickButton::setState(double x, double y)
{
    const double length = std::hypot(x, y);
    double normalizedX = 0.0;
    double normalizedY = 0.0;
    if (x != 0.0) {
        normalizedX = x / length;
    }
    if (y != 0.0) {
        normalizedY = y / length;
    }

    double clampedLength = length;
    if (clampedLength > radius_) {
        clampedLength = radius_;
    }
    const double scaledLength = std::pow(clampedLength / radius_, 2.0);
    const double stateX = normalizedX * scaledLength;
    const double stateY = normalizedY * scaledLength;

    const double halfWidth = static_cast<double>(width()) / 2.0;
    const double halfHeight = static_cast<double>(height()) / 2.0;
    spotPos_ = QPoint(static_cast<int>(halfWidth * (1.0 + stateX)), static_cast<int>(halfHeight * (1.0 - stateY)));
    update();

    if (state_.size() == 2 && state_[0] == stateX && state_[1] == stateY) {
        return;
    }
    state_ = {stateX, stateY};
    emit sigStateChanged(this, state_);
}

void JoystickButton::mousePressEvent(QMouseEvent* event)
{
    setChecked(true);
    pressPos_ = event->position();
    event->accept();
}

void JoystickButton::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF delta = event->position() - pressPos_;
    setState(delta.x(), -delta.y());
}

void JoystickButton::mouseReleaseEvent(QMouseEvent* /*event*/)
{
    setChecked(false);
    setState(0.0, 0.0);
}

void JoystickButton::wheelEvent(QWheelEvent* event)
{
    event->accept();
}

void JoystickButton::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
}

void JoystickButton::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setBrush(QBrush(QColor(0, 0, 0)));
    painter.drawEllipse(spotPos_.x() - 3, spotPos_.y() - 3, 6, 6);
}

void JoystickButton::resizeEvent(QResizeEvent* event)
{
    if (state_.size() == 2) {
        setState(state_[0], state_[1]);
    }
    QPushButton::resizeEvent(event);
}

} // namespace cppqtgraph::widgets
