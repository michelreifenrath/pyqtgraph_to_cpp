#include <pyqtgraph/SignalProxy.hpp>
#include <pyqtgraph/ThreadsafeTimer.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

namespace {

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }

    return true;
}

#define CHECK(expression) \
    do { \
        if (!check((expression), #expression, __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (false)

class ApplicationGuard {
public:
    ApplicationGuard(int& argc, char** argv)
    {
        if (QCoreApplication::instance() == nullptr) {
            application_ = std::make_unique<QCoreApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QCoreApplication> application_;
};

bool waitUntil(const std::function<bool()>& predicate, int timeoutMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (predicate()) {
            return true;
        }
        QThread::msleep(1);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    return predicate();
}

bool testOracleFixtureDocumentsPinnedBehavior()
{
#ifndef PYQTGRAPH_CPP_P2_08_FIXTURE
#error "PYQTGRAPH_CPP_P2_08_FIXTURE must be defined"
#endif
    QFile fixture(QString::fromUtf8(PYQTGRAPH_CPP_P2_08_FIXTURE));
    CHECK(fixture.open(QIODevice::ReadOnly));
    const QByteArray contents = fixture.readAll();
    CHECK(contents.contains("a20028b98294b9cc8770f2015a92eb342224b788"));
    CHECK(contents.contains("no_rate_limit_restart_ms"));
    CHECK(contents.contains("rate_limit_first_signal_ms"));
    CHECK(contents.contains("flush_without_args_returns_false"));
    CHECK(contents.contains("disconnect_blocks_future_signals"));
    return true;
}

bool testThreadsafeTimerTypeShape()
{
    using pyqtgraph::ThreadsafeTimer;

    static_assert(std::is_base_of_v<QObject, ThreadsafeTimer>);
    static_assert(std::is_constructible_v<ThreadsafeTimer>);
    static_assert(std::is_constructible_v<ThreadsafeTimer, QObject*>);
    static_assert(!std::is_copy_constructible_v<ThreadsafeTimer>);
    static_assert(!std::is_copy_assignable_v<ThreadsafeTimer>);

    ThreadsafeTimer timer;
    CHECK(timer.thread() == QCoreApplication::instance()->thread());
    return true;
}

bool testThreadsafeTimerStartStopOnGuiThread()
{
    pyqtgraph::ThreadsafeTimer timer;
    int timeoutCount = 0;
    QObject::connect(&timer, &pyqtgraph::ThreadsafeTimer::timeout, [&timeoutCount]() { ++timeoutCount; });

    timer.start(15);
    CHECK(waitUntil([&timeoutCount]() { return timeoutCount >= 1; }, 250));

    timer.start(80);
    timer.stop();
    const int stoppedCount = timeoutCount;
    QThread::msleep(110);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    CHECK(timeoutCount == stoppedCount);
    return true;
}

bool testThreadsafeTimerQueuedStartStopFromWorkerThread()
{
    pyqtgraph::ThreadsafeTimer timer;
    int timeoutCount = 0;
    QObject::connect(&timer, &pyqtgraph::ThreadsafeTimer::timeout, [&timeoutCount]() { ++timeoutCount; });

    std::unique_ptr<QThread> worker(QThread::create([&timer]() { timer.start(20); }));
    worker->start();
    worker->wait();
    CHECK(waitUntil([&timeoutCount]() { return timeoutCount >= 1; }, 300));

    worker.reset(QThread::create([&timer]() {
        timer.start(120);
        timer.stop();
    }));
    worker->start();
    worker->wait();
    const int stoppedCount = timeoutCount;
    QThread::msleep(150);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    CHECK(timeoutCount == stoppedCount);
    return true;
}

bool testThreadsafeTimerParentedWorkerThreadRequestsReachAppTimer()
{
    QThread ownerThread;
    QObject context;
    context.moveToThread(&ownerThread);
    ownerThread.start();

    QObject* parent = nullptr;
    pyqtgraph::ThreadsafeTimer* timer = nullptr;
    bool constructedOnWorker = false;
    CHECK(QMetaObject::invokeMethod(&context, [&]() {
        parent = new QObject();
        timer = new pyqtgraph::ThreadsafeTimer(parent);
        constructedOnWorker = timer->thread() == &ownerThread;
    }, Qt::BlockingQueuedConnection));
    CHECK(parent != nullptr);
    CHECK(timer != nullptr);
    CHECK(constructedOnWorker);

    int timeoutCount = 0;
    QObject::connect(timer, &pyqtgraph::ThreadsafeTimer::timeout, QCoreApplication::instance(),
        [&timeoutCount]() { ++timeoutCount; }, Qt::QueuedConnection);

    CHECK(QMetaObject::invokeMethod(&context, [&]() { timer->start(20); },
        Qt::BlockingQueuedConnection));
    CHECK(waitUntil([&timeoutCount]() { return timeoutCount >= 1; }, 300));

    CHECK(QMetaObject::invokeMethod(&context, [&]() {
        delete parent;
        parent = nullptr;
        timer = nullptr;
        context.moveToThread(QCoreApplication::instance()->thread());
    }, Qt::BlockingQueuedConnection));
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    ownerThread.quit();
    ownerThread.wait();
    return true;
}

bool testSignalProxyTypeShapeAndFlushFalse()
{
    using pyqtgraph::SignalProxy;

    static_assert(std::is_base_of_v<QObject, SignalProxy>);
    static_assert(std::is_constructible_v<SignalProxy>);
    static_assert(std::is_constructible_v<SignalProxy, double>);
    static_assert(std::is_constructible_v<SignalProxy, double, double>);
    static_assert(std::is_constructible_v<SignalProxy, double, double, bool>);
    static_assert(std::is_constructible_v<SignalProxy, double, double, bool, QObject*>);
    static_assert(!std::is_copy_constructible_v<SignalProxy>);
    static_assert(!std::is_copy_assignable_v<SignalProxy>);

    SignalProxy proxy(0.01, 0.0, false);
    CHECK(!proxy.flush());
    return true;
}

bool testSignalProxyDelayedCoalescesLatestArgs()
{
    pyqtgraph::SignalProxy proxy(0.04, 0.0, false);
    int delayedCount = 0;
    QVariantList lastArgs;
    QObject::connect(&proxy, &pyqtgraph::SignalProxy::sigDelayed,
        [&delayedCount, &lastArgs](const QVariantList& args) {
            ++delayedCount;
            lastArgs = args;
        });

    proxy.signalReceived(QVariant(1));
    QThread::msleep(15);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    proxy.signalReceived(QVariant(2), QVariant(QStringLiteral("latest")));

    CHECK(waitUntil([&delayedCount]() { return delayedCount == 1; }, 300));
    CHECK(lastArgs.size() == 2);
    CHECK(lastArgs.at(0).toInt() == 2);
    CHECK(lastArgs.at(1).toString() == QStringLiteral("latest"));
    CHECK(!proxy.flush());
    return true;
}

bool testSignalProxyFlushTrueClearsAndEmits()
{
    pyqtgraph::SignalProxy proxy(0.2, 0.0, false);
    int delayedCount = 0;
    QVariantList lastArgs;
    QObject::connect(&proxy, &pyqtgraph::SignalProxy::sigDelayed,
        [&delayedCount, &lastArgs](const QVariantList& args) {
            ++delayedCount;
            lastArgs = args;
        });

    proxy.signalReceived(QVariant(QStringLiteral("manual")));
    CHECK(proxy.flush());
    CHECK(delayedCount == 1);
    CHECK(lastArgs.size() == 1);
    CHECK(lastArgs.at(0).toString() == QStringLiteral("manual"));
    CHECK(!proxy.flush());
    return true;
}

bool testSignalProxyRateLimitThrottlesAndKeepsLatestArgs()
{
    pyqtgraph::SignalProxy proxy(0.2, 20.0, false);
    int delayedCount = 0;
    QVariantList lastArgs;
    QObject::connect(&proxy, &pyqtgraph::SignalProxy::sigDelayed,
        [&delayedCount, &lastArgs](const QVariantList& args) {
            ++delayedCount;
            lastArgs = args;
        });

    proxy.signalReceived(QVariant(1));
    CHECK(waitUntil([&delayedCount]() { return delayedCount == 1; }, 120));
    proxy.signalReceived(QVariant(2));
    proxy.signalReceived(QVariant(3));
    QThread::msleep(20);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    CHECK(delayedCount == 1);
    CHECK(waitUntil([&delayedCount]() { return delayedCount == 2; }, 200));
    CHECK(lastArgs.size() == 1);
    CHECK(lastArgs.at(0).toInt() == 3);
    return true;
}

bool testSignalProxyDisconnectBlocksFutureSignals()
{
    pyqtgraph::SignalProxy proxy(0.01, 0.0, false);
    int delayedCount = 0;
    QObject::connect(&proxy, &pyqtgraph::SignalProxy::sigDelayed, [&delayedCount](const QVariantList&) {
        ++delayedCount;
    });

    proxy.signalReceived(QVariant(1));
    proxy.disconnect();
    CHECK(!proxy.flush());
    proxy.signalReceived(QVariant(2));
    QThread::msleep(40);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    CHECK(delayedCount == 0);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ApplicationGuard application(argc, argv);

    const bool ok = testOracleFixtureDocumentsPinnedBehavior()
        && testThreadsafeTimerTypeShape()
        && testThreadsafeTimerStartStopOnGuiThread()
        && testThreadsafeTimerQueuedStartStopFromWorkerThread()
        && testThreadsafeTimerParentedWorkerThreadRequestsReachAppTimer()
        && testSignalProxyTypeShapeAndFlushFalse()
        && testSignalProxyDelayedCoalescesLatestArgs()
        && testSignalProxyFlushTrueClearsAndEmits()
        && testSignalProxyRateLimitThrottlesAndKeepsLatestArgs()
        && testSignalProxyDisconnectBlocksFutureSignals();
    return ok ? 0 : 1;
}
