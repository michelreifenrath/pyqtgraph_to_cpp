// Source note: translated/adapted from PyQtGraph pyqtgraph/examples/_buildParamTypes.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/buildParamTypes.hpp"
#include "../../../include/cppqtgraph/parametertree/Parameter.hpp"
#include "../../../include/cppqtgraph/parametertree/parameterTypes/SliderParameter.hpp"

#include <QtCore/QObject>
#include <vector>

namespace cppqtgraph::parametertree {

namespace {

QVariant normalizedOptionValue(const QVariant& value)
{
    if (value.typeId() == QMetaType::QString && value.toString().isEmpty()) {
        return QVariant();
    }
    return value;
}

void connectOptionRow(Parameter* widgetParam, Parameter* optionParam)
{
    QObject::connect(optionParam,
                     &Parameter::sigValueChanged,
                     widgetParam,
                     [widgetParam, optionParam](Parameter* /*source*/, const QVariant& value) {
                         widgetParam->setOpts(
                             {{optionParam->name(), normalizedOptionValue(value)}});
                     });
}

void applyInitialOptions(const std::shared_ptr<Parameter>& widgetParam,
                         const std::vector<std::shared_ptr<Parameter>>& optionRows)
{
    for (const auto& option : optionRows) {
        const QVariant value = option->options().contains(QStringLiteral("value"))
            ? option->value()
            : QVariant();
        widgetParam->setOpts({{option->name(), normalizedOptionValue(value)}});
    }
}

std::shared_ptr<Parameter> makeSampleGroup(const QString& chType, const QVariantList& childSpecs)
{
    QVariantList metaSpecs;
    QVariantList optSpecs;
    QVariantMap widgetSpec;

    for (const QVariant& childSpec : childSpecs) {
        const QVariantMap spec = childSpec.toMap();
        const QString name = spec.value(QStringLiteral("name")).toString();
        if (name == QStringLiteral("widget")) {
            widgetSpec = spec;
            continue;
        }
        if (name.contains(QLatin1Char(' '))) {
            metaSpecs.append(childSpec);
        } else {
            optSpecs.append(childSpec);
        }
    }

    auto widgetParam = Parameter::create(widgetSpec);
    if (widgetParam->hasDefault()) {
        widgetParam->setDefault(widgetParam->value());
    }

    std::vector<std::shared_ptr<Parameter>> optionRows;
    optionRows.reserve(static_cast<std::size_t>(optSpecs.size()));
    for (const QVariant& optionSpec : optSpecs) {
        auto optionParam = Parameter::create(optionSpec.toMap());
        connectOptionRow(widgetParam.get(), optionParam.get());
        optionRows.push_back(optionParam);
    }
    applyInitialOptions(widgetParam, optionRows);

    auto group = Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("Sample %1").arg(chType.at(0).toUpper() + chType.mid(1))},
        {QStringLiteral("type"), QStringLiteral("group")},
        {QStringLiteral("expanded"), false},
    });

    for (const QVariant& metaSpec : metaSpecs) {
        group->addChild(Parameter::create(metaSpec.toMap()));
    }
    group->addChild(widgetParam);
    for (const auto& option : optionRows) {
        group->addChild(option);
    }
    return group;
}

std::shared_ptr<Parameter> makeMetaGroup(const QString& name, const QVariantList& childSpecs)
{
    auto group = Parameter::create(QVariantMap{{QStringLiteral("name"), name},
                                               {QStringLiteral("type"), QStringLiteral("group")},
                                               {QStringLiteral("expanded"), false}});
    for (const QVariant& childSpec : childSpecs) {
        group->addChild(Parameter::create(childSpec.toMap()));
    }
    return group;
}

std::shared_ptr<Parameter> makeListSampleGroup()
{
    return makeSampleGroup(QStringLiteral("list"),
                           QVariantList{
                               QVariant::fromValue(QVariantMap{
                                   {QStringLiteral("name"), QStringLiteral("widget")},
                                   {QStringLiteral("type"), QStringLiteral("list")},
                                   {QStringLiteral("limits"),
                                    QVariantList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}}}),
                               QVariant::fromValue(QVariantMap{
                                   {QStringLiteral("name"), QStringLiteral("limits")},
                                   {QStringLiteral("type"), QStringLiteral("list")},
                                   {QStringLiteral("limits"),
                                    QVariantMap{{QStringLiteral("default"),
                                                 QVariantList{QStringLiteral("a"), QStringLiteral("b"),
                                                                QStringLiteral("c")}}}},
                                   {QStringLiteral("value"),
                                    QVariantList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}}}),
                           });
}

