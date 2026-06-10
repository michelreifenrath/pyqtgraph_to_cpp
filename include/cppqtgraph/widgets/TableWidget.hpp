#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/TableWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtWidgets/QTableWidget>

#include <functional>
#include <optional>

namespace cppqtgraph::widgets {

enum class TableSortMode {
    Value,
    Text,
    Index
};

class TableWidget;

class TableWidgetItem : public QTableWidgetItem {
public:
    TableWidgetItem(const QVariant& value, int index);

    void setEditable(bool editable);
    void setSortMode(TableSortMode mode);

    [[nodiscard]] QVariant value() const { return value_; }
    void setValue(const QVariant& value);

    void itemChanged();

    bool operator<(const QTableWidgetItem& other) const override;

private:
    void updateText();
    void textChanged();

    QVariant value_;
    QString cachedText_;
    bool blockValueChange_ = false;
    TableSortMode sortMode_ = TableSortMode::Value;
    int index_ = 0;
};

class TableWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit TableWidget(QWidget* parent = nullptr, bool editable = false, bool sortable = true);

    TableWidget(const TableWidget&) = delete;
    TableWidget& operator=(const TableWidget&) = delete;
    TableWidget(TableWidget&&) = delete;
    TableWidget& operator=(TableWidget&&) = delete;

    void clear();

    void setData(const QVariant& data);
    void appendData(const QVariant& data);

    void setEditable(bool editable);
    void setSortMode(int column, TableSortMode mode);

    [[nodiscard]] QString serialize(bool useSelection = false) const;

private:
    struct IteratorResult {
        std::function<QVariantList(const QVariant&)> rowIterator;
        QStringList headers;
    };

    [[nodiscard]] IteratorResult iteratorFn(const QVariant& data) const;
    void deferSort(const std::function<void()>& fn);
    void setRow(int row, const QVariantList& values);

    bool editable_ = false;
    bool horizontalHeadersSet_ = false;
    bool verticalHeadersSet_ = false;
    QList<TableWidgetItem*> items_;
    QHash<int, TableSortMode> sortModes_;
    std::optional<bool> deferredSorting_;
};

} // namespace cppqtgraph::widgets
