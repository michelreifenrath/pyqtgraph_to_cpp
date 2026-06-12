#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QSpinBox;
class QWidget;

namespace cppqtgraph::graphicsItems::ViewBoxAxisConfig {

class Ui_Form {
public:
    void setupUi(QWidget* form);

    QLabel* linkLabel = nullptr;
    QComboBox* linkCombo = nullptr;
    QSpinBox* autoPercentSpin = nullptr;
    QRadioButton* autoRadio = nullptr;
    QRadioButton* manualRadio = nullptr;
    QLineEdit* minText = nullptr;
    QLineEdit* maxText = nullptr;
    QCheckBox* invertCheck = nullptr;
    QCheckBox* mouseCheck = nullptr;
    QCheckBox* visibleOnlyCheck = nullptr;
    QCheckBox* autoPanCheck = nullptr;
};

} // namespace cppqtgraph::graphicsItems::ViewBoxAxisConfig
