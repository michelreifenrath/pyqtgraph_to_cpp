// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/GraphicsView.py,
// pyqtgraph/widgets/GraphicsLayoutWidget.py, and pyqtgraph/graphicsItems/GraphicsLayout.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/GraphicsView.hpp"

#include "../../../include/pyqtgraph/graphicsItems/GraphicsLayout.hpp"
#include "../../../include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp"
#include "../../../include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp"
#include "../../../include/pyqtgraph/widgets/GraphicsLayoutWidget.hpp"

#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsGridLayout>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace pyqtgraph::widgets {

GraphicsView::GraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , scene_(std::make_unique<GraphicsScene::GraphicsScene>(2, 5.0, this))
{
    setCacheMode(QGraphicsView::CacheBackground);
    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setMouseTracking(true);
    setBackgroundBrush(Qt::black);
    setScene(scene_.get());

    auto* defaultCentral = new QGraphicsWidget();
    defaultCentral->setLayout(new QGraphicsGridLayout());
    internallyOwnedDefaultCentral_ = defaultCentral;
    setCentralWidget(defaultCentral);
    updateMatrix();
}

GraphicsView::~GraphicsView()
{
    setScene(nullptr);
}

GraphicsScene::GraphicsScene* GraphicsView::graphicsScene() noexcept
{
    return scene_.get();
}

const GraphicsScene::GraphicsScene* GraphicsView::graphicsScene() const noexcept
{
    return scene_.get();
}

void GraphicsView::setCentralItem(QGraphicsWidget* item)
{
    setCentralWidget(item);
}

void GraphicsView::setCentralWidget(QGraphicsWidget* item)
{
    if (centralWidget_ != nullptr) {
        scene_->removeItem(centralWidget_);
        if (centralWidget_ == internallyOwnedDefaultCentral_) {
            delete centralWidget_.data();
            internallyOwnedDefaultCentral_.clear();
        }
    }

    centralWidget_ = item;
    if (centralWidget_ != nullptr) {
        scene_->addItem(centralWidget_);
        updateCentralGeometry();
    }
}

QGraphicsWidget* GraphicsView::centralItem() const noexcept
{
    return centralWidget_;
}

QGraphicsWidget* GraphicsView::centralWidget() const noexcept
{
    return centralWidget_;
}

void GraphicsView::addItem(QGraphicsItem* item)
{
    scene_->addItem(item);
}

void GraphicsView::removeItem(QGraphicsItem* item)
{
    scene_->removeItem(item);
}

void GraphicsView::enableMouse(bool enabled)
{
    mouseEnabled_ = enabled;
    autoPixelRange_ = !enabled;
}

bool GraphicsView::mouseEnabled() const noexcept
{
    return mouseEnabled_;
}

void GraphicsView::setRange(const QRectF& rect, qreal padding, bool lockAspect, bool propagate, bool disableAutoPixel)
{
    if (disableAutoPixel) {
        autoPixelRange_ = false;
    }
    aspectLocked_ = lockAspect;

    const QRectF newRange = paddedRect(rect, padding);
    const bool scaleChanged = !qFuzzyCompare(range_.width(), newRange.width())
        || !qFuzzyCompare(range_.height(), newRange.height());
    range_ = newRange;
    updateCentralGeometry();
    updateMatrix(propagate);
    if (scaleChanged) {
        Q_EMIT sigScaleChanged(this);
    }
}

void GraphicsView::setXRange(qreal left, qreal right, qreal padding)
{
    QRectF updated = range_;
    updated.setLeft(left);
    updated.setRight(right);
    setRange(updated, padding, aspectLocked_, false);
}

void GraphicsView::setYRange(qreal top, qreal bottom, qreal padding)
{
    QRectF updated = range_;
    updated.setTop(top);
    updated.setBottom(bottom);
    setRange(updated, padding, aspectLocked_, false);
}

void GraphicsView::translateRange(qreal dx, qreal dy)
{
    range_.adjust(dx, dy, dx, dy);
    updateCentralGeometry();
    updateMatrix();
}

