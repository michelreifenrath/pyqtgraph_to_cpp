// Source note: translated/adapted from PyQtGraph pyqtgraph/icons/__init__.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/icons/graphIcons.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtGui/QPixmap>

namespace cppqtgraph::icons {
namespace {

#include "auto_icon_data.inc"

QPixmap loadAutoPixmap(const QSize& size)
{
    const QByteArray bytes(reinterpret_cast<const char*>(src_cppqtgraph_icons_auto_png),
        static_cast<int>(src_cppqtgraph_icons_auto_png_len));
    QPixmap pixmap;
    if (!pixmap.loadFromData(bytes, "PNG")) {
        return {};
    }
    if (!size.isValid()) {
        return pixmap;
    }
    return pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

} // namespace

QPixmap getGraphPixmap(const QString& name, const QSize& size)
{
    if (name == QStringLiteral("auto")) {
        return loadAutoPixmap(size);
    }
    return {};
}

} // namespace cppqtgraph::icons
