// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/BusyCursor.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/BusyCursor.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtGui/QCursor>
#include <QtWidgets/QApplication>

namespace pyqtgraph::widgets {

BusyCursor::BusyCursor()
{
    QCoreApplication* app = QCoreApplication::instance();
    const bool inGuiThread = app != nullptr && QThread::currentThread() == app->thread();
    if (inGuiThread) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        active_ = true;
    }
}

BusyCursor::~BusyCursor()
{
    if (active_) {
        QApplication::restoreOverrideCursor();
    }
}

} // namespace pyqtgraph::widgets
