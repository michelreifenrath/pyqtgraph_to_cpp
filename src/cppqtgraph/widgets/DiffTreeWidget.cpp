// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/DiffTreeWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/DiffTreeWidget.hpp"

#include "../../../include/cppqtgraph/functions.hpp"
#include "../../../include/cppqtgraph/widgets/DataTreeWidget.hpp"

#include <QtCore/QMetaType>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTreeWidgetItem>

#include <cmath>
#include <limits>

namespace cppqtgraph::widgets {

namespace {

constexpr qint64 kIntNanSentinel = std::numeric_limits<qint64>::min();

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

bool valuesClose(double a, double b)
{
    return std::fabs(a - b) <= 1e-9 * std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
}

bool isNanLike(const QVariant& value)
{
    if (!value.canConvert<double>()) {
        return false;
    }
    const double numeric = value.toDouble();
    if (std::isnan(numeric)) {
        return true;
    }
    if (value.canConvert<qint64>() && value.toLongLong() == kIntNanSentinel) {
        return true;
    }
    return false;
}

QSet<QString> childKeys(const QList<DataTreeWidget::ChildNode>& children)
{
    QSet<QString> keys;
    for (const DataTreeWidget::ChildNode& child : children) {
        keys.insert(child.pathSegment.toString());
    }
    return keys;
}

} // namespace

DiffTreeWidget::DiffTreeWidget(QWidget* parent)
    : QWidget(parent)
    , layout_(new QHBoxLayout(this))
{
    for (int index = 0; index < 2; ++index) {
        trees_[index] = new DataTreeWidget(this);
        layout_->addWidget(trees_[index]);
    }
}

DiffTreeWidget::DiffTreeWidget(QWidget* parent, const QVariant& a, const QVariant& b)
    : DiffTreeWidget(parent)
{
    setData(a, b);
}

void DiffTreeWidget::setData(const QVariant& a, const QVariant& b)
{
    data_[0] = a;
    data_[1] = b;
    trees_[0]->setData(a);
    trees_[1]->setData(b);
    compare(a, b);
}

void DiffTreeWidget::compare(const QVariant& a, const QVariant& b, const QVariantList& path)
{
    const QColor bad = cppqtgraph::mkColor(255, 200, 200);

    const DataTreeWidget::ParseResult parsedA = trees_[0]->parse(a);
    const DataTreeWidget::ParseResult parsedB = trees_[1]->parse(b);

    if (parsedA.typeStr != parsedB.typeStr) {
        setColor(path, 1, bad);
    }
    if (parsedA.desc != parsedB.desc) {
        setColor(path, 2, bad);
    }

    if (a.typeId() == QMetaType::QVariantMap && b.typeId() == QMetaType::QVariantMap) {
        const QVariantMap mapA = a.toMap();
        const QVariantMap mapB = b.toMap();

        if (isArrayMap(mapA) && isArrayMap(mapB)) {
            const QVariantList shapeA = arrayShape(mapA);
            const QVariantList shapeB = arrayShape(mapB);
            if (shapeA == shapeB && arrayDtype(mapA) == arrayDtype(mapB)) {
                compareArrays(shapeA, arrayValues(mapA), arrayValues(mapB), path);
            }
            return;
        }

        const QSet<QString> keysA = childKeys(parsedA.children);
        const QSet<QString> keysB = childKeys(parsedB.children);
        for (const QString& key : keysA - keysB) {
            QVariantList childPath = path;
            childPath.append(key);
            setColor(childPath, 0, bad, 0);
        }
        for (const QString& key : keysB - keysA) {
            QVariantList childPath = path;
            childPath.append(key);
            setColor(childPath, 0, bad, 1);
        }
        for (const QString& key : keysA & keysB) {
            QVariantList childPath = path;
            childPath.append(key);
            compare(mapA.value(key), mapB.value(key), childPath);
        }
        return;
    }

    if (a.typeId() == QMetaType::QVariantList && b.typeId() == QMetaType::QVariantList) {
        const QVariantList listA = a.toList();
        const QVariantList listB = b.toList();
        const int maxLength = std::max(listA.size(), listB.size());
        for (int index = 0; index < maxLength; ++index) {
            QVariantList childPath = path;
            childPath.append(index);
            if (index >= listA.size()) {
                setColor(childPath, 0, bad, 1);
            } else if (index >= listB.size()) {
                setColor(childPath, 0, bad, 0);
            } else {
                compare(listA.at(index), listB.at(index), childPath);
            }
        }
    }
}

void DiffTreeWidget::compareArrays(
    const QVariantList& shape,
    const QVariantList& valuesA,
    const QVariantList& valuesB,
    const QVariantList& path)
{
    Q_UNUSED(shape);
    const QColor bad = cppqtgraph::mkColor(255, 200, 200);

    bool allEqual = valuesA.size() == valuesB.size();
    if (allEqual) {
        for (int index = 0; index < valuesA.size(); ++index) {
            const QVariant valueA = valuesA.at(index);
            const QVariant valueB = valuesB.at(index);
            const bool nanA = isNanLike(valueA);
            const bool nanB = isNanLike(valueB);
            if (nanA != nanB) {
                allEqual = false;
                break;
            }
            if (!nanA && !valuesClose(valueA.toDouble(), valueB.toDouble())) {
                allEqual = false;
                break;
            }
        }
    }

    if (allEqual) {
        return;
    }

    QTreeWidgetItem* nodeA = trees_[0]->nodeAtPath(path);
    QTreeWidgetItem* nodeB = trees_[1]->nodeAtPath(path);
    if (nodeA == nullptr || nodeB == nullptr || nodeA->childCount() == 0 || nodeB->childCount() == 0) {
        return;
    }

    const QBrush brush = cppqtgraph::mkBrush(bad);
    nodeA->child(0)->setBackground(0, brush);
    nodeB->child(0)->setBackground(0, brush);
}

void DiffTreeWidget::setColor(const QVariantList& path, int column, const QColor& color, int treeIndex)
{
    const QBrush brush = cppqtgraph::mkBrush(color);
    const int start = treeIndex < 0 ? 0 : treeIndex;
    const int end = treeIndex < 0 ? 1 : treeIndex;
    for (int index = start; index <= end; ++index) {
        QTreeWidgetItem* item = trees_[index]->nodeAtPath(path);
        if (item != nullptr) {
            item->setBackground(column, brush);
        }
    }
}

DataTreeWidget* DiffTreeWidget::tree(int index) const
{
    if (index < 0 || index >= static_cast<int>(trees_.size())) {
        return nullptr;
    }
    return trees_[index];
}

} // namespace cppqtgraph::widgets
