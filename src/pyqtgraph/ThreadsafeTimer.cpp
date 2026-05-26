// Source note: translated/adapted from PyQtGraph pyqtgraph/ThreadsafeTimer.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/pyqtgraph/ThreadsafeTimer.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

namespace pyqtgraph {

ThreadsafeTimer::ThreadsafeTimer(QObject* parent)
    : QObject(parent)
{
    QObject::connect(&timer_, &QTimer::timeout, this, &ThreadsafeTimer::timerFinished);

    if (QCoreApplication::instance() != nullptr) {
        QThread* appThread = QCoreApplication::instance()->thread();
        timer_.moveToThread(appThread);
        if (parent == nullptr) {
            moveToThread(appThread);
        }
    }

    QObject::connect(this, &ThreadsafeTimer::sigTimerStopRequested, this, &ThreadsafeTimer::stop,
        Qt::QueuedConnection);
    QObject::connect(this, &ThreadsafeTimer::sigTimerStartRequested, this, &ThreadsafeTimer::start,
        Qt::QueuedConnection);
}

ThreadsafeTimer::~ThreadsafeTimer()
{
    stop();
}

void ThreadsafeTimer::start(int timeoutMs)
{
    const bool isGuiThread = QCoreApplication::instance() == nullptr
        || QThread::currentThread() == QCoreApplication::instance()->thread();
    if (isGuiThread) {
        timer_.start(timeoutMs);
        return;
    }

    emit sigTimerStartRequested(timeoutMs);
}

void ThreadsafeTimer::stop()
{
    const bool isGuiThread = QCoreApplication::instance() == nullptr
        || QThread::currentThread() == QCoreApplication::instance()->thread();
    if (isGuiThread) {
        timer_.stop();
        return;
    }

    emit sigTimerStopRequested();
}

void ThreadsafeTimer::timerFinished()
{
    emit timeout();
}

} // namespace pyqtgraph
