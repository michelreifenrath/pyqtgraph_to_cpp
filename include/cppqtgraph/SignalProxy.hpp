#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/SignalProxy.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QObject>
#include <QVariant>
#include <QVariantList>

#include <memory>

namespace cppqtgraph {

class SignalProxy : public QObject {
    Q_OBJECT

public:
    explicit SignalProxy(double delaySeconds = 0.3, double rateLimit = 0.0, bool threadSafe = true,
        QObject* parent = nullptr);
    ~SignalProxy() override;

    SignalProxy(const SignalProxy&) = delete;
    SignalProxy& operator=(const SignalProxy&) = delete;
    SignalProxy(SignalProxy&&) = delete;
    SignalProxy& operator=(SignalProxy&&) = delete;

    void setDelay(double delaySeconds) noexcept;
    [[nodiscard]] double delay() const noexcept;
    [[nodiscard]] double rateLimit() const noexcept;

public slots:
    bool flush();
    void disconnect();
    void signalReceived();
    void signalReceived(const QVariant& arg);
    void signalReceived(const QVariant& arg0, const QVariant& arg1);

signals:
    void sigDelayed(QVariantList args);

private:
    void signalReceived(QVariantList args);
    void startTimer(int timeoutMs);
    void stopTimer();

    double delaySeconds_ = 0.3;
    double rateLimit_ = 0.0;
    QVariantList args_;
    bool hasArgs_ = false;
    bool blockSignal_ = false;
    bool threadSafe_ = true;
    double lastFlushTime_ = -1.0;
    std::unique_ptr<QObject> timer_;
};

} // namespace cppqtgraph
