// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/Parameter.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/Parameter.hpp"
#include "../../../include/cppqtgraph/parametertree/ParameterItem.hpp"

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
    QVariantMap changed;
    for (auto it = opts.cbegin(); it != opts.cend(); ++it) {
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
    : Parameter(std::move(opts), parent)
{
}

ParameterItem* SimpleParameter::makeTreeItem(int depth)
{
    return new WidgetParameterItem(this, depth);
}

ActionParameter::ActionParameter(QVariantMap opts, QObject* parent)
    : Parameter(std::move(opts), parent)
{
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
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("cmaplut"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("color"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
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
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("pen"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("progress"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("slider"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("str"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
    registerParameterType(QStringLiteral("text"),
                          [](const QVariantMap& opts) { return std::make_shared<SimpleParameter>(opts); });
}

} // namespace cppqtgraph::parametertree
