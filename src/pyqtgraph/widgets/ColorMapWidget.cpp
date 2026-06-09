// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/ColorMapWidget.hpp"

#include <QtCore/QRegularExpression>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <algorithm>
#include <cmath>
#include <limits>

namespace pyqtgraph::widgets {
namespace {

double clip01(double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

std::array<double, 4> colorToFloat(const QColor& color)
{
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

std::array<std::uint8_t, 4> floatToByte(const std::array<double, 4>& color)
{
    return {static_cast<std::uint8_t>(clip01(color[0]) * 255.0),
        static_cast<std::uint8_t>(clip01(color[1]) * 255.0),
        static_cast<std::uint8_t>(clip01(color[2]) * 255.0),
        static_cast<std::uint8_t>(clip01(color[3]) * 255.0)};
}

QVariantMap channelsToVariant(const ColorMapChannels& channels)
{
    QVariantMap channelState;
    channelState.insert(QStringLiteral("Red"), channels.red);
    channelState.insert(QStringLiteral("Green"), channels.green);
    channelState.insert(QStringLiteral("Blue"), channels.blue);
    channelState.insert(QStringLiteral("Alpha"), channels.alpha);
    return channelState;
}

ColorMapChannels channelsFromVariant(const QVariantMap& channelState)
{
    ColorMapChannels channels;
    channels.red = channelState.value(QStringLiteral("Red"), true).toBool();
    channels.green = channelState.value(QStringLiteral("Green"), true).toBool();
    channels.blue = channelState.value(QStringLiteral("Blue"), true).toBool();
    channels.alpha = channelState.value(QStringLiteral("Alpha"), true).toBool();
    return channels;
}

QImage lookupStripImage(const pyqtgraph::ColorMap& colorMap, int width)
{
    const auto lut = colorMap.getLookupTable(0.0, 1.0, static_cast<std::size_t>(width), true, pyqtgraph::ColorMap::OutputMode::Byte);
    if (lut.bytes.empty() || lut.rows() == 0 || lut.channels < 3) {
        return {};
    }

    QImage image(width, 1, QImage::Format_RGBA8888);
    for (int x = 0; x < width; ++x) {
        const std::size_t offset = static_cast<std::size_t>(x) * lut.channels;
        const QColor color(lut.bytes[offset],
            lut.bytes[offset + 1],
            lut.bytes[offset + 2],
            lut.channels >= 4 ? lut.bytes[offset + 3] : 255);
        image.setPixelColor(x, 0, color);
    }
    return image;
}

} // namespace

ColorMapWidget::ColorMapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(120, 48);
}

void ColorMapWidget::setFields(const QVector<QPair<QString, ColorMapFieldOptions>>& fields)
{
    fields_.clear();
    for (const auto& [name, options] : fields) {
        fields_.insert(name, options);
    }
    emitMapChanged();
}

QVector<QString> ColorMapWidget::fieldNames() const
{
    return fields_.keys();
}

RangeColorMapMapping* ColorMapWidget::addColorMap(const QString& fieldName, const QString& name)
{
    const auto fieldIt = fields_.constFind(fieldName);
    if (fieldIt == fields_.cend()) {
        return nullptr;
    }

    const ColorMapFieldOptions& options = fieldIt.value();
    QString mappingName = name.isEmpty() ? fieldName : name;
    if (mappingNameExists(mappingName)) {
        mappingName = incrementMappingName(mappingName);
    }
    if (options.mode == QStringLiteral("enum")) {
        EnumColorMapMapping mapping;
        mapping.name = mappingName;
        mapping.fieldName = fieldName;
        for (double value : options.values) {
            mapping.valueColors.push_back({value, QColor(128, 128, 128)});
        }
        applyDefaults(mapping, options);
        enumMappings_.push_back(mapping);
        mappingOrder_.push_back(mappingName);
        update();
        emitMapChanged();
        return nullptr;
    }

    RangeColorMapMapping mapping;
    mapping.name = mappingName;
    mapping.fieldName = fieldName;
    applyDefaults(mapping, options);
    rangeMappings_.push_back(mapping);
    mappingOrder_.push_back(mappingName);
    update();
    emitMapChanged();
    return &rangeMappings_.back();
}

void ColorMapWidget::applyDefaults(RangeColorMapMapping& mapping, const ColorMapFieldOptions& options) const
{
    for (auto it = options.defaults.cbegin(); it != options.defaults.cend(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        if (key == QStringLiteral("colormap") && value.typeId() == QMetaType::QVariantMap) {
            const QVariantMap colorMapState = value.toMap();
            std::vector<double> positions;
            std::vector<QColor> colors;
            for (const QVariant& position : colorMapState.value(QStringLiteral("positions")).toList()) {
                positions.push_back(position.toDouble());
            }
            for (const QVariant& color : colorMapState.value(QStringLiteral("colors")).toList()) {
                colors.push_back(color.value<QColor>());
            }
            mapping.colorMap = pyqtgraph::ColorMap(std::move(positions),
                std::move(colors),
                colorMapState.value(QStringLiteral("name")).toString());
        } else if (key == QStringLiteral("Operation")) {
            mapping.operation = operationFromString(value.toString());
        } else if (key == QStringLiteral("Enabled")) {
            mapping.enabled = value.toBool();
        } else if (key == QStringLiteral("Min")) {
            mapping.minValue = value.toDouble();
        } else if (key == QStringLiteral("Max")) {
            mapping.maxValue = value.toDouble();
        } else if (key == QStringLiteral("NaN")) {
            mapping.nanColor = value.value<QColor>();
        } else if (key == QStringLiteral("Channels") && value.typeId() == QMetaType::QVariantMap) {
            mapping.channels = channelsFromVariant(value.toMap());
        }
    }
}

void ColorMapWidget::applyDefaults(EnumColorMapMapping& mapping, const ColorMapFieldOptions& options) const
{
    for (auto it = options.defaults.cbegin(); it != options.defaults.cend(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        if (key == QStringLiteral("colormap") && value.typeId() == QMetaType::QVariantList) {
            const QVariantList colors = value.toList();
            for (int index = 0; index < colors.size() && index < mapping.valueColors.size(); ++index) {
                mapping.valueColors[index].second = colors[index].value<QColor>();
            }
        } else if (key == QStringLiteral("Operation")) {
            mapping.operation = operationFromString(value.toString());
        } else if (key == QStringLiteral("Enabled")) {
            mapping.enabled = value.toBool();
        } else if (key == QStringLiteral("Default")) {
            mapping.defaultColor = value.value<QColor>();
        } else if (key == QStringLiteral("Channels") && value.typeId() == QMetaType::QVariantMap) {
            mapping.channels = channelsFromVariant(value.toMap());
        }
    }
}

ColorMapOperation ColorMapWidget::operationFromString(const QString& value)
{
    if (value == QStringLiteral("Add")) {
        return ColorMapOperation::Add;
    }
    if (value == QStringLiteral("Multiply")) {
        return ColorMapOperation::Multiply;
    }
    if (value == QStringLiteral("Set")) {
        return ColorMapOperation::Set;
    }
    return ColorMapOperation::Overlay;
}

QString ColorMapWidget::operationToString(ColorMapOperation operation)
{
    switch (operation) {
    case ColorMapOperation::Add:
        return QStringLiteral("Add");
    case ColorMapOperation::Multiply:
        return QStringLiteral("Multiply");
    case ColorMapOperation::Set:
        return QStringLiteral("Set");
    case ColorMapOperation::Overlay:
    default:
        return QStringLiteral("Overlay");
    }
}

void ColorMapWidget::combineColors(std::array<double, 4>& colors,
    const std::array<double, 4>& incoming,
    const ColorMapChannels& channels,
    ColorMapOperation operation)
{
    const auto applyChannel = [&](int index, double combined) {
        if ((index == 0 && channels.red) || (index == 1 && channels.green) || (index == 2 && channels.blue)
            || (index == 3 && channels.alpha)) {
            colors[static_cast<std::size_t>(index)] = combined;
        }
    };

    switch (operation) {
    case ColorMapOperation::Add:
        for (int index = 0; index < 4; ++index) {
            applyChannel(index, colors[static_cast<std::size_t>(index)] + incoming[static_cast<std::size_t>(index)]);
        }
        break;
    case ColorMapOperation::Multiply:
        for (int index = 0; index < 4; ++index) {
            applyChannel(index, colors[static_cast<std::size_t>(index)] * incoming[static_cast<std::size_t>(index)]);
        }
        break;
    case ColorMapOperation::Set:
        for (int index = 0; index < 4; ++index) {
            applyChannel(index, incoming[static_cast<std::size_t>(index)]);
        }
        break;
    case ColorMapOperation::Overlay:
    default: {
        const double alpha = incoming[3];
        for (int index = 0; index < 3; ++index) {
            colors[static_cast<std::size_t>(index)] = colors[static_cast<std::size_t>(index)] * (1.0 - alpha)
                + incoming[static_cast<std::size_t>(index)] * alpha;
        }
        colors[3] = colors[3] + (1.0 - colors[3]) * alpha;
        break;
    }
    }
}

std::vector<std::array<double, 4>> ColorMapWidget::map(const ColorMapRecordArray& data, ColorMapOutputMode mode) const
{
    std::vector<std::array<double, 4>> colors(static_cast<std::size_t>(data.size()), {0.0, 0.0, 0.0, 0.0});

    for (const QString& mappingName : mappingOrder_) {
        if (const RangeColorMapMapping* rangeMapping = findRangeMapping(mappingName)) {
            if (!rangeMapping->enabled) {
                continue;
            }
            const RangeColorMapMapping& mapping = *rangeMapping;
            const double span = mapping.maxValue - mapping.minValue;
            std::vector<std::array<double, 4>> mapped(colors.size(), {0.0, 0.0, 0.0, 0.0});
            for (int row = 0; row < data.size(); ++row) {
                const auto valueIt = data[row].constFind(mapping.fieldName);
                if (valueIt == data[row].cend()) {
                    continue;
                }
                const double raw = valueIt.value();
                if (!std::isfinite(raw)) {
                    mapped[static_cast<std::size_t>(row)] = colorToFloat(mapping.nanColor);
                    continue;
                }
                const double scaled = span == 0.0 ? 0.0 : clip01((raw - mapping.minValue) / span);
                mapped[static_cast<std::size_t>(row)] = mapping.colorMap.mapToFloat(scaled);
            }
            for (std::size_t row = 0; row < colors.size(); ++row) {
                combineColors(colors[row], mapped[row], mapping.channels, mapping.operation);
            }
            continue;
        }
        if (const EnumColorMapMapping* enumMapping = findEnumMapping(mappingName)) {
            if (!enumMapping->enabled) {
                continue;
            }
            const EnumColorMapMapping& mapping = *enumMapping;
            std::vector<std::array<double, 4>> mapped(colors.size(), colorToFloat(mapping.defaultColor));
            for (int row = 0; row < data.size(); ++row) {
                const auto valueIt = data[row].constFind(mapping.fieldName);
                if (valueIt == data[row].cend()) {
                    continue;
                }
                const double raw = valueIt.value();
                for (const auto& [maskValue, color] : mapping.valueColors) {
                    if (raw == maskValue) {
                        mapped[static_cast<std::size_t>(row)] = colorToFloat(color);
                        break;
                    }
                }
            }
            for (std::size_t row = 0; row < colors.size(); ++row) {
                combineColors(colors[row], mapped[row], mapping.channels, mapping.operation);
            }
        }
    }

    for (auto& color : colors) {
        for (double& channel : color) {
            channel = clip01(channel);
        }
    }

    if (mode == ColorMapOutputMode::Byte) {
        for (auto& color : colors) {
            const auto bytes = floatToByte(color);
            for (int index = 0; index < 4; ++index) {
                color[static_cast<std::size_t>(index)] = static_cast<double>(bytes[static_cast<std::size_t>(index)]) / 255.0;
            }
        }
    }

    return colors;
}

std::vector<std::array<std::uint8_t, 4>> ColorMapWidget::mapBytes(const ColorMapRecordArray& data) const
{
    const auto floats = map(data, ColorMapOutputMode::Float);
    std::vector<std::array<std::uint8_t, 4>> bytes(floats.size());
    for (std::size_t index = 0; index < floats.size(); ++index) {
        bytes[index] = floatToByte(floats[index]);
    }
    return bytes;
}

QVariantMap ColorMapWidget::saveState() const
{
    QVariantMap items;
    for (const RangeColorMapMapping& mapping : rangeMappings_) {
        QVariantMap itemState;
        itemState.insert(QStringLiteral("field"), mapping.fieldName);
        itemState.insert(QStringLiteral("mode"), QStringLiteral("range"));
        itemState.insert(QStringLiteral("Min"), mapping.minValue);
        itemState.insert(QStringLiteral("Max"), mapping.maxValue);
        itemState.insert(QStringLiteral("Operation"), operationToString(mapping.operation));
        itemState.insert(QStringLiteral("Enabled"), mapping.enabled);
        itemState.insert(QStringLiteral("NaN"), mapping.nanColor);
        itemState.insert(QStringLiteral("Channels"), channelsToVariant(mapping.channels));
        QVariantList positions;
        QVariantList colors;
        for (double position : mapping.colorMap.positions()) {
            positions.push_back(position);
        }
        for (const QColor& color : mapping.colorMap.colors()) {
            colors.push_back(color);
        }
        QVariantMap colorMapState;
        colorMapState.insert(QStringLiteral("positions"), positions);
        colorMapState.insert(QStringLiteral("colors"), colors);
        colorMapState.insert(QStringLiteral("name"), mapping.colorMap.name());
        itemState.insert(QStringLiteral("colormap"), colorMapState);
        items.insert(mapping.name, itemState);
    }
    for (const EnumColorMapMapping& mapping : enumMappings_) {
        QVariantMap itemState;
        itemState.insert(QStringLiteral("field"), mapping.fieldName);
        itemState.insert(QStringLiteral("mode"), QStringLiteral("enum"));
        itemState.insert(QStringLiteral("Operation"), operationToString(mapping.operation));
        itemState.insert(QStringLiteral("Enabled"), mapping.enabled);
        itemState.insert(QStringLiteral("Default"), mapping.defaultColor);
        itemState.insert(QStringLiteral("Channels"), channelsToVariant(mapping.channels));
        QVariantList values;
        for (const auto& [maskValue, color] : mapping.valueColors) {
            QVariantMap entry;
            entry.insert(QStringLiteral("maskValue"), maskValue);
            entry.insert(QStringLiteral("color"), color);
            values.push_back(entry);
        }
        itemState.insert(QStringLiteral("values"), values);
        items.insert(mapping.name, itemState);
    }

    QVariantMap fields;
    for (auto it = fields_.cbegin(); it != fields_.cend(); ++it) {
        QVariantMap fieldState;
        fieldState.insert(QStringLiteral("mode"), it.value().mode);
        fieldState.insert(QStringLiteral("units"), it.value().units);
        QVariantList values;
        for (double value : it.value().values) {
            values.push_back(value);
        }
        fieldState.insert(QStringLiteral("values"), values);
        fields.insert(it.key(), fieldState);
    }

    QVariantList itemOrder;
    for (const QString& name : mappingOrder_) {
        itemOrder.push_back(name);
    }

    QVariantMap state;
    state.insert(QStringLiteral("fields"), fields);
    state.insert(QStringLiteral("items"), items);
    state.insert(QStringLiteral("itemOrder"), itemOrder);
    return state;
}

void ColorMapWidget::restoreState(const QVariantMap& state)
{
    rangeMappings_.clear();
    enumMappings_.clear();
    mappingOrder_.clear();

    const QVariantMap fields = state.value(QStringLiteral("fields")).toMap();
    QVector<QPair<QString, ColorMapFieldOptions>> fieldList;
    for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
        const QVariantMap fieldState = it.value().toMap();
        ColorMapFieldOptions options;
        options.mode = fieldState.value(QStringLiteral("mode"), QStringLiteral("range")).toString();
        options.units = fieldState.value(QStringLiteral("units")).toString();
        for (const QVariant& value : fieldState.value(QStringLiteral("values")).toList()) {
            options.values.push_back(value.toDouble());
        }
        fieldList.push_back({it.key(), options});
    }
    setFields(fieldList);

    const QVariantMap items = state.value(QStringLiteral("items")).toMap();
    QStringList orderedItemNames;
    const QVariantList itemOrder = state.value(QStringLiteral("itemOrder")).toList();
    if (!itemOrder.isEmpty()) {
        for (const QVariant& itemName : itemOrder) {
            orderedItemNames.push_back(itemName.toString());
        }
    } else {
        for (auto it = items.cbegin(); it != items.cend(); ++it) {
            orderedItemNames.push_back(it.key());
        }
    }
    for (const QString& itemName : orderedItemNames) {
        const auto itemIt = items.constFind(itemName);
        if (itemIt == items.cend()) {
            continue;
        }
        const QVariantMap itemState = itemIt.value().toMap();
        const QString fieldName = itemState.value(QStringLiteral("field")).toString();
        addColorMap(fieldName, itemName);
        if (itemState.value(QStringLiteral("mode")).toString() == QStringLiteral("enum")) {
            EnumColorMapMapping* mapping = findEnumMapping(itemName);
            if (mapping == nullptr) {
                continue;
            }
            mapping->operation = operationFromString(itemState.value(QStringLiteral("Operation")).toString());
            mapping->enabled = itemState.value(QStringLiteral("Enabled"), true).toBool();
            mapping->defaultColor = itemState.value(QStringLiteral("Default")).value<QColor>();
            if (itemState.contains(QStringLiteral("Channels"))) {
                mapping->channels = channelsFromVariant(itemState.value(QStringLiteral("Channels")).toMap());
            }
            mapping->valueColors.clear();
            for (const QVariant& entryVariant : itemState.value(QStringLiteral("values")).toList()) {
                const QVariantMap entry = entryVariant.toMap();
                mapping->valueColors.push_back({entry.value(QStringLiteral("maskValue")).toDouble(),
                    entry.value(QStringLiteral("color")).value<QColor>()});
            }
        } else {
            RangeColorMapMapping* mapping = findRangeMapping(itemName);
            if (mapping == nullptr) {
                continue;
            }
            mapping->minValue = itemState.value(QStringLiteral("Min")).toDouble();
            mapping->maxValue = itemState.value(QStringLiteral("Max")).toDouble();
            mapping->operation = operationFromString(itemState.value(QStringLiteral("Operation")).toString());
            mapping->enabled = itemState.value(QStringLiteral("Enabled"), true).toBool();
            if (itemState.contains(QStringLiteral("NaN"))) {
                mapping->nanColor = itemState.value(QStringLiteral("NaN")).value<QColor>();
            }
            if (itemState.contains(QStringLiteral("Channels"))) {
                mapping->channels = channelsFromVariant(itemState.value(QStringLiteral("Channels")).toMap());
            }
            if (itemState.contains(QStringLiteral("colormap"))) {
                const QVariantMap colorMapState = itemState.value(QStringLiteral("colormap")).toMap();
                std::vector<double> positions;
                std::vector<QColor> colors;
                for (const QVariant& position : colorMapState.value(QStringLiteral("positions")).toList()) {
                    positions.push_back(position.toDouble());
                }
                for (const QVariant& color : colorMapState.value(QStringLiteral("colors")).toList()) {
                    colors.push_back(color.value<QColor>());
                }
                mapping->colorMap = pyqtgraph::ColorMap(std::move(positions),
                    std::move(colors),
                    colorMapState.value(QStringLiteral("name")).toString());
            }
        }
    }
    update();
    emitMapChanged();
}

void ColorMapWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());

