// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/ParameterItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/ParameterItem.hpp"
#include "../../../include/cppqtgraph/parametertree/ParameterTree.hpp"

#include <QtCore/QObject>
#include <QtCore/QSignalBlocker>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

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
    return value.toString();
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

WidgetParameterItem::WidgetParameterItem(Parameter* param, int depth)
    : ParameterItem(param, depth)
{
    editor_ = new QLineEdit();
    auto* filter = new EditorEventFilter(this, editor_);
    editor_->installEventFilter(filter);

    displayLabel_ = new QLabel();
    defaultBtn_ = new QPushButton(QStringLiteral("↺"));
    defaultBtn_->setAutoDefault(false);
    defaultBtn_->setFixedSize(20, 20);

    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(editor_, 1);
    layout->addWidget(displayLabel_, 1);
    layout->addStretch(0);
    layout->addWidget(defaultBtn_);

    layoutWidget_ = new QWidget();
    layoutWidget_->setLayout(layout);

    new WidgetHooks(this, qobject_cast<QLineEdit*>(editor_), defaultBtn_);

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
        tree->setItemWidget(this, 1, layoutWidget_);
        hideEditor();
        selected(false);
    }
}

void WidgetParameterItem::valueChanged(Parameter* param, const QVariant& val)
{
    ParameterItem::valueChanged(param, val);
    if (updatingWidget_) {
        return;
    }

    updatingWidget_ = true;
    if (auto* lineEdit = qobject_cast<QLineEdit*>(editor_)) {
        const QSignalBlocker blocker(lineEdit);
        lineEdit->setText(val.toString());
    }
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

    if (opts.contains(QStringLiteral("enabled")) && editor_ != nullptr) {
        editor_->setEnabled(opts.value(QStringLiteral("enabled")).toBool());
        updateDefaultBtn();
    }
    if (opts.contains(QStringLiteral("readonly")) && editor_ != nullptr) {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(editor_)) {
            lineEdit->setReadOnly(opts.value(QStringLiteral("readonly")).toBool());
        }
        updateDefaultBtn();
    }
    if (opts.contains(QStringLiteral("tip")) && editor_ != nullptr) {
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
    } else if (hideWidget_) {
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

    const auto* lineEdit = qobject_cast<const QLineEdit*>(editor_);
    if (lineEdit == nullptr) {
        return;
    }

    param_->setValue(lineEdit->text());
    updateDisplayLabel();
    updateDefaultBtn();
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
    displayLabel_->setText(display.toString());
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

} // namespace cppqtgraph::parametertree
