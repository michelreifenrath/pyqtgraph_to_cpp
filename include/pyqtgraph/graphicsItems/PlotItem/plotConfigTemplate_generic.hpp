#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

class QCheckBox;
class QFrame;
class QGroupBox;
class QLabel;
class QListWidget;
class QRadioButton;
class QSlider;
class QSpinBox;
class QWidget;

namespace pyqtgraph::graphicsItems::PlotItemConfig {

class Ui_Form {
public:
    void setupUi(QWidget* form);

    QGroupBox* averageGroup = nullptr;
    QListWidget* avgParamList = nullptr;
    QFrame* decimateGroup = nullptr;
    QCheckBox* clipToViewCheck = nullptr;
    QCheckBox* maxTracesCheck = nullptr;
    QCheckBox* downsampleCheck = nullptr;
    QRadioButton* peakRadio = nullptr;
    QSpinBox* maxTracesSpin = nullptr;
    QCheckBox* forgetTracesCheck = nullptr;
    QRadioButton* meanRadio = nullptr;
    QRadioButton* subsampleRadio = nullptr;
    QCheckBox* autoDownsampleCheck = nullptr;
    QSpinBox* downsampleSpin = nullptr;
    QFrame* transformGroup = nullptr;
    QCheckBox* logXCheck = nullptr;
    QCheckBox* derivativeCheck = nullptr;
    QCheckBox* phasemapCheck = nullptr;
    QCheckBox* fftCheck = nullptr;
    QCheckBox* logYCheck = nullptr;
    QCheckBox* subtractMeanCheck = nullptr;
    QGroupBox* pointsGroup = nullptr;
    QCheckBox* autoPointsCheck = nullptr;
    QFrame* gridGroup = nullptr;
    QCheckBox* xGridCheck = nullptr;
    QCheckBox* yGridCheck = nullptr;
    QSlider* gridAlphaSlider = nullptr;
    QLabel* opacityLabel = nullptr;
    QGroupBox* alphaGroup = nullptr;
    QCheckBox* autoAlphaCheck = nullptr;
    QSlider* alphaSlider = nullptr;
};

} // namespace pyqtgraph::graphicsItems::PlotItemConfig
