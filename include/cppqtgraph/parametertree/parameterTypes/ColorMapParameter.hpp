#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/colormap.py
// and pyqtgraph/parametertree/parameterTypes/colormaplut.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/colormap.hpp>
#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>

namespace cppqtgraph::parametertree {

class ColorMapParameterItem final : public WidgetParameterItem {
public:
    ColorMapParameterItem(Parameter* param, int depth);

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
};

class ColorMapParameter final : public Parameter {
public:
    explicit ColorMapParameter(QVariantMap opts, QObject* parent = nullptr);

    QVariant setValue(const QVariant& value, bool blockSignal = false) override;
    ParameterItem* makeTreeItem(int depth) override;

private:
    [[nodiscard]] QVariant interpretValue(const QVariant& value) const;
};

class ColorMapLutParameterItem final : public WidgetParameterItem {
public:
    ColorMapLutParameterItem(Parameter* param, int depth);

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
};

class ColorMapLutParameter final : public Parameter {
public:
    explicit ColorMapLutParameter(QVariantMap opts, QObject* parent = nullptr);

    QVariant setValue(const QVariant& value, bool blockSignal = false) override;
    ParameterItem* makeTreeItem(int depth) override;

private:
    [[nodiscard]] QVariant interpretValue(const QVariant& value) const;
};

} // namespace cppqtgraph::parametertree
