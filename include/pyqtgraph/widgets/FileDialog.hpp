#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/FileDialog.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QFileDialog>

namespace pyqtgraph::widgets {

class FileDialog : public QFileDialog {
    Q_OBJECT

public:
    explicit FileDialog(QWidget* parent = nullptr);
    explicit FileDialog(QWidget* parent, const QString& caption, const QString& directory, const QString& filter);

    FileDialog(const FileDialog&) = delete;
    FileDialog& operator=(const FileDialog&) = delete;
    FileDialog(FileDialog&&) = delete;
    FileDialog& operator=(FileDialog&&) = delete;

private:
    void applyPlatformOptions();
};

} // namespace pyqtgraph::widgets
