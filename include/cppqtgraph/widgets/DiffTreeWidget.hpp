#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/DiffTreeWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QVariant>
#include <QtWidgets/QWidget>

#include <array>

class QHBoxLayout;

namespace cppqtgraph::widgets {

class DataTreeWidget;

class DiffTreeWidget : public QWidget {
public:
    explicit DiffTreeWidget(QWidget* parent = nullptr);
    DiffTreeWidget(QWidget* parent, const QVariant& a, const QVariant& b);

    DiffTreeWidget(const DiffTreeWidget&) = delete;
    DiffTreeWidget& operator=(const DiffTreeWidget&) = delete;
    DiffTreeWidget(DiffTreeWidget&&) = delete;
    DiffTreeWidget& operator=(DiffTreeWidget&&) = delete;

    void setData(const QVariant& a, const QVariant& b);
    void compare(const QVariant& a, const QVariant& b, const QVariantList& path = {});

    [[nodiscard]] DataTreeWidget* tree(int index) const;
    [[nodiscard]] const std::array<DataTreeWidget*, 2>& trees() const { return trees_; }

private:
    void compareArrays(const QVariantList& shapeA, const QVariantList& valuesA, const QVariantList& valuesB, const QVariantList& path);
    void setColor(const QVariantList& path, int column, const QColor& color, int treeIndex = -1);

    QHBoxLayout* layout_ = nullptr;
    std::array<DataTreeWidget*, 2> trees_ = {nullptr, nullptr};
    std::array<QVariant, 2> data_;
};

} // namespace cppqtgraph::widgets