std::shared_ptr<Parameter> makeFloatSampleGroup()
{
    return makeSampleGroup(
        QStringLiteral("float"),
        QVariantList{
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("Float Information")},
                                            {QStringLiteral("type"), QStringLiteral("str")},
                                            {QStringLiteral("readonly"), true},
                                            {QStringLiteral("value"),
                                             QStringLiteral("Note that all options except \"finite\" also apply to "
                                                            "\"int\" parameters")}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                                            {QStringLiteral("type"), QStringLiteral("float")},
                                            {QStringLiteral("value"), 0.0}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("step")},
                                            {QStringLiteral("type"), QStringLiteral("float")},
                                            {QStringLiteral("limits"), QVariantList{QVariant(0), QVariant()}},
                                            {QStringLiteral("value"), 1.0}}),
            QVariant::fromValue(
                QVariantMap{{QStringLiteral("name"), QStringLiteral("limits")},
                            {QStringLiteral("type"), QStringLiteral("list")},
                            {QStringLiteral("limits"),
                             QVariantMap{{QStringLiteral("[0, None]"), QVariantList{0, QVariant()}},
                                           {QStringLiteral("[1, 5]"), QVariantList{1, 5}}}}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("suffix")},
                                            {QStringLiteral("type"), QStringLiteral("list")},
                                            {QStringLiteral("limits"),
                                             QVariantList{QStringLiteral("Hz"), QStringLiteral("s"), QStringLiteral("m")}}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("siPrefix")},
                                            {QStringLiteral("type"), QStringLiteral("bool")},
                                            {QStringLiteral("value"), true}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("finite")},
                                            {QStringLiteral("type"), QStringLiteral("bool")},
                                            {QStringLiteral("value"), true}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("dec")},
                                            {QStringLiteral("type"), QStringLiteral("bool")},
                                            {QStringLiteral("value"), false}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("minStep")},
                                            {QStringLiteral("type"), QStringLiteral("float")},
                                            {QStringLiteral("value"), 1.0e-12}}),
        });
}

QVariantList spanToVariantList(const std::vector<double>& span)
{
    QVariantList values;
    values.reserve(static_cast<int>(span.size()));
    for (double value : span) {
        values.append(value);
    }
    return values;
}

std::shared_ptr<Parameter> makeChecklistSampleGroup()
{
    return makeSampleGroup(
        QStringLiteral("checklist"),
        QVariantList{
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                                            {QStringLiteral("type"), QStringLiteral("checklist")},
                                            {QStringLiteral("limits"),
                                             QVariantList{QStringLiteral("one"), QStringLiteral("two"),
                                                            QStringLiteral("three"), QStringLiteral("four")}}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("limits")},
                                            {QStringLiteral("type"), QStringLiteral("checklist")},
                                            {QStringLiteral("limits"),
                                             QVariantList{QStringLiteral("one"), QStringLiteral("two"),
                                                            QStringLiteral("three"), QStringLiteral("four")}}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("exclusive")},
                                            {QStringLiteral("type"), QStringLiteral("bool")},
                                            {QStringLiteral("value"), false}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("delay")},
                                            {QStringLiteral("type"), QStringLiteral("float")},
                                            {QStringLiteral("value"), 1.0},
                                            {QStringLiteral("limits"), QVariantList{QVariant(0), QVariant()}}}),
        });
}

