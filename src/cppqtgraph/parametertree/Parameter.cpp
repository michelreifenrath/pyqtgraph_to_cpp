// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/Parameter.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/Parameter.hpp"
#include "../../../include/cppqtgraph/parametertree/ParameterItem.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/BoolParameterItem.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/ChecklistParameter.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/NumericParameterItem.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/RadioParameterItem.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/SliderParameter.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/StrParameterItem.hpp"

#include <cppqtgraph/functions.hpp>

#include <QtGui/QColor>
#include <QtWidgets/QLineEdit>

#include <algorithm>
#include <stdexcept>

namespace cppqtgraph::parametertree {

namespace {

QHash<QString, ParameterFactory>& parameterTypeRegistry()
{
    static QHash<QString, ParameterFactory> registry;
    return registry;
}

bool variantEqual(const QVariant& left, const QVariant& right)
{
    return left == right;
}

QVariantMap normalizeListOpts(QVariantMap opts)
{
    if (!opts.contains(QStringLiteral("limits")) && opts.contains(QStringLiteral("values"))) {
        opts.insert(QStringLiteral("limits"), opts.value(QStringLiteral("values")));
        opts.remove(QStringLiteral("values"));
    }
    if (!opts.contains(QStringLiteral("limits"))) {
        opts.insert(QStringLiteral("limits"), QVariantList{});
    }
    return opts;
}

QVariant interpretColorValue(const QVariant& value)
{
    if (!value.isValid()) {
        return value;
    }
    if (value.metaType().id() == QMetaType::QColor) {
        return value;
    }
    return QVariant::fromValue(mkColor(value.toString()));
}

QVariantMap normalizeColorOpts(QVariantMap opts)
{
    if (opts.contains(QStringLiteral("value"))) {
        opts.insert(QStringLiteral("value"), interpretColorValue(opts.value(QStringLiteral("value"))));
    }
    if (opts.contains(QStringLiteral("default"))) {
        opts.insert(QStringLiteral("default"), interpretColorValue(opts.value(QStringLiteral("default"))));
    }
    return opts;
}

QVariantList listLimitValuesFromOpts(const QVariantMap& opts)
{
    const QVariant limits = opts.value(QStringLiteral("limits"));
    if (limits.canConvert<QVariantList>()) {
        QVariantList values;
        for (const QVariant& entry : limits.toList()) {
            if (entry.metaType().id() == QMetaType::QVariantList) {
                const QVariantList pair = entry.toList();
                if (pair.size() >= 2) {
                    values.append(pair.at(1));
                }
            } else {
                values.append(entry);
            }
        }
        return values;
    }
    if (limits.canConvert<QVariantMap>()) {
        QVariantList values;
        for (auto it = limits.toMap().constBegin(); it != limits.toMap().constEnd(); ++it) {
            values.append(it.value());
        }
        return values;
    }
    return {};
}

bool valueInListLimits(const QVariant& value, const QVariantMap& opts)
{
    const QVariantList allowed = listLimitValuesFromOpts(opts);
    if (allowed.isEmpty()) {
        return true;
    }
    for (const QVariant& entry : allowed) {
        if (entry == value) {
            return true;
        }
    }
    return false;
}

void enforceListValueInLimits(QVariantMap& opts)
{
    if (!opts.contains(QStringLiteral("value"))) {
        return;
    }
    if (valueInListLimits(opts.value(QStringLiteral("value")), opts)) {
        return;
    }
    const QVariantList allowed = listLimitValuesFromOpts(opts);
    if (!allowed.isEmpty()) {
        opts.insert(QStringLiteral("value"), allowed.front());
    }
}

QVariant interpretSimpleValue(const QString& paramType, const QVariant& value)
{
    if (paramType == QStringLiteral("bool")) {
        return value.toBool();
    }
    if (paramType == QStringLiteral("int")) {
        return value.toInt();
    }
    if (paramType == QStringLiteral("float")) {
        return value.toDouble();
    }
    if (paramType == QStringLiteral("str")) {
        return value.toString();
    }
    return value;
}

QVariantMap normalizeSimpleOpts(QVariantMap opts)
{
    const QString paramType = opts.value(QStringLiteral("type")).toString();
    if (opts.contains(QStringLiteral("value"))) {
        opts.insert(QStringLiteral("value"), interpretSimpleValue(paramType, opts.value(QStringLiteral("value"))));
    }
    if (opts.contains(QStringLiteral("default"))) {
        opts.insert(QStringLiteral("default"), interpretSimpleValue(paramType, opts.value(QStringLiteral("default"))));
    }
    return opts;
}

} // namespace

void registerParameterType(const QString& typeName, ParameterFactory factory)
{
    parameterTypeRegistry().insert(typeName, std::move(factory));
}

Parameter::Parameter(QVariantMap opts, QObject* parent)
    : QObject(parent)
    , opts_(std::move(opts))
{
    if (!opts_.contains(QStringLiteral("name"))) {
        throw std::invalid_argument("Parameter must have a name specified");
    }
    if (!opts_.contains(QStringLiteral("default")) && opts_.contains(QStringLiteral("value"))) {
        opts_.insert(QStringLiteral("default"), opts_.value(QStringLiteral("value")));
    }

    const bool hadExplicitValue = opts_.contains(QStringLiteral("value"));
    if (hadExplicitValue) {
        const QVariant explicitValue = opts_.value(QStringLiteral("value"));
        if (explicitValue.isValid() || explicitValue.typeId() == QMetaType::QString) {
            setValue(explicitValue, true);
        }
    } else if (opts_.contains(QStringLiteral("default"))) {
        setValue(opts_.value(QStringLiteral("default")), true);
    }
    modifiedSinceReset_ = hadExplicitValue;
}

std::shared_ptr<Parameter> Parameter::create(QVariantMap opts)
{
    registerBuiltinParameterTypes();

    const QVariantList childSpecs = opts.take(QStringLiteral("children")).toList();
    const QString type = opts.value(QStringLiteral("type")).toString();

    ParameterFactory factory = parameterTypeRegistry().value(type);
    std::shared_ptr<Parameter> param;
    if (factory) {
        param = factory(opts);
    } else if (type.isEmpty()) {
        param = std::make_shared<Parameter>(opts);
    } else {
        param = std::make_shared<SimpleParameter>(opts);
    }

    for (const QVariant& childSpec : childSpecs) {
        param->addChild(Parameter::create(childSpec.toMap()));
    }
    return param;
}

QString Parameter::name() const
{
    return opts_.value(QStringLiteral("name")).toString();
}

QString Parameter::title() const
{
    const QVariant title = opts_.value(QStringLiteral("title"));
    if (title.isValid() && !title.toString().isEmpty()) {
        return title.toString();
    }
    return name();
}

QString Parameter::type() const
{
    return opts_.value(QStringLiteral("type")).toString();
}

QVariant Parameter::value() const
{
    return opts_.value(QStringLiteral("value"));
}

Parameter* Parameter::child(const QString& childName) const
{
    return names_.value(childName, nullptr);
}

void Parameter::addChild(std::shared_ptr<Parameter> child)
{
    insertChild(static_cast<int>(children_.size()), std::move(child));
}

void Parameter::insertChild(int index, std::shared_ptr<Parameter> child)
{
    if (child == nullptr) {
        return;
    }

    const QString childName = child->name();
    if (names_.contains(childName) && names_.value(childName) != child.get()) {
        throw std::invalid_argument(
            QStringLiteral("Already have child named %1").arg(childName).toStdString());
    }

    if (child->parent_ != nullptr) {
        child->parent_->removeChild(child.get());
    }

    names_.insert(childName, child.get());
    children_.insert(children_.begin() + index, child);
    child->parent_ = this;
    emit sigChildAdded(this, child.get(), index);
    for (ParameterItem* item : items_) {
        item->childAdded(this, child.get(), index);
    }
}

void Parameter::removeChild(Parameter* child)
{
    if (child == nullptr) {
        return;
    }

    const QString childName = child->name();
    if (!names_.contains(childName) || names_.value(childName) != child) {
        throw std::runtime_error("Parameter is not a child; can't remove.");
    }

    names_.remove(childName);

    std::shared_ptr<Parameter> retained;
    auto& siblings = children_;
    const auto it = std::find_if(siblings.begin(),
                                 siblings.end(),
                                 [child](const std::shared_ptr<Parameter>& entry) {
                                     return entry.get() == child;
                                 });
    if (it != siblings.end()) {
        retained = *it;
        siblings.erase(it);
    }

    child->parent_ = nullptr;
    emit sigChildRemoved(this, child);
    for (ParameterItem* item : items_) {
        item->childRemoved(this, child);
    }
}

QVariant Parameter::setValue(const QVariant& value, bool blockSignal)
{
    if (variantEqual(opts_.value(QStringLiteral("value")), value)) {
        return value;
    }

    modifiedSinceReset_ = true;
    opts_.insert(QStringLiteral("value"), value);
    if (!blockSignal) {
        const QVariant current = opts_.value(QStringLiteral("value"));
        emit sigValueChanged(this, current);
        for (ParameterItem* item : items_) {
            item->valueChanged(this, current);
        }
    }
    return opts_.value(QStringLiteral("value"));
}

void Parameter::notifyValueChanging(const QVariant& value)
{
    emit sigValueChanging(this, value);
}

QString Parameter::setName(const QString& name)
{
    const QString oldName = opts_.value(QStringLiteral("name")).toString();
    if (oldName == name) {
        return name;
    }

    QString actualName = name;
    if (parent_ != nullptr) {
        if (parent_->names_.contains(name) && parent_->names_.value(name) != this) {
            return oldName;
        }
        parent_->names_.remove(oldName);
        parent_->names_.insert(actualName, this);
    }

    opts_.insert(QStringLiteral("name"), actualName);
    emit sigNameChanged(this, actualName);
    for (ParameterItem* item : items_) {
        item->nameChanged(this, actualName);
    }
    return actualName;
}

void Parameter::setOpts(const QVariantMap& opts)
{
    QVariantMap adjusted = opts;
    if (adjusted.contains(QStringLiteral("disabled"))) {
        adjusted.insert(QStringLiteral("enabled"), !adjusted.take(QStringLiteral("disabled")).toBool());
    }

    QVariantMap changed;
    for (auto it = adjusted.cbegin(); it != adjusted.cend(); ++it) {
        const QString& key = it.key();
        if (key == QStringLiteral("value")) {
            setValue(it.value());
        } else if (key == QStringLiteral("name")) {
            setName(it.value().toString());
        } else if (key == QStringLiteral("default")) {
            setDefault(it.value());
        } else if (!opts_.contains(key) || opts_.value(key) != it.value()) {
            opts_.insert(key, it.value());
            changed.insert(key, it.value());
        }
    }

    if (!changed.isEmpty()) {
        emit sigOptionsChanged(this, changed);
        for (ParameterItem* item : items_) {
            item->optsChanged(this, changed);
        }
    }
}

void Parameter::emitStateChanged(const QString& changeDesc, const QVariant& data)
{
    emit sigStateChanged(this, changeDesc, data);
}

void Parameter::setDefault(const QVariant& val, bool updatePristineValues)
{
    if (variantEqual(opts_.value(QStringLiteral("default")), val)) {
        return;
    }

    opts_.insert(QStringLiteral("default"), val);
    if (!opts_.contains(QStringLiteral("value"))
        || (updatePristineValues && !valueModifiedSinceResetToDefault())) {
        setToDefault();
    }
    if (!valueIsDefault()) {
        modifiedSinceReset_ = true;
    }
    emit sigDefaultChanged(this, val);
    for (ParameterItem* item : items_) {
        item->defaultChanged(this, val);
    }
}

void Parameter::setToDefault()
{
    if (!hasDefault()) {
        throw std::runtime_error("No default value set");
    }
    setValue(defaultValue());
    modifiedSinceReset_ = false;
}

bool Parameter::hasDefault() const
{
    return opts_.contains(QStringLiteral("default"));
}

QVariant Parameter::defaultValue() const
{
    return opts_.value(QStringLiteral("default"));
}

bool Parameter::valueIsDefault() const
{
    return hasDefault() && variantEqual(value(), defaultValue());
}

bool Parameter::valueModifiedSinceResetToDefault() const
{
    return modifiedSinceReset_;
}

bool Parameter::writable() const
{
    return !readonly();
}

bool Parameter::readonly() const
{
    return opts_.value(QStringLiteral("readonly"), false).toBool();
}

bool Parameter::enabled() const
{
    return opts_.value(QStringLiteral("enabled"), true).toBool();
}

ParameterItem* Parameter::makeTreeItem(int depth)
{
    return new ParameterItem(this, depth);
}

void Parameter::registerItem(ParameterItem* item)
{
    if (item == nullptr) {
        return;
    }
    if (std::find(items_.begin(), items_.end(), item) == items_.end()) {
        items_.push_back(item);
    }
}

void Parameter::unregisterItem(ParameterItem* item)
{
    items_.erase(std::remove(items_.begin(), items_.end(), item), items_.end());
}

GroupParameter::GroupParameter(QVariantMap opts, QObject* parent)
    : Parameter(std::move(opts), parent)
{
}

SimpleParameter::SimpleParameter(QVariantMap opts, QObject* parent)
    : Parameter(normalizeSimpleOpts(std::move(opts)), parent)
{
}

QVariant SimpleParameter::interpretValue(const QVariant& value) const
{
    return interpretSimpleValue(type(), value);
}

QVariant SimpleParameter::setValue(const QVariant& value, bool blockSignal)
{
    return Parameter::setValue(interpretValue(value), blockSignal);
}

ParameterItem* SimpleParameter::makeTreeItem(int depth)
{
    const QString paramType = type();
    if (paramType == QStringLiteral("bool")) {
        return new BoolParameterItem(this, depth);
    }
    if (paramType == QStringLiteral("str")) {
        return new StrParameterItem(this, depth);
    }
    if (paramType == QStringLiteral("int") || paramType == QStringLiteral("float")) {
        return new NumericParameterItem(this, depth);
    }
    return new WidgetParameterItem(this, depth, new QLineEdit());
}

ListParameter::ListParameter(QVariantMap opts, QObject* parent)
    : Parameter(normalizeListOpts(std::move(opts)), parent)
{
    QVariantMap adjusted = opts_;
    enforceListValueInLimits(adjusted);
    if (adjusted.value(QStringLiteral("value")) != opts_.value(QStringLiteral("value"))) {
        setValue(adjusted.value(QStringLiteral("value")), true);
    }
}

ParameterItem* ListParameter::makeTreeItem(int depth)
{
    return new ListParameterItem(this, depth);
}

QVariant ListParameter::setValue(const QVariant& value, bool blockSignal)
{
    QVariant adjusted = value;
    if (!valueInListLimits(value, opts_)) {
        const QVariantList allowed = listLimitValuesFromOpts(opts_);
        if (!allowed.isEmpty()) {
            adjusted = allowed.front();
        }
    }
    return Parameter::setValue(adjusted, blockSignal);
}

void ListParameter::setOpts(const QVariantMap& opts)
{
    Parameter::setOpts(opts);
    if (!opts.contains(QStringLiteral("limits")) && !opts.contains(QStringLiteral("values"))) {
        return;
    }

    QVariantMap adjusted = opts_;
    enforceListValueInLimits(adjusted);
    if (adjusted.value(QStringLiteral("value")) != opts_.value(QStringLiteral("value"))) {
        setValue(adjusted.value(QStringLiteral("value")), false);
    }
}

ColorParameter::ColorParameter(QVariantMap opts, QObject* parent)
    : Parameter(normalizeColorOpts(std::move(opts)), parent)
{
}

ParameterItem* ColorParameter::makeTreeItem(int depth)
{
    return new ColorParameterItem(this, depth);
}

QVariant ColorParameter::setValue(const QVariant& value, bool blockSignal)
{
    return Parameter::setValue(interpretColorValue(value), blockSignal);
}

TextParameter::TextParameter(QVariantMap opts, QObject* parent)
    : Parameter(std::move(opts), parent)
{
}

ParameterItem* TextParameter::makeTreeItem(int depth)
{
    return new TextParameterItem(this, depth);
}

ActionParameter::ActionParameter(QVariantMap opts, QObject* parent)
    : Parameter(std::move(opts), parent)
{
}

void ActionParameter::activate()
{
    emit sigActivated(this);
    emitStateChanged(QStringLiteral("activated"));
}

ParameterItem* ActionParameter::makeTreeItem(int depth)
{
    return new ActionParameterItem(this, depth);
}

void registerBuiltinParameterTypes()
{
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    registerParameterType(QStringLiteral("group"),
                          [](const QVariantMap& opts) { return std::make_shared<GroupParameter>(opts); });
    registerParameterType(QStringLiteral("action"),
                          [](const QVariantMap& opts) { return std::make_shared<ActionParameter>(opts); });
    registerParameterType(QStringLiteral("bool"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("calendar"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("checklist"),
                          [](const QVariantMap& opts) { return std::make_shared<ChecklistParameter>(opts); });
    registerParameterType(QStringLiteral("cmaplut"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("color"),
                          [](const QVariantMap& opts) { return std::make_shared<ColorParameter>(opts); });
    registerParameterType(QStringLiteral("colormap"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("file"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("float"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("font"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("int"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("list"),
                          [](const QVariantMap& opts) { return std::make_shared<ListParameter>(opts); });
    registerParameterType(QStringLiteral("pen"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("progress"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("radio"),
                          [](const QVariantMap& opts) { return std::make_shared<RadioParameter>(opts); });
    registerParameterType(QStringLiteral("slider"),
                          [](const QVariantMap& opts) { return std::make_shared<SliderParameter>(opts); });
    registerParameterType(QStringLiteral("str"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("text"),
                          [](const QVariantMap& opts) { return std::make_shared<TextParameter>(opts); });
}

} // namespace cppqtgraph::parametertree
