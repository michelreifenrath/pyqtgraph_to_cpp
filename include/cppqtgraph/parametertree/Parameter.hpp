#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/Parameter.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/widgets/TreeWidget.hpp>

#include <QHash>
#include <QVariantMap>

#include <functional>
#include <memory>
#include <vector>

namespace cppqtgraph::parametertree {

class Parameter;
class ParameterItem;

using ParameterFactory = std::function<std::shared_ptr<Parameter>(const QVariantMap& opts)>;

void registerParameterType(const QString& typeName, ParameterFactory factory);

class Parameter {
public:
    explicit Parameter(QVariantMap opts);
    virtual ~Parameter() = default;

    static std::shared_ptr<Parameter> create(QVariantMap opts);

    [[nodiscard]] QString name() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString type() const;
    [[nodiscard]] QVariant value() const;
    [[nodiscard]] const QVariantMap& options() const { return opts_; }

    [[nodiscard]] Parameter* parent() const { return parent_; }
    [[nodiscard]] const std::vector<std::shared_ptr<Parameter>>& children() const { return children_; }
    [[nodiscard]] Parameter* child(const QString& childName) const;

    void addChild(std::shared_ptr<Parameter> child);
    void insertChild(int index, std::shared_ptr<Parameter> child);

    virtual ParameterItem* makeTreeItem(int depth = 0);

protected:
    QVariantMap opts_;
    Parameter* parent_ = nullptr;
    std::vector<std::shared_ptr<Parameter>> children_;
    QHash<QString, Parameter*> names_;
};

class GroupParameter final : public Parameter {
public:
    explicit GroupParameter(QVariantMap opts);
};

class SimpleParameter final : public Parameter {
public:
    explicit SimpleParameter(QVariantMap opts);
};

class ActionParameter final : public Parameter {
public:
    explicit ActionParameter(QVariantMap opts);
};

class ParameterItem : public widgets::TreeWidgetItem {
public:
    ParameterItem(Parameter* param, int depth);

    void treeWidgetChanged();
    [[nodiscard]] Parameter* parameter() const { return param_; }
    [[nodiscard]] int depth() const { return depth_; }

private:
    Parameter* param_ = nullptr;
    int depth_ = 0;
};

void registerBuiltinParameterTypes();

} // namespace cppqtgraph::parametertree
