#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/ThreadsafeTimer.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QObject>
#include <QTimer>

namespace pyqtgraph {

class ThreadsafeTimer : public QObject {
    Q_OBJECT

public:
    explicit ThreadsafeTimer(QObject* parent = nullptr);
    ~ThreadsafeTimer() override;

    ThreadsafeTimer(const ThreadsafeTimer&) = delete;
    ThreadsafeTimer& operator=(const ThreadsafeTimer&) = delete;
    ThreadsafeTimer(ThreadsafeTimer&&) = delete;
    ThreadsafeTimer& operator=(ThreadsafeTimer&&) = delete;

public slots:
    void start(int timeoutMs);
    void stop();

signals:
    void timeout();
    void sigTimerStopRequested();
    void sigTimerStartRequested(int timeoutMs);

private slots:
    void timerFinished();

private:
    QTimer* timer_ = nullptr;
};

} // namespace pyqtgraph
