// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapMenu.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/ColorMapMenu.hpp"

#include <QtCore/QMetaType>
#include <QtCore/QVariant>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidgetAction>

#include <QRegularExpression>

#include <algorithm>
#include <stdexcept>

namespace pyqtgraph::widgets {
namespace {

struct MetaTypeRegistrar {
    MetaTypeRegistrar()
    {
        qRegisterMetaType<ColorMapMenuActionData>();
    }
};

const MetaTypeRegistrar metaTypeRegistrar;


QImage imageFromLookupBytes(const std::vector<std::uint8_t>& bytes, std::size_t rows, std::size_t channels)
{
    if (bytes.empty() || rows == 0 || channels < 3) {
        return {};
    }

    QImage image(static_cast<int>(rows), 1, QImage::Format_RGBA8888);
    for (std::size_t row = 0; row < rows; ++row) {
        const std::size_t offset = row * channels;
        const QColor color(bytes[offset],
            bytes[offset + 1],
            bytes[offset + 2],
            channels >= 4 ? bytes[offset + 3] : 255);
        image.setPixelColor(static_cast<int>(row), 0, color);
    }
    return image;
}

QWidget* buildMenuEntryWidget(const pyqtgraph::ColorMap& colorMap, const QString& text)
{
    const auto lut = colorMap.getLookupTable(0.0, 1.0, 32, true, pyqtgraph::ColorMap::OutputMode::Byte);
    const QImage image = imageFromLookupBytes(lut.bytes, lut.rows(), lut.channels);
    const QPixmap pixmap = QPixmap::fromImage(image);

    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(1, 1, 1, 1);

    auto* preview = new QLabel;
    preview->setScaledContents(true);
    preview->setPixmap(pixmap);

    auto* label = new QLabel(text);
    layout->addWidget(preview, 0);
    layout->addWidget(label, 1);
    return widget;
}

pyqtgraph::ColorMap resolveColorMap(const QString& name, const QString& source, const std::optional<pyqtgraph::ColorMap>& embeddedMap)
{
    if (embeddedMap.has_value()) {
        return *embeddedMap;
    }
    if (name.isEmpty()) {
        return pyqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
    }
    if (const auto resolved = pyqtgraph::get(name, source)) {
        return *resolved;
    }
    throw std::runtime_error("ColorMapMenu could not resolve colormap");
}

QStringList sortedFilenames(const QStringList& names)
{
    static const QRegularExpression pattern(QStringLiteral("(\\d+)"));
    QStringList sorted = names;
    std::sort(sorted.begin(), sorted.end(), [](const QString& left, const QString& right) {
        const QStringList leftParts = left.split(pattern);
        const QStringList rightParts = right.split(pattern);
        const int count = std::min(leftParts.size(), rightParts.size());
        for (int index = 0; index < count; ++index) {
            bool leftOk = false;
            bool rightOk = false;
            const int leftNumber = leftParts[index].toInt(&leftOk);
            const int rightNumber = rightParts[index].toInt(&rightOk);
            if (leftOk && rightOk && leftNumber != rightNumber) {
                return leftNumber < rightNumber;
            }
            if (leftParts[index] != rightParts[index]) {
                return leftParts[index] < rightParts[index];
            }
        }
        return left.size() < right.size();
    });
    return sorted;
}

} // namespace

ColorMapMenu::ColorMapMenu(QWidget* parent,
    const std::vector<ColorMapMenuSpecifier>& userList,
    bool /*showGradientSubMenu*/,
    bool showColorMapSubMenus)
    : QMenu(parent)
    , showColorMapSubMenus_(showColorMapSubMenus)
{
    setTitle(QStringLiteral("ColorMaps"));
    connect(this, &QMenu::triggered, this, &ColorMapMenu::onTriggered);

    auto* noneAction = addAction(QStringLiteral("None"));
    ColorMapMenuActionData noneData;
    noneAction->setData(QVariant::fromValue(noneData));

    for (const ColorMapMenuSpecifier& item : userList) {
        ColorMapMenuActionData data;
        if (item.map.has_value()) {
            data.name = item.name.isEmpty() ? item.map->name() : item.name;
            data.embeddedMap = item.map;
        } else {
            data.name = item.name;
            data.source = item.source.value_or(QString());
        }
        addMenuEntryAction(this, data.name.isEmpty() ? QStringLiteral("custom") : data.name, data);
    }

    if (showColorMapSubMenus_) {
        addSeparator();
        localSubMenu_ = addMenu(QStringLiteral("local"));
        connect(localSubMenu_, &QMenu::aboutToShow, this, &ColorMapMenu::buildLocalSubMenu);
    }
}

pyqtgraph::ColorMap ColorMapMenu::actionDataToColorMap(const ColorMapMenuActionData& data)
{
    if (data.embeddedMap.has_value()) {
        return *data.embeddedMap;
    }
    if (data.name.isEmpty()) {
        return pyqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
    }
    return resolveColorMap(data.name, data.source, std::nullopt);
}

void ColorMapMenu::onTriggered(QAction* action)
{
    if (action == nullptr || !action->data().isValid()) {
        return;
    }
    const QVariant variant = action->data();
    if (!variant.canConvert<ColorMapMenuActionData>()) {
        return;
    }
    const auto data = variant.value<ColorMapMenuActionData>();
    emit sigColorMapTriggered(actionDataToColorMap(data));
}

void ColorMapMenu::buildLocalSubMenu()
{
    if (localSubMenuBuilt_ || localSubMenu_ == nullptr) {
        return;
    }
    populateLocalSubMenu(localSubMenu_);
    localSubMenuBuilt_ = true;
    disconnect(localSubMenu_, &QMenu::aboutToShow, this, &ColorMapMenu::buildLocalSubMenu);
}

void ColorMapMenu::populateLocalSubMenu(QMenu* menu)
{
    if (menu == nullptr) {
        return;
    }

    QStringList names;
    for (const QString& name : pyqtgraph::listMaps()) {
        if (!name.startsWith(QStringLiteral("CET")) && !name.startsWith(QStringLiteral("PAL-relaxed"))) {
            names.push_back(name);
        }
    }
    if (names.isEmpty()) {
        for (const QString& name : pyqtgraph::listMaps()) {
            if (!name.startsWith(QStringLiteral("CET"))) {
                names.push_back(name);
            }
        }
    }
    populateSubMenu(menu, names, QString(), true);
}

void ColorMapMenu::populateSubMenu(QMenu* menu, const QStringList& names, const QString& source, bool sortNames)
{
    if (menu == nullptr) {
        return;
    }

    const QStringList resolvedNames = sortNames ? sortedFilenames(names) : names;
    for (const QString& name : resolvedNames) {
        ColorMapMenuActionData data;
        data.name = name;
        data.source = source;
        addMenuEntryAction(menu, name, data);
    }
}

void ColorMapMenu::addMenuEntryAction(QMenu* menu, const QString& name, const ColorMapMenuActionData& data)
{
    if (menu == nullptr) {
        return;
    }

    const pyqtgraph::ColorMap previewMap = actionDataToColorMap(data);
    auto* action = new QWidgetAction(menu);
    action->setData(QVariant::fromValue(data));
    action->setDefaultWidget(buildMenuEntryWidget(previewMap, name));
    menu->addAction(action);
}

} // namespace pyqtgraph::widgets
