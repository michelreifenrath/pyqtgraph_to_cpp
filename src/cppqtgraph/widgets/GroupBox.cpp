// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GroupBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/GroupBox.hpp"

#include <QtCore/QRectF>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QWidget>

#include <algorithm>

namespace {

void paintPathButtonIndicator(QPainter& painter, const QRectF& geom, const QPainterPath& path)
{
    if (path.isEmpty()) {
        return;
    }
    const QRectF pathBounds = path.boundingRect();
    if (pathBounds.width() <= 0.0 || pathBounds.height() <= 0.0) {
        return;
    }
    const qreal scale = std::min(geom.width() / pathBounds.width(), geom.height() / pathBounds.height());
    painter.save();
    painter.translate(geom.center());
    painter.scale(scale, scale);
    painter.translate(-pathBounds.center());
    painter.drawPath(path);
    painter.restore();
}

class CollapseHandle : public QPushButton {
public:
    explicit CollapseHandle(QWidget* parent = nullptr)
        : QPushButton(parent)
    {
        setFixedSize(12, 12);
        setFlat(true);
        setStyleSheet(QStringLiteral("border: none;"));
    }

    void setIndicatorPath(const QPainterPath& path)
    {
        indicatorPath_ = path;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPushButton::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        const QRectF geom(0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height()));
        paintPathButtonIndicator(painter, geom, indicatorPath_);
    }

private:
    QPainterPath indicatorPath_;
};

QPainterPath makeOpenIndicatorPath()
{
    QPainterPath path;
    path.moveTo(-1, 0);
    path.lineTo(1, 0);
    path.lineTo(0, 1);
    path.lineTo(-1, 0);
    return path;
}

QPainterPath makeCloseIndicatorPath()
{
    QPainterPath path;
    path.moveTo(0, -1);
    path.lineTo(0, 1);
    path.lineTo(1, 0);
    path.lineTo(0, -1);
    return path;
}

} // namespace

namespace cppqtgraph::widgets {

GroupBox::GroupBox(QWidget* parent)
    : QGroupBox(parent)
    , lastSizePolicy_(sizePolicy())
{
    initializeCollapseHandle();
}

GroupBox::GroupBox(const QString& title, QWidget* parent)
    : QGroupBox(parent)
    , lastSizePolicy_(sizePolicy())
{
    initializeCollapseHandle();
    setTitle(title);
}

void GroupBox::initializeCollapseHandle()
{
    auto* handle = new CollapseHandle(this);
    handle->setIndicatorPath(makeOpenIndicatorPath());
    handle->move(3, 3);
    collapseHandle_ = handle;
    QObject::connect(handle, &QPushButton::clicked, this, &GroupBox::toggleCollapsed);
}

void GroupBox::toggleCollapsed()
{
    setCollapsed(!collapsed_);
}

void GroupBox::setCollapsed(bool collapsed)
{
    if (collapsed == collapsed_) {
        return;
    }

    auto* handle = static_cast<CollapseHandle*>(collapseHandle_);
    if (collapsed) {
        handle->setIndicatorPath(makeCloseIndicatorPath());
        setClosingSizePolicy();
    } else {
        handle->setIndicatorPath(makeOpenIndicatorPath());
        QGroupBox::setSizePolicy(lastSizePolicy_);
    }

    applyChildVisibility(collapsed);
    collapsed_ = collapsed;
    emit sigCollapseChanged(collapsed_);
}

void GroupBox::applyChildVisibility(bool collapsed)
{
    const auto children = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        if (child != collapseHandle_) {
            child->setVisible(!collapsed);
        }
    }
    if (collapseHandle_ != nullptr) {
        collapseHandle_->setVisible(true);
        collapseHandle_->raise();
    }
}

void GroupBox::setTitle(const QString& title)
{
    QGroupBox::setTitle(QStringLiteral("   ") + title);
}

void GroupBox::setSizePolicy(QSizePolicy policy)
{
    QGroupBox::setSizePolicy(policy);
    lastSizePolicy_ = sizePolicy();
}

void GroupBox::setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical)
{
    QGroupBox::setSizePolicy(horizontal, vertical);
    lastSizePolicy_ = sizePolicy();
}

void GroupBox::setClosingSizePolicy()
{
    QGroupBox::setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    lastSizePolicy_ = sizePolicy();
}

} // namespace cppqtgraph::widgets
