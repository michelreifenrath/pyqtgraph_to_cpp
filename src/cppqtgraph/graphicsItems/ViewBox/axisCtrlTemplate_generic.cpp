// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.hpp"

#include <QtCore/QCoreApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

namespace cppqtgraph::graphicsItems::ViewBoxAxisConfig {

void Ui_Form::setupUi(QWidget* form)
{
    form->setObjectName(QStringLiteral("Form"));
    form->setMaximumWidth(200);
    auto* layout = new QGridLayout(form);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    manualRadio = new QRadioButton(form);
    manualRadio->setObjectName(QStringLiteral("manualRadio"));
    manualRadio->setText(QCoreApplication::translate("ViewBox", "Manual"));
    layout->addWidget(manualRadio, 1, 0, 1, 2);

    minText = new QLineEdit(form);
    minText->setObjectName(QStringLiteral("minText"));
    minText->setText(QStringLiteral("0"));
    layout->addWidget(minText, 1, 2, 1, 1);

    maxText = new QLineEdit(form);
    maxText->setObjectName(QStringLiteral("maxText"));
    maxText->setText(QStringLiteral("0"));
    layout->addWidget(maxText, 1, 3, 1, 1);

    autoRadio = new QRadioButton(form);
    autoRadio->setObjectName(QStringLiteral("autoRadio"));
    autoRadio->setChecked(true);
    autoRadio->setText(QCoreApplication::translate("ViewBox", "Auto"));
    layout->addWidget(autoRadio, 2, 0, 1, 2);

    autoPercentSpin = new QSpinBox(form);
    autoPercentSpin->setObjectName(QStringLiteral("autoPercentSpin"));
    autoPercentSpin->setMinimum(1);
    autoPercentSpin->setMaximum(100);
    autoPercentSpin->setValue(100);
    autoPercentSpin->setSuffix(QCoreApplication::translate("ViewBox", "%"));
    layout->addWidget(autoPercentSpin, 2, 2, 1, 2);

    visibleOnlyCheck = new QCheckBox(form);
    visibleOnlyCheck->setObjectName(QStringLiteral("visibleOnlyCheck"));
    visibleOnlyCheck->setText(QCoreApplication::translate("ViewBox", "Visible Data Only"));
    visibleOnlyCheck->setEnabled(false);
    layout->addWidget(visibleOnlyCheck, 3, 2, 1, 2);

    autoPanCheck = new QCheckBox(form);
    autoPanCheck->setObjectName(QStringLiteral("autoPanCheck"));
    autoPanCheck->setText(QCoreApplication::translate("ViewBox", "Auto Pan Only"));
    autoPanCheck->setEnabled(false);
    layout->addWidget(autoPanCheck, 4, 2, 1, 2);

    invertCheck = new QCheckBox(form);
    invertCheck->setObjectName(QStringLiteral("invertCheck"));
    invertCheck->setText(QCoreApplication::translate("ViewBox", "Invert Axis"));
    layout->addWidget(invertCheck, 5, 0, 1, 4);

    mouseCheck = new QCheckBox(form);
    mouseCheck->setObjectName(QStringLiteral("mouseCheck"));
    mouseCheck->setChecked(true);
    mouseCheck->setText(QCoreApplication::translate("ViewBox", "Mouse Enabled"));
    layout->addWidget(mouseCheck, 6, 0, 1, 4);

    linkLabel = new QLabel(form);
    linkLabel->setObjectName(QStringLiteral("linkLabel"));
    linkLabel->setText(QCoreApplication::translate("ViewBox", "Link Axis:"));
    layout->addWidget(linkLabel, 7, 0, 1, 2);

    linkCombo = new QComboBox(form);
    linkCombo->setObjectName(QStringLiteral("linkCombo"));
    linkCombo->addItem(QString{});
    linkCombo->setEnabled(false);
    layout->addWidget(linkCombo, 7, 2, 1, 2);
}

} // namespace cppqtgraph::graphicsItems::ViewBoxAxisConfig
