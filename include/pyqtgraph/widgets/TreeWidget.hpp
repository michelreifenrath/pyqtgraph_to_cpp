#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/TreeWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QTreeWidget>

#include <QHash>
#include <QPointer>

class QWidget;

namespace pyqtgraph::widgets {

class TreeWidget;

class TreeWidgetItem : public QTreeWidgetItem {
public:
    TreeWidgetItem() = default;
    explicit TreeWidgetItem(const QStringList& strings);
    explicit TreeWidgetItem(QTreeWidgetItem* parent);
    explicit TreeWidgetItem(const QStringList& strings, QTreeWidgetItem* parent);

    void setChecked(int column, bool checked);
    [[nodiscard]] bool isChecked(int column) const;

    void setExpanded(bool expanded);
    [[nodiscard]] bool isExpanded() const;

    void setWidget(int column, QWidget* widget);
    void removeWidget(int column);

    void addChild(QTreeWidgetItem* child);
    void addChildren(const QList<QTreeWidgetItem*>& children);
    void insertChild(int index, QTreeWidgetItem* child);
    void insertChildren(int index, const QList<QTreeWidgetItem*>& children);
    void removeChild(QTreeWidgetItem* child);
    QTreeWidgetItem* takeChild(int index);
    QList<QTreeWidgetItem*> takeChildren();

    void treeWidgetChanged();

    void setData(int column, int role, const QVariant& value) override;

private:
    QHash<int, QPointer<QWidget>> widgets_;
    bool expanded_ = false;
};

class TreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit TreeWidget(QWidget* parent = nullptr);

    TreeWidget(const TreeWidget&) = delete;
    TreeWidget& operator=(const TreeWidget&) = delete;
    TreeWidget(TreeWidget&&) = delete;
    TreeWidget& operator=(TreeWidget&&) = delete;

    void setItemWidget(QTreeWidgetItem* item, int column, QWidget* widget);
    [[nodiscard]] QWidget* itemWidget(QTreeWidgetItem* item, int column) const;

    void addTopLevelItem(QTreeWidgetItem* item);
    void addTopLevelItems(const QList<QTreeWidgetItem*>& items);
    void insertTopLevelItem(int index, QTreeWidgetItem* item);
    void insertTopLevelItems(int index, const QList<QTreeWidgetItem*>& items);

    void setColumnCount(int columns);

    static void informTreeWidgetChange(QTreeWidgetItem* item);

signals:
    void sigItemMoved(QTreeWidgetItem* item, QTreeWidgetItem* parent, int index);
    void sigItemCheckStateChanged(QTreeWidgetItem* item, int column);
    void sigItemTextChanged(QTreeWidgetItem* item, int column);
    void sigColumnCountChanged(TreeWidget* tree, int count);

private:
    QList<QWidget*> placeholders_;
};

} // namespace pyqtgraph::widgets
