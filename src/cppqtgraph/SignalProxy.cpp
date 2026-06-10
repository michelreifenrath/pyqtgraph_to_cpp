// Source note: translated/adapted from PyQtGraph pyqtgraph/SignalProxy.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/SignalProxy.hpp"

#include "../../include/cppqtgraph/ThreadsafeTimer.hpp"

#include <QtCore/QTimer>

#include <algorithm>
#include <chrono>
#include <utility>

namespace cppqtgraph {
namespace {

double nowSeconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

int timeoutMilliseconds(double seconds)
{
    return static_cast<int>(seconds * 1000.0) + 1;
}

} // namespace

SignalProxy::SignalProxy(double delaySeconds, double rateLimit, bool threadSafe, QObject* parent)
    : QObject(parent)
    , delaySeconds_(delaySeconds)
    , rateLimit_(rateLimit)
    , threadSafe_(threadSafe)
{
    if (threadSafe_) {
        auto timer = std::make_unique<ThreadsafeTimer>();
        QObject::connect(timer.get(), &ThreadsafeTimer::timeout, this, &SignalProxy::flush);
        timer_ = std::move(timer);
    } else {
        auto timer = std::make_unique<QTimer>();
        QObject::connect(timer.get(), &QTimer::timeout, this, &SignalProxy::flush);
        timer_ = std::move(timer);
    }
}

SignalProxy::~SignalProxy() = default;

void SignalProxy::setDelay(double delaySeconds) noexcept
{
    delaySeconds_ = delaySeconds;
}

double SignalProxy::delay() const noexcept
{
    return delaySeconds_;
}

double SignalProxy::rateLimit() const noexcept
{
    return rateLimit_;
}

bool SignalProxy::flush()
{
    if (!hasArgs_ || blockSignal_) {
        return false;
    }

    const QVariantList args = args_;
    args_.clear();
    hasArgs_ = false;
    stopTimer();
    lastFlushTime_ = nowSeconds();
    emit sigDelayed(args);
    return true;
}

void SignalProxy::disconnect()
{
    blockSignal_ = true;
    stopTimer();
}

void SignalProxy::signalReceived()
{
    signalReceived(QVariantList{});
}

void SignalProxy::signalReceived(const QVariant& arg)
{
    signalReceived(QVariantList{arg});
}

void SignalProxy::signalReceived(const QVariant& arg0, const QVariant& arg1)
{
    signalReceived(QVariantList{arg0, arg1});
}

void SignalProxy::signalReceived(QVariantList args)
{
    if (blockSignal_) {
        return;
    }

    args_ = std::move(args);
    hasArgs_ = true;

    stopTimer();
    if (rateLimit_ == 0.0) {
        startTimer(timeoutMilliseconds(delaySeconds_));
        return;
    }

    const double leakTime = lastFlushTime_ < 0.0 ? 0.0 : std::max(0.0, (lastFlushTime_ + (1.0 / rateLimit_)) - nowSeconds());
    startTimer(timeoutMilliseconds(std::min(leakTime, delaySeconds_)));
}

void SignalProxy::startTimer(int timeoutMs)
{
    if (auto* timer = qobject_cast<ThreadsafeTimer*>(timer_.get())) {
        timer->start(timeoutMs);
        return;
    }

    auto* timer = qobject_cast<QTimer*>(timer_.get());
    Q_ASSERT(timer != nullptr);
    timer->start(timeoutMs);
}

void SignalProxy::stopTimer()
{
    if (auto* timer = qobject_cast<ThreadsafeTimer*>(timer_.get())) {
        timer->stop();
        return;
    }

    auto* timer = qobject_cast<QTimer*>(timer_.get());
    Q_ASSERT(timer != nullptr);
    timer->stop();
}

} // namespace cppqtgraph
