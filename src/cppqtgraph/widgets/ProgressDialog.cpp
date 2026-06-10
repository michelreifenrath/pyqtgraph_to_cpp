// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ProgressDialog.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/ProgressDialog.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtGui/QCursor>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

namespace cppqtgraph::widgets {

QList<ProgressDialog*>& ProgressDialog::activeDialogs()
{
    static QList<ProgressDialog*> dialogs;
    return dialogs;
}

ProgressDialog::ProgressDialog(const QString& labelText, int minimum, int maximum, std::optional<QString> cancelText,
    QWidget* parent, int wait, bool busyCursor, bool disable, bool nested)
    : QProgressDialog(labelText, cancelText.value_or(QStringLiteral("Cancel")), minimum, maximum, parent)
    , busyCursor_(busyCursor)
    , nested_(nested)
{
    const bool isGuiThread = QThread::currentThread() == QCoreApplication::instance()->thread();
    disabled_ = disable || !isGuiThread;
    if (disabled_) {
        return;
    }

    const bool noCancel = !cancelText.has_value();

    if (nested && !activeDialogs().isEmpty()) {
        setMinimumDuration(static_cast<int>(1LL << 30));
    } else {
        setMinimumDuration(wait);
    }

    setWindowModality(Qt::WindowModal);
    QProgressDialog::setValue(QProgressDialog::minimum());
    if (noCancel) {
        setCancelButton(nullptr);
    }
}

ProgressDialog::~ProgressDialog()
{
    finish();
}

void ProgressDialog::begin()
{
    if (disabled_ || active_) {
        return;
    }

    if (busyCursor_) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

    activeDialogs().append(this);
    active_ = true;
}

void ProgressDialog::finish()
{
    if (disabled_ || finished_) {
        return;
    }

    if (busyCursor_ && active_) {
        QApplication::restoreOverrideCursor();
    }

    if (active_) {
        activeDialogs().removeOne(this);
        active_ = false;
    }

    if (!disabled_) {
        QProgressDialog::setValue(QProgressDialog::maximum());
    }

    finished_ = true;
}

ProgressDialog& ProgressDialog::operator+=(int value)
{
    if (disabled_) {
        return *this;
    }
    setValue(this->value() + value);
    return *this;
}

bool ProgressDialog::wasCanceled() const
{
    if (disabled_) {
        return false;
    }
    return QProgressDialog::wasCanceled();
}

bool ProgressDialog::hasCancelButton() const
{
    if (disabled_) {
        return false;
    }
    return findChild<QPushButton*>(QString(), Qt::FindDirectChildrenOnly) != nullptr;
}

void ProgressDialog::setValue(int value)
{
    if (disabled_) {
        return;
    }

    QProgressDialog::setValue(value);

    if (windowModality() == Qt::WindowModal) {
        const auto now = std::chrono::steady_clock::now();
        if (!hasProcessedEvents_
            || std::chrono::duration<double>(now - lastProcessEvents_).count() > 0.2) {
            QApplication::processEvents();
            lastProcessEvents_ = now;
            hasProcessedEvents_ = true;
        }
    }
}

void ProgressDialog::setLabelText(const QString& text)
{
    if (disabled_) {
        return;
    }
    QProgressDialog::setLabelText(text);
}

void ProgressDialog::setMaximum(int maximum)
{
    if (disabled_) {
        return;
    }
    QProgressDialog::setMaximum(maximum);
}

void ProgressDialog::setMinimum(int minimum)
{
    if (disabled_) {
        return;
    }
    QProgressDialog::setMinimum(minimum);
}

int ProgressDialog::value() const
{
    if (disabled_) {
        return 0;
    }
    return QProgressDialog::value();
}

int ProgressDialog::maximum() const
{
    if (disabled_) {
        return 0;
    }
    return QProgressDialog::maximum();
}

int ProgressDialog::minimum() const
{
    if (disabled_) {
        return 0;
    }
    return QProgressDialog::minimum();
}

} // namespace cppqtgraph::widgets
