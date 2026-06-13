// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/Parameter.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/Parameter.hpp"

#include <algorithm>
#include <stdexcept>

namespace cppqtgraph::parametertree {

namespace {

QHash<QString, ParameterFactory>& parameterTypeRegistry()
{
    static QHash<QString, ParameterFactory> registry;
    return registry;
}

QString displayValue(const Parameter& param)
{
    if (param.type() == QStringLiteral("group") || param.type() == QStringLiteral("action")) {
        return {};
    }
    const QVariant value = param.value();
    if (!value.isValid()) {
        return {};
    }
    return value.toString();
}

} // namespace

void registerParameterType(const QString& typeName, ParameterFactory factory)
{
    parameterTypeRegistry().insert(typeName, std::move(factory));
}

Parameter::Parameter(QVariantMap opts)
    : opts_(std::move(opts))
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
        child->parent_->names_.remove(child->name());
        auto& siblings = child->parent_->children_;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        child->parent_ = nullptr;
    }

    names_.insert(childName, child.get());
    children_.insert(children_.begin() + index, child);
    child->parent_ = this;
}

ParameterItem* Parameter::makeTreeItem(int depth)
{
    return new ParameterItem(this, depth);
}

GroupParameter::GroupParameter(QVariantMap opts)
    : Parameter(std::move(opts))
{
}

SimpleParameter::SimpleParameter(QVariantMap opts)
    : Parameter(std::move(opts))
{
}

ActionParameter::ActionParameter(QVariantMap opts)
    : Parameter(std::move(opts))
{
}

ParameterItem::ParameterItem(Parameter* param, int depth)
    : widgets::TreeWidgetItem(QStringList{param->title(), displayValue(*param)})
    , param_(param)
    , depth_(depth)
{
}

void ParameterItem::treeWidgetChanged()
{
    if (param_ == nullptr) {
        return;
    }

    const QVariantMap& opts = param_->options();
    setHidden(!opts.value(QStringLiteral("visible"), true).toBool());
    setExpanded(opts.value(QStringLiteral("expanded"), true).toBool());
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
