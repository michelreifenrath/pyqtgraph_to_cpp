// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/VerticalLabel.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/VerticalLabel.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

namespace cppqtgraph::widgets {

VerticalLabel::VerticalLabel(const QString& text, const QString& orientation, bool forceWidth)
    : QLabel(text)
    , forceWidth_(forceWidth)
{
    setOrientation(orientation);
}

void VerticalLabel::setOrientation(const QString& orientation)
{
    if (orientation_ == orientation) {
        return;
    }
    orientation_ = orientation;
    update();
    updateGeometry();
}

QSize VerticalLabel::sizeHint() const
{
    if (orientation_ == QStringLiteral("vertical")) {
        if (hasTextHint_) {
            return QSize(textHint_.height(), textHint_.width());
        }
        return QSize(19, 50);
    }
    if (hasTextHint_) {
        return QSize(textHint_.width(), textHint_.height());
    }
    return QSize(50, 19);
}

void VerticalLabel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QRect region;
    if (orientation_ == QStringLiteral("vertical")) {
        painter.rotate(-90.0);
        region = QRect(-height(), 0, height(), width());
    } else {
        region = contentsRect();
    }

    const Qt::Alignment textAlignment = QLabel::alignment();
    textHint_ = painter.boundingRect(region, static_cast<int>(textAlignment), text());
    painter.drawText(region, static_cast<int>(textAlignment), text());
    hasTextHint_ = true;
    painter.end();

    if (orientation_ == QStringLiteral("vertical")) {
        setMaximumWidth(textHint_.height());
        setMinimumWidth(0);
        setMaximumHeight(16777215);
        if (forceWidth_) {
            setMinimumHeight(textHint_.width());
        } else {
            setMinimumHeight(0);
        }
    } else {
        setMaximumHeight(textHint_.height());
        setMinimumHeight(0);
        setMaximumWidth(16777215);
        if (forceWidth_) {
            setMinimumWidth(textHint_.width());
        } else {
            setMinimumWidth(0);
        }
    }
}

} // namespace cppqtgraph::widgets
