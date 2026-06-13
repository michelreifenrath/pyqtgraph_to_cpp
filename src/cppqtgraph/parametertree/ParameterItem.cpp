// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/ParameterItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/ParameterItem.hpp"
#include "../../../include/cppqtgraph/parametertree/Parameter.hpp"
#include "../../../include/cppqtgraph/parametertree/ParameterTree.hpp"

#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/widgets/ColorButton.hpp>
#include <cppqtgraph/widgets/ComboBox.hpp>

#include <QtCore/QObject>
#include <QtCore/QSignalBlocker>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>
#include <QtGui/QShortcut>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeWidgetItem>

#include <stdexcept>

namespace cppqtgraph::parametertree {

namespace {

QString displayValue(const Parameter& param)
{
    if (param.type() == QStringLiteral("group") || param.type() == QStringLiteral("action")) {
        return {};
    }
    const QVariant value = param.value();
    if (!value.isValid()) {
        return {};
    }
    if (value.metaType().id() == QMetaType::QColor) {
        return value.value<QColor>().name(QColor::HexRgb);
    }
    return value.toString();
}

QVariant listLimitsFromOptions(const QVariantMap& opts)
{
    if (opts.contains(QStringLiteral("limits"))) {
        return opts.value(QStringLiteral("limits"));
    }
    if (opts.contains(QStringLiteral("values"))) {
        return opts.value(QStringLiteral("values"));
    }
    return QVariant();
}

QVariantList listLimitValues(const QVariant& limits)
{
    QVariantList values;
    if (limits.canConvert<QVariantList>()) {
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
        for (auto it = limits.toMap().constBegin(); it != limits.toMap().constEnd(); ++it) {
            values.append(it.value());
        }
    }
    return values;
}

bool valueInLimits(const QVariant& value, const QVariant& limits)
{
    const QVariantList allowed = listLimitValues(limits);
    for (const QVariant& entry : allowed) {
        if (entry == value) {
            return true;
        }
    }
    return allowed.isEmpty();
}

QVariant firstLimitValue(const QVariant& limits)
{
    const QVariantList allowed = listLimitValues(limits);
    if (allowed.isEmpty()) {
        return QVariant();
    }
    return allowed.front();
}

class EditorEventFilter final : public QObject {
public:
    explicit EditorEventFilter(WidgetParameterItem* item, QObject* parent = nullptr)
        : QObject(parent)
        , item_(item)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        Q_UNUSED(watched);
        if (item_ == nullptr) {
            return false;
        }
        if (event->type() == QEvent::FocusOut) {
            item_->widgetValueChanged();
        }
        if (event->type() != QEvent::KeyPress) {
            return false;
        }
        return item_->handleEditorKeyPress(static_cast<QKeyEvent*>(event));
    }

private:
    WidgetParameterItem* item_ = nullptr;
};

class WidgetHooks final : public QObject {
public:
    WidgetHooks(WidgetParameterItem* item, QLineEdit* editor, QPushButton* defaultBtn)
        : QObject(editor)
        , item_(item)
    {
        QObject::connect(editor, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (item_ != nullptr) {
                item_->editorValueChanging(text);
            }
        });
        QObject::connect(editor, &QLineEdit::editingFinished, this, [this]() {
            if (item_ != nullptr) {
                item_->widgetValueChanged();
            }
        });
        QObject::connect(defaultBtn, &QPushButton::clicked, this, [this]() {
            if (item_ != nullptr) {
                item_->defaultClicked();
            }
        });
    }

private:
    WidgetParameterItem* item_ = nullptr;
};

} // namespace

ParameterItem::ParameterItem(Parameter* param, int depth)
    : widgets::TreeWidgetItem(QStringList{param->title(), displayValue(*param)})
    , param_(param)
    , depth_(depth)
{
    if (param_ != nullptr) {
        param_->registerItem(this);
    }
    updateFlags();
}

