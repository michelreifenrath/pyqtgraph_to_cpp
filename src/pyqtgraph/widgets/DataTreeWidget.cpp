// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/DataTreeWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/DataTreeWidget.hpp"

#include <QtCore/QMetaType>
#include <QtWidgets/QPlainTextEdit>

namespace pyqtgraph::widgets {

namespace {

QString pathKey(const QVariantList& path)
{
    QStringList parts;
    for (const QVariant& segment : path) {
        const QString text = segment.toString();
        parts.append(QStringLiteral("%1:%2:%3")
                         .arg(segment.metaType().id())
                         .arg(text.size())
                         .arg(text));
    }
    return parts.join(QChar(0x1E));
}

bool isArrayMap(const QVariantMap& map)
{
    return map.contains(QStringLiteral("__pyqtgraph_ndarray__"));
}

QVariantList arrayShape(const QVariantMap& map)
{
    return map.value(QStringLiteral("shape")).toList();
}

QVariantList arrayValues(const QVariantMap& map)
{
    return map.value(QStringLiteral("values")).toList();
}

QString arrayDtype(const QVariantMap& map)
{
    return map.value(QStringLiteral("dtype")).toString();
}

QString formatArrayValues(const QVariantList& values)
{
    QStringList rows;
    for (const QVariant& value : values) {
        rows.append(value.toString());
    }
    return rows.join(QLatin1Char('\n'));
}

QWidget* makeReadOnlyTextWidget(const QString& text, QWidget* parent)
{
    auto* widget = new QPlainTextEdit(parent);
    widget->setPlainText(text);
    widget->setMaximumHeight(200);
    widget->setReadOnly(true);
    return widget;
}

} // namespace

DataTreeWidget::DataTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setColumnCount(3);
    setHeaderLabels({QStringLiteral("key / index"), QStringLiteral("type"), QStringLiteral("value")});
    setAlternatingRowColors(true);
}

DataTreeWidget::DataTreeWidget(QWidget* parent, const QVariant& data)
    : DataTreeWidget(parent)
{
    setData(data);
}

void DataTreeWidget::setData(const QVariant& data, bool hideRoot)
{
    for (const QPointer<QWidget>& widget : widgets_) {
        delete widget.data();
    }
    widgets_.clear();
    clear();
    nodes_.clear();
    buildTree(data, invisibleRootItem(), QString(), hideRoot, {});
    expandToDepth(3);
    resizeColumnToContents(0);
}

void DataTreeWidget::buildTree(
    const QVariant& data,
    QTreeWidgetItem* parent,
    const QString& name,
    bool hideRoot,
    const QVariantList& path)
{
    QTreeWidgetItem* node = parent;
    if (!hideRoot) {
        node = new QTreeWidgetItem({name, QString(), QString()});
        parent->addChild(node);
    }

    nodes_.insert(pathKey(path), node);

    const ParseResult parsed = parse(data, this);

    QString desc = parsed.desc;
    if (desc.size() > 100) {
        desc = desc.left(97) + QStringLiteral("...");
    }

    QWidget* widget = parsed.widget;
    if (widget == nullptr && parsed.desc.size() > 100) {
        widget = makeReadOnlyTextWidget(data.toString(), this);
    }

    node->setText(1, parsed.typeStr);
    node->setText(2, desc);

    if (widget != nullptr) {
        widgets_.append(widget);
        auto* subnode = new QTreeWidgetItem({QString(), QString(), QString()});
        node->addChild(subnode);
        setItemWidget(subnode, 0, widget);
        subnode->setFirstColumnSpanned(true);
    }

    for (const ChildNode& child : parsed.children) {
        QVariantList childPath = path;
        childPath.append(child.pathSegment);
        buildTree(child.value, node, child.pathSegment.toString(), false, childPath);
    }
}

DataTreeWidget::ParseResult DataTreeWidget::parse(const QVariant& data, QWidget* widgetParent) const
{
    ParseResult result;
    result.typeStr = QString::fromLatin1(data.typeName());

    if (data.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = data.toMap();
        if (isArrayMap(map)) {
            const QVariantList shape = arrayShape(map);
            QStringList shapeParts;
            for (const QVariant& extent : shape) {
                shapeParts.append(extent.toString());
            }
            result.typeStr = QStringLiteral("ndarray");
            result.desc = QStringLiteral("shape=%1 dtype=%2")
                              .arg(shapeParts.join(QLatin1Char('x')), arrayDtype(map));
            if (widgetParent != nullptr) {
                result.widget = makeReadOnlyTextWidget(formatArrayValues(arrayValues(map)), widgetParent);
            }
            return result;
        }

        result.typeStr = QStringLiteral("dict");
        result.desc = QStringLiteral("length=%1").arg(map.size());
        QStringList keys = map.keys();
        keys.sort();
        for (const QString& key : keys) {
            result.children.append({key, map.value(key)});
        }
        return result;
    }

    if (data.typeId() == QMetaType::QVariantList) {
        const QVariantList list = data.toList();
        result.typeStr = QStringLiteral("list");
        result.desc = QStringLiteral("length=%1").arg(list.size());
        for (int index = 0; index < list.size(); ++index) {
            result.children.append({index, list.at(index)});
        }
        return result;
    }

    result.desc = data.toString();
    return result;
}

QTreeWidgetItem* DataTreeWidget::nodeAtPath(const QVariantList& path) const
{
    return nodes_.value(pathKey(path), nullptr);
}

} // namespace pyqtgraph::widgets
