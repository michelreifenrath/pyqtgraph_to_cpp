// Source note: translated/adapted from PyQtGraph pyqtgraph/ThreadsafeTimer.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/ThreadsafeTimer.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>

namespace cppqtgraph {

ThreadsafeTimer::ThreadsafeTimer(QObject* parent)
    : QObject(parent)
    , timer_(new QTimer())
{
    QObject::connect(timer_, &QTimer::timeout, this, &ThreadsafeTimer::timerFinished,
        Qt::DirectConnection);

    if (QCoreApplication::instance() != nullptr) {
        QThread* appThread = QCoreApplication::instance()->thread();
        timer_->moveToThread(appThread);
        if (parent == nullptr) {
            moveToThread(appThread);
        }
    }

    QObject::connect(this, &ThreadsafeTimer::sigTimerStopRequested, timer_, &QTimer::stop,
        Qt::QueuedConnection);
    QObject::connect(this, &ThreadsafeTimer::sigTimerStartRequested, timer_,
        static_cast<void (QTimer::*)(int)>(&QTimer::start), Qt::QueuedConnection);
}

ThreadsafeTimer::~ThreadsafeTimer()
{
    QTimer* timer = timer_;
    timer_ = nullptr;
    if (timer == nullptr) {
        return;
    }

    if (timer->thread() == QThread::currentThread()) {
        timer->stop();
        delete timer;
        return;
    }

    QMetaObject::invokeMethod(timer, [timer]() {
        timer->stop();
        timer->deleteLater();
    }, Qt::QueuedConnection);
}

void ThreadsafeTimer::start(int timeoutMs)
{
    const bool isGuiThread = QCoreApplication::instance() == nullptr
        || QThread::currentThread() == QCoreApplication::instance()->thread();
    if (isGuiThread) {
        timer_->start(timeoutMs);
        return;
    }

    emit sigTimerStartRequested(timeoutMs);
}

void ThreadsafeTimer::stop()
{
    const bool isGuiThread = QCoreApplication::instance() == nullptr
        || QThread::currentThread() == QCoreApplication::instance()->thread();
    if (isGuiThread) {
        timer_->stop();
        return;
    }

    emit sigTimerStopRequested();
}

void ThreadsafeTimer::timerFinished()
{
    emit timeout();
}

} // namespace cppqtgraph
