#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/FeedbackButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QPushButton>

#include <optional>

namespace cppqtgraph::widgets {

class FeedbackButton : public QPushButton {
    Q_OBJECT

public:
    explicit FeedbackButton(QWidget* parent = nullptr);
    explicit FeedbackButton(const QString& text, QWidget* parent = nullptr);

    FeedbackButton(const FeedbackButton&) = delete;
    FeedbackButton& operator=(const FeedbackButton&) = delete;
    FeedbackButton(FeedbackButton&&) = delete;
    FeedbackButton& operator=(FeedbackButton&&) = delete;

    void feedback(bool success, const QString& message = QString(), const QString& tip = QString(),
        bool limitedTime = true);
    void success(const QString& message = QString(), const QString& tip = QString(), bool limitedTime = true);
    void failure(const QString& message = QString(), const QString& tip = QString(), bool limitedTime = true);
    void processing(const QString& message = QStringLiteral("Processing.."), const QString& tip = QString(),
        bool processEvents = true);
    void reset();

    void setText(const QString& text);
    void setText(const std::optional<QString>& text, bool temporary);
    void setToolTip(const QString& text);
    void setToolTip(const std::optional<QString>& text, bool temporary);
    void setStyleSheet(const QString& style);
    void setStyleSheet(const std::optional<QString>& style, bool temporary);

signals:
    void sigCallSuccess(const QString& message, const QString& tip, bool limitedTime);
    void sigCallFailure(const QString& message, const QString& tip, bool limitedTime);
    void sigCallProcess(const QString& message, const QString& tip, bool processEvents);
    void sigReset();

private slots:
    void onCallSuccess(const QString& message, const QString& tip, bool limitedTime);
    void onCallFailure(const QString& message, const QString& tip, bool limitedTime);
    void onCallProcess(const QString& message, const QString& tip, bool processEvents);
    void onCallReset();
    void startBlink(const QString& color, const QString& message, const QString& tip, bool limitedTime);
    void borderOn();
    void borderOff();
    void restoreOrigText();
    void restoreOrigToolTip();

private:
    [[nodiscard]] static bool isGuiThread();

    QString origText_;
    QString origStyle_;
    QString origTip_;
    bool limitedTime_ = true;
    QString indStyle_;
    int count_ = 0;
};

} // namespace cppqtgraph::widgets
