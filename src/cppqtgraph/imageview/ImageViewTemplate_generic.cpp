// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageViewTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/imageview/ImageViewTemplate_generic.hpp"

#include <QtCore/QCoreApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace cppqtgraph::imageview {

void Ui_Form::setupUi(QWidget* form)
{
    auto* rootLayout = new QVBoxLayout(form);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    splitter = new QSplitter(Qt::Vertical, form);
    splitter->setObjectName(QStringLiteral("splitter"));

    auto* topRow = new QWidget(splitter);
    topRow->setObjectName(QStringLiteral("topRow"));
    horizontalLayout = new QHBoxLayout(topRow);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    graphicsLayout = new QVBoxLayout();
    graphicsLayout->setContentsMargins(0, 0, 0, 0);
    graphicsLayout->setSpacing(0);

    graphicsContainer = new QWidget(topRow);
    graphicsContainer->setObjectName(QStringLiteral("graphicsContainer"));
    graphicsLayout->addWidget(graphicsContainer);

    auto* histogramColumn = new QWidget(topRow);
    histogramColumn->setObjectName(QStringLiteral("histogramColumn"));
    auto* histogramColumnLayout = new QVBoxLayout(histogramColumn);
    histogramColumnLayout->setContentsMargins(0, 0, 0, 0);
    histogramColumnLayout->setSpacing(0);

    histogramContainer = new QWidget(histogramColumn);
    histogramContainer->setObjectName(QStringLiteral("histogramContainer"));
    histogramContainer->setMinimumWidth(64);

    roiBtn = new QPushButton(histogramColumn);
    roiBtn->setObjectName(QStringLiteral("roiBtn"));
    roiBtn->setCheckable(true);
    roiBtn->setText(QCoreApplication::translate("ImageView", "ROI"));

    menuBtn = new QPushButton(histogramColumn);
    menuBtn->setObjectName(QStringLiteral("menuBtn"));
    menuBtn->setText(QCoreApplication::translate("ImageView", "Menu"));

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(0);
    buttonRow->addWidget(roiBtn);
    buttonRow->addWidget(menuBtn);

    histogramColumnLayout->addWidget(histogramContainer, 1);
    histogramColumnLayout->addLayout(buttonRow, 0);

    horizontalLayout->addLayout(graphicsLayout, 1);
    horizontalLayout->addWidget(histogramColumn);

    roiPlotContainer = new QWidget(splitter);
    roiPlotContainer->setObjectName(QStringLiteral("roiPlotContainer"));
    roiPlotContainer->setMinimumHeight(40);

    splitter->addWidget(topRow);
    splitter->addWidget(roiPlotContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    rootLayout->addWidget(splitter, 1);

    normGroup = new QGroupBox(form);
    normGroup->setObjectName(QStringLiteral("normGroup"));
    normGroup->setTitle(QCoreApplication::translate("ImageView", "Normalization"));
    auto* normLayout = new QGridLayout(normGroup);
    normLayout->setContentsMargins(0, 0, 0, 0);
    normLayout->setSpacing(0);

    auto* operationLabel = new QLabel(normGroup);
    operationLabel->setText(QCoreApplication::translate("ImageView", "Operation:"));
    QFont boldFont = operationLabel->font();
    boldFont.setBold(true);
    operationLabel->setFont(boldFont);
    normLayout->addWidget(operationLabel, 0, 0);

    normDivideRadio = new QRadioButton(normGroup);
    normDivideRadio->setObjectName(QStringLiteral("normDivideRadio"));
    normDivideRadio->setText(QCoreApplication::translate("ImageView", "Divide"));
    normLayout->addWidget(normDivideRadio, 0, 1);

    normSubtractRadio = new QRadioButton(normGroup);
    normSubtractRadio->setObjectName(QStringLiteral("normSubtractRadio"));
    normSubtractRadio->setText(QCoreApplication::translate("ImageView", "Subtract"));
    normLayout->addWidget(normSubtractRadio, 0, 2);

    normOffRadio = new QRadioButton(normGroup);
    normOffRadio->setObjectName(QStringLiteral("normOffRadio"));
    normOffRadio->setChecked(true);
    normOffRadio->setText(QCoreApplication::translate("ImageView", "Off"));
    normLayout->addWidget(normOffRadio, 0, 3);

    auto* meanLabel = new QLabel(normGroup);
    meanLabel->setText(QCoreApplication::translate("ImageView", "Mean:"));
    meanLabel->setFont(boldFont);
    normLayout->addWidget(meanLabel, 1, 0);

    normROICheck = new QCheckBox(normGroup);
    normROICheck->setObjectName(QStringLiteral("normROICheck"));
    normROICheck->setText(QCoreApplication::translate("ImageView", "ROI"));
    normLayout->addWidget(normROICheck, 1, 1);

    normFrameCheck = new QCheckBox(normGroup);
    normFrameCheck->setObjectName(QStringLiteral("normFrameCheck"));
    normFrameCheck->setText(QCoreApplication::translate("ImageView", "Frame"));
    normLayout->addWidget(normFrameCheck, 1, 2);

    normTimeRangeCheck = new QCheckBox(normGroup);
    normTimeRangeCheck->setObjectName(QStringLiteral("normTimeRangeCheck"));
    normTimeRangeCheck->setText(QCoreApplication::translate("ImageView", "Time range"));
    normLayout->addWidget(normTimeRangeCheck, 1, 3);

    normGroup->hide();
    rootLayout->addWidget(normGroup, 0);
}

} // namespace cppqtgraph::imageview
