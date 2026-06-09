// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/FeedbackButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/FeedbackButton.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

namespace pyqtgraph::widgets {

bool FeedbackButton::isGuiThread()
{
    const auto* app = QCoreApplication::instance();
    return app != nullptr && QThread::currentThread() == app->thread();
}

FeedbackButton::FeedbackButton(QWidget* parent)
    : QPushButton(parent)
{
    origText_ = text();
    origStyle_ = styleSheet();
    origTip_ = toolTip();

    connect(this, &FeedbackButton::sigCallSuccess, this, &FeedbackButton::onCallSuccess);
    connect(this, &FeedbackButton::sigCallFailure, this, &FeedbackButton::onCallFailure);
    connect(this, &FeedbackButton::sigCallProcess, this, &FeedbackButton::onCallProcess);
    connect(this, &FeedbackButton::sigReset, this, &FeedbackButton::onCallReset);
}

FeedbackButton::FeedbackButton(const QString& text, QWidget* parent)
    : FeedbackButton(parent)
{
    setText(text);
}

void FeedbackButton::feedback(bool success, const QString& message, const QString& tip, bool limitedTime)
{
    if (success) {
        this->success(message, tip, limitedTime);
    } else {
        this->failure(message, tip, limitedTime);
    }
}

void FeedbackButton::success(const QString& message, const QString& tip, bool limitedTime)
{
    if (isGuiThread()) {
        setEnabled(true);
        startBlink(QStringLiteral("#0F0"), message, tip, limitedTime);
    } else {
        emit sigCallSuccess(message, tip, limitedTime);
    }
}

void FeedbackButton::failure(const QString& message, const QString& tip, bool limitedTime)
{
    if (isGuiThread()) {
        setEnabled(true);
        startBlink(QStringLiteral("#F00"), message, tip, limitedTime);
    } else {
        emit sigCallFailure(message, tip, limitedTime);
    }
}

void FeedbackButton::processing(const QString& message, const QString& tip, bool processEvents)
{
    if (isGuiThread()) {
        setEnabled(false);
        setText(message, true);
        setToolTip(tip, true);
        if (processEvents) {
            QApplication::processEvents();
        }
    } else {
        emit sigCallProcess(message, tip, processEvents);
    }
}

void FeedbackButton::reset()
{
    if (isGuiThread()) {
        limitedTime_ = true;
        setText(std::nullopt, false);
        setToolTip(std::nullopt, false);
        setStyleSheet(std::nullopt, false);
    } else {
        emit sigReset();
    }
}

void FeedbackButton::setText(const QString& text)
{
    setText(text, false);
}

void FeedbackButton::setText(const std::optional<QString>& text, bool temporary)
{
    const QString resolved = text.value_or(origText_);
    QPushButton::setText(resolved);
    if (!temporary) {
        origText_ = resolved;
    }
}

void FeedbackButton::setToolTip(const QString& text)
{
    setToolTip(text, false);
}

void FeedbackButton::setToolTip(const std::optional<QString>& text, bool temporary)
{
    const QString resolved = text.value_or(origTip_);
    QPushButton::setToolTip(resolved);
    if (!temporary) {
        origTip_ = resolved;
    }
}

void FeedbackButton::setStyleSheet(const QString& style)
{
    setStyleSheet(style, false);
}

void FeedbackButton::setStyleSheet(const std::optional<QString>& style, bool temporary)
{
    const QString resolved = style.value_or(origStyle_);
    QPushButton::setStyleSheet(resolved);
    if (!temporary) {
        origStyle_ = resolved;
    }
}

void FeedbackButton::onCallSuccess(const QString& message, const QString& tip, bool limitedTime)
{
    success(message, tip, limitedTime);
}

void FeedbackButton::onCallFailure(const QString& message, const QString& tip, bool limitedTime)
{
    failure(message, tip, limitedTime);
}

void FeedbackButton::onCallProcess(const QString& message, const QString& tip, bool processEvents)
{
    processing(message, tip, processEvents);
}

void FeedbackButton::onCallReset()
{
    reset();
}

void FeedbackButton::startBlink(const QString& color, const QString& message, const QString& tip, bool limitedTime)
{
    setFixedHeight(height());

    if (!message.isNull()) {
        setText(message, true);
    }
    setToolTip(tip, true);
    count_ = 0;
    indStyle_ = QStringLiteral("QPushButton {background-color: %1}").arg(color);
    limitedTime_ = limitedTime;
    borderOn();
    if (limitedTime) {
        QTimer::singleShot(2000, this, &FeedbackButton::restoreOrigText);
        QTimer::singleShot(10000, this, &FeedbackButton::restoreOrigToolTip);
    }
}

void FeedbackButton::borderOn()
{
    setStyleSheet(indStyle_, true);
    if (limitedTime_ || count_ <= 2) {
        QTimer::singleShot(100, this, &FeedbackButton::borderOff);
    }
}

void FeedbackButton::borderOff()
{
    setStyleSheet(std::nullopt, false);
    ++count_;
    if (count_ >= 2) {
        if (limitedTime_) {
            return;
        }
    }
    QTimer::singleShot(30, this, &FeedbackButton::borderOn);
}

void FeedbackButton::restoreOrigText()
{
    setText(std::nullopt, false);
}

void FeedbackButton::restoreOrigToolTip()
{
    setToolTip(std::nullopt, false);
}

} // namespace pyqtgraph::widgets
