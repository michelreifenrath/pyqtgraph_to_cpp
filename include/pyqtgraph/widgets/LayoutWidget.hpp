#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/LayoutWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QGridLayout;
class QLabel;

namespace pyqtgraph::widgets {

class LayoutWidget : public QWidget {
    Q_OBJECT

public:
    static constexpr int kAutoRow = -1;
    static constexpr int kNextRow = -2;
    static constexpr int kAutoCol = -1;

    explicit LayoutWidget(QWidget* parent = nullptr);

    LayoutWidget(const LayoutWidget&) = delete;
    LayoutWidget& operator=(const LayoutWidget&) = delete;
    LayoutWidget(LayoutWidget&&) = delete;
    LayoutWidget& operator=(LayoutWidget&&) = delete;

    QGridLayout* gridLayout = nullptr;
    QHash<QWidget*, QPair<int, int>> items;
    QHash<int, QHash<int, QWidget*>> rows;
    int currentRow = 0;
    int currentCol = 0;

    void nextRow();
    int nextColumn(int colspan = 1);
    int nextCol(int colspan = 1);

    QLabel* addLabel(const QString& text = QStringLiteral(" "), int row = kAutoRow, int col = kAutoCol,
                     int rowspan = 1, int colspan = 1);
    LayoutWidget* addLayout(int row = kAutoRow, int col = kAutoCol, int rowspan = 1, int colspan = 1);
    void addWidget(QWidget* item, int row = kAutoRow, int col = kAutoCol, int rowspan = 1, int colspan = 1);
    [[nodiscard]] QWidget* getWidget(int row, int col) const;
};

} // namespace pyqtgraph::widgets
