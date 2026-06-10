#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/DataTreeWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QVariant>
#include <QtWidgets/QTreeWidget>

#include <QHash>
#include <QList>
#include <QPointer>

class QWidget;

namespace cppqtgraph::widgets {

class DataTreeWidget : public QTreeWidget {
public:
    struct ChildNode {
        QVariant pathSegment;
        QVariant value;
    };

    struct ParseResult {
        QString typeStr;
        QString desc;
        QList<ChildNode> children;
        QWidget* widget = nullptr;
    };

    explicit DataTreeWidget(QWidget* parent = nullptr);
    DataTreeWidget(QWidget* parent, const QVariant& data);

    DataTreeWidget(const DataTreeWidget&) = delete;
    DataTreeWidget& operator=(const DataTreeWidget&) = delete;
    DataTreeWidget(DataTreeWidget&&) = delete;
    DataTreeWidget& operator=(DataTreeWidget&&) = delete;

    void setData(const QVariant& data, bool hideRoot = false);

    [[nodiscard]] ParseResult parse(const QVariant& data, QWidget* widgetParent = nullptr) const;
    [[nodiscard]] QTreeWidgetItem* nodeAtPath(const QVariantList& path) const;

private:
    void buildTree(
        const QVariant& data,
        QTreeWidgetItem* parent,
        const QString& name,
        bool hideRoot,
        const QVariantList& path);

    QList<QPointer<QWidget>> widgets_;
    QHash<QString, QTreeWidgetItem*> nodes_;
};

} // namespace cppqtgraph::widgets
