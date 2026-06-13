#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/parametertree/parameterTypes/slider.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/Parameter.hpp>

#include <QString>
#include <vector>

class QLabel;
class QSlider;

namespace cppqtgraph::parametertree {

std::vector<double> linspaceMinusPiToPi(int count);
std::vector<double> arangeSquared(int count);

class SliderParameterItem final : public WidgetParameterItem {
public:
    SliderParameterItem(Parameter* param, int depth);

    void optsChanged(Parameter* param, const QVariantMap& opts) override;

protected:
    void bindEditor(QWidget* editor) override;
    QVariant readEditorValue() const override;
    void writeEditorValue(const QVariant& val) override;
    void configureEditor(QWidget* editor) override;
    void updateDisplayLabel(const QVariant& value = QVariant()) override;

private:
    void rebuildSpan(const QVariantMap& opts, const QVariantMap& changed);
    int spanToSliderValue(double value) const;
    QString prettyTextValue(int sliderIndex) const;

    QSlider* slider_ = nullptr;
    QLabel* sliderLabel_ = nullptr;
    std::vector<double> span_;
    QStringList charSpan_;
    QString suffix_;
    bool useSpanMode_ = false;
};

class SliderParameter final : public Parameter {
public:
    explicit SliderParameter(QVariantMap opts, QObject* parent = nullptr);

    QVariant setValue(const QVariant& value, bool blockSignal = false) override;
    void setOpts(const QVariantMap& opts) override;
    ParameterItem* makeTreeItem(int depth) override;
};

} // namespace cppqtgraph::parametertree
