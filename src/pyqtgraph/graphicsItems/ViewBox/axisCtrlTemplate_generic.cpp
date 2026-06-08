// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace pyqtgraph::graphicsItems::ViewBoxAxisConfig {

void Ui_Form::setupUi(QWidget* form)
{
    form->setObjectName(QStringLiteral("Form"));
    auto* layout = new QVBoxLayout(form);
    mouseCheck = new QCheckBox(form);
    mouseCheck->setObjectName(QStringLiteral("mouseCheck"));
    layout->addWidget(mouseCheck);
    manualRadio = new QRadioButton(form);
    manualRadio->setObjectName(QStringLiteral("manualRadio"));
    layout->addWidget(manualRadio);
    autoRadio = new QRadioButton(form);
    autoRadio->setObjectName(QStringLiteral("autoRadio"));
    layout->addWidget(autoRadio);
    auto* rangeLayout = new QGridLayout();
    minLabel = new QLabel(form);
    minLabel->setObjectName(QStringLiteral("minLabel"));
    rangeLayout->addWidget(minLabel, 0, 0);
    maxLabel = new QLabel(form);
    maxLabel->setObjectName(QStringLiteral("maxLabel"));
    rangeLayout->addWidget(maxLabel, 1, 0);
    layout->addLayout(rangeLayout);
}

} // namespace pyqtgraph::graphicsItems::ViewBoxAxisConfig