ParameterItem::~ParameterItem()
{
    if (param_ != nullptr) {
        param_->unregisterItem(this);
    }
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

void ParameterItem::valueChanged(Parameter* /*param*/, const QVariant& val)
{
    setText(1, val.toString());
}

void ParameterItem::childAdded(Parameter* /*param*/, Parameter* child, int pos)
{
    if (child == nullptr) {
        return;
    }

    ParameterItem* item = child->makeTreeItem(depth_ + 1);
    insertChild(pos, item);
    item->treeWidgetChanged();

    const auto& grandchildren = child->children();
    for (int i = 0; i < static_cast<int>(grandchildren.size()); ++i) {
        item->childAdded(child, grandchildren[static_cast<std::size_t>(i)].get(), i);
    }
}

void ParameterItem::childRemoved(Parameter* /*param*/, Parameter* removedChild)
{
    if (removedChild == nullptr) {
        return;
    }

    for (int i = 0; i < childCount(); ++i) {
        if (auto* item = dynamic_cast<ParameterItem*>(child(i));
            item != nullptr && item->parameter() == removedChild) {
            takeChild(i);
            delete item;
            break;
        }
    }
}

void ParameterItem::nameChanged(Parameter* /*param*/, const QString& /*name*/)
{
    if (param_->options().value(QStringLiteral("title")).toString().isEmpty()) {
        titleChanged();
    }
}

void ParameterItem::defaultChanged(Parameter* /*param*/, const QVariant& /*defaultValue*/)
{
}

void ParameterItem::optsChanged(Parameter* /*param*/, const QVariantMap& opts)
{
    if (opts.contains(QStringLiteral("visible"))) {
        setHidden(!opts.value(QStringLiteral("visible")).toBool());
    }
    if (opts.contains(QStringLiteral("expanded"))) {
        const bool expanded = opts.value(QStringLiteral("expanded")).toBool();
        if (isExpanded() != expanded) {
            setExpanded(expanded);
        }
    }
    if (opts.contains(QStringLiteral("title"))) {
        titleChanged();
    }
    updateFlags();
}

void ParameterItem::parentChanged(Parameter* /*param*/, Parameter* /*parent*/)
{
}

void ParameterItem::columnChangedEvent(int col)
{
    if (col != 0 || !param_->options().value(QStringLiteral("title")).toString().isEmpty()) {
        return;
    }
    if (ignoreNameColumnChange_) {
        return;
    }

    const QString newName = param_->setName(text(0));
    ignoreNameColumnChange_ = true;
    nameChanged(param_, newName);
    ignoreNameColumnChange_ = false;
}

void ParameterItem::expandedChangedEvent(bool expanded)
{
    if (param_->options().value(QStringLiteral("syncExpanded"), false).toBool()) {
        param_->setOpts({{QStringLiteral("expanded"), expanded}});
    }
}

void ParameterItem::selected(bool /*sel*/)
{
}

bool ParameterItem::isFocusable() const
{
    return false;
}

void ParameterItem::setFocus()
{
}

void ParameterItem::focusNext(bool forward)
{
    if (auto* tree = dynamic_cast<ParameterTree*>(treeWidget())) {
        tree->focusNext(this, forward);
    }
}

void ParameterItem::updateFlags()
{
    if (param_ == nullptr) {
        return;
    }

    const QVariantMap& opts = param_->options();
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (opts.value(QStringLiteral("renamable"), false).toBool()) {
        if (opts.contains(QStringLiteral("title")) && !opts.value(QStringLiteral("title")).toString().isEmpty()) {
            throw std::runtime_error("Cannot make parameter with both title and renamable.");
        }
        flags |= Qt::ItemIsEditable;
    }
    if (opts.value(QStringLiteral("movable"), false).toBool()) {
        flags |= Qt::ItemIsDragEnabled;
    }
    if (opts.value(QStringLiteral("dropEnabled"), false).toBool()) {
        flags |= Qt::ItemIsDropEnabled;
    }
    setFlags(flags);
}

void ParameterItem::titleChanged()
{
    if (param_ == nullptr) {
        return;
    }

    const QString title = param_->title();
    if (title.isEmpty() || title == QStringLiteral("params")) {
        return;
    }

    setText(0, title);
    const QFontMetrics metrics(font(0));
    QSize size = metrics.size(Qt::TextSingleLine, text(0));
    size.setHeight(static_cast<int>(size.height() * 1.35));
    size.setWidth(static_cast<int>(size.width() * 1.15));
    setSizeHint(0, size);
}

WidgetParameterItem::WidgetParameterItem(Parameter* param, int depth, QWidget* editor, WidgetParameterItemOptions options)
    : ParameterItem(param, depth)
    , editor_(editor)
    , hideWhenDeselected_(options.hideWhenDeselected)
    , asSubItem_(options.asSubItem)
{
    if (editor_ == nullptr) {
        throw std::invalid_argument("WidgetParameterItem requires an editor widget");
    }

    if (asSubItem_) {
        subItem_ = new QTreeWidgetItem();
        subItem_->setFlags(Qt::NoItemFlags);
        addChild(subItem_);
    }

    configureEditor(editor_);
    bindEditor(editor_);

    auto* filter = new EditorEventFilter(this, editor_);
    editor_->installEventFilter(filter);

    displayLabel_ = new QLabel();
    defaultBtn_ = new QPushButton(QStringLiteral("↺"));
    defaultBtn_->setAutoDefault(false);
    defaultBtn_->setFixedSize(20, 20);

    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    if (!asSubItem_) {
        layout->addWidget(editor_, 1);
    }
    layout->addWidget(displayLabel_, 1);
    layout->addStretch(0);
    layout->addWidget(defaultBtn_);

    layoutWidget_ = new QWidget();
    layoutWidget_->setLayout(layout);

    if (auto* lineEdit = qobject_cast<QLineEdit*>(editor_)) {
        new WidgetHooks(this, lineEdit, defaultBtn_);
    } else {
        QObject::connect(defaultBtn_, &QPushButton::clicked, defaultBtn_, [this]() { defaultClicked(); });
    }

    valueChanged(param, param->value());
    updateDefaultBtn();
    optsChanged(param, param->options());
}

WidgetParameterItem::~WidgetParameterItem() = default;

void WidgetParameterItem::treeWidgetChanged()
{
    ParameterItem::treeWidgetChanged();
    if (layoutWidget_ == nullptr) {
        return;
    }

    if (auto* tree = treeWidget()) {
        if (asSubItem_ && subItem_ != nullptr) {
            subItem_->setFirstColumnSpanned(true);
            tree->setItemWidget(subItem_, 0, editor_);
            subItem_->setSizeHint(0, QSize(300, 100));
            setSizeHint(1, defaultBtn_->sizeHint());
        }
        tree->setItemWidget(this, 1, layoutWidget_);
        if (hideWhenDeselected_) {
            hideEditor();
            selected(false);
        } else {
            showEditor();
        }
    }
}

void WidgetParameterItem::valueChanged(Parameter* param, const QVariant& val)
{
    ParameterItem::valueChanged(param, val);
    if (updatingWidget_) {
        return;
    }

    updatingWidget_ = true;
    writeEditorValue(val);
    updateDisplayLabel(val);
    updateDefaultBtn();
    updatingWidget_ = false;
}

void WidgetParameterItem::defaultChanged(Parameter* param, const QVariant& defaultValue)
{
    ParameterItem::defaultChanged(param, defaultValue);
    updateDefaultBtn();
}

void WidgetParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    ParameterItem::optsChanged(param, opts);

    if (editor_ == nullptr) {
        return;
    }

    if (opts.contains(QStringLiteral("enabled"))) {
        editor_->setEnabled(opts.value(QStringLiteral("enabled")).toBool());
        updateDefaultBtn();
    }
    if (opts.contains(QStringLiteral("readonly"))) {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(editor_)) {
            lineEdit->setReadOnly(opts.value(QStringLiteral("readonly")).toBool());
        } else if (auto* textEdit = qobject_cast<QTextEdit*>(editor_)) {
            textEdit->setReadOnly(opts.value(QStringLiteral("readonly")).toBool());
        } else {
            editor_->setEnabled(param_->enabled() && !opts.value(QStringLiteral("readonly")).toBool());
        }
        updateDefaultBtn();
    }
    if (opts.contains(QStringLiteral("tip"))) {
        editor_->setToolTip(opts.value(QStringLiteral("tip")).toString());
    }
}

