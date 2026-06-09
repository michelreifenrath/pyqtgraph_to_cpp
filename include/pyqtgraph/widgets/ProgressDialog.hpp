#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ProgressDialog.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QProgressDialog>

#include <chrono>
#include <optional>

namespace pyqtgraph::widgets {

class ProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(const QString& labelText, int minimum = 0, int maximum = 100,
        std::optional<QString> cancelText = QStringLiteral("Cancel"), QWidget* parent = nullptr, int wait = 250,
        bool busyCursor = false, bool disable = false, bool nested = false);

    ProgressDialog(const ProgressDialog&) = delete;
    ProgressDialog& operator=(const ProgressDialog&) = delete;
    ProgressDialog(ProgressDialog&&) = delete;
    ProgressDialog& operator=(ProgressDialog&&) = delete;

    ~ProgressDialog() override;

    [[nodiscard]] bool disabled() const { return disabled_; }
    [[nodiscard]] bool hasCancelButton() const;

    void begin();
    void finish();

    ProgressDialog& operator+=(int value);

    [[nodiscard]] bool wasCanceled() const;

    void setValue(int value);
    void setLabelText(const QString& text);
    void setMaximum(int maximum);
    void setMinimum(int minimum);
    [[nodiscard]] int value() const;
    [[nodiscard]] int maximum() const;
    [[nodiscard]] int minimum() const;

private:
    static QList<ProgressDialog*>& activeDialogs();

    bool disabled_ = false;
    bool busyCursor_ = false;
    bool nested_ = false;
    bool active_ = false;
    bool finished_ = false;
    std::chrono::steady_clock::time_point lastProcessEvents_{};
    bool hasProcessedEvents_ = false;
};

} // namespace pyqtgraph::widgets