void GraphicsView::scaleRange(qreal sx, qreal sy, QPointF center)
{
    if (qFuzzyIsNull(sx) || qFuzzyIsNull(sy)) {
        return;
    }
    if (aspectLocked_) {
        sx = sy;
    }
    if (center.isNull()) {
        center = range_.center();
    }

    const qreal width = range_.width() / sx;
    const qreal height = range_.height() / sy;
    range_ = QRectF(center.x() - (center.x() - range_.left()) / sx,
                    center.y() - (center.y() - range_.top()) / sy,
                    width,
                    height);
    updateCentralGeometry();
    updateMatrix();
    Q_EMIT sigScaleChanged(this);
}

void GraphicsView::setAspectLocked(bool locked)
{
    aspectLocked_ = locked;
    updateMatrix();
}

QRectF GraphicsView::range() const noexcept
{
    return range_;
}

QRectF GraphicsView::viewRect() const
{
    const QRect viewportRect(QPoint(0, 0), viewport()->size());
    bool invertible = false;
    const QTransform inverse = viewportTransform().inverted(&invertible);
    if (!invertible) {
        return range_;
    }
    return inverse.mapRect(QRectF(viewportRect));
}

QRectF GraphicsView::visibleRange() const
{
    return viewRect();
}

void GraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (autoPixelRange_) {
        const QSize viewportSize = viewport()->size();
        range_ = QRectF(0.0, 0.0, static_cast<qreal>(viewportSize.width()), static_cast<qreal>(viewportSize.height()));
    }
    setRange(range_, 0.0, aspectLocked_, true, false);
}

void GraphicsView::wheelEvent(QWheelEvent* event)
{
    QGraphicsView::wheelEvent(event);
    if (!mouseEnabled_) {
        return;
    }

    int delta = event->angleDelta().x();
    if (delta == 0) {
        delta = event->angleDelta().y();
    }
    const qreal scale = std::pow(1.001, static_cast<qreal>(delta));
    scaleRange(scale, scale);
}

void GraphicsView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);
    if (!mouseEnabled_) {
        return;
    }
    lastMousePos_ = event->position();
    hasLastMousePos_ = true;
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if (!mouseEnabled_) {
        return;
    }
    Q_EMIT sigMouseReleased(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF localPos = event->position();
    if (!hasLastMousePos_) {
        lastMousePos_ = localPos;
        hasLastMousePos_ = true;
    }
    const QPointF delta = localPos - lastMousePos_;
    lastMousePos_ = localPos;

    QGraphicsView::mouseMoveEvent(event);
    if (!mouseEnabled_) {
        return;
    }

    Q_EMIT sigSceneMouseMoved(mapToScene(localPos.toPoint()));
    if (event->buttons().testFlag(Qt::MiddleButton) || event->buttons().testFlag(Qt::LeftButton)) {
        const QRectF pixel = viewportTransform().inverted().mapRect(QRectF(0.0, 0.0, 1.0, 1.0));
        translateRange(-delta.x() * pixel.width(), -delta.y() * pixel.height());
    } else if (event->buttons().testFlag(Qt::RightButton)) {
        const qreal sx = std::pow(1.01, std::clamp(delta.x(), -50.0, 50.0));
        const qreal sy = std::pow(1.01, std::clamp(-delta.y(), -50.0, 50.0));
        scaleRange(sx, sy, mapToScene(localPos.toPoint()));
    }
}

void GraphicsView::updateMatrix(bool /*propagate*/)
{
    if (scene_ != nullptr) {
        scene_->setSceneRect(range_);
    }
    QGraphicsView::setSceneRect(range_);
    if (autoPixelRange_) {
        resetTransform();
    } else {
        fitInView(range_, aspectLocked_ ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio);
    }

    Q_EMIT sigDeviceRangeChanged(this, range_);
    Q_EMIT sigDeviceTransformChanged(this);
}

void GraphicsView::updateCentralGeometry()
{
    if (centralWidget_ != nullptr) {
        centralWidget_->setGeometry(range_);
    }
}

QRectF GraphicsView::paddedRect(const QRectF& rect, qreal padding) const
{
    const qreal px = rect.width() * padding;
    const qreal py = rect.height() * padding;
    return rect.adjusted(-px, -py, px, py);
}

GraphicsLayoutWidget::GraphicsLayoutWidget(QWidget* parent)
    : GraphicsLayoutWidget(parent, false, std::nullopt, QString{})
{
}

