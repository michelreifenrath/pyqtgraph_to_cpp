// Source note: translated/adapted from PyQtGraph pyqtgraph/WidgetGroup.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../include/cppqtgraph/WidgetGroup.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

#include <algorithm>
#include <stdexcept>

namespace cppqtgraph {
namespace {

bool isNumericVariant(const QVariant& value)
{
    bool ok = false;
    (void)value.toDouble(&ok);
    return ok;
}

} // namespace

WidgetGroup::WidgetGroup(QObject* parent)
    : QObject(parent)
{
}

WidgetGroup::~WidgetGroup() = default;

bool WidgetGroup::acceptsType(const QObject* object) const
{
    return qobject_cast<const QSpinBox*>(object) != nullptr
        || qobject_cast<const QDoubleSpinBox*>(object) != nullptr
        || qobject_cast<const QSplitter*>(object) != nullptr
        || qobject_cast<const QCheckBox*>(object) != nullptr
        || qobject_cast<const QComboBox*>(object) != nullptr
        || qobject_cast<const QGroupBox*>(object) != nullptr
        || qobject_cast<const QLineEdit*>(object) != nullptr
        || qobject_cast<const QRadioButton*>(object) != nullptr
        || qobject_cast<const QSlider*>(object) != nullptr;
}

void WidgetGroup::addWidget(QWidget* widget, const QString& name, std::optional<double> scale)
{
    if (widget == nullptr) {
        throw std::invalid_argument("Cannot add a null widget to WidgetGroup");
    }
    if (!acceptsType(widget)) {
        throw std::invalid_argument("Widget type not supported by WidgetGroup");
    }

    const QString resolvedName = name.isNull() || name.isEmpty() ? widget->objectName() : name;
    if (resolvedName.isEmpty()) {
        throw std::invalid_argument("Cannot add widget without a name");
    }

    if (Entry* existing = findEntry(widget)) {
        QObject::disconnect(existing->connection);
        cache_.remove(existing->name);
        existing->name = resolvedName;
        existing->scale = scale;
        existing->uncached = false;
        readWidget(widget);
        existing->uncached = !connectChangeSignal(widget, existing->connection);
        return;
    }

    Entry entry;
    entry.widget = widget;
    entry.name = resolvedName;
    entry.scale = scale;
    entries_.push_back(entry);
    readWidget(widget);
    entries_.back().uncached = !connectChangeSignal(widget, entries_.back().connection);
}

QWidget* WidgetGroup::findWidget(const QString& name) const
{
    for (const Entry& entry : entries_) {
        if (!entry.widget.isNull() && entry.name == name) {
            return entry.widget.data();
        }
    }
    return nullptr;
}

WidgetGroup::Entry* WidgetGroup::findEntry(QWidget* widget)
{
    for (Entry& entry : entries_) {
        if (entry.widget == widget) {
            return &entry;
        }
    }
    return nullptr;
}

const WidgetGroup::Entry* WidgetGroup::findEntry(QWidget* widget) const
{
    for (const Entry& entry : entries_) {
        if (entry.widget == widget) {
            return &entry;
        }
    }
    return nullptr;
}

bool WidgetGroup::checkForChildren(const QObject* object) const
{
    return qobject_cast<const QSplitter*>(object) != nullptr || qobject_cast<const QGroupBox*>(object) != nullptr;
}

void WidgetGroup::autoAdd(QObject* object)
{
    if (object == nullptr) {
        return;
    }

    const bool accepted = acceptsType(object);
    if (accepted) {
        auto* widget = qobject_cast<QWidget*>(object);
        addWidget(widget);
    }

    if (!accepted || checkForChildren(object)) {
        const QObjectList childObjects = object->children();
        for (QObject* child : childObjects) {
            autoAdd(child);
        }
    }
}

bool WidgetGroup::connectChangeSignal(QWidget* widget, QMetaObject::Connection& connection)
{
    if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        connection = QObject::connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
        connection = QObject::connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (qobject_cast<QSplitter*>(widget) != nullptr) {
        connection = QMetaObject::Connection{};
        return false;
    }
    if (auto* check = qobject_cast<QCheckBox*>(widget)) {
        connection = QObject::connect(check, qOverload<int>(&QCheckBox::stateChanged), this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        connection = QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (auto* group = qobject_cast<QGroupBox*>(widget)) {
        connection = QObject::connect(group, &QGroupBox::toggled, this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (auto* line = qobject_cast<QLineEdit*>(widget)) {
        connection = QObject::connect(line, &QLineEdit::editingFinished, this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (auto* radio = qobject_cast<QRadioButton*>(widget)) {
        connection = QObject::connect(radio, &QRadioButton::toggled, this, &WidgetGroup::widgetChanged);
        return true;
    }
    if (auto* slider = qobject_cast<QSlider*>(widget)) {
        connection = QObject::connect(slider, &QSlider::valueChanged, this, &WidgetGroup::widgetChanged);
        return true;
    }

    connection = QMetaObject::Connection{};
    return false;
}

QVariant WidgetGroup::comboState(QComboBox* combo) const
{
    const int index = combo->currentIndex();
    const QVariant data = combo->itemData(index);
    if (data.isValid() && !data.isNull()) {
        return data;
    }
    return combo->itemText(index);
}

void WidgetGroup::setComboState(QComboBox* combo, const QVariant& value) const
{
    if (value.userType() == QMetaType::Int) {
        const int index = combo->findData(value);
        if (index > -1) {
            combo->setCurrentIndex(index);
            return;
        }
    }
    combo->setCurrentIndex(combo->findText(value.toString()));
}

QVariant WidgetGroup::valueForWidget(QWidget* widget) const
{
    if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        return spin->value();
    }
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
        return spin->value();
    }
    if (auto* splitter = qobject_cast<QSplitter*>(widget)) {
        return QString::fromLatin1(splitter->saveState().toPercentEncoding());
    }
    if (auto* check = qobject_cast<QCheckBox*>(widget)) {
        return check->isChecked();
    }
    if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        return comboState(combo);
    }
    if (auto* group = qobject_cast<QGroupBox*>(widget)) {
        return group->isChecked();
    }
    if (auto* line = qobject_cast<QLineEdit*>(widget)) {
        return line->text();
    }
    if (auto* radio = qobject_cast<QRadioButton*>(widget)) {
        return radio->isChecked();
    }
    if (auto* slider = qobject_cast<QSlider*>(widget)) {
        return slider->value();
    }
    return {};
}

QVariant WidgetGroup::readWidget(QWidget* widget)
{
    Entry* entry = findEntry(widget);
    if (entry == nullptr || entry->widget.isNull()) {
        return {};
    }

    QVariant value = valueForWidget(widget);
    if (entry->scale.has_value() && isNumericVariant(value)) {
        value = value.toDouble() / *entry->scale;
    }

    cache_.insert(entry->name, value);
    return value;
}

void WidgetGroup::restoreSplitter(QSplitter* splitter, const QVariant& value) const
{
    if (value.userType() == QMetaType::QVariantList || value.canConvert<QVariantList>()) {
        const QVariantList list = value.toList();
        QList<int> sizes;
        sizes.reserve(list.size());
        for (const QVariant& item : list) {
            sizes.push_back(item.toInt());
        }
        splitter->setSizes(sizes);
    } else if (value.canConvert<QString>()) {
        const QByteArray encoded = value.toString().toUtf8();
        splitter->restoreState(QByteArray::fromPercentEncoding(encoded));
    }

    if (splitter->count() > 0) {
        const QList<int> sizes = splitter->sizes();
        const bool anyVisible = std::any_of(sizes.cbegin(), sizes.cend(), [](int size) { return size > 0; });
        if (!anyVisible) {
            QList<int> fallback;
            fallback.fill(50, splitter->count());
            splitter->setSizes(fallback);
        }
    }
}

void WidgetGroup::setWidget(QWidget* widget, const QVariant& value)
{
    Entry* entry = findEntry(widget);
    if (entry == nullptr || entry->widget.isNull()) {
        return;
    }

    QVariant scaledValue = value;
    if (entry->scale.has_value() && isNumericVariant(scaledValue)) {
        scaledValue = scaledValue.toDouble() * *entry->scale;
    }

    if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        spin->setValue(scaledValue.toInt());
    } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
        spin->setValue(scaledValue.toDouble());
    } else if (auto* splitter = qobject_cast<QSplitter*>(widget)) {
        restoreSplitter(splitter, scaledValue);
    } else if (auto* check = qobject_cast<QCheckBox*>(widget)) {
        check->setChecked(scaledValue.toBool());
    } else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        setComboState(combo, scaledValue);
    } else if (auto* group = qobject_cast<QGroupBox*>(widget)) {
        group->setChecked(scaledValue.toBool());
    } else if (auto* line = qobject_cast<QLineEdit*>(widget)) {
        line->setText(scaledValue.toString());
    } else if (auto* radio = qobject_cast<QRadioButton*>(widget)) {
        radio->setChecked(scaledValue.toBool());
    } else if (auto* slider = qobject_cast<QSlider*>(widget)) {
        slider->setValue(scaledValue.toInt());
    }
}

void WidgetGroup::setScale(QWidget* widget, std::optional<double> scale)
{
    Entry* entry = findEntry(widget);
    if (entry == nullptr) {
        return;
    }
    const QVariant value = readWidget(widget);
    entry->scale = scale;
    setWidget(widget, value);
}

QVariantMap WidgetGroup::state()
{
    for (Entry& entry : entries_) {
        if (entry.uncached && !entry.widget.isNull()) {
            readWidget(entry.widget.data());
        }
    }
    return cache_;
}

void WidgetGroup::setState(const QVariantMap& state)
{
    for (Entry& entry : entries_) {
        if (!entry.widget.isNull() && state.contains(entry.name)) {
            setWidget(entry.widget.data(), state.value(entry.name));
        }
    }
}

void WidgetGroup::widgetChanged()
{
    auto* widget = qobject_cast<QWidget*>(sender());
    Entry* entry = findEntry(widget);
    if (entry == nullptr) {
        return;
    }

    const QVariant previous = cache_.value(entry->name);
    const QVariant current = readWidget(widget);
    if (previous != current) {
        emit sigChanged(entry->name, current);
    }
}

} // namespace cppqtgraph
