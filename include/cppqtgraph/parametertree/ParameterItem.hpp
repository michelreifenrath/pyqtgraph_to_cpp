#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/ParameterItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/widgets/TreeWidget.hpp>

#include <QVariant>

class QLabel;
class QPushButton;
class QKeyEvent;
class QWidget;

namespace cppqtgraph::widgets {
class ComboBox;
}

namespace cppqtgraph::parametertree {

class Parameter;

class ParameterItem : public widgets::TreeWidgetItem {
public:
    ParameterItem(Parameter* param, int depth);
    ~ParameterItem() override;

    void treeWidgetChanged();
    [[nodiscard]] Parameter* parameter() const { return param_; }
    [[nodiscard]] int depth() const { return depth_; }

    virtual void valueChanged(Parameter* param, const QVariant& val);
    virtual void childAdded(Parameter* param, Parameter* child, int pos);
    virtual void childRemoved(Parameter* param, Parameter* child);
    virtual void nameChanged(Parameter* param, const QString& name);
    virtual void defaultChanged(Parameter* param, const QVariant& defaultValue);
    virtual void optsChanged(Parameter* param, const QVariantMap& opts);
    virtual void parentChanged(Parameter* param, Parameter* parent);

    virtual void columnChangedEvent(int col);
    virtual void expandedChangedEvent(bool expanded);
    virtual void selected(bool sel);

    [[nodiscard]] virtual bool isFocusable() const;
    virtual void setFocus();
    void focusNext(bool forward = true);

protected:
    void updateFlags();
    void titleChanged();

    Parameter* param_ = nullptr;
    int depth_ = 0;
    bool ignoreNameColumnChange_ = false;
};

struct WidgetParameterItemOptions {
    bool hideWhenDeselected = true;
    bool asSubItem = false;
};

class WidgetParameterItem : public ParameterItem {
public:
    WidgetParameterItem(Parameter* param, int depth, QWidget* editor, WidgetParameterItemOptions options = {});
    ~WidgetParameterItem() override;

    void treeWidgetChanged();
    void valueChanged(Parameter* param, const QVariant& val) override;
    void defaultChanged(Parameter* param, const QVariant& defaultValue) override;
    void optsChanged(Parameter* param, const QVariantMap& opts) override;
    void selected(bool sel) override;

    [[nodiscard]] bool isFocusable() const override;
    void setFocus() override;

    [[nodiscard]] QWidget* editorWidget() const { return editor_; }
    [[nodiscard]] QPushButton* defaultButton() const { return defaultBtn_; }

    bool handleEditorKeyPress(QKeyEvent* event);

    void widgetValueChanged();
    void editorValueChanging(const QVariant& value);
    void defaultClicked();

protected:
    virtual void bindEditor(QWidget* editor);
    virtual QVariant readEditorValue() const;
    virtual void writeEditorValue(const QVariant& val);
    virtual void configureEditor(QWidget* editor);

    void updateDisplayLabel(const QVariant& value = QVariant());
    void updateDefaultBtn();
    void showEditor();
    void hideEditor();

    QWidget* layoutWidget_ = nullptr;
    QWidget* editor_ = nullptr;
    QLabel* displayLabel_ = nullptr;
    QPushButton* defaultBtn_ = nullptr;
    QTreeWidgetItem* subItem_ = nullptr;
    bool hideWhenDeselected_ = true;
    bool asSubItem_ = false;
    bool updatingWidget_ = false;
};

class ListParameterItem final : public WidgetParameterItem {
public:
    ListParameterItem(Parameter* param, int depth);

    void optsChanged(Parameter* param, const QVariantMap& opts) override;

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;

private:
    void updateLimits(const QVariant& limits);

    widgets::ComboBox* combo_ = nullptr;
};

class ColorParameterItem final : public WidgetParameterItem {
public:
    ColorParameterItem(Parameter* param, int depth);

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
};

class TextParameterItem final : public WidgetParameterItem {
public:
    TextParameterItem(Parameter* param, int depth);

    void optsChanged(Parameter* param, const QVariantMap& opts) override;

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
};

} // namespace cppqtgraph::parametertree
