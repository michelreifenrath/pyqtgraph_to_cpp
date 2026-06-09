#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/CheckTable.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>
#include <QtWidgets/QWidget>

class QCheckBox;
class QGridLayout;
class QLabel;

namespace pyqtgraph::widgets {

class CheckTable : public QWidget {
    Q_OBJECT

public:
    explicit CheckTable(const QStringList& columns, QWidget* parent = nullptr);
    ~CheckTable() override;

    CheckTable(const CheckTable&) = delete;
    CheckTable& operator=(const CheckTable&) = delete;
    CheckTable(CheckTable&&) = delete;
    CheckTable& operator=(CheckTable&&) = delete;

    [[nodiscard]] const QStringList& columns() const { return columns_; }
    [[nodiscard]] const QStringList& rowNames() const { return rowNames_; }

    void updateRows(const QStringList& rows);
    void addRow(const QString& name);
    void removeRow(const QString& name);

    [[nodiscard]] QCheckBox* checkBox(const QString& rowName, const QString& columnName) const;

    [[nodiscard]] QVariantMap saveState() const;
    void restoreState(const QVariantMap& state);

signals:
    void sigStateChanged(const QString& row, const QString& column, int state);

private:
    void checkChanged(int state);

    QGridLayout* layout_ = nullptr;
    QStringList columns_;
    QStringList headers_;
    QStringList rowNames_;
    QList<QList<QWidget*>> rowWidgets_;
    QHash<QString, QList<bool>> oldRows_;
};

} // namespace pyqtgraph::widgets
