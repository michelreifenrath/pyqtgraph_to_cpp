// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageViewTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/imageview/ImageViewTemplate_generic.hpp"

#include <QtWidgets/QHBoxLayout>
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

    histogramContainer = new QWidget(topRow);
    histogramContainer->setObjectName(QStringLiteral("histogramContainer"));
    histogramContainer->setMinimumWidth(64);

    horizontalLayout->addLayout(graphicsLayout, 1);
    horizontalLayout->addWidget(histogramContainer);

    roiPlotContainer = new QWidget(splitter);
    roiPlotContainer->setObjectName(QStringLiteral("roiPlotContainer"));
    roiPlotContainer->setMinimumHeight(40);

    splitter->addWidget(topRow);
    splitter->addWidget(roiPlotContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    rootLayout->addWidget(splitter);
}

} // namespace cppqtgraph::imageview