GraphicsLayoutWidget::GraphicsLayoutWidget(QWidget* parent, bool showWidget, std::optional<QSize> size, const QString& title)
    : GraphicsView(parent)
    , ci(new graphicsItems::GraphicsLayout())
{
    setCentralItem(ci);
    if (size.has_value()) {
        resize(*size);
    }
    if (!title.isNull() && !title.isEmpty()) {
        setWindowTitle(title);
    }
    if (showWidget) {
        show();
    }
}

GraphicsLayoutWidget::~GraphicsLayoutWidget() = default;

graphicsItems::GraphicsLayout* GraphicsLayoutWidget::graphicsLayout() noexcept
{
    return ci;
}

const graphicsItems::GraphicsLayout* GraphicsLayoutWidget::graphicsLayout() const noexcept
{
    return ci;
}

void GraphicsLayoutWidget::nextRow()
{
    ci->nextRow();
}

void GraphicsLayoutWidget::nextColumn()
{
    ci->nextColumn();
}

void GraphicsLayoutWidget::nextCol()
{
    ci->nextCol();
}

graphicsItems::PlotItem* GraphicsLayoutWidget::addPlot(int row, int col, int rowspan, int colspan)
{
    return ci->addPlot(row, col, rowspan, colspan);
}

graphicsItems::ViewBox* GraphicsLayoutWidget::addViewBox(int row, int col, int rowspan, int colspan)
{
    return ci->addViewBox(row, col, rowspan, colspan);
}

graphicsItems::GraphicsLayout* GraphicsLayoutWidget::addLayout(int row, int col, int rowspan, int colspan)
{
    return ci->addLayout(row, col, rowspan, colspan);
}

QGraphicsWidget* GraphicsLayoutWidget::addLabel(const QString& text, int row, int col, int rowspan, int colspan)
{
    return ci->addLabel(text, row, col, rowspan, colspan);
}

void GraphicsLayoutWidget::addItem(QGraphicsWidget* item, int row, int col, int rowspan, int colspan)
{
    ci->addItem(item, row, col, rowspan, colspan);
}

QGraphicsWidget* GraphicsLayoutWidget::getItem(int row, int col) const
{
    return ci->getItem(row, col);
}

int GraphicsLayoutWidget::itemIndex(const QGraphicsWidget* item) const
{
    return ci->itemIndex(item);
}

void GraphicsLayoutWidget::removeItem(QGraphicsWidget* item)
{
    ci->removeItem(item);
}

void GraphicsLayoutWidget::clear()
{
    ci->clear();
}

} // namespace pyqtgraph::widgets

