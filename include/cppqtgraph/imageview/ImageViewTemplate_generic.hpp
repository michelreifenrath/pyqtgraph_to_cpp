#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/imageview/ImageViewTemplate_generic.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

class QHBoxLayout;
class QSplitter;
class QVBoxLayout;
class QWidget;

namespace cppqtgraph::imageview {

class Ui_Form {
public:
    void setupUi(QWidget* form);

    QWidget* graphicsContainer = nullptr;
    QWidget* histogramContainer = nullptr;
    QWidget* roiPlotContainer = nullptr;
    QSplitter* splitter = nullptr;
    QHBoxLayout* horizontalLayout = nullptr;
    QVBoxLayout* graphicsLayout = nullptr;
};

} // namespace cppqtgraph::imageview
