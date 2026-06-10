#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/colormap.hpp>

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

class QPaintEvent;

namespace cppqtgraph::widgets {

enum class ColorMapOperation {
    Overlay,
    Add,
    Multiply,
    Set,
};

enum class ColorMapOutputMode {
    Byte,
    Float,
};

struct ColorMapChannels final {
    bool red = true;
    bool green = true;
    bool blue = true;
    bool alpha = true;
};

struct ColorMapFieldOptions final {
    QString mode{QStringLiteral("range")};
    QString units;
    QVector<double> values;
    QMap<QString, QVariant> defaults;
};

struct RangeColorMapMapping final {
    QString name;
    QString fieldName;
    double minValue = 0.0;
    double maxValue = 1.0;
    ColorMapOperation operation = ColorMapOperation::Overlay;
    ColorMapChannels channels;
    bool enabled = true;
    QColor nanColor = QColor(128, 128, 128);
    cppqtgraph::ColorMap colorMap = cppqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
};

struct EnumColorMapMapping final {
    QString name;
    QString fieldName;
    ColorMapOperation operation = ColorMapOperation::Overlay;
    ColorMapChannels channels;
    bool enabled = true;
    QColor defaultColor = QColor(128, 128, 128);
    QVector<QPair<double, QColor>> valueColors;
};

using ColorMapRecord = QMap<QString, double>;
using ColorMapRecordArray = QVector<ColorMapRecord>;

class ColorMapWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ColorMapWidget(QWidget* parent = nullptr);

    void setFields(const QVector<QPair<QString, ColorMapFieldOptions>>& fields);
    [[nodiscard]] QVector<QString> fieldNames() const;

    RangeColorMapMapping* addColorMap(const QString& fieldName, const QString& name = {});
    [[nodiscard]] const QVector<RangeColorMapMapping>& rangeMappings() const noexcept { return rangeMappings_; }
    [[nodiscard]] const QVector<EnumColorMapMapping>& enumMappings() const noexcept { return enumMappings_; }

    [[nodiscard]] std::vector<std::array<double, 4>> map(const ColorMapRecordArray& data,
        ColorMapOutputMode mode = ColorMapOutputMode::Byte) const;
    [[nodiscard]] std::vector<std::array<std::uint8_t, 4>> mapBytes(const ColorMapRecordArray& data) const;

    [[nodiscard]] QVariantMap saveState() const;
    void restoreState(const QVariantMap& state);

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void sigColorMapChanged(ColorMapWidget* widget);

private:
    void emitMapChanged();
    [[nodiscard]] RangeColorMapMapping* findRangeMapping(const QString& name);
    [[nodiscard]] EnumColorMapMapping* findEnumMapping(const QString& name);
    [[nodiscard]] const RangeColorMapMapping* findRangeMapping(const QString& name) const;
    [[nodiscard]] const EnumColorMapMapping* findEnumMapping(const QString& name) const;
    void applyDefaults(RangeColorMapMapping& mapping, const ColorMapFieldOptions& options) const;
    void applyDefaults(EnumColorMapMapping& mapping, const ColorMapFieldOptions& options) const;
    [[nodiscard]] bool mappingNameExists(const QString& name) const;
    [[nodiscard]] QString incrementMappingName(const QString& name) const;
    static ColorMapOperation operationFromString(const QString& value);
    static QString operationToString(ColorMapOperation operation);
    static void combineColors(std::array<double, 4>& colors,
        const std::array<double, 4>& incoming,
        const ColorMapChannels& channels,
        ColorMapOperation operation);

    QMap<QString, ColorMapFieldOptions> fields_;
    QVector<RangeColorMapMapping> rangeMappings_;
    QVector<EnumColorMapMapping> enumMappings_;
    QVector<QString> mappingOrder_;
};

} // namespace cppqtgraph::widgets
