#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/Parameter.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QHash>
#include <QObject>
#include <QVariantMap>

#include <functional>
#include <memory>
#include <vector>

namespace cppqtgraph::parametertree {

class Parameter;
class ParameterItem;

using ParameterFactory = std::function<std::shared_ptr<Parameter>(const QVariantMap& opts)>;

void registerParameterType(const QString& typeName, ParameterFactory factory);

class Parameter : public QObject {
    Q_OBJECT

public:
    explicit Parameter(QVariantMap opts, QObject* parent = nullptr);
    ~Parameter() override = default;

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
    void removeChild(Parameter* child);

    QVariant setValue(const QVariant& value, bool blockSignal = false);
    void notifyValueChanging(const QVariant& value);
    QString setName(const QString& name);
    void setOpts(const QVariantMap& opts);
    void setDefault(const QVariant& val, bool updatePristineValues = false);
    void setToDefault();

    [[nodiscard]] bool hasDefault() const;
    [[nodiscard]] QVariant defaultValue() const;
    [[nodiscard]] bool valueIsDefault() const;
    [[nodiscard]] bool valueModifiedSinceResetToDefault() const;
    [[nodiscard]] bool writable() const;
    [[nodiscard]] bool readonly() const;
    [[nodiscard]] bool enabled() const;

    virtual ParameterItem* makeTreeItem(int depth = 0);

    void registerItem(ParameterItem* item);
    void unregisterItem(ParameterItem* item);

signals:
    void sigValueChanged(Parameter* param, const QVariant& value);
    void sigValueChanging(Parameter* param, const QVariant& value);
    void sigChildAdded(Parameter* param, Parameter* child, int index);
    void sigChildRemoved(Parameter* param, Parameter* child);
    void sigNameChanged(Parameter* param, const QString& name);
    void sigDefaultChanged(Parameter* param, const QVariant& defaultValue);
    void sigOptionsChanged(Parameter* param, const QVariantMap& changedOpts);

protected:
    QVariantMap opts_;
    Parameter* parent_ = nullptr;
    std::vector<std::shared_ptr<Parameter>> children_;
    QHash<QString, Parameter*> names_;
    bool modifiedSinceReset_ = false;
    std::vector<ParameterItem*> items_;
};

class GroupParameter final : public Parameter {
public:
    explicit GroupParameter(QVariantMap opts, QObject* parent = nullptr);
};

class SimpleParameter final : public Parameter {
public:
    explicit SimpleParameter(QVariantMap opts, QObject* parent = nullptr);

    ParameterItem* makeTreeItem(int depth = 0) override;
};

class ListParameter final : public Parameter {
public:
    explicit ListParameter(QVariantMap opts, QObject* parent = nullptr);

    ParameterItem* makeTreeItem(int depth = 0) override;
};

class ColorParameter final : public Parameter {
public:
    explicit ColorParameter(QVariantMap opts, QObject* parent = nullptr);

    ParameterItem* makeTreeItem(int depth = 0) override;
};

class TextParameter final : public Parameter {
public:
    explicit TextParameter(QVariantMap opts, QObject* parent = nullptr);

    ParameterItem* makeTreeItem(int depth = 0) override;
};

class ActionParameter final : public Parameter {
public:
    explicit ActionParameter(QVariantMap opts, QObject* parent = nullptr);
};

void registerBuiltinParameterTypes();

} // namespace cppqtgraph::parametertree
