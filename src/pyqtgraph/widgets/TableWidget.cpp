// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/TableWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/TableWidget.hpp"

#include <QtCore/QMetaType>
#include <QtWidgets/QHeaderView>

namespace pyqtgraph::widgets {

namespace {

QVariantList variantListFromVariant(const QVariant& value)
{
    const QVariantList list = value.toList();
    if (!list.isEmpty()) {
        return list;
    }
    return {value};
}

QString formatValue(const QVariant& value)
{
    if (value.metaType().id() == QMetaType::Double || value.metaType().id() == QMetaType::Float) {
        return QString::asprintf("%0.3g", value.toDouble());
    }
    return value.toString();
}

bool tryAsDouble(const QVariant& value, double* out)
{
    if (value.metaType().id() == QMetaType::Double || value.metaType().id() == QMetaType::Float) {
        *out = value.toDouble();
        return true;
    }
    if (value.metaType().id() == QMetaType::Int) {
        *out = static_cast<double>(value.toInt());
        return true;
    }
    if (value.metaType().id() == QMetaType::QString) {
        bool ok = false;
        const double converted = value.toString().toDouble(&ok);
        if (ok) {
            *out = converted;
            return true;
        }
    }
    return false;
}

QVariant convertTextToValue(const QVariant& currentValue, const QString& text)
{
    if (currentValue.metaType().id() == QMetaType::Int) {
        bool ok = false;
        const int converted = text.toInt(&ok);
        if (ok) {
            return converted;
        }
    } else if (currentValue.metaType().id() == QMetaType::Double
               || currentValue.metaType().id() == QMetaType::Float) {
        bool ok = false;
        const double converted = text.toDouble(&ok);
        if (ok) {
            return converted;
        }
    } else if (currentValue.metaType().id() == QMetaType::Bool) {
        const QString lowered = text.trimmed().toLower();
        if (lowered == QStringLiteral("true") || lowered == QStringLiteral("1")) {
            return true;
        }
        if (lowered == QStringLiteral("false") || lowered == QStringLiteral("0")) {
            return false;
        }
    }
    return text;
}

} // namespace

TableWidgetItem::TableWidgetItem(const QVariant& value, int index)
    : value_(value)
    , index_(index)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setValue(value);
}

void TableWidgetItem::setEditable(bool editable)
{
    if (editable) {
        setFlags(flags() | Qt::ItemIsEditable);
    } else {
        setFlags(flags() & ~Qt::ItemIsEditable);
    }
}

void TableWidgetItem::setSortMode(TableSortMode mode)
{
    sortMode_ = mode;
}

void TableWidgetItem::setValue(const QVariant& value)
{
    value_ = value;
    updateText();
}

void TableWidgetItem::itemChanged()
{
    if (text() != cachedText_) {
        textChanged();
    }
}

bool TableWidgetItem::operator<(const QTableWidgetItem& other) const
{
    const auto* otherItem = dynamic_cast<const TableWidgetItem*>(&other);
    if (sortMode_ == TableSortMode::Index && otherItem != nullptr) {
        return index_ < otherItem->index_;
    }
    if (sortMode_ == TableSortMode::Value && otherItem != nullptr) {
        const QVariant left = value_;
        const QVariant right = otherItem->value_;
        double leftValue = 0.0;
        double rightValue = 0.0;
        if (tryAsDouble(left, &leftValue) && tryAsDouble(right, &rightValue)) {
            return leftValue < rightValue;
        }
        return left.toString() < right.toString();
    }
    return text() < other.text();
}

void TableWidgetItem::updateText()
{
    blockValueChange_ = true;
    cachedText_ = formatValue(value_);
    setText(cachedText_);
    blockValueChange_ = false;
}

void TableWidgetItem::textChanged()
{
    cachedText_ = text();
    if (blockValueChange_) {
        return;
    }
    value_ = convertTextToValue(value_, text());
}

TableWidget::TableWidget(QWidget* parent, bool editable, bool sortable)
    : QTableWidget(parent)
    , editable_(editable)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    clear();
    setEditable(editable);
    setSortingEnabled(sortable);
    connect(this, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (auto* tableItem = dynamic_cast<TableWidgetItem*>(item)) {
            tableItem->itemChanged();
        }
    });
}

void TableWidget::clear()
{
    QTableWidget::clear();
    verticalHeadersSet_ = false;
    horizontalHeadersSet_ = false;
    items_.clear();
    setRowCount(0);
    setColumnCount(0);
    sortModes_.clear();
}

TableWidget::IteratorResult TableWidget::iteratorFn(const QVariant& data) const
{
    IteratorResult result;
    if (!data.isValid()) {
        return result;
    }

    if (data.metaType().id() == QMetaType::QVariantList) {
        result.rowIterator = [](const QVariant& row) {
            return row.toList();
        };
        return result;
    }

    if (data.metaType().id() == QMetaType::QVariantMap) {
        const QVariantMap map = data.toMap();
        QStringList keys = map.keys();
        keys.sort();
        result.headers = keys;
        result.rowIterator = [keys](const QVariant& value) {
            const QVariantMap map = value.toMap();
            QVariantList row;
            row.reserve(keys.size());
            for (const QString& key : keys) {
                row.append(map.value(key));
            }
            return row;
        };
        return result;
    }

    if (data.canConvert<QVariantMap>()) {
        const QVariantMap map = data.toMap();
        QStringList keys = map.keys();
        keys.sort();
        result.headers = keys;
        result.rowIterator = [keys](const QVariant& value) {
            const QVariantMap rowMap = value.toMap();
            QVariantList row;
            row.reserve(keys.size());
            for (const QString& key : keys) {
                row.append(rowMap.value(key));
            }
            return row;
        };
        return result;
    }

    result.rowIterator = variantListFromVariant;
    return result;
}

