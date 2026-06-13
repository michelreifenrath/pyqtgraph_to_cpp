#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/checklist.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/SignalProxy.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/Parameter.hpp>

#include <QHash>
#include <QStringList>

#include <memory>

class QButtonGroup;
class QPushButton;
class QWidget;

namespace cppqtgraph::parametertree {

struct ChecklistMapping {
    QHash<QString, QVariant> forward;
    QVariantList values;
    QStringList names;
};

ChecklistMapping makeChecklistMapping(const QVariant& limits);

class ChecklistParameterItem final : public ParameterItem {
public:
    ChecklistParameterItem(Parameter* param, int depth);
    ~ChecklistParameterItem() override;

    void treeWidgetChanged() override;
    void childAdded(Parameter* param, Parameter* child, int pos) override;
    void childRemoved(Parameter* param, Parameter* child) override;
    void valueChanged(Parameter* param, const QVariant& val) override;
    void optsChanged(Parameter* param, const QVariantMap& opts) override;
    void expandedChangedEvent(bool expanded) override;

private:
    void selectAllClicked();
    void clearAllClicked();
    void defaultClicked();
    void updateDefaultBtn();

    QWidget* metaBtnWidget_ = nullptr;
    QHash<QString, QPushButton*> metaBtns_;
    QButtonGroup* btnGrp_ = nullptr;
};

class ChecklistParameter final : public GroupParameter {
public:
    explicit ChecklistParameter(QVariantMap opts, QObject* parent = nullptr);
    ~ChecklistParameter() override;

    void cancelPendingChanges();
    QVariant setValue(const QVariant& value, bool blockSignal = false) override;
    void setOpts(const QVariantMap& opts) override;
    void setToDefault();
    ParameterItem* makeTreeItem(int depth) override;

private:
    QVariant childrenValue() const;
    void onChildChanged(Parameter* child, const QVariant& value);
    void updateLimits(const QVariant& limits);
    void finishChildChanges(const QVariantList& args);
    std::pair<QStringList, QVariantList> intersectionWithLimits(const QVariantList& values) const;

    ChecklistMapping mapping_;
    QVariant targetValue_;
    std::unique_ptr<cppqtgraph::SignalProxy> valChangingProxy_;
    bool updatingChildren_ = false;
};

} // namespace cppqtgraph::parametertree
