// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/TreeWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/TreeWidget.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>

namespace pyqtgraph::widgets {

namespace {

class ItemWidgetPlaceholder : public QWidget {
public:
    explicit ItemWidgetPlaceholder(QWidget* child, QWidget* parent = nullptr)
        : QWidget(parent)
        , realChild_(child)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
        if (realChild_ != nullptr) {
            setSizePolicy(realChild_->sizePolicy());
            setMinimumHeight(realChild_->minimumHeight());
            setMinimumWidth(realChild_->minimumWidth());
            layout->addWidget(realChild_);
        }
    }

    [[nodiscard]] QWidget* realChild() const { return realChild_; }

private:
    QWidget* realChild_ = nullptr;
};

TreeWidgetItem* asTreeWidgetItem(QTreeWidgetItem* item)
{
    return dynamic_cast<TreeWidgetItem*>(item);
}

} // namespace

TreeWidgetItem::TreeWidgetItem(const QStringList& strings)
    : QTreeWidgetItem(strings)
{
}

TreeWidgetItem::TreeWidgetItem(QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent)
{
}

TreeWidgetItem::TreeWidgetItem(const QStringList& strings, QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent)
{
    for (int i = 0; i < strings.size(); ++i) {
        setText(i, strings.at(i));
    }
}

void TreeWidgetItem::setChecked(int column, bool checked)
{
    setData(column, Qt::CheckStateRole, checked ? Qt::Checked : Qt::Unchecked);
}

bool TreeWidgetItem::isChecked(int column) const
{
    return checkState(column) == Qt::Checked;
}

void TreeWidgetItem::setExpanded(bool expanded)
{
    expanded_ = expanded;
    QTreeWidgetItem::setExpanded(expanded);
}

bool TreeWidgetItem::isExpanded() const
{
    return expanded_;
}

void TreeWidgetItem::setWidget(int column, QWidget* widget)
{
    if (widgets_.contains(column)) {
        removeWidget(column);
    }
    widgets_.insert(column, widget);
    if (auto* tree = treeWidget()) {
        if (auto* treeWidget = dynamic_cast<TreeWidget*>(tree)) {
            treeWidget->setItemWidget(this, column, widget);
        }
    }
}

void TreeWidgetItem::removeWidget(int column)
{
    widgets_.remove(column);
    if (auto* tree = treeWidget()) {
        tree->removeItemWidget(this, column);
    }
}

void TreeWidgetItem::addChild(QTreeWidgetItem* child)
{
    QTreeWidgetItem::addChild(child);
    TreeWidget::informTreeWidgetChange(child);
}

void TreeWidgetItem::addChildren(const QList<QTreeWidgetItem*>& children)
{
    QTreeWidgetItem::addChildren(children);
    for (QTreeWidgetItem* child : children) {
        TreeWidget::informTreeWidgetChange(child);
    }
}

void TreeWidgetItem::insertChild(int index, QTreeWidgetItem* child)
{
    QTreeWidgetItem::insertChild(index, child);
    TreeWidget::informTreeWidgetChange(child);
}

void TreeWidgetItem::insertChildren(int index, const QList<QTreeWidgetItem*>& children)
{
    QTreeWidgetItem::insertChildren(index, children);
    for (QTreeWidgetItem* child : children) {
        TreeWidget::informTreeWidgetChange(child);
    }
}

void TreeWidgetItem::removeChild(QTreeWidgetItem* child)
{
    QTreeWidgetItem::removeChild(child);
    TreeWidget::informTreeWidgetChange(child);
}

QTreeWidgetItem* TreeWidgetItem::takeChild(int index)
{
    QTreeWidgetItem* child = QTreeWidgetItem::takeChild(index);
    TreeWidget::informTreeWidgetChange(child);
    return child;
}

QList<QTreeWidgetItem*> TreeWidgetItem::takeChildren()
{
    QList<QTreeWidgetItem*> children = QTreeWidgetItem::takeChildren();
    for (QTreeWidgetItem* child : children) {
        TreeWidget::informTreeWidgetChange(child);
    }
    return children;
}

void TreeWidgetItem::treeWidgetChanged()
{
    auto* tree = treeWidget();
    if (tree == nullptr) {
        return;
    }
    auto* treeWidget = dynamic_cast<TreeWidget*>(tree);
    if (treeWidget == nullptr) {
        return;
    }
    for (auto it = widgets_.cbegin(); it != widgets_.cend(); ++it) {
        if (it.value() != nullptr) {
            treeWidget->setItemWidget(this, it.key(), it.value());
        }
    }
    QTreeWidgetItem::setExpanded(expanded_);
}

void TreeWidgetItem::setData(int column, int role, const QVariant& value)
{
    const auto previousCheckState = checkState(column);
    const QString previousText = text(column);
    QTreeWidgetItem::setData(column, role, value);

    auto* tree = treeWidget();
    if (tree == nullptr) {
        return;
    }
    auto* treeWidget = dynamic_cast<TreeWidget*>(tree);
    if (treeWidget == nullptr) {
        return;
    }
    if (role == Qt::CheckStateRole && previousCheckState != checkState(column)) {
        emit treeWidget->sigItemCheckStateChanged(this, column);
    } else if ((role == Qt::DisplayRole || role == Qt::EditRole) && previousText != text(column)) {
        emit treeWidget->sigItemTextChanged(this, column);
    }
}

TreeWidget::TreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
}

void TreeWidget::setItemWidget(QTreeWidgetItem* item, int column, QWidget* widget)
{
    auto* placeholder = new ItemWidgetPlaceholder(widget);
    placeholders_.append(placeholder);
    QTreeWidget::setItemWidget(item, column, placeholder);
}

QWidget* TreeWidget::itemWidget(QTreeWidgetItem* item, int column) const
{
    QWidget* widget = QTreeWidget::itemWidget(item, column);
    if (auto* placeholder = dynamic_cast<ItemWidgetPlaceholder*>(widget)) {
        return placeholder->realChild();
    }
    return widget;
}

void TreeWidget::addTopLevelItem(QTreeWidgetItem* item)
{
    QTreeWidget::addTopLevelItem(item);
    informTreeWidgetChange(item);
}

void TreeWidget::addTopLevelItems(const QList<QTreeWidgetItem*>& items)
{
    QTreeWidget::addTopLevelItems(items);
    for (QTreeWidgetItem* item : items) {
        informTreeWidgetChange(item);
    }
}

void TreeWidget::insertTopLevelItem(int index, QTreeWidgetItem* item)
{
    QTreeWidget::insertTopLevelItem(index, item);
    informTreeWidgetChange(item);
}

void TreeWidget::insertTopLevelItems(int index, const QList<QTreeWidgetItem*>& items)
{
    QTreeWidget::insertTopLevelItems(index, items);
    for (QTreeWidgetItem* item : items) {
        informTreeWidgetChange(item);
    }
}

void TreeWidget::setColumnCount(int columns)
{
    QTreeWidget::setColumnCount(columns);
    emit sigColumnCountChanged(this, columns);
}

void TreeWidget::informTreeWidgetChange(QTreeWidgetItem* item)
{
    if (item == nullptr) {
        return;
    }
    if (auto* treeItem = asTreeWidgetItem(item)) {
        treeItem->treeWidgetChanged();
    }
    for (int i = 0; i < item->childCount(); ++i) {
        informTreeWidgetChange(item->child(i));
    }
}

} // namespace pyqtgraph::widgets
