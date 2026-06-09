#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GroupBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtWidgets/QGroupBox>

namespace pyqtgraph::widgets {

class GroupBox : public QGroupBox {
    Q_OBJECT

public:
    explicit GroupBox(QWidget* parent = nullptr);
    explicit GroupBox(const QString& title, QWidget* parent = nullptr);

    GroupBox(const GroupBox&) = delete;
    GroupBox& operator=(const GroupBox&) = delete;
    GroupBox(GroupBox&&) = delete;
    GroupBox& operator=(GroupBox&&) = delete;

    [[nodiscard]] bool collapsed() const { return collapsed_; }

    void setCollapsed(bool collapsed);
    void toggleCollapsed();

    void setTitle(const QString& title);

    void setSizePolicy(QSizePolicy policy);
    void setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical);

signals:
    void sigCollapseChanged(bool collapsed);

private:
    void initializeCollapseHandle();
    void applyChildVisibility(bool collapsed);
    void setClosingSizePolicy();

    QWidget* collapseHandle_ = nullptr;
    bool collapsed_ = false;
    QSizePolicy lastSizePolicy_;
};

} // namespace pyqtgraph::widgets
