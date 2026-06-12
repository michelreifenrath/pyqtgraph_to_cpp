// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp"

#include <QtCore/QCoreApplication>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidgetAction>

#include <cmath>
#include <limits>

namespace cppqtgraph::graphicsItems {
namespace {

constexpr int kXAxis = ViewBox::XAxis;
constexpr int kYAxis = ViewBox::YAxis;

} // namespace

ViewBoxMenu::ViewBoxMenu(ViewBox* view, QWidget* parent)
    : QMenu(parent)
    , view_(view)
{
    setTitle(QCoreApplication::translate("ViewBox", "ViewBox options"));

    viewAllAction_ = addAction(QCoreApplication::translate("ViewBox", "View All"));
    connect(viewAllAction_, &QAction::triggered, this, &ViewBoxMenu::onViewAll);

    addSeparator();

    setupAxisMenu(kXAxis, QStringLiteral("X %1").arg(QCoreApplication::translate("ViewBox", "axis")));
    setupAxisMenu(kYAxis, QStringLiteral("Y %1").arg(QCoreApplication::translate("ViewBox", "axis")));

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

void ViewBoxMenu::setupAxisMenu(int axis, const QString& title)
{
    QMenu* menu = addMenu(title);
    AxisCtrl& ctrl = axisCtrls_[static_cast<std::size_t>(axis)];
    ctrl.widget = new QWidget(this);
    ctrl.ui.setupUi(ctrl.widget);
    ctrl.action = new QWidgetAction(this);
    ctrl.action->setDefaultWidget(ctrl.widget);
    menu->addAction(ctrl.action);

    if (axis == kXAxis) {
        connect(ctrl.ui.mouseCheck, &QCheckBox::toggled, this, &ViewBoxMenu::onXMouseToggled);
        connect(ctrl.ui.manualRadio, &QRadioButton::clicked, this, &ViewBoxMenu::onXManualClicked);
        connect(ctrl.ui.minText, &QLineEdit::editingFinished, this, &ViewBoxMenu::onXRangeTextChanged);
        connect(ctrl.ui.maxText, &QLineEdit::editingFinished, this, &ViewBoxMenu::onXRangeTextChanged);
        connect(ctrl.ui.autoRadio, &QRadioButton::clicked, this, &ViewBoxMenu::onXAutoClicked);
        connect(ctrl.ui.autoPercentSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ViewBoxMenu::onXAutoSpinChanged);
        connect(ctrl.ui.invertCheck, &QCheckBox::toggled, this, &ViewBoxMenu::onXInvertToggled);
    } else {
        connect(ctrl.ui.mouseCheck, &QCheckBox::toggled, this, &ViewBoxMenu::onYMouseToggled);
        connect(ctrl.ui.manualRadio, &QRadioButton::clicked, this, &ViewBoxMenu::onYManualClicked);
        connect(ctrl.ui.minText, &QLineEdit::editingFinished, this, &ViewBoxMenu::onYRangeTextChanged);
        connect(ctrl.ui.maxText, &QLineEdit::editingFinished, this, &ViewBoxMenu::onYRangeTextChanged);
        connect(ctrl.ui.autoRadio, &QRadioButton::clicked, this, &ViewBoxMenu::onYAutoClicked);
        connect(ctrl.ui.autoPercentSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ViewBoxMenu::onYAutoSpinChanged);
        connect(ctrl.ui.invertCheck, &QCheckBox::toggled, this, &ViewBoxMenu::onYInvertToggled);
    }
}

ViewBox* ViewBoxMenu::view() const noexcept
{
    return view_;
}

std::pair<qreal, qreal> ViewBoxMenu::validatedRangeText(int axis) const
{
    const AxisCtrl& ctrl = axisCtrls_[static_cast<std::size_t>(axis)];
    bool minOk = false;
    bool maxOk = false;
    const qreal minValue = ctrl.ui.minText->text().toDouble(&minOk);
    const qreal maxValue = ctrl.ui.maxText->text().toDouble(&maxOk);
    if (!minOk || !maxOk) {
        const auto range = view_->targetRange()[static_cast<std::size_t>(axis)];
        return {range[0], range[1]};
    }
    return {minValue, maxValue};
}

void ViewBoxMenu::updateState()
{
    if (view_ == nullptr) {
        return;
    }

    const auto targetRange = view_->targetRange();
    const auto autoRange = view_->autoRangeEnabled();
    const auto mouseEnabled = view_->mouseEnabled();

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

    for (int axis : {kXAxis, kYAxis}) {
        AxisCtrl& ctrl = axisCtrls_[static_cast<std::size_t>(axis)];
        if (ctrl.ui.minText == nullptr) {
            continue;
        }

        const auto range = targetRange[static_cast<std::size_t>(axis)];
        ctrl.ui.minText->blockSignals(true);
        ctrl.ui.maxText->blockSignals(true);
        ctrl.ui.minText->setText(QString::number(range[0], 'g', 5));
        ctrl.ui.maxText->setText(QString::number(range[1], 'g', 5));
        ctrl.ui.minText->blockSignals(false);
        ctrl.ui.maxText->blockSignals(false);

        ctrl.ui.autoRadio->blockSignals(true);
        ctrl.ui.manualRadio->blockSignals(true);
        if (autoRange[static_cast<std::size_t>(axis)]) {
            ctrl.ui.autoRadio->setChecked(true);
        } else {
            ctrl.ui.manualRadio->setChecked(true);
        }
        ctrl.ui.autoRadio->blockSignals(false);
        ctrl.ui.manualRadio->blockSignals(false);

        ctrl.ui.mouseCheck->blockSignals(true);
        ctrl.ui.mouseCheck->setChecked(mouseEnabled[static_cast<std::size_t>(axis)]);
        ctrl.ui.mouseCheck->blockSignals(false);

        ctrl.ui.invertCheck->blockSignals(true);
        ctrl.ui.invertCheck->setChecked(axis == kXAxis ? view_->xInverted() : view_->yInverted());
        ctrl.ui.invertCheck->blockSignals(false);
    }
}

void ViewBoxMenu::onViewAll()
{
    if (view_ != nullptr) {
        view_->autoRange();
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

void ViewBoxMenu::onXMouseToggled(bool checked)
{
    if (view_ != nullptr) {
        view_->setMouseEnabled(checked, std::nullopt);
        updateState();
    }
}

void ViewBoxMenu::onYMouseToggled(bool checked)
{
    if (view_ != nullptr) {
        view_->setMouseEnabled(std::nullopt, checked);
        updateState();
    }
}

void ViewBoxMenu::onXManualClicked()
{
    if (view_ != nullptr) {
        view_->enableAutoRange(ViewBox::XAxis, false);
        updateState();
    }
}

void ViewBoxMenu::onYManualClicked()
{
    if (view_ != nullptr) {
        view_->enableAutoRange(ViewBox::YAxis, false);
        updateState();
    }
}

void ViewBoxMenu::onXRangeTextChanged()
{
    if (view_ == nullptr) {
        return;
    }
    axisCtrls_[static_cast<std::size_t>(kXAxis)].ui.manualRadio->setChecked(true);
    const auto [minValue, maxValue] = validatedRangeText(kXAxis);
    view_->setXRange(minValue, maxValue, 0.0);
    updateState();
}

void ViewBoxMenu::onYRangeTextChanged()
{
    if (view_ == nullptr) {
        return;
    }
    axisCtrls_[static_cast<std::size_t>(kYAxis)].ui.manualRadio->setChecked(true);
    const auto [minValue, maxValue] = validatedRangeText(kYAxis);
    view_->setYRange(minValue, maxValue, 0.0);
    updateState();
}

void ViewBoxMenu::onXAutoClicked()
{
    if (view_ == nullptr) {
        return;
    }
    const qreal padding = axisCtrls_[static_cast<std::size_t>(kXAxis)].ui.autoPercentSpin->value() / 100.0;
    view_->enableAutoRange(ViewBox::XAxis, true);
    view_->autoRange(padding);
    updateState();
}

void ViewBoxMenu::onYAutoClicked()
{
    if (view_ == nullptr) {
        return;
    }
    const qreal padding = axisCtrls_[static_cast<std::size_t>(kYAxis)].ui.autoPercentSpin->value() / 100.0;
    view_->enableAutoRange(ViewBox::YAxis, true);
    view_->autoRange(padding);
    updateState();
}

void ViewBoxMenu::onXAutoSpinChanged(int value)
{
    if (view_ == nullptr) {
        return;
    }
    axisCtrls_[static_cast<std::size_t>(kXAxis)].ui.autoRadio->setChecked(true);
    view_->enableAutoRange(ViewBox::XAxis, true);
    view_->autoRange(value / 100.0);
    updateState();
}

void ViewBoxMenu::onYAutoSpinChanged(int value)
{
    if (view_ == nullptr) {
        return;
    }
    axisCtrls_[static_cast<std::size_t>(kYAxis)].ui.autoRadio->setChecked(true);
    view_->enableAutoRange(ViewBox::YAxis, true);
    view_->autoRange(value / 100.0);
    updateState();
}

void ViewBoxMenu::onXInvertToggled(bool checked)
{
    if (view_ != nullptr) {
        view_->invertX(checked);
        updateState();
    }
}

void ViewBoxMenu::onYInvertToggled(bool checked)
{
    if (view_ != nullptr) {
        view_->invertY(checked);
        updateState();
    }
}

} // namespace cppqtgraph::graphicsItems
