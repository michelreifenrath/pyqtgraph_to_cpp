// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/ParameterTree.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/parametertree/ParameterTree.hpp"

#include <QtWidgets/QHeaderView>

namespace cppqtgraph::parametertree {

ParameterTree::ParameterTree(QWidget* parent, bool showHeader)
    : widgets::TreeWidget(parent)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setAnimated(false);
    setColumnCount(2);
    setHeaderLabels({QStringLiteral("Parameter"), QStringLiteral("Value")});
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setHeaderHidden(!showHeader);
    setRootIsDecorated(false);
    setAlternatingRowColors(true);
}

void ParameterTree::setParameters(const std::shared_ptr<Parameter>& param, bool showTop)
{
    clearParameters();
    paramSet_ = param;
    if (paramSet_ != nullptr) {
        addParameters(paramSet_.get(), nullptr, 0, showTop);
    }
}

void ParameterTree::addParameters(Parameter* param, QTreeWidgetItem* root, int depth, bool showTop)
{
    if (param == nullptr) {
        return;
    }

    ParameterItem* item = param->makeTreeItem(depth);
    if (root == nullptr) {
        root = invisibleRootItem();
        if (!showTop) {
            item->setText(0, QString());
            item->setSizeHint(0, QSize(1, 1));
            item->setSizeHint(1, QSize(1, 1));
            depth -= 1;
        }
    }
    root->addChild(item);
    item->treeWidgetChanged();
    widgets::TreeWidget::informTreeWidgetChange(item);

    for (const std::shared_ptr<Parameter>& child : param->children()) {
        addParameters(child.get(), item, depth + 1, true);
    }
}

void ParameterTree::clearParameters()
{
    invisibleRootItem()->takeChildren();
    paramSet_.reset();
}

} // namespace cppqtgraph::parametertree
