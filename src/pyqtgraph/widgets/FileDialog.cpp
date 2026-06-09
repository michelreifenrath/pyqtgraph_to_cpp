// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/FileDialog.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/FileDialog.hpp"

namespace pyqtgraph::widgets {

void FileDialog::applyPlatformOptions()
{
#if defined(Q_OS_MACOS)
    setOption(QFileDialog::Option::DontUseNativeDialog);
#endif
}

FileDialog::FileDialog(QWidget* parent)
    : QFileDialog(parent)
{
    applyPlatformOptions();
}

FileDialog::FileDialog(QWidget* parent, const QString& caption, const QString& directory, const QString& filter)
    : QFileDialog(parent, caption, directory, filter)
{
    applyPlatformOptions();
}

} // namespace pyqtgraph::widgets
