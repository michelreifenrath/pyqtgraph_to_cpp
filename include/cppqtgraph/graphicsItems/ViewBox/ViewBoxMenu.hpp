#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "axisCtrlTemplate_generic.hpp"

#include <QtWidgets/QMenu>

class QAction;
class QActionGroup;
class QWidget;
class QWidgetAction;

namespace cppqtgraph::graphicsItems {

class ViewBox;

class ViewBoxMenu : public QMenu {
    Q_OBJECT

public:
    explicit ViewBoxMenu(ViewBox* view, QWidget* parent = nullptr);
    ~ViewBoxMenu() override;

    ViewBoxMenu(const ViewBoxMenu&) = delete;
    ViewBoxMenu& operator=(const ViewBoxMenu&) = delete;
    ViewBoxMenu(ViewBoxMenu&&) = delete;
    ViewBoxMenu& operator=(ViewBoxMenu&&) = delete;

    [[nodiscard]] ViewBox* view() const noexcept;
    void updateState();

private slots:
    void onViewAll();
    void onMouseModeTriggered(QAction* action);
    void onXMouseToggled(bool checked);
    void onYMouseToggled(bool checked);
    void onXManualClicked();
    void onYManualClicked();
    void onXRangeTextChanged();
    void onYRangeTextChanged();
    void onXAutoClicked();
    void onYAutoClicked();
    void onXAutoSpinChanged(int value);
    void onYAutoSpinChanged(int value);
    void onXInvertToggled(bool checked);
    void onYInvertToggled(bool checked);

private:
    struct AxisCtrl {
        ViewBoxAxisConfig::Ui_Form ui;
        QWidget* widget = nullptr;
        QWidgetAction* action = nullptr;
    };

    void setupAxisMenu(int axis, const QString& title);
    [[nodiscard]] std::pair<qreal, qreal> validatedRangeText(int axis) const;

    ViewBox* view_ = nullptr;
    QAction* viewAllAction_ = nullptr;
    QAction* panModeAction_ = nullptr;
    QAction* rectModeAction_ = nullptr;
    QActionGroup* mouseModeGroup_ = nullptr;
    std::array<AxisCtrl, 2> axisCtrls_{};
};

} // namespace cppqtgraph::graphicsItems
