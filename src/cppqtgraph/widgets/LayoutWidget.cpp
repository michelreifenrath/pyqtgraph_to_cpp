// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/LayoutWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/widgets/LayoutWidget.hpp"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

namespace cppqtgraph::widgets {

LayoutWidget::LayoutWidget(QWidget* parent)
    : QWidget(parent)
{
    gridLayout = new QGridLayout(this);
    setLayout(gridLayout);
}

void LayoutWidget::nextRow()
{
    ++currentRow;
    currentCol = 0;
}

int LayoutWidget::nextColumn(int colspan)
{
    currentCol += colspan;
    return currentCol - colspan;
}

int LayoutWidget::nextCol(int colspan)
{
    return nextColumn(colspan);
}

QLabel* LayoutWidget::addLabel(const QString& text, int row, int col, int rowspan, int colspan)
{
    auto* label = new QLabel(text, this);
    addWidget(label, row, col, rowspan, colspan);
    return label;
}

LayoutWidget* LayoutWidget::addLayout(int row, int col, int rowspan, int colspan)
{
    auto* nested = new LayoutWidget(this);
    addWidget(nested, row, col, rowspan, colspan);
    return nested;
}

void LayoutWidget::addWidget(QWidget* item, int row, int col, int rowspan, int colspan)
{
    int resolvedRow = row;
    if (resolvedRow == kNextRow) {
        nextRow();
        resolvedRow = currentRow;
    } else if (resolvedRow == kAutoRow) {
        resolvedRow = currentRow;
    }

    int resolvedCol = col;
    if (resolvedCol == kAutoCol) {
        resolvedCol = nextCol(colspan);
    }

    if (!rows.contains(resolvedRow)) {
        rows[resolvedRow] = {};
    }
    rows[resolvedRow][resolvedCol] = item;
    items[item] = {resolvedRow, resolvedCol};

    gridLayout->addWidget(item, resolvedRow, resolvedCol, rowspan, colspan);
}

QWidget* LayoutWidget::getWidget(int row, int col) const
{
    if (!rows.contains(row)) {
        return nullptr;
    }
    const QHash<int, QWidget*>& rowMap = rows[row];
    if (!rowMap.contains(col)) {
        return nullptr;
    }
    return rowMap[col];
}

} // namespace cppqtgraph::widgets
