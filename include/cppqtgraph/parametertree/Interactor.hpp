#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/interactive.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QVariantMap>

#include <memory>
#include <vector>

namespace cppqtgraph::parametertree {

class Parameter;

struct InteractorParamSpec {
    QString name;
    QString type;
    QVariant defaultValue;
};

struct InteractorFunctionSpec {
    QString name;
    std::vector<InteractorParamSpec> params;
};

class Interactor {
public:
    explicit Interactor(std::shared_ptr<Parameter> parent = nullptr);

    [[nodiscard]] QVariantMap functionToParameterDict(const InteractorFunctionSpec& spec) const;
    std::shared_ptr<Parameter> addFunction(const InteractorFunctionSpec& spec);

    [[nodiscard]] std::shared_ptr<Parameter> parent() const { return parent_; }

private:
    QVariantMap runActionTemplate_{
        {QStringLiteral("type"), QStringLiteral("action")},
        {QStringLiteral("defaultName"), QStringLiteral("Run")},
    };
    std::shared_ptr<Parameter> parent_;
};

} // namespace cppqtgraph::parametertree
