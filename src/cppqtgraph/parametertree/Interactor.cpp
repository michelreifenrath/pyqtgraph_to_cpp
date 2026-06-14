// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/interactive.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Interactor.hpp>
#include <cppqtgraph/parametertree/Parameter.hpp>

namespace cppqtgraph::parametertree {

Interactor::Interactor(std::shared_ptr<Parameter> parent)
    : parent_(std::move(parent))
{
}

QVariantMap Interactor::functionToParameterDict(const InteractorFunctionSpec& spec) const
{
    QVariantList children;
    children.reserve(static_cast<int>(spec.params.size()));
    for (const InteractorParamSpec& param : spec.params) {
        children.append(QVariantMap{
            {QStringLiteral("name"), param.name},
            {QStringLiteral("title"), param.name},
            {QStringLiteral("type"), param.type},
            {QStringLiteral("value"), param.defaultValue},
            {QStringLiteral("default"), param.defaultValue},
        });
    }

    QVariantMap buttonOpts = runActionTemplate_;
    const QString defaultName =
        buttonOpts.value(QStringLiteral("defaultName"), QStringLiteral("Run")).toString();
    buttonOpts.insert(QStringLiteral("name"), defaultName);
    buttonOpts.insert(QStringLiteral("visible"), false);

    return QVariantMap{
        {QStringLiteral("name"), spec.name},
        {QStringLiteral("title"), spec.name},
        {QStringLiteral("type"), QStringLiteral("_actiongroup")},
        {QStringLiteral("button"), buttonOpts},
        {QStringLiteral("children"), children},
    };
}

std::shared_ptr<Parameter> Interactor::addFunction(const InteractorFunctionSpec& spec)
{
    auto param = Parameter::create(functionToParameterDict(spec));
    if (parent_ != nullptr) {
        parent_->addChild(param);
    }
    return param;
}

} // namespace cppqtgraph::parametertree
