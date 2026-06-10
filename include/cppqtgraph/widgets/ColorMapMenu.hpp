#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/colormap.hpp>

#include <QtWidgets/QMenu>

#include <optional>
#include <vector>

namespace cppqtgraph::widgets {

struct ColorMapMenuActionData {
    QString name;
    QString source;
    std::optional<cppqtgraph::ColorMap> embeddedMap;
};

struct ColorMapMenuSpecifier {
    QString name;
    std::optional<QString> source;
    std::optional<cppqtgraph::ColorMap> map;
};

class ColorMapMenu : public QMenu {
    Q_OBJECT

public:
    explicit ColorMapMenu(QWidget* parent = nullptr,
        const std::vector<ColorMapMenuSpecifier>& userList = {},
        bool showGradientSubMenu = false,
        bool showColorMapSubMenus = false);

    [[nodiscard]] static cppqtgraph::ColorMap actionDataToColorMap(const ColorMapMenuActionData& data);

public slots:
    void onTriggered(QAction* action);
    void buildLocalSubMenu();

signals:
    void sigColorMapTriggered(const cppqtgraph::ColorMap& colorMap);

private:
    void addMenuEntryAction(QMenu* menu, const QString& name, const ColorMapMenuActionData& data);
    void populateLocalSubMenu(QMenu* menu);
    void populateSubMenu(QMenu* menu, const QStringList& names, const QString& source, bool sortNames);

    bool showColorMapSubMenus_{false};
    QMenu* localSubMenu_{nullptr};
    bool localSubMenuBuilt_{false};
};

} // namespace cppqtgraph::widgets

Q_DECLARE_METATYPE(cppqtgraph::widgets::ColorMapMenuActionData)
