// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/checklist.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/parametertree/parameterTypes/ChecklistParameter.hpp"
#include "../../../../include/cppqtgraph/parametertree/Parameter.hpp"
#include "../../../../include/cppqtgraph/parametertree/ParameterItem.hpp"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>

#include <stdexcept>

namespace cppqtgraph::parametertree {

namespace {

bool variantEqual(const QVariant& left, const QVariant& right)
{
    return left == right;
}

QVariantList variantListFromValue(const QVariant& value)
{
    if (value.metaType().id() == QMetaType::QVariantList) {
        return value.toList();
    }
    if (value.isValid() && !value.isNull()) {
        return QVariantList{value};
    }
    return {};
}

} // namespace

ChecklistMapping makeChecklistMapping(const QVariant& limits)
{
    ChecklistMapping mapping;
    if (limits.canConvert<QVariantMap>()) {
        const QVariantMap limitMap = limits.toMap();
        for (auto it = limitMap.constBegin(); it != limitMap.constEnd(); ++it) {
            mapping.forward.insert(it.key(), it.value());
            mapping.values.append(it.value());
            mapping.names.append(it.key());
        }
        return mapping;
    }

    const QVariantList entries = limits.toList();
    for (const QVariant& entry : entries) {
        const QString name = entry.toString();
        mapping.forward.insert(name, entry);
        mapping.values.append(entry);
        mapping.names.append(name);
    }
    return mapping;
}

ChecklistParameterItem::ChecklistParameterItem(Parameter* param, int depth)
    : ParameterItem(param, depth)
    , btnGrp_(new QButtonGroup())
{
    btnGrp_->setExclusive(false);

    metaBtnWidget_ = new QWidget();
    auto* layout = new QHBoxLayout(metaBtnWidget_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addStretch(0);

    for (const QString& title : {QStringLiteral("Clear"), QStringLiteral("Select")}) {
        auto* btn = new QPushButton(QStringLiteral("%1 All").arg(title));
        metaBtns_.insert(title, btn);
        layout->addWidget(btn);
        if (title == QStringLiteral("Clear")) {
            QObject::connect(btn, &QPushButton::clicked, btn, [this]() { clearAllClicked(); });
        } else {
            QObject::connect(btn, &QPushButton::clicked, btn, [this]() { selectAllClicked(); });
        }
    }

    auto* defaultBtn = new QPushButton(QStringLiteral("↺"));
    defaultBtn->setAutoDefault(false);
    defaultBtn->setFixedSize(20, 20);
    metaBtns_.insert(QStringLiteral("default"), defaultBtn);
    layout->addWidget(defaultBtn);
    QObject::connect(defaultBtn, &QPushButton::clicked, defaultBtn, [this]() { defaultClicked(); });

    updateDefaultBtn();
}

ChecklistParameterItem::~ChecklistParameterItem() = default;

void ChecklistParameterItem::treeWidgetChanged()
{
    ParameterItem::treeWidgetChanged();
    if (auto* tree = treeWidget()) {
        tree->setItemWidget(this, 1, metaBtnWidget_);
    }
}

void ChecklistParameterItem::childAdded(Parameter* /*param*/, Parameter* child, int pos)
{
    if (child == nullptr) {
        return;
    }

    ParameterItem* item = child->makeTreeItem(depth_ + 1);
    insertChild(pos, item);
    item->treeWidgetChanged();

    if (auto* widgetItem = dynamic_cast<WidgetParameterItem*>(item)) {
        if (QWidget* editor = widgetItem->editorWidget()) {
            if (auto* button = qobject_cast<QAbstractButton*>(editor)) {
                btnGrp_->addButton(button);
            }
        }
    }

    const auto& grandchildren = child->children();
    for (int i = 0; i < static_cast<int>(grandchildren.size()); ++i) {
        item->childAdded(child, grandchildren[static_cast<std::size_t>(i)].get(), i);
    }
}

void ChecklistParameterItem::childRemoved(Parameter* /*param*/, Parameter* removedChild)
{
    if (removedChild == nullptr) {
        return;
    }

    for (int i = 0; i < childCount(); ++i) {
        if (auto* item = dynamic_cast<ParameterItem*>(child(i));
            item != nullptr && item->parameter() == removedChild) {
            if (auto* widgetItem = dynamic_cast<WidgetParameterItem*>(item)) {
                if (QWidget* editor = widgetItem->editorWidget()) {
                    if (auto* button = qobject_cast<QAbstractButton*>(editor)) {
                        btnGrp_->removeButton(button);
                    }
                }
            }
            takeChild(i);
            delete item;
            break;
        }
    }
}

void ChecklistParameterItem::valueChanged(Parameter* param, const QVariant& val)
{
    ParameterItem::valueChanged(param, val);
    updateDefaultBtn();
}

void ChecklistParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    ParameterItem::optsChanged(param, opts);

    if (opts.contains(QStringLiteral("expanded"))) {
        const bool expanded = opts.value(QStringLiteral("expanded")).toBool();
        for (auto it = metaBtns_.constBegin(); it != metaBtns_.constEnd(); ++it) {
            it.value()->setVisible(expanded);
        }
    }

    const bool exclusive = opts.contains(QStringLiteral("exclusive"))
        ? opts.value(QStringLiteral("exclusive")).toBool()
        : param->options().value(QStringLiteral("exclusive"), false).toBool();
    const bool enabled = opts.contains(QStringLiteral("enabled"))
        ? opts.value(QStringLiteral("enabled")).toBool()
        : param->options().value(QStringLiteral("enabled"), true).toBool();

    for (auto it = metaBtns_.constBegin(); it != metaBtns_.constEnd(); ++it) {
        if (it.key() != QStringLiteral("default")) {
            it.value()->setDisabled(exclusive || !enabled);
        }
    }
    btnGrp_->setExclusive(exclusive);

    if (!opts.contains(QStringLiteral("limits"))
        && (opts.contains(QStringLiteral("enabled")) || opts.contains(QStringLiteral("readonly")))) {
        updateDefaultBtn();
    }
}

void ChecklistParameterItem::expandedChangedEvent(bool expanded)
{
    for (auto it = metaBtns_.constBegin(); it != metaBtns_.constEnd(); ++it) {
        it.value()->setVisible(expanded);
    }
}

void ChecklistParameterItem::selectAllClicked()
{
    auto* checklist = dynamic_cast<ChecklistParameter*>(param_);
    if (checklist == nullptr) {
        return;
    }
    checklist->cancelPendingChanges();
    const QVariant limits = checklist->options().value(QStringLiteral("limits"));
    checklist->setValue(makeChecklistMapping(limits).values);
}

void ChecklistParameterItem::clearAllClicked()
{
    auto* checklist = dynamic_cast<ChecklistParameter*>(param_);
    if (checklist == nullptr) {
        return;
    }
    checklist->cancelPendingChanges();
    checklist->setValue(QVariantList{});
}

void ChecklistParameterItem::defaultClicked()
{
    if (param_ == nullptr || !param_->hasDefault()) {
        return;
    }
    if (auto* checklist = dynamic_cast<ChecklistParameter*>(param_)) {
        checklist->setToDefault();
    }
    updateDefaultBtn();
}

void ChecklistParameterItem::updateDefaultBtn()
{
    auto* defaultBtn = metaBtns_.value(QStringLiteral("default"));
    if (defaultBtn == nullptr || param_ == nullptr) {
        return;
    }
    defaultBtn->setEnabled(param_->valueModifiedSinceResetToDefault() && param_->enabled() && param_->writable());
    defaultBtn->setVisible(param_->hasDefault() && !param_->readonly());
}

ChecklistParameter::ChecklistParameter(QVariantMap opts, QObject* parent)
    : GroupParameter([&opts]() {
          if (opts.contains(QStringLiteral("children"))) {
              throw std::invalid_argument("Cannot pass 'children' to ChecklistParameter. Pass a 'value' key only.");
          }
          opts.insert(QStringLiteral("type"), QStringLiteral("checklist"));
          if (!opts.contains(QStringLiteral("limits"))) {
              opts.insert(QStringLiteral("limits"), QVariantList{});
          }
          if (!opts.contains(QStringLiteral("exclusive"))) {
              opts.insert(QStringLiteral("exclusive"), false);
          }
          if (!opts.contains(QStringLiteral("value"))) {
              opts.insert(QStringLiteral("value"), opts.value(QStringLiteral("limits")));
          }
          return opts;
      }(),
      parent)
{
    mapping_ = makeChecklistMapping(opts_.value(QStringLiteral("limits")));

    const double delay = opts_.value(QStringLiteral("delay"), 1.0).toDouble();
    valChangingProxy_ = std::make_unique<cppqtgraph::SignalProxy>(delay, 0.0, false, this);
    QObject::connect(this,
                     &Parameter::sigValueChanging,
                     valChangingProxy_.get(),
                     [this](Parameter* param, const QVariant& value) {
                         valChangingProxy_->signalReceived(QVariant::fromValue(param), value);
                     });
    QObject::connect(valChangingProxy_.get(),
                     &cppqtgraph::SignalProxy::sigDelayed,
                     this,
                     [this](const QVariantList& args) { finishChildChanges(args); });

    QObject::connect(this, &Parameter::sigOptionsChanged, this, [this](Parameter*, const QVariantMap& changed) {
        if (changed.contains(QStringLiteral("exclusive"))) {
            updateLimits(opts_.value(QStringLiteral("limits")));
        }
        if (changed.contains(QStringLiteral("delay")) && valChangingProxy_ != nullptr) {
            valChangingProxy_->setDelay(changed.value(QStringLiteral("delay")).toDouble());
        }
    });

    if (!mapping_.names.isEmpty()) {
        updateLimits(opts_.value(QStringLiteral("limits")));
        setValue(opts_.value(QStringLiteral("value")), true);
    }
}

ChecklistParameter::~ChecklistParameter() = default;

void ChecklistParameter::cancelPendingChanges()
{
    if (valChangingProxy_ != nullptr) {
        valChangingProxy_->cancelPending();
    }
}

QVariant ChecklistParameter::childrenValue() const
{
    QVariantList vals;
    for (const auto& child : children()) {
        if (child->value().toBool()) {
            vals.append(mapping_.forward.value(child->name()));
        }
    }

    const bool exclusive = opts_.value(QStringLiteral("exclusive"), false).toBool();
    if (vals.isEmpty() && exclusive) {
        return QVariant();
    }
    if (exclusive) {
        return vals.front();
    }
    return vals;
}

void ChecklistParameter::onChildChanged(Parameter* child, const QVariant& value)
{
    if (updatingChildren_) {
        return;
    }

    QVariant changingValue;
    if (opts_.value(QStringLiteral("exclusive"), false).toBool() && value.toBool()) {
        changingValue = mapping_.forward.value(child->name());
    } else {
        changingValue = childrenValue();
    }
    notifyValueChanging(changingValue);
}

void ChecklistParameter::updateLimits(const QVariant& limits)
{
    QHash<QString, bool> oldStates;
    for (const auto& child : children()) {
        oldStates.insert(child->name(), child->value().toBool());
    }

    while (!children().empty()) {
        removeChild(children().back().get());
    }

    mapping_ = makeChecklistMapping(limits);
    opts_.insert(QStringLiteral("limits"), limits);

    const bool exclusive = opts_.value(QStringLiteral("exclusive"), false).toBool();
    const QString childType = exclusive ? QStringLiteral("radio") : QStringLiteral("bool");

    for (const QString& childName : mapping_.names) {
        const bool newVal = oldStates.value(childName, false);
        auto child = Parameter::create(QVariantMap{{QStringLiteral("name"), childName},
                                                   {QStringLiteral("type"), childType},
                                                   {QStringLiteral("value"), newVal}});
        QObject::connect(child.get(), &Parameter::sigValueChanged, this, [this](Parameter* ch, const QVariant& val) {
            onChildChanged(ch, val);
        });
        addChild(child);
    }

    const QVariant preserved = opts_.contains(QStringLiteral("value")) ? opts_.value(QStringLiteral("value")) : QVariant();
    if (preserved.isValid()) {
        setValue(preserved, true);
    }
}

void ChecklistParameter::finishChildChanges(const QVariantList& args)
{
    if (args.size() < 2) {
        return;
    }
    setValue(args.at(1));
}

std::pair<QStringList, QVariantList> ChecklistParameter::intersectionWithLimits(const QVariantList& values) const
{
    QStringList allowedNames;
    QVariantList allowedValues;
    for (const QVariant& val : values) {
        for (int i = 0; i < mapping_.values.size(); ++i) {
            if (variantEqual(mapping_.values.at(i), val)) {
                allowedNames.append(mapping_.names.at(i));
                allowedValues.append(val);
                break;
            }
        }
    }
    return {allowedNames, allowedValues};
}

QVariant ChecklistParameter::setValue(const QVariant& value, bool blockSignal)
{
    targetValue_ = value;

    QVariantList values = variantListFromValue(value);
    auto [names, matchedValues] = intersectionWithLimits(values);
    QVariant valueToSet = matchedValues;

    if (opts_.value(QStringLiteral("exclusive"), false).toBool()) {
        if (!mapping_.names.isEmpty() && names.isEmpty()) {
            names.append(mapping_.names.front());
        }
        if (names.size() > 1) {
            names = names.mid(0, 1);
        }
        if (names.isEmpty()) {
            valueToSet = QVariant();
        } else {
            valueToSet = mapping_.forward.value(names.front());
        }
    } else {
        valueToSet = matchedValues;
    }

    updatingChildren_ = true;
    for (const auto& child : children()) {
        const bool checked = names.contains(child->name());
        child->setValue(checked, true);
    }
    updatingChildren_ = false;

    return GroupParameter::setValue(valueToSet, blockSignal);
}

void ChecklistParameter::setOpts(const QVariantMap& opts)
{
    GroupParameter::setOpts(opts);
    if (opts.contains(QStringLiteral("limits"))) {
        updateLimits(opts.value(QStringLiteral("limits")));
    }
}

void ChecklistParameter::setToDefault()
{
    cancelPendingChanges();
    GroupParameter::setToDefault();
}

ParameterItem* ChecklistParameter::makeTreeItem(int depth)
{
    return new ChecklistParameterItem(this, depth);
}

} // namespace cppqtgraph::parametertree