void WidgetParameterItem::selected(bool sel)
{
    ParameterItem::selected(sel);
    if (editor_ == nullptr) {
        return;
    }
    if (sel && param_->writable()) {
        showEditor();
    } else if (hideWhenDeselected_) {
        hideEditor();
    }
}

bool WidgetParameterItem::isFocusable() const
{
    return param_->options().value(QStringLiteral("visible"), true).toBool() && param_->enabled()
        && param_->writable();
}

void WidgetParameterItem::setFocus()
{
    showEditor();
}

bool WidgetParameterItem::handleEditorKeyPress(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Tab) {
        focusNext(true);
        return true;
    }
    if (event->key() == Qt::Key_Backtab) {
        focusNext(false);
        return true;
    }
    return false;
}

void WidgetParameterItem::widgetValueChanged()
{
    if (updatingWidget_ || param_ == nullptr) {
        return;
    }

    param_->setValue(readEditorValue());
    updateDisplayLabel();
    updateDefaultBtn();
}

void WidgetParameterItem::editorValueChanging(const QVariant& value)
{
    if (updatingWidget_ || param_ == nullptr) {
        return;
    }
    param_->notifyValueChanging(value);
}

void WidgetParameterItem::defaultClicked()
{
    if (param_ == nullptr || !param_->hasDefault()) {
        return;
    }
    param_->setToDefault();
    updateDefaultBtn();
}

