// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp"

#include <QtCore/QCoreApplication>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>

namespace cppqtgraph::graphicsItems {

ViewBoxMenu::ViewBoxMenu(ViewBox* view, QWidget* parent)
    : QMenu(parent)
    , view_(view)
{
    setTitle(QCoreApplication::translate("ViewBox", "ViewBox options"));

    viewAllAction_ = addAction(QCoreApplication::translate("ViewBox", "View All"));
    connect(viewAllAction_, &QAction::triggered, this, &ViewBoxMenu::onViewAll);

    addSeparator();

    xAutoRangeAction_ = addAction(QCoreApplication::translate("ViewBox", "X Auto Range"));
    xAutoRangeAction_->setCheckable(true);
    connect(xAutoRangeAction_, &QAction::toggled, this, &ViewBoxMenu::onXAutoRangeToggled);

    yAutoRangeAction_ = addAction(QCoreApplication::translate("ViewBox", "Y Auto Range"));
    yAutoRangeAction_->setCheckable(true);
    connect(yAutoRangeAction_, &QAction::toggled, this, &ViewBoxMenu::onYAutoRangeToggled);

    QMenu* mouseModeMenu = addMenu(QCoreApplication::translate("ViewBox", "Mouse Mode"));
    mouseModeGroup_ = new QActionGroup(this);
    panModeAction_ = mouseModeMenu->addAction(QCoreApplication::translate("ViewBox", "3 button"));
    rectModeAction_ = mouseModeMenu->addAction(QCoreApplication::translate("ViewBox", "1 button"));
    panModeAction_->setCheckable(true);
    rectModeAction_->setCheckable(true);
    mouseModeGroup_->addAction(panModeAction_);
    mouseModeGroup_->addAction(rectModeAction_);
    connect(mouseModeGroup_, &QActionGroup::triggered, this, &ViewBoxMenu::onMouseModeTriggered);

    if (view_ != nullptr) {
        connect(view_, &ViewBox::sigStateChanged, this, &ViewBoxMenu::updateState);
        updateState();
    }
}

ViewBoxMenu::~ViewBoxMenu() = default;

ViewBox* ViewBoxMenu::view() const noexcept
{
    return view_;
}

void ViewBoxMenu::updateState()
{
    if (view_ == nullptr) {
        return;
    }

    const auto autoRange = view_->autoRangeEnabled();
    if (xAutoRangeAction_ != nullptr) {
        xAutoRangeAction_->blockSignals(true);
        xAutoRangeAction_->setChecked(autoRange[ViewBox::XAxis]);
        xAutoRangeAction_->blockSignals(false);
    }
    if (yAutoRangeAction_ != nullptr) {
        yAutoRangeAction_->blockSignals(true);
        yAutoRangeAction_->setChecked(autoRange[ViewBox::YAxis]);
        yAutoRangeAction_->blockSignals(false);
    }

    if (panModeAction_ != nullptr && rectModeAction_ != nullptr) {
        panModeAction_->blockSignals(true);
        rectModeAction_->blockSignals(true);
        if (view_->mouseMode() == ViewBox::PanMode) {
            panModeAction_->setChecked(true);
        } else {
            rectModeAction_->setChecked(true);
        }
        panModeAction_->blockSignals(false);
        rectModeAction_->blockSignals(false);
    }
}

void ViewBoxMenu::onViewAll()
{
    if (view_ != nullptr) {
        view_->autoRange();
        updateState();
    }
}

void ViewBoxMenu::onXAutoRangeToggled(bool checked)
{
    if (view_ != nullptr) {
        view_->enableAutoRange(ViewBox::XAxis, checked);
        updateState();
    }
}

void ViewBoxMenu::onYAutoRangeToggled(bool checked)
{
    if (view_ != nullptr) {
        view_->enableAutoRange(ViewBox::YAxis, checked);
        updateState();
    }
}

void ViewBoxMenu::onMouseModeTriggered(QAction* action)
{
    if (view_ == nullptr || action == nullptr) {
        return;
    }
    if (action == panModeAction_) {
        view_->setMouseMode(ViewBox::PanMode);
    } else if (action == rectModeAction_) {
        view_->setMouseMode(ViewBox::RectMode);
    }
    updateState();
}

} // namespace cppqtgraph::graphicsItems