    const int stripHeight = 16;
    int y = 4;
    const int stripWidth = std::max(32, width() - 8);

    for (const RangeColorMapMapping& mapping : rangeMappings_) {
        if (!mapping.enabled) {
            continue;
        }
        const QRect stripRect(4, y, stripWidth, stripHeight);
        const QImage image = lookupStripImage(mapping.colorMap, stripWidth);
        if (!image.isNull()) {
            painter.drawImage(stripRect, image.scaled(stripRect.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
        } else {
            painter.fillRect(stripRect, Qt::darkGray);
        }
        painter.setPen(Qt::white);
        painter.drawText(stripRect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, mapping.name);
        y += stripHeight + 4;
    }

    for (const EnumColorMapMapping& mapping : enumMappings_) {
        if (!mapping.enabled) {
            continue;
        }
        int x = 4;
        for (const auto& [maskValue, color] : mapping.valueColors) {
            Q_UNUSED(maskValue);
            const QRect swatchRect(x, y, stripHeight, stripHeight);
            painter.fillRect(swatchRect, color);
            painter.setPen(Qt::white);
            painter.drawRect(swatchRect);
            x += stripHeight + 2;
        }
        painter.drawText(QRect(x + 4, y, stripWidth - x, stripHeight), Qt::AlignVCenter | Qt::AlignLeft, mapping.name);
        y += stripHeight + 4;
    }
}

void ColorMapWidget::emitMapChanged()
{
    Q_EMIT sigColorMapChanged(this);
}

bool ColorMapWidget::mappingNameExists(const QString& name) const
{
    for (const RangeColorMapMapping& mapping : rangeMappings_) {
        if (mapping.name == name) {
            return true;
        }
    }
    for (const EnumColorMapMapping& mapping : enumMappings_) {
        if (mapping.name == name) {
            return true;
        }
    }
    return false;
}

QString ColorMapWidget::incrementMappingName(const QString& name) const
{
    static const QRegularExpression pattern(QStringLiteral("^([^\\d]*)(\\d*)$"));
    const QRegularExpressionMatch match = pattern.match(name);
    const QString base = match.captured(1);
    const QString numString = match.captured(2);
    int numLength = numString.size();
    int number = 0;
    if (numLength == 0) {
        number = 2;
        numLength = 1;
    } else {
        number = numString.toInt();
    }

    while (true) {
        const QString candidate = base + QStringLiteral("%1").arg(number, numLength, 10, QChar('0'));
        if (!mappingNameExists(candidate)) {
            return candidate;
        }
        ++number;
    }
}

RangeColorMapMapping* ColorMapWidget::findRangeMapping(const QString& name)
{
    for (RangeColorMapMapping& mapping : rangeMappings_) {
        if (mapping.name == name) {
            return &mapping;
        }
    }
    return nullptr;
}

const RangeColorMapMapping* ColorMapWidget::findRangeMapping(const QString& name) const
{
    for (const RangeColorMapMapping& mapping : rangeMappings_) {
        if (mapping.name == name) {
            return &mapping;
        }
    }
    return nullptr;
}

EnumColorMapMapping* ColorMapWidget::findEnumMapping(const QString& name)
{
    for (EnumColorMapMapping& mapping : enumMappings_) {
        if (mapping.name == name) {
            return &mapping;
        }
    }
    return nullptr;
}

const EnumColorMapMapping* ColorMapWidget::findEnumMapping(const QString& name) const
{
    for (const EnumColorMapMapping& mapping : enumMappings_) {
        if (mapping.name == name) {
            return &mapping;
        }
    }
    return nullptr;
}

} // namespace pyqtgraph::widgets
