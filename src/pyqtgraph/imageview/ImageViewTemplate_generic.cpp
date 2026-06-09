// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageViewTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/imageview/ImageViewTemplate_generic.hpp"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace pyqtgraph::imageview {

void Ui_Form::setupUi(QWidget* form)
{
    horizontalLayout = new QHBoxLayout(form);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    graphicsLayout = new QVBoxLayout();
    graphicsLayout->setContentsMargins(0, 0, 0, 0);
    graphicsLayout->setSpacing(0);

    graphicsContainer = new QWidget(form);
    graphicsContainer->setObjectName(QStringLiteral("graphicsContainer"));
    graphicsLayout->addWidget(graphicsContainer);

    histogramContainer = new QWidget(form);
    histogramContainer->setObjectName(QStringLiteral("histogramContainer"));
    histogramContainer->setMinimumWidth(64);

    horizontalLayout->addLayout(graphicsLayout, 1);
    horizontalLayout->addWidget(histogramContainer);
}

} // namespace pyqtgraph::imageview
