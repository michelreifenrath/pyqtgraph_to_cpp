#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QMenu>

class QAction;
class QActionGroup;

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
    void onXAutoRangeToggled(bool checked);
    void onYAutoRangeToggled(bool checked);
    void onMouseModeTriggered(QAction* action);

private:
    ViewBox* view_ = nullptr;
    QAction* viewAllAction_ = nullptr;
    QAction* xAutoRangeAction_ = nullptr;
    QAction* yAutoRangeAction_ = nullptr;
    QAction* panModeAction_ = nullptr;
    QAction* rectModeAction_ = nullptr;
    QActionGroup* mouseModeGroup_ = nullptr;
};

} // namespace cppqtgraph::graphicsItems
