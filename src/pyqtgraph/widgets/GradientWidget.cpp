// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GradientWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/GradientWidget.hpp"

#include <QtCore/QRectF>
#include <QtGui/QPainter>
#include <QtWidgets/QFrame>

namespace pyqtgraph::widgets {

GradientWidget::GradientWidget(QWidget* parent, const QString& orientation)
    : GraphicsView(parent)
{
    setCacheMode(QGraphicsView::CacheNone);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setFrameShape(QFrame::NoFrame);

    auto* editor = new graphicsItems::GradientEditorItem(orientation);
    installEditor(editor);
    setOrientation(orientation);
}

void GradientWidget::installEditor(graphicsItems::GradientEditorItem* editor)
{
    if (item_ == editor) {
        return;
    }

    if (item_ != nullptr) {
        disconnect(item_, nullptr, this, nullptr);
        setCentralWidget(nullptr);
        delete item_;
        item_ = nullptr;
    }

    item_ = editor;
    if (item_ == nullptr) {
        return;
    }

    connect(item_, &graphicsItems::GradientEditorItem::sigGradientChanged, this, &GradientWidget::sigGradientChanged);
    connect(item_,
        &graphicsItems::GradientEditorItem::sigGradientChangeFinished,
        this,
        &GradientWidget::sigGradientChangeFinished);
    setCentralItem(item_);
    updateViewRange();
}

void GradientWidget::setOrientation(const QString& orientation)
{
    if (orientation != orientation_
        && (orientation == QStringLiteral("bottom") || orientation == QStringLiteral("top")
            || orientation == QStringLiteral("left") || orientation == QStringLiteral("right"))) {
        const graphicsItems::GradientEditorState state = item_ != nullptr ? item_->saveState() : graphicsItems::GradientEditorState{};
        const qreal length = item_ != nullptr ? item_->length() : 100.0;
        installEditor(new graphicsItems::GradientEditorItem(orientation));
        item_->setLength(length);
        item_->restoreState(state);
        orientation_ = orientation;
    } else if (orientation == orientation_) {
        // keep current editor
    } else {
        orientation_ = orientation;
    }
    applyOrientationSizing();
}

void GradientWidget::setMaxDim(int maxDim)
{
    if (maxDim >= 0) {
        maxDim_ = maxDim;
    }
    applyOrientationSizing();
}

void GradientWidget::applyOrientationSizing()
{
    if (orientation_ == QStringLiteral("bottom") || orientation_ == QStringLiteral("top")) {
        setFixedHeight(maxDim_);
        setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        setFixedWidth(maxDim_);
        setMaximumHeight(QWIDGETSIZE_MAX);
    }
    updateViewRange();
}

void GradientWidget::updateViewRange()
{
    if (item_ == nullptr) {
        return;
    }

    QRectF viewRect = item_->childrenBoundingRect();
    if (viewRect.isEmpty()) {
        viewRect = QRectF(0.0, -static_cast<qreal>(maxDim_), item_->length(), static_cast<qreal>(maxDim_));
    }
    setRange(viewRect.adjusted(-1.0, -1.0, 1.0, 1.0), 0.0);
}

pyqtgraph::ColorMap GradientWidget::colorMap() const
{
    return item_ != nullptr ? item_->colorMap() : pyqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
}

graphicsItems::GradientEditorState GradientWidget::saveState() const
{
    return item_ != nullptr ? item_->saveState() : graphicsItems::GradientEditorState{};
}

void GradientWidget::restoreState(const graphicsItems::GradientEditorState& state)
{
    if (item_ != nullptr) {
        item_->restoreState(state);
        updateViewRange();
    }
}

void GradientWidget::setLength(qreal length)
{
    if (item_ != nullptr) {
        item_->setLength(length);
        updateViewRange();
    }
}

} // namespace pyqtgraph::widgets