std::shared_ptr<Parameter> makeSliderSampleGroup()
{
    const QVariantList linspaceSpan = spanToVariantList(linspaceMinusPiToPi(50));
    const QVariantList arangeSpan = spanToVariantList(arangeSquared(10));

    auto group = makeSampleGroup(
        QStringLiteral("slider"),
        QVariantList{
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                                            {QStringLiteral("type"), QStringLiteral("slider")},
                                            {QStringLiteral("limits"), QVariantList{0, 100}}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("step")},
                                            {QStringLiteral("type"), QStringLiteral("float")},
                                            {QStringLiteral("limits"), QVariantList{QVariant(0), QVariant()}},
                                            {QStringLiteral("value"), 1.0}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("format")},
                                            {QStringLiteral("type"), QStringLiteral("str")},
                                            {QStringLiteral("value"), QStringLiteral("{0:>3}")}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("precision")},
                                            {QStringLiteral("type"), QStringLiteral("int")},
                                            {QStringLiteral("value"), 2},
                                            {QStringLiteral("limits"), QVariantList{QVariant(1), QVariant()}}}),
            QVariant::fromValue(
                QVariantMap{{QStringLiteral("name"), QStringLiteral("span")},
                            {QStringLiteral("type"), QStringLiteral("list")},
                            {QStringLiteral("limits"),
                             QVariantMap{{QStringLiteral("linspace(-pi, pi)"), linspaceSpan},
                                           {QStringLiteral("arange(10)**2"), arangeSpan}}}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("How to Set")},
                                            {QStringLiteral("type"), QStringLiteral("list")},
                                            {QStringLiteral("limits"),
                                             QVariantList{QStringLiteral("Use span"), QStringLiteral("Use step + limits")}}}),
        });

    Parameter* widgetParam = group->child(QStringLiteral("widget"));
    Parameter* howToSet = group->child(QStringLiteral("How to Set"));
    if (widgetParam != nullptr && howToSet != nullptr) {
        QObject::connect(howToSet,
                         &Parameter::sigValueChanged,
                         widgetParam,
                         [widgetParam](Parameter* /*source*/, const QVariant& value) {
                             if (value.toString() == QStringLiteral("Use span")) {
                                 const QVariant span = widgetParam->options().value(QStringLiteral("span"));
                                 QVariantMap opts{{QStringLiteral("span"), span}};
                                 opts.insert(QStringLiteral("limits"), QVariant());
                                 widgetParam->setOpts(opts);
                             } else {
                                 const QVariant limits = widgetParam->options().value(QStringLiteral("limits"));
                                 QVariantMap opts{{QStringLiteral("limits"), limits}};
                                 opts.insert(QStringLiteral("span"), QVariant());
                                 widgetParam->setOpts(opts);
                             }
                         });
    }
    return group;
}

std::shared_ptr<Parameter> makeActionSampleGroup()
{
    return makeSampleGroup(
        QStringLiteral("action"),
        QVariantList{
            QVariant::fromValue(
                QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")}, {QStringLiteral("type"), QStringLiteral("action")}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("shortcut")},
                                            {QStringLiteral("type"), QStringLiteral("str")},
                                            {QStringLiteral("value"), QStringLiteral("Ctrl+Shift+P")}}),
            QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("icon")},
                                            {QStringLiteral("type"), QStringLiteral("file")},
                                            {QStringLiteral("value"), QVariant()},
                                            {QStringLiteral("nameFilter"),
                                             QStringLiteral("Images (*.png *.jpg *.bmp *.jpeg *.svg)")}}),
        });
}

std::shared_ptr<Parameter> makeNoExtraOptionsGroup()
{
    return makeMetaGroup(QStringLiteral("No Extra Options"),
                         QVariantList{
                             QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("int")},
                                                             {QStringLiteral("type"), QStringLiteral("int")},
                                                             {QStringLiteral("value"), 10}}),
                             QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("str")},
                                                             {QStringLiteral("type"), QStringLiteral("str")},
                                                             {QStringLiteral("value"), QStringLiteral("Hi, world!")}}),
                             QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("bool")},
                                                             {QStringLiteral("type"), QStringLiteral("bool")},
                                                             {QStringLiteral("value"), false}}),
                         });
}

void connectExpandCollapseActions(Parameter* exampleParams)
{
    const auto activate = [exampleParams](Parameter* action) {
        const bool expand = action->name() == QStringLiteral("Expand All");
        for (const auto& child : exampleParams->children()) {
            if (child->type() == QStringLiteral("group")) {
                child->setOpts({{QStringLiteral("expanded"), expand}});
            }
        }
    };

    for (const QString& name : {QStringLiteral("Collapse All"), QStringLiteral("Expand All")}) {
        auto button = Parameter::create(
            QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("type"), QStringLiteral("action")}});
        if (auto* action = dynamic_cast<ActionParameter*>(button.get())) {
            QObject::connect(action, &ActionParameter::sigActivated, exampleParams, [activate](Parameter* param) {
                activate(param);
            });
        }
        exampleParams->insertChild(0, button);
    }
}

} // namespace

std::shared_ptr<Parameter> buildExampleParametersGroup()
{
    auto exampleParams = Parameter::create(QVariantMap{{QStringLiteral("name"), QStringLiteral("Example Parameters")},
                                                     {QStringLiteral("type"), QStringLiteral("group")}});
    exampleParams->addChild(makeListSampleGroup());
    exampleParams->addChild(makeFloatSampleGroup());
    exampleParams->addChild(makeChecklistSampleGroup());
    exampleParams->addChild(makeSliderSampleGroup());
    exampleParams->addChild(makeActionSampleGroup());
    exampleParams->addChild(makeNoExtraOptionsGroup());
    connectExpandCollapseActions(exampleParams.get());
    return exampleParams;
}

} // namespace cppqtgraph::parametertree
