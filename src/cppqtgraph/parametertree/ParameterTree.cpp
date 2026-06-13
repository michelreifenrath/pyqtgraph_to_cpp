// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/ParameterTree.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/ParameterTree.hpp"
#include "../../../include/cppqtgraph/parametertree/ParameterItem.hpp"

#include <QtWidgets/QHeaderView>

namespace cppqtgraph::parametertree {

ParameterTree::ParameterTree(QWidget* parent, bool showHeader)
    : widgets::TreeWidget(parent)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setAnimated(false);
    setColumnCount(2);
    setHeaderLabels({QStringLiteral("Parameter"), QStringLiteral("Value")});
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setHeaderHidden(!showHeader);
    setRootIsDecorated(false);
    setAlternatingRowColors(true);

    connect(this, &QTreeWidget::itemChanged, this, &ParameterTree::itemChangedEvent);
    connect(this, &QTreeWidget::itemExpanded, this, &ParameterTree::itemExpandedEvent);
    connect(this, &QTreeWidget::itemCollapsed, this, &ParameterTree::itemCollapsedEvent);
    connect(this, &QTreeWidget::currentItemChanged, this, &ParameterTree::currentItemChangedEvent);
}

void ParameterTree::setParameters(const std::shared_ptr<Parameter>& param, bool showTop)
{
    clearParameters();
    paramSet_ = param;
    if (paramSet_ != nullptr) {
        addParameters(paramSet_.get(), nullptr, 0, showTop);
    }
}

void ParameterTree::addParameters(Parameter* param, QTreeWidgetItem* root, int depth, bool showTop)
{
    if (param == nullptr) {
        return;
    }

    ParameterItem* item = param->makeTreeItem(depth);
    if (root == nullptr) {
        root = invisibleRootItem();
        if (!showTop) {
            item->setText(0, QString());
            item->setSizeHint(0, QSize(1, 1));
            item->setSizeHint(1, QSize(1, 1));
            depth -= 1;
        }
    }
    root->addChild(item);
    item->treeWidgetChanged();
    widgets::TreeWidget::informTreeWidgetChange(item);

    for (const std::shared_ptr<Parameter>& child : param->children()) {
        addParameters(child.get(), item, depth + 1, true);
    }
}

void ParameterTree::clearParameters()
{
    const QList<QTreeWidgetItem*> children = invisibleRootItem()->takeChildren();
    for (QTreeWidgetItem* child : children) {
        delete child;
    }
    paramSet_.reset();
    lastSel_ = nullptr;
}

void ParameterTree::focusNext(ParameterItem* item, bool forward)
{
    while (item != nullptr) {
        QTreeWidgetItem* parent = item->parent();
        if (parent == nullptr) {
            return;
        }

        ParameterItem* nextItem = nextFocusableChild(parent, item, forward);
        if (nextItem != nullptr) {
            nextItem->setFocus();
            setCurrentItem(nextItem);
            return;
        }
        item = dynamic_cast<ParameterItem*>(parent);
    }
}

void ParameterTree::focusPrevious(ParameterItem* item)
{
    focusNext(item, false);
}

ParameterItem* ParameterTree::nextFocusableChild(QTreeWidgetItem* root,
                                               QTreeWidgetItem* startItem,
                                               bool forward) const
{
    int index = 0;
    if (startItem == nullptr) {
        index = forward ? 0 : root->childCount() - 1;
    } else if (forward) {
        index = root->indexOfChild(startItem) + 1;
    } else {
        index = root->indexOfChild(startItem) - 1;
    }

    auto tryChild = [&](int i) -> ParameterItem* {
        auto* item = dynamic_cast<ParameterItem*>(root->child(i));
        if (item != nullptr && item->isFocusable()) {
            return item;
        }
        if (item != nullptr) {
            if (ParameterItem* nested = nextFocusableChild(item, nullptr, forward)) {
                return nested;
            }
        }
        return nullptr;
    };

    if (forward) {
        for (int i = index; i < root->childCount(); ++i) {
            if (ParameterItem* found = tryChild(i)) {
                return found;
            }
        }
    } else {
        for (int i = index; i >= 0; --i) {
            if (ParameterItem* found = tryChild(i)) {
                return found;
            }
        }
    }
    return nullptr;
}

void ParameterTree::currentItemChangedEvent(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (auto* prev = dynamic_cast<ParameterItem*>(previous)) {
        prev->selected(false);
    }
    if (auto* curr = dynamic_cast<ParameterItem*>(current)) {
        curr->selected(true);
    }
    lastSel_ = current;
}

void ParameterTree::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);
    QTreeWidget::selectionChanged(selected, deselected);
}

void ParameterTree::itemChangedEvent(QTreeWidgetItem* item, int col)
{
    if (auto* paramItem = dynamic_cast<ParameterItem*>(item)) {
        paramItem->columnChangedEvent(col);
    }
}

void ParameterTree::itemExpandedEvent(QTreeWidgetItem* item)
{
    if (auto* paramItem = dynamic_cast<ParameterItem*>(item)) {
        paramItem->expandedChangedEvent(true);
    }
}

void ParameterTree::itemCollapsedEvent(QTreeWidgetItem* item)
{
    if (auto* paramItem = dynamic_cast<ParameterItem*>(item)) {
        paramItem->expandedChangedEvent(false);
    }
}

} // namespace cppqtgraph::parametertree
