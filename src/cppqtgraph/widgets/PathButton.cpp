// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PathButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/PathButton.hpp"

#include <cppqtgraph/functions.hpp>

#include <QtCore/QRectF>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <algorithm>

namespace cppqtgraph::widgets {

PathButton::PathButton(QWidget* parent)
    : QPushButton(parent)
    , pen_(cppqtgraph::mkPen(QStringLiteral("k")))
{
    setFixedSize(30, 30);
}

PathButton::PathButton(QWidget* parent, const QPainterPath& path, int width, int height, int margin)
    : PathButton(parent)
{
    margin_ = margin;
    setFixedSize(width, height);
    setPath(path);
}

void PathButton::setMargin(int margin)
{
    margin_ = margin;
    update();
}

void PathButton::setPath(const QPainterPath& path)
{
    path_ = path;
    update();
}

void PathButton::setPen(const QPen& pen)
{
    pen_ = pen;
    update();
}

void PathButton::setPen(const QString& color)
{
    QString resolved = color;
    if (resolved == QStringLiteral("default")) {
        resolved = QStringLiteral("k");
    }
    setPen(cppqtgraph::mkPen(resolved));
}

void PathButton::setBrush(const QBrush& brush)
{
    brush_ = brush;
    update();
}

void PathButton::setBrush(std::nullptr_t)
{
    setBrush(cppqtgraph::mkBrush(nullptr));
}

void PathButton::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event);

    if (path_.isEmpty()) {
        return;
    }

    const QRectF pathBounds = path_.boundingRect();
    if (pathBounds.width() <= 0.0 || pathBounds.height() <= 0.0) {
        return;
    }

    const QRectF geom = QRectF(0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height()))
                            .adjusted(static_cast<qreal>(margin_),
                                static_cast<qreal>(margin_),
                                static_cast<qreal>(-margin_),
                                static_cast<qreal>(-margin_));
    const qreal scale = std::min(geom.width() / pathBounds.width(), geom.height() / pathBounds.height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(geom.center());
    painter.scale(scale, scale);
    painter.translate(-pathBounds.center());
    painter.setPen(pen_);
    painter.setBrush(brush_);
    painter.drawPath(path_);
}

} // namespace cppqtgraph::widgets
