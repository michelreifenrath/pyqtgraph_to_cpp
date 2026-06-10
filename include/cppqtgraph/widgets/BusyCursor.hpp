#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/BusyCursor.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

namespace cppqtgraph::widgets {

class BusyCursor {
public:
    BusyCursor();
    ~BusyCursor();

    BusyCursor(const BusyCursor&) = delete;
    BusyCursor& operator=(const BusyCursor&) = delete;
    BusyCursor(BusyCursor&&) = delete;
    BusyCursor& operator=(BusyCursor&&) = delete;

private:
    bool active_ = false;
};

} // namespace cppqtgraph::widgets
