// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GroupBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/GroupBox.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QWidget>

namespace {

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
        const QRectF bounds = rect().adjusted(1, 1, -1, -1);
        painter.save();
        painter.translate(bounds.center());
        painter.scale(bounds.width() / 2.0, bounds.height() / 2.0);
        painter.drawPath(indicatorPath_);
        painter.restore();
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

namespace pyqtgraph::widgets {

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

} // namespace pyqtgraph::widgets
