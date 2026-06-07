// Source note: translated/adapted from PyQtGraph pyqtgraph/colors/palette.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/colors/palette.hpp"

#include <QColor>
#include <QPalette>
#include <QString>

namespace pyqtgraph::colors {
namespace {

QColor hex(const char* value)
{
    return QColor(QString::fromLatin1(value));
}

} // namespace

QPalette getQDarkStyleDarkQPalette()
{
    const QColor bgDark = hex("#19232D");
    const QColor bgNormal = hex("#37414F");
    const QColor bgLight = hex("#455364");
    const QColor fgDark = hex("#9DA9B5");
    const QColor fgNormal = hex("#E0E1E3");
    const QColor fgLight = hex("#F0F0F0");
    const QColor selLight = hex("#346792");

    QPalette palette(bgDark);
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        palette.setColor(group, QPalette::Base, bgDark);
        palette.setColor(group, QPalette::Window, bgDark);
        palette.setColor(group, QPalette::WindowText, fgNormal);
        palette.setColor(group, QPalette::AlternateBase, bgLight);
        palette.setColor(group, QPalette::Button, bgLight);
        palette.setColor(group, QPalette::ButtonText, fgLight);
        palette.setColor(group, QPalette::Highlight, selLight);
        palette.setColor(group, QPalette::HighlightedText, fgLight);
        palette.setColor(group, QPalette::Text, fgLight);
        palette.setColor(group, QPalette::ToolTipBase, bgLight);
        palette.setColor(group, QPalette::ToolTipText, fgLight);
    }
    palette.setColor(QPalette::Disabled, QPalette::Base, bgNormal);
    palette.setColor(QPalette::Disabled, QPalette::Button, bgNormal);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, fgDark);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, fgDark);
    palette.setColor(QPalette::Disabled, QPalette::Text, fgDark);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, bgLight);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, fgDark);
    return palette;
}

QPalette getQDarkStyleLightQPalette()
{
    const QColor bgDark = hex("#ACB1B6");
    const QColor bgNormal = hex("#E0E1E3");
    const QColor bgLight = hex("#FAFAFA");
    const QColor fgDark = hex("#19223D");
    const QColor fgNormal = hex("#293544");
    const QColor fgLight = hex("#788D9C");
    const QColor selLight = hex("#9FCBFF");

    QPalette palette(bgLight);
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        palette.setColor(group, QPalette::Base, bgLight);
        palette.setColor(group, QPalette::Window, bgLight);
        palette.setColor(group, QPalette::WindowText, fgNormal);
        palette.setColor(group, QPalette::AlternateBase, bgNormal);
        palette.setColor(group, QPalette::Button, bgNormal);
        palette.setColor(group, QPalette::ButtonText, fgDark);
        palette.setColor(group, QPalette::Highlight, selLight);
        palette.setColor(group, QPalette::HighlightedText, fgDark);
        palette.setColor(group, QPalette::Text, fgDark);
        palette.setColor(group, QPalette::ToolTipBase, bgDark);
        palette.setColor(group, QPalette::ToolTipText, fgDark);
    }
    palette.setColor(QPalette::Disabled, QPalette::Base, bgNormal);
    palette.setColor(QPalette::Disabled, QPalette::Button, bgNormal);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, fgLight);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, fgLight);
    palette.setColor(QPalette::Disabled, QPalette::Text, fgLight);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, selLight);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, fgNormal);
    return palette;
}

} // namespace pyqtgraph::colors
