#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QMenu>

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

private:
    ViewBox* view_ = nullptr;
};

} // namespace cppqtgraph::graphicsItems
