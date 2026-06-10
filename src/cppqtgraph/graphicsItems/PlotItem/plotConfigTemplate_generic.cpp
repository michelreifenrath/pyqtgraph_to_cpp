// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.hpp"

#include <QtCore/Qt>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace cppqtgraph::graphicsItems::PlotItemConfig {

void Ui_Form::setupUi(QWidget* form)
{
    form->setObjectName(QStringLiteral("Form"));
    form->resize(481, 840);

    transformGroup = new QFrame(form);
    transformGroup->setObjectName(QStringLiteral("transformGroup"));
    auto* transformLayout = new QGridLayout(transformGroup);
    transformLayout->setContentsMargins(0, 0, 0, 0);
    transformLayout->setSpacing(0);

    fftCheck = new QCheckBox(transformGroup);
    fftCheck->setObjectName(QStringLiteral("fftCheck"));
    fftCheck->setText(QStringLiteral("Power Spectrum (FFT)"));
    transformLayout->addWidget(fftCheck, 0, 0, 1, 1);

    subtractMeanCheck = new QCheckBox(transformGroup);
    subtractMeanCheck->setObjectName(QStringLiteral("subtractMeanCheck"));
    subtractMeanCheck->setText(QStringLiteral("Subtract Mean"));
    transformLayout->addWidget(subtractMeanCheck, 1, 0, 1, 1);

    logXCheck = new QCheckBox(transformGroup);
    logXCheck->setObjectName(QStringLiteral("logXCheck"));
    logXCheck->setText(QStringLiteral("Log X"));
    transformLayout->addWidget(logXCheck, 2, 0, 1, 1);

    logYCheck = new QCheckBox(transformGroup);
    logYCheck->setObjectName(QStringLiteral("logYCheck"));
    logYCheck->setText(QStringLiteral("Log Y"));
    transformLayout->addWidget(logYCheck, 3, 0, 1, 1);

    derivativeCheck = new QCheckBox(transformGroup);
    derivativeCheck->setObjectName(QStringLiteral("derivativeCheck"));
    derivativeCheck->setText(QStringLiteral("dy/dx"));
    transformLayout->addWidget(derivativeCheck, 4, 0, 1, 1);

    phasemapCheck = new QCheckBox(transformGroup);
    phasemapCheck->setObjectName(QStringLiteral("phasemapCheck"));
    phasemapCheck->setText(QStringLiteral("Y vs. Y'"));
    transformLayout->addWidget(phasemapCheck, 5, 0, 1, 1);

    decimateGroup = new QFrame(form);
    decimateGroup->setObjectName(QStringLiteral("decimateGroup"));
    auto* decimateLayout = new QGridLayout(decimateGroup);
    decimateLayout->setContentsMargins(0, 0, 0, 0);
    decimateLayout->setSpacing(0);

    downsampleCheck = new QCheckBox(decimateGroup);
    downsampleCheck->setObjectName(QStringLiteral("downsampleCheck"));
    downsampleCheck->setText(QStringLiteral("Downsample"));
    decimateLayout->addWidget(downsampleCheck, 0, 0, 1, 3);

    downsampleSpin = new QSpinBox(decimateGroup);
    downsampleSpin->setObjectName(QStringLiteral("downsampleSpin"));
    downsampleSpin->setMinimum(1);
    downsampleSpin->setMaximum(100000);
    downsampleSpin->setValue(1);
    downsampleSpin->setSuffix(QStringLiteral("x"));
    decimateLayout->addWidget(downsampleSpin, 1, 1, 1, 1);

    autoDownsampleCheck = new QCheckBox(decimateGroup);
    autoDownsampleCheck->setObjectName(QStringLiteral("autoDownsampleCheck"));
    autoDownsampleCheck->setText(QStringLiteral("Auto"));
    autoDownsampleCheck->setChecked(true);
    decimateLayout->addWidget(autoDownsampleCheck, 1, 2, 1, 1);

    subsampleRadio = new QRadioButton(decimateGroup);
    subsampleRadio->setObjectName(QStringLiteral("subsampleRadio"));
    subsampleRadio->setText(QStringLiteral("Subsample"));
    decimateLayout->addWidget(subsampleRadio, 2, 1, 1, 2);

    meanRadio = new QRadioButton(decimateGroup);
    meanRadio->setObjectName(QStringLiteral("meanRadio"));
    meanRadio->setText(QStringLiteral("Mean"));
    decimateLayout->addWidget(meanRadio, 3, 1, 1, 2);

    peakRadio = new QRadioButton(decimateGroup);
    peakRadio->setObjectName(QStringLiteral("peakRadio"));
    peakRadio->setText(QStringLiteral("Peak"));
    peakRadio->setChecked(true);
    decimateLayout->addWidget(peakRadio, 6, 1, 1, 2);

    clipToViewCheck = new QCheckBox(decimateGroup);
    clipToViewCheck->setObjectName(QStringLiteral("clipToViewCheck"));
    clipToViewCheck->setText(QStringLiteral("Clip to View"));
    decimateLayout->addWidget(clipToViewCheck, 7, 0, 1, 3);

    maxTracesCheck = new QCheckBox(decimateGroup);
    maxTracesCheck->setObjectName(QStringLiteral("maxTracesCheck"));
    maxTracesCheck->setText(QStringLiteral("Max Traces:"));
    decimateLayout->addWidget(maxTracesCheck, 8, 0, 1, 2);

    maxTracesSpin = new QSpinBox(decimateGroup);
    maxTracesSpin->setObjectName(QStringLiteral("maxTracesSpin"));
    decimateLayout->addWidget(maxTracesSpin, 8, 2, 1, 1);

    forgetTracesCheck = new QCheckBox(decimateGroup);
    forgetTracesCheck->setObjectName(QStringLiteral("forgetTracesCheck"));
    forgetTracesCheck->setText(QStringLiteral("Forget hidden traces"));
    decimateLayout->addWidget(forgetTracesCheck, 9, 0, 1, 3);

    alphaGroup = new QGroupBox(form);
    alphaGroup->setObjectName(QStringLiteral("alphaGroup"));
    alphaGroup->setTitle(QStringLiteral("Alpha"));
    alphaGroup->setCheckable(true);
    alphaGroup->setChecked(true);
    auto* alphaLayout = new QHBoxLayout(alphaGroup);

    autoAlphaCheck = new QCheckBox(alphaGroup);
    autoAlphaCheck->setObjectName(QStringLiteral("autoAlphaCheck"));
    autoAlphaCheck->setText(QStringLiteral("Auto"));
    autoAlphaCheck->setChecked(false);
    alphaLayout->addWidget(autoAlphaCheck);

    alphaSlider = new QSlider(Qt::Horizontal, alphaGroup);
    alphaSlider->setObjectName(QStringLiteral("alphaSlider"));
    alphaSlider->setMaximum(1000);
    alphaSlider->setValue(1000);
    alphaLayout->addWidget(alphaSlider);

    gridGroup = new QFrame(form);
    gridGroup->setObjectName(QStringLiteral("gridGroup"));
    auto* gridLayout = new QGridLayout(gridGroup);

    xGridCheck = new QCheckBox(gridGroup);
    xGridCheck->setObjectName(QStringLiteral("xGridCheck"));
    xGridCheck->setText(QStringLiteral("Show X Grid"));
    gridLayout->addWidget(xGridCheck, 0, 0, 1, 2);

    yGridCheck = new QCheckBox(gridGroup);
    yGridCheck->setObjectName(QStringLiteral("yGridCheck"));
    yGridCheck->setText(QStringLiteral("Show Y Grid"));
    gridLayout->addWidget(yGridCheck, 1, 0, 1, 2);

    opacityLabel = new QLabel(gridGroup);
    opacityLabel->setObjectName(QStringLiteral("label"));
    opacityLabel->setText(QStringLiteral("Opacity"));
    gridLayout->addWidget(opacityLabel, 2, 0, 1, 1);

    gridAlphaSlider = new QSlider(Qt::Horizontal, gridGroup);
    gridAlphaSlider->setObjectName(QStringLiteral("gridAlphaSlider"));
    gridAlphaSlider->setMaximum(255);
    gridAlphaSlider->setValue(128);
    gridLayout->addWidget(gridAlphaSlider, 2, 1, 1, 1);

    pointsGroup = new QGroupBox(form);
    pointsGroup->setObjectName(QStringLiteral("pointsGroup"));
    pointsGroup->setTitle(QStringLiteral("Points"));
    pointsGroup->setCheckable(true);
    pointsGroup->setChecked(true);
    auto* pointsLayout = new QVBoxLayout(pointsGroup);

    autoPointsCheck = new QCheckBox(pointsGroup);
    autoPointsCheck->setObjectName(QStringLiteral("autoPointsCheck"));
    autoPointsCheck->setText(QStringLiteral("Auto"));
    autoPointsCheck->setChecked(true);
    pointsLayout->addWidget(autoPointsCheck);

    averageGroup = new QGroupBox(form);
    averageGroup->setObjectName(QStringLiteral("averageGroup"));
    averageGroup->setTitle(QStringLiteral("Average"));
    averageGroup->setCheckable(true);
    averageGroup->setChecked(false);
    auto* averageLayout = new QGridLayout(averageGroup);
    averageLayout->setContentsMargins(0, 0, 0, 0);
    averageLayout->setSpacing(0);

    avgParamList = new QListWidget(averageGroup);
    avgParamList->setObjectName(QStringLiteral("avgParamList"));
    averageLayout->addWidget(avgParamList, 0, 0, 1, 1);
}

} // namespace cppqtgraph::graphicsItems::PlotItemConfig