namespace pyqtgraph::graphicsItems {

namespace {

class SimpleLabelItem : public GraphicsWidget {
public:
    explicit SimpleLabelItem(QString text, QGraphicsItem* parent = nullptr)
        : GraphicsWidget(parent)
        , text_(std::move(text))
    {
        setPreferredSize(80.0, 24.0);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override
    {
        GraphicsWidget::paint(painter, option, widget);
        painter->setPen(Qt::white);
        painter->drawText(rect(), Qt::AlignCenter, text_);
    }

private:
    QString text_;
};

} // namespace

GraphicsLayout::GraphicsLayout(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
    , layout_(new QGraphicsGridLayout())
{
    setLayout(layout_);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

GraphicsLayout::~GraphicsLayout() = default;

void GraphicsLayout::nextRow()
{
    ++currentRow_;
    currentCol_ = -1;
    nextColumn();
}

void GraphicsLayout::nextColumn()
{
    ++currentCol_;
    while (getItem(currentRow_, currentCol_) != nullptr) {
        ++currentCol_;
    }
}

void GraphicsLayout::nextCol()
{
    nextColumn();
}

PlotItem* GraphicsLayout::addPlot(int row, int col, int rowspan, int colspan)
{
    auto* plot = new PlotItem();
    addItem(plot, row, col, rowspan, colspan);
    return plot;
}

ViewBox* GraphicsLayout::addViewBox(int row, int col, int rowspan, int colspan)
{
    auto* viewBox = new ViewBox();
    addItem(viewBox, row, col, rowspan, colspan);
    return viewBox;
}

GraphicsLayout* GraphicsLayout::addLayout(int row, int col, int rowspan, int colspan)
{
    auto* nested = new GraphicsLayout();
    addItem(nested, row, col, rowspan, colspan);
    return nested;
}

QGraphicsWidget* GraphicsLayout::addLabel(const QString& text, int row, int col, int rowspan, int colspan)
{
    auto* label = new SimpleLabelItem(text);
    addItem(label, row, col, rowspan, colspan);
    return label;
}

void GraphicsLayout::addItem(QGraphicsWidget* item, int row, int col, int rowspan, int colspan)
{
    if (item == nullptr) {
        return;
    }

    const Cell cell = resolveCell(row, col);
    const int safeRowspan = std::max(1, rowspan);
    const int safeColspan = std::max(1, colspan);
    forgetCells(item);
    rememberCells(item, cell.first, cell.second, safeRowspan, safeColspan);

    layout_->addItem(item, cell.first, cell.second, safeRowspan, safeColspan);
    layout_->activate();
    nextColumn();
}

QGraphicsWidget* GraphicsLayout::getItem(int row, int col) const
{
    const auto it = rows_.find(Cell{row, col});
    if (it == rows_.end()) {
        return nullptr;
    }
    return it->second;
}

int GraphicsLayout::itemIndex(const QGraphicsWidget* item) const
{
    if (item == nullptr) {
        return -1;
    }

    for (int index = 0; index < layout_->count(); ++index) {
        const QGraphicsLayoutItem* layoutItem = layout_->itemAt(index);
        if (layoutItem != nullptr && layoutItem->graphicsItem() == item) {
            return index;
        }
    }
    return -1;
}

void GraphicsLayout::removeItem(QGraphicsWidget* item)
{
    const int index = itemIndex(item);
    if (index < 0) {
        return;
    }

    layout_->removeAt(index);
    forgetCells(item);
    if (scene() != nullptr) {
        scene()->removeItem(item);
    }
    item->setParentItem(nullptr);
    layout_->invalidate();
    layout_->activate();
    updateGeometry();
    update();
}

void GraphicsLayout::clear()
{
    const std::vector<QGraphicsWidget*> items = [&]() {
        std::vector<QGraphicsWidget*> result;
        result.reserve(items_.size());
        for (const auto& [item, cells] : items_) {
            Q_UNUSED(cells);
            result.push_back(item);
        }
        return result;
    }();

    for (QGraphicsWidget* item : items) {
        removeItem(item);
    }
    currentRow_ = 0;
    currentCol_ = 0;
}

void GraphicsLayout::setContentsMargins(qreal left, qreal top, qreal right, qreal bottom)
{
    layout_->setContentsMargins(left, top, right, bottom);
    layout_->activate();
}

void GraphicsLayout::setSpacing(qreal spacing)
{
    layout_->setSpacing(spacing);
    layout_->activate();
}

int GraphicsLayout::currentRow() const noexcept
{
    return currentRow_;
}

int GraphicsLayout::currentColumn() const noexcept
{
    return currentCol_;
}

int GraphicsLayout::currentCol() const noexcept
{
    return currentCol_;
}

QGraphicsGridLayout* GraphicsLayout::gridLayout() noexcept
{
    return layout_;
}

const QGraphicsGridLayout* GraphicsLayout::gridLayout() const noexcept
{
    return layout_;
}

GraphicsLayout::Cell GraphicsLayout::resolveCell(int row, int col) const
{
    return Cell{row < 0 ? currentRow_ : row, col < 0 ? currentCol_ : col};
}

void GraphicsLayout::rememberCells(QGraphicsWidget* item, int row, int col, int rowspan, int colspan)
{
    auto& cells = items_[item];
    cells.clear();
    for (int rowOffset = 0; rowOffset < rowspan; ++rowOffset) {
        for (int colOffset = 0; colOffset < colspan; ++colOffset) {
            const Cell cell{row + rowOffset, col + colOffset};
            rows_[cell] = item;
            cells.push_back(cell);
        }
    }
}

void GraphicsLayout::forgetCells(QGraphicsWidget* item)
{
    const auto itemIt = items_.find(item);
    if (itemIt == items_.end()) {
        return;
    }

    for (const Cell& cell : itemIt->second) {
        const auto rowIt = rows_.find(cell);
        if (rowIt != rows_.end() && rowIt->second == item) {
            rows_.erase(rowIt);
        }
    }
    items_.erase(itemIt);
}

} // namespace pyqtgraph::graphicsItems