void TableWidget::deferSort(const std::function<void()>& fn)
{
    bool restoreSorting = false;
    if (!deferredSorting_.has_value()) {
        deferredSorting_ = isSortingEnabled();
        restoreSorting = true;
        setSortingEnabled(false);
    }
    try {
        fn();
    } catch (...) {
        if (restoreSorting) {
            setSortingEnabled(deferredSorting_.value());
            deferredSorting_.reset();
        }
        throw;
    }
    if (restoreSorting) {
        setSortingEnabled(deferredSorting_.value());
        deferredSorting_.reset();
    }
}

void TableWidget::setData(const QVariant& data)
{
    clear();
    appendData(data);
    resizeColumnsToContents();
}

void TableWidget::appendData(const QVariant& data)
{
    deferSort([&]() {
        const int startRow = rowCount();

        if (!data.isValid()) {
            clear();
            return;
        }

        QVariantList rows;
        QStringList verticalHeaders;
        if (data.metaType().id() == QMetaType::QVariantList) {
            rows = data.toList();
        } else if (data.metaType().id() == QMetaType::QVariantMap) {
            const QVariantMap map = data.toMap();
            QStringList keys = map.keys();
            keys.sort();
            verticalHeaders = keys;
            for (const QString& key : keys) {
                rows.append(map.value(key));
            }
        } else {
            rows = {data};
        }

        if (rows.isEmpty()) {
            return;
        }

        const IteratorResult inner = iteratorFn(rows.first());
        if (!inner.rowIterator) {
            clear();
            return;
        }

        const QVariantList firstValues = inner.rowIterator(rows.first());
        setColumnCount(firstValues.size());

        if (!verticalHeadersSet_ && !verticalHeaders.isEmpty()) {
            QStringList labels;
            labels.reserve(rowCount());
            for (int i = 0; i < rowCount(); ++i) {
                if (verticalHeaderItem(i) != nullptr) {
                    labels.append(verticalHeaderItem(i)->text());
                }
            }
            setRowCount(startRow + verticalHeaders.size());
            setVerticalHeaderLabels(labels + verticalHeaders);
            verticalHeadersSet_ = true;
        }

        if (!horizontalHeadersSet_ && !inner.headers.isEmpty()) {
            setHorizontalHeaderLabels(inner.headers);
            horizontalHeadersSet_ = true;
        }

        int rowIndex = startRow;
        setRow(rowIndex, firstValues);
        for (int i = 1; i < rows.size(); ++i) {
            ++rowIndex;
            setRow(rowIndex, inner.rowIterator(rows.at(i)));
        }

        if (deferredSorting_.value_or(isSortingEnabled()) && horizontalHeadersSet_
            && horizontalHeader()->sortIndicatorSection() >= columnCount()) {
            sortByColumn(0, Qt::AscendingOrder);
        }
    });
}

void TableWidget::setRow(int row, const QVariantList& values)
{
    if (row > rowCount() - 1) {
        setRowCount(row + 1);
    }
    for (int col = 0; col < values.size(); ++col) {
        auto* item = new TableWidgetItem(values.at(col), row);
        item->setEditable(editable_);
        if (sortModes_.contains(col)) {
            item->setSortMode(sortModes_.value(col));
        }
        items_.append(item);
        setItem(row, col, item);
        item->setValue(values.at(col));
    }
}

void TableWidget::setEditable(bool editable)
{
    editable_ = editable;
    for (TableWidgetItem* item : items_) {
        if (item != nullptr) {
            item->setEditable(editable);
        }
    }
}

void TableWidget::setSortMode(int column, TableSortMode mode)
{
    for (int row = 0; row < rowCount(); ++row) {
        if (auto* item = dynamic_cast<TableWidgetItem*>(this->item(row, column))) {
            item->setSortMode(mode);
        }
    }
    sortModes_.insert(column, mode);
}

QString TableWidget::serialize(bool useSelection) const
{
    QList<int> rows;
    QList<int> columns;

    if (useSelection) {
        const QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        if (ranges.isEmpty()) {
            return {};
        }
        const QTableWidgetSelectionRange selection = ranges.first();
        for (int row = selection.topRow(); row <= selection.bottomRow(); ++row) {
            rows.append(row);
        }
        for (int column = selection.leftColumn(); column <= selection.rightColumn(); ++column) {
            columns.append(column);
        }
    } else {
        for (int row = 0; row < rowCount(); ++row) {
            rows.append(row);
        }
        for (int column = 0; column < columnCount(); ++column) {
            columns.append(column);
        }
    }

    QStringList lines;
    if (horizontalHeadersSet_) {
        QStringList headerRow;
        if (verticalHeadersSet_) {
            headerRow.append(QString());
        }
        for (int column : columns) {
            if (horizontalHeaderItem(column) != nullptr) {
                headerRow.append(horizontalHeaderItem(column)->text());
            } else {
                headerRow.append(QString());
            }
        }
        lines.append(headerRow.join(QStringLiteral("\t")));
    }

    for (int row : rows) {
        QStringList rowValues;
        if (verticalHeadersSet_) {
            if (verticalHeaderItem(row) != nullptr) {
                rowValues.append(verticalHeaderItem(row)->text());
            } else {
                rowValues.append(QString());
            }
        }
        for (int column : columns) {
            if (const auto* item = dynamic_cast<const TableWidgetItem*>(this->item(row, column))) {
                rowValues.append(item->value().toString());
            } else {
                rowValues.append(QString());
            }
        }
        lines.append(rowValues.join(QStringLiteral("\t")));
    }

    return lines.join(QStringLiteral("\n")) + QStringLiteral("\n");
}

} // namespace pyqtgraph::widgets
