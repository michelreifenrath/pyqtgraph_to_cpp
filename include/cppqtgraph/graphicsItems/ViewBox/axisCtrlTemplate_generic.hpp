#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

class QCheckBox;
class QLabel;
class QRadioButton;
class QWidget;

namespace cppqtgraph::graphicsItems::ViewBoxAxisConfig {

class Ui_Form {
public:
    void setupUi(QWidget* form);

    QCheckBox* mouseCheck = nullptr;
    QRadioButton* manualRadio = nullptr;
    QRadioButton* autoRadio = nullptr;
    QLabel* minLabel = nullptr;
    QLabel* maxLabel = nullptr;
};

} // namespace cppqtgraph::graphicsItems::ViewBoxAxisConfig
