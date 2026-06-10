// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp"

#include <QtCore/QCoreApplication>

namespace cppqtgraph::graphicsItems {

ViewBoxMenu::ViewBoxMenu(ViewBox* view, QWidget* parent)
    : QMenu(parent)
    , view_(view)
{
    setTitle(QCoreApplication::translate("ViewBox", "ViewBox options"));
}

ViewBoxMenu::~ViewBoxMenu() = default;

ViewBox* ViewBoxMenu::view() const noexcept
{
    return view_;
}

} // namespace cppqtgraph::graphicsItems