void WidgetParameterItem::updateDisplayLabel(const QVariant& value)
{
    if (displayLabel_ == nullptr) {
        return;
    }
    const QVariant display = value.isValid() ? value : param_->value();
    if (display.metaType().id() == QMetaType::QColor) {
        displayLabel_->setText(display.value<QColor>().name(QColor::HexRgb));
    } else {
        displayLabel_->setText(display.toString());
    }
}

void WidgetParameterItem::updateDefaultBtn()
{
    if (defaultBtn_ == nullptr || param_ == nullptr) {
        return;
    }

    defaultBtn_->setEnabled(param_->valueModifiedSinceResetToDefault() && param_->enabled() && param_->writable());
    defaultBtn_->setVisible(param_->hasDefault() && !param_->readonly());
}

void WidgetParameterItem::showEditor()
{
    if (editor_ == nullptr || displayLabel_ == nullptr) {
        return;
    }
    editor_->show();
    displayLabel_->hide();
    editor_->setFocus(Qt::OtherFocusReason);
}

void WidgetParameterItem::hideEditor()
{
    if (editor_ == nullptr || displayLabel_ == nullptr) {
        return;
    }
    editor_->hide();
    displayLabel_->show();
}

void WidgetParameterItem::bindEditor(QWidget* editor)
{
    // Default QLineEdit bindings are installed via WidgetHooks in the constructor.
    Q_UNUSED(editor);
}

QVariant WidgetParameterItem::readEditorValue() const
{
    if (auto* lineEdit = qobject_cast<const QLineEdit*>(editor_)) {
        return lineEdit->text();
    }
    return QVariant();
}

void WidgetParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* lineEdit = qobject_cast<QLineEdit*>(editor_)) {
        const QSignalBlocker blocker(lineEdit);
        lineEdit->setText(val.toString());
    }
}

void WidgetParameterItem::configureEditor(QWidget* editor)
{
    Q_UNUSED(editor);
}

ListParameterItem::ListParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param, depth, new widgets::ComboBox())
{
    combo_ = qobject_cast<widgets::ComboBox*>(editor_);
    bindEditor(editor_);
    updateLimits(listLimitsFromOptions(param->options()));
}

void ListParameterItem::bindEditor(QWidget* editor)
{
    combo_ = qobject_cast<widgets::ComboBox*>(editor);
    if (combo_ == nullptr) {
        return;
    }
    QObject::connect(combo_, qOverload<int>(&QComboBox::currentIndexChanged), combo_, [this](int) {
        widgetValueChanged();
    });
}

QVariant ListParameterItem::readEditorValue() const
{
    if (combo_ == nullptr) {
        return QVariant();
    }
    return combo_->value();
}

void ListParameterItem::writeEditorValue(const QVariant& val)
{
    if (combo_ == nullptr) {
        return;
    }
    const QSignalBlocker blocker(combo_);
    try {
        combo_->setValue(val);
    } catch (const std::exception&) {
        if (combo_->count() > 0) {
            combo_->setCurrentIndex(0);
            if (param_ != nullptr && param_->options().contains(QStringLiteral("value"))) {
                const QVariant first = combo_->value();
                if (first.isValid() && param_->value() != first) {
                    param_->setValue(first);
                }
            }
        }
    }
}

void ListParameterItem::configureEditor(QWidget* editor)
{
    if (auto* combo = qobject_cast<widgets::ComboBox*>(editor)) {
        combo->setMaximumHeight(20);
    }
}

void ListParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    WidgetParameterItem::optsChanged(param, opts);
    if (opts.contains(QStringLiteral("limits")) || opts.contains(QStringLiteral("values"))) {
        updateLimits(listLimitsFromOptions(param->options()));
    }
}

void ListParameterItem::updateLimits(const QVariant& limits)
{
    if (combo_ == nullptr || param_ == nullptr) {
        return;
    }

    updatingWidget_ = true;
    const QSignalBlocker blocker(combo_);
    const QVariant normalized = limits.isValid() ? limits : QVariant(QStringList{QString()});
    combo_->setItems(normalized);

    if (param_->options().contains(QStringLiteral("value"))) {
        if (!valueInLimits(param_->value(), normalized)) {
            const QVariant first = firstLimitValue(normalized);
            if (first.isValid()) {
                param_->setValue(first);
            }
        } else {
            writeEditorValue(param_->value());
        }
    } else if (combo_->count() > 0) {
        combo_->setCurrentIndex(0);
    }
    updateDisplayLabel(combo_->currentText());
    updatingWidget_ = false;
}

ColorParameterItem::ColorParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param, depth, new widgets::ColorButton(), WidgetParameterItemOptions{.hideWhenDeselected = false})
{
    bindEditor(editor_);
    writeEditorValue(param->value());
}

void ColorParameterItem::bindEditor(QWidget* editor)
{
    auto* button = qobject_cast<widgets::ColorButton*>(editor);
    if (button == nullptr) {
        return;
    }
    QObject::connect(button, &widgets::ColorButton::sigColorChanged, button, [this](widgets::ColorButton*) {
        widgetValueChanged();
    });
    QObject::connect(button, &widgets::ColorButton::sigColorChanging, button, [this](widgets::ColorButton* btn) {
        editorValueChanging(QVariant::fromValue(btn->color()));
    });
}

QVariant ColorParameterItem::readEditorValue() const
{
    if (auto* button = qobject_cast<const widgets::ColorButton*>(editor_)) {
        return QVariant::fromValue(button->color());
    }
    return QVariant();
}

void ColorParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* button = qobject_cast<widgets::ColorButton*>(editor_)) {
        QColor color = val.metaType().id() == QMetaType::QColor ? val.value<QColor>() : mkColor(val.toString());
        button->setColor(color, true);
    }
}

void ColorParameterItem::configureEditor(QWidget* editor)
{
    if (auto* button = qobject_cast<widgets::ColorButton*>(editor)) {
        button->setFlat(true);
    }
}

TextParameterItem::TextParameterItem(Parameter* param, int depth)
    : WidgetParameterItem(param,
                          depth,
                          new QTextEdit(),
                          WidgetParameterItemOptions{.hideWhenDeselected = false, .asSubItem = true})
{
    bindEditor(editor_);
    writeEditorValue(param->value());
}

void TextParameterItem::bindEditor(QWidget* editor)
{
    auto* textEdit = qobject_cast<QTextEdit*>(editor);
    if (textEdit == nullptr) {
        return;
    }
    QObject::connect(textEdit, &QTextEdit::textChanged, textEdit, [this]() { widgetValueChanged(); });
}

QVariant TextParameterItem::readEditorValue() const
{
    if (auto* textEdit = qobject_cast<const QTextEdit*>(editor_)) {
        return textEdit->toPlainText();
    }
    return QVariant();
}

void TextParameterItem::writeEditorValue(const QVariant& val)
{
    if (auto* textEdit = qobject_cast<QTextEdit*>(editor_)) {
        const QSignalBlocker blocker(textEdit);
        textEdit->setPlainText(val.toString());
    }
}

void TextParameterItem::configureEditor(QWidget* editor)
{
    if (auto* textEdit = qobject_cast<QTextEdit*>(editor)) {
        textEdit->setReadOnly(param_->readonly());
    }
}

void TextParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    WidgetParameterItem::optsChanged(param, opts);
    if (opts.contains(QStringLiteral("readonly"))) {
        if (auto* textEdit = qobject_cast<QTextEdit*>(editor_)) {
            textEdit->setReadOnly(opts.value(QStringLiteral("readonly")).toBool());
        }
    }
}

namespace {

bool actionIsInteractive(const Parameter* param)
{
    if (param == nullptr) {
        return false;
    }
    const QVariantMap& options = param->options();
    return options.value(QStringLiteral("enabled"), true).toBool()
        && options.value(QStringLiteral("visible"), true).toBool();
}

} // namespace

