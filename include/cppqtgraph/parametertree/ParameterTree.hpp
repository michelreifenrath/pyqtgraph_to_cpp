#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/ParameterTree.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/widgets/TreeWidget.hpp>

#include <memory>

class QTreeWidgetItem;

namespace cppqtgraph::parametertree {

class ParameterItem;

class ParameterTree : public widgets::TreeWidget {
    Q_OBJECT

public:
    explicit ParameterTree(QWidget* parent = nullptr, bool showHeader = true);

    void setParameters(const std::shared_ptr<Parameter>& param, bool showTop = true);
    void addParameters(Parameter* param,
                       QTreeWidgetItem* root = nullptr,
                       int depth = 0,
                       bool showTop = true);
    void clearParameters();

    [[nodiscard]] Parameter* parameters() const { return paramSet_.get(); }

    void focusNext(ParameterItem* item, bool forward = true);
    void focusPrevious(ParameterItem* item);
    [[nodiscard]] ParameterItem* nextFocusableChild(QTreeWidgetItem* root,
                                                    QTreeWidgetItem* startItem = nullptr,
                                                    bool forward = true) const;

protected:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void itemChangedEvent(QTreeWidgetItem* item, int col);
    void itemExpandedEvent(QTreeWidgetItem* item);
    void itemCollapsedEvent(QTreeWidgetItem* item);

private:
    std::shared_ptr<Parameter> paramSet_;
    QTreeWidgetItem* lastSel_ = nullptr;
};

} // namespace cppqtgraph::parametertree
