// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/CheckTable.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/CheckTable.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

namespace cppqtgraph::widgets {

CheckTable::CheckTable(const QStringList& columns, QWidget* parent)
    : QWidget(parent)
    , columns_(columns)
{
    layout_ = new QGridLayout();
    layout_->setSpacing(0);
    setLayout(layout_);

    int column = 1;
    for (const QString& header : columns_) {
        auto* label = new QLabel(header, this);
        headers_.append(header);
        layout_->addWidget(label, 0, column);
        ++column;
    }
}

CheckTable::~CheckTable() = default;

void CheckTable::updateRows(const QStringList& rows)
{
    for (const QString& existing : QStringList(rowNames_)) {
        if (!rows.contains(existing)) {
            removeRow(existing);
        }
    }
    for (const QString& row : rows) {
        if (!rowNames_.contains(row)) {
            addRow(row);
        }
    }
}

void CheckTable::addRow(const QString& name)
{
    auto* label = new QLabel(name, this);
    const int row = rowNames_.size() + 1;
    layout_->addWidget(label, row, 0);

    QList<QWidget*> widgets;
    widgets.append(label);

    int column = 1;
    for (const QString& columnName : columns_) {
        auto* check = new QCheckBox(QString(), this);
        check->setProperty("rowName", name);
        check->setProperty("columnName", columnName);
        layout_->addWidget(check, row, column);
        widgets.append(check);

        if (oldRows_.contains(name) && oldRows_.value(name).size() > column - 1) {
            check->setChecked(oldRows_.value(name).at(column - 1));
        }

        connect(check, qOverload<int>(&QCheckBox::stateChanged), this, &CheckTable::checkChanged);
        ++column;
    }

    rowNames_.append(name);
    rowWidgets_.append(widgets);
}

void CheckTable::removeRow(const QString& name)
{
    const int row = rowNames_.indexOf(name);
    if (row < 0) {
        return;
    }

    const QVariantMap state = saveState();
    const QVariantList rows = state.value(QStringLiteral("rows")).toList();
    if (row < rows.size()) {
        const QVariantList rowState = rows.at(row).toList();
        QList<bool> checked;
        for (int i = 1; i < rowState.size(); ++i) {
            checked.append(rowState.at(i).toBool());
        }
        oldRows_.insert(name, checked);
    }

    rowNames_.removeAt(row);
    const QList<QWidget*> widgets = rowWidgets_.takeAt(row);
    for (QWidget* widget : widgets) {
        if (auto* check = qobject_cast<QCheckBox*>(widget)) {
            disconnect(check, qOverload<int>(&QCheckBox::stateChanged), this, &CheckTable::checkChanged);
        }
        layout_->removeWidget(widget);
        delete widget;
    }

    for (int i = row; i < rowNames_.size(); ++i) {
        const QList<QWidget*>& rowWidgets = rowWidgets_[i];
        for (int j = 0; j < rowWidgets.size(); ++j) {
            QWidget* widget = rowWidgets[j];
            widget->setParent(nullptr);
            layout_->addWidget(widget, i + 1, j);
        }
    }
}

QCheckBox* CheckTable::checkBox(const QString& rowName, const QString& columnName) const
{
    const int row = rowNames_.indexOf(rowName);
    if (row < 0) {
        return nullptr;
    }
    const int column = columns_.indexOf(columnName);
    if (column < 0) {
        return nullptr;
    }
    QWidget* widget = rowWidgets_.at(row).at(column + 1);
    return qobject_cast<QCheckBox*>(widget);
}

void CheckTable::checkChanged(int state)
{
    auto* check = qobject_cast<QCheckBox*>(sender());
    if (check == nullptr) {
        return;
    }
    emit sigStateChanged(check->property("rowName").toString(),
        check->property("columnName").toString(),
        static_cast<int>(state));
}

QVariantMap CheckTable::saveState() const
{
    QVariantList rows;
    for (int i = 0; i < rowNames_.size(); ++i) {
        QVariantList row;
        row.append(rowNames_.at(i));
        for (int j = 1; j < rowWidgets_.at(i).size(); ++j) {
            const auto* check = qobject_cast<const QCheckBox*>(rowWidgets_.at(i).at(j));
            row.append(check != nullptr && check->isChecked());
        }
        rows.append(QVariant(row));
    }
    QVariantMap state;
    state.insert(QStringLiteral("cols"), columns_);
    state.insert(QStringLiteral("rows"), rows);
    return state;
}

void CheckTable::restoreState(const QVariantMap& state)
{
    const QVariantList rows = state.value(QStringLiteral("rows")).toList();
    QStringList rowNames;
    rowNames.reserve(rows.size());
    for (const QVariant& rowVariant : rows) {
        const QVariantList row = rowVariant.toList();
        if (!row.isEmpty()) {
            rowNames.append(row.at(0).toString());
        }
    }
    updateRows(rowNames);

    for (const QVariant& rowVariant : rows) {
        const QVariantList row = rowVariant.toList();
        if (row.isEmpty()) {
            continue;
        }
        const QString rowName = row.at(0).toString();
        const int rowIndex = rowNames_.indexOf(rowName);
        if (rowIndex < 0) {
            continue;
        }
        for (int i = 1; i < row.size() && i < rowWidgets_.at(rowIndex).size(); ++i) {
            if (auto* check = qobject_cast<QCheckBox*>(rowWidgets_.at(rowIndex).at(i))) {
                check->setChecked(row.at(i).toBool());
            }
        }
    }
}

} // namespace cppqtgraph::widgets