ActionParameterItem::ActionParameterItem(Parameter* param, int depth)
    : ParameterItem(param, depth)
{
    layoutWidget_ = new QWidget();
    auto* layout = new QHBoxLayout(layoutWidget_);
    layout->setContentsMargins(0, 0, 0, 0);

    button_ = new QPushButton(layoutWidget_);
    button_->setAutoDefault(false);
    QObject::connect(button_, &QPushButton::clicked, button_, [this]() {
        if (auto* action = dynamic_cast<ActionParameter*>(param_)) {
            action->activate();
        }
    });
    layout->addWidget(button_);
    layout->addStretch();

    updateButton();
    updateShortcut();
}

ActionParameterItem::~ActionParameterItem()
{
    delete shortcut_;
}

void ActionParameterItem::nameChanged(Parameter* param, const QString& name)
{
    ParameterItem::nameChanged(param, name);
    updateButton();
}

void ActionParameterItem::treeWidgetChanged()
{
    ParameterItem::treeWidgetChanged();
    if (layoutWidget_ == nullptr) {
        return;
    }

    if (auto* tree = treeWidget()) {
        setFirstColumnSpanned(true);
        tree->setItemWidget(this, 0, layoutWidget_);
    }
    updateShortcut();
}

void ActionParameterItem::optsChanged(Parameter* param, const QVariantMap& opts)
{
    ParameterItem::optsChanged(param, opts);
    if (opts.contains(QStringLiteral("enabled"))) {
        if (button_ != nullptr) {
            button_->setEnabled(opts.value(QStringLiteral("enabled")).toBool());
        }
        if (shortcut_ != nullptr) {
            shortcut_->setEnabled(actionIsInteractive(param));
        }
    }
    if (opts.contains(QStringLiteral("visible"))) {
        if (button_ != nullptr) {
            button_->setVisible(opts.value(QStringLiteral("visible")).toBool());
        }
        if (shortcut_ != nullptr) {
            shortcut_->setEnabled(actionIsInteractive(param));
        }
    }
    if (opts.contains(QStringLiteral("tip"))) {
        if (button_ != nullptr) {
            button_->setToolTip(opts.value(QStringLiteral("tip")).toString());
        }
    }
    if (opts.contains(QStringLiteral("shortcut"))) {
        updateShortcut();
    }
    if (opts.contains(QStringLiteral("title")) || opts.contains(QStringLiteral("name"))
        || opts.contains(QStringLiteral("icon"))) {
        updateButton();
    }
}

void ActionParameterItem::updateButton()
{
    if (button_ == nullptr || param_ == nullptr) {
        return;
    }

    button_->setText(param_->title());
    if (param_->options().contains(QStringLiteral("tip"))) {
        button_->setToolTip(param_->options().value(QStringLiteral("tip")).toString());
    }
    button_->setVisible(param_->options().value(QStringLiteral("visible"), true).toBool());
    button_->setEnabled(param_->options().value(QStringLiteral("enabled"), true).toBool());
    if (param_->options().contains(QStringLiteral("icon"))) {
        const QVariant iconValue = param_->options().value(QStringLiteral("icon"));
        if (iconValue.isValid() && !iconValue.isNull()) {
            button_->setIcon(QIcon(iconValue.toString()));
        } else {
            button_->setIcon(QIcon());
        }
    }
    setSizeHint(0, button_->sizeHint());
}

void ActionParameterItem::updateShortcut()
{
    delete shortcut_;
    shortcut_ = nullptr;
    if (param_ == nullptr || treeWidget() == nullptr) {
        return;
    }

    const QString shortcutText = param_->options().value(QStringLiteral("shortcut")).toString();
    if (shortcutText.isEmpty()) {
        return;
    }

    shortcut_ = new QShortcut(QKeySequence(shortcutText), treeWidget());
    shortcut_->setContext(Qt::WidgetWithChildrenShortcut);
    shortcut_->setEnabled(actionIsInteractive(param_));
    QObject::connect(shortcut_, &QShortcut::activated, shortcut_, [this]() {
        if (!actionIsInteractive(param_)) {
            return;
        }
        if (auto* action = dynamic_cast<ActionParameter*>(param_)) {
            action->activate();
        }
    });
}

} // namespace cppqtgraph::parametertree
