// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/InfiniteLine.py,
// pyqtgraph/graphicsItems/LinearRegionItem.py, and pyqtgraph/graphicsItems/TargetItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/InfiniteLine.hpp"
#include "../../../include/cppqtgraph/graphicsItems/LinearRegionItem.hpp"
#include "../../../include/cppqtgraph/graphicsItems/TargetItem.hpp"

#include "../../../include/cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace cppqtgraph::graphicsItems {
namespace {

InfiniteLine::Bounds finiteBounds(std::pair<qreal, qreal> bounds)
{
    return InfiniteLine::Bounds{bounds.first, bounds.second};
}

ViewBox* findViewBox(QGraphicsItem* item)
{
    for (QGraphicsItem* current = item; current != nullptr; current = current->parentItem()) {
        if (auto* viewBox = dynamic_cast<ViewBox*>(current); viewBox != nullptr) {
            return viewBox;
        }
    }
    return nullptr;
}

QRectF fallbackViewRect()
{
    return QRectF(-1000.0, -1000.0, 2000.0, 2000.0);
}

QRectF localViewRect(const QGraphicsItem* item, ViewBox* viewBox)
{
    if (viewBox == nullptr) {
        return fallbackViewRect();
    }

    QRectF view = viewBox->viewRect();
    std::vector<const QGraphicsItem*> ancestors;
    for (const QGraphicsItem* current = item; current != nullptr && current != viewBox; current = current->parentItem()) {
        ancestors.push_back(current);
    }
    for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
        if ((*it)->parentItem() == viewBox) {
            continue;
        }
        view = (*it)->mapRectFromParent(view);
    }
    return view;
}

QPainterPath crosshairPath()
{
    QPainterPath path;
    path.addEllipse(QPointF(0.0, 0.0), 0.35, 0.35);
    path.moveTo(-0.5, 0.0);
    path.lineTo(-0.2, 0.0);
    path.moveTo(0.2, 0.0);
    path.lineTo(0.5, 0.0);
    path.moveTo(0.0, -0.5);
    path.lineTo(0.0, -0.2);
    path.moveTo(0.0, 0.2);
    path.lineTo(0.0, 0.5);
    return path;
}

} // namespace

InfiniteLine::InfiniteLine(qreal pos, qreal angle, bool movable, std::optional<Bounds> bounds, QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setPen(QPen(QColor(200, 200, 100), 1.0));
    setHoverPen(QPen(QColor(255, 0, 0), 1.0));
    if (bounds.has_value()) {
        bounds_ = *bounds;
    }
    setAngle(angle);
    setMovable(movable);
    setPos(pos);
}

InfiniteLine::InfiniteLine(qreal pos, qreal angle, bool movable, std::pair<qreal, qreal> bounds, QGraphicsItem* parent)
    : InfiniteLine(pos, angle, movable, finiteBounds(bounds), parent)
{
}

InfiniteLine::InfiniteLine(const QPointF& pos, qreal angle, bool movable, std::optional<Bounds> bounds, QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setPen(QPen(QColor(200, 200, 100), 1.0));
    setHoverPen(QPen(QColor(255, 0, 0), 1.0));
    if (bounds.has_value()) {
        bounds_ = *bounds;
    }
    setAngle(angle);
    setMovable(movable);
    setPos(pos);
}

InfiniteLine::InfiniteLine(const QPointF& pos, qreal angle, bool movable, std::pair<qreal, qreal> bounds, QGraphicsItem* parent)
    : InfiniteLine(pos, angle, movable, finiteBounds(bounds), parent)
{
}

InfiniteLine::~InfiniteLine() = default;

void InfiniteLine::setMovable(bool movable)
{
    movable_ = movable;
    setAcceptHoverEvents(movable_);
}

bool InfiniteLine::movable() const noexcept
{
    return movable_;
}

void InfiniteLine::setBounds(std::optional<Bounds> bounds)
{
    bounds_ = bounds.value_or(Bounds{std::nullopt, std::nullopt});
    setPos(position_);
}

void InfiniteLine::setBounds(std::pair<qreal, qreal> bounds)
{
    setBounds(finiteBounds(bounds));
}

InfiniteLine::Bounds InfiniteLine::bounds() const
{
    return bounds_;
}

void InfiniteLine::setAngle(qreal angle)
{
    if (qFuzzyCompare(angle_ + 1.0, angle + 1.0)) {
        return;
    }
    prepareGeometryChange();
    angle_ = angle;
    resetTransform();
    setRotation(angle_);
    invalidateBounds();
    update();
}

qreal InfiniteLine::angle() const noexcept
{
    return angle_;
}

void InfiniteLine::setPos(qreal value)
{
    if (std::fmod(std::abs(angle_), 180.0) == 90.0) {
        setPos(QPointF(value, 0.0));
    } else if (std::fmod(std::abs(angle_), 180.0) == 0.0) {
        setPos(QPointF(0.0, value));
    } else {
        setPos(QPointF(value, 0.0));
    }
}

void InfiniteLine::setPos(qreal x, qreal y)
{
    setPos(QPointF(x, y));
}

void InfiniteLine::setPos(const QPointF& pos)
{
    const QPointF newPos = clampPosition(pos);
    if (qFuzzyCompare(position_.x() + 1.0, newPos.x() + 1.0) && qFuzzyCompare(position_.y() + 1.0, newPos.y() + 1.0)) {
        return;
    }

    position_ = newPos;
    GraphicsObject::setPos(position_);
    invalidateBounds();
    emit sigPositionChanged(this);
}

void InfiniteLine::setValue(qreal value)
{
    setPos(value);
}

qreal InfiniteLine::getXPos() const noexcept
{
    return position_.x();
}

qreal InfiniteLine::getYPos() const noexcept
{
    return position_.y();
}

QPointF InfiniteLine::getPos() const noexcept
{
    return position_;
}

qreal InfiniteLine::value() const noexcept
{
    if (std::fmod(std::abs(angle_), 180.0) == 0.0) {
        return getYPos();
    }
    return getXPos();
}

void InfiniteLine::setSpan(qreal min, qreal max)
{
    const std::pair<qreal, qreal> next{min, max};
    if (span_ == next) {
        return;
    }
    prepareGeometryChange();
    span_ = next;
    update();
}

std::pair<qreal, qreal> InfiniteLine::span() const noexcept
{
    return span_;
}

void InfiniteLine::setPen(const QPen& pen)
{
    pen_ = pen;
    if (!mouseHovering_) {
        currentPen_ = pen_;
    }
    update();
}

QPen InfiniteLine::pen() const
{
    return pen_;
}

void InfiniteLine::setHoverPen(const QPen& pen)
{
    hoverPen_ = pen;
    if (!hoverPen_.isCosmetic()) {
        hoverPen_.setCosmetic(pen_.isCosmetic());
    }
    if (mouseHovering_) {
        currentPen_ = hoverPen_;
    }
    update();
}

QPen InfiniteLine::hoverPen() const
{
    return hoverPen_;
}

void InfiniteLine::setMouseHover(bool hover)
{
    if (mouseHovering_ == hover) {
        return;
    }
    mouseHovering_ = hover;
    currentPen_ = mouseHovering_ ? hoverPen_ : pen_;
    update();
}

bool InfiniteLine::mouseHovering() const noexcept
{
    return mouseHovering_;
}

void InfiniteLine::setName(const QString& name)
{
    name_ = name;
}

QString InfiniteLine::name() const
{
    return name_;
}

std::optional<QRectF> InfiniteLine::autoRangeBoundsRect() const
{
    return std::nullopt;
}

QRectF InfiniteLine::boundingRect() const
{
    const QRectF view = localViewRect(this, findViewBox(parentItem()));

    const qreal penWidth = std::max<qreal>({1.0, pen_.widthF(), hoverPen_.widthF()});
    QRectF rect(view.left(), -penWidth - 2.0, view.width(), 2.0 * (penWidth + 2.0));
    const qreal left = rect.left() + rect.width() * span_.first;
    const qreal right = rect.left() + rect.width() * span_.second;
    rect.setLeft(left);
    rect.setRight(right);
    return rect.normalized();
}

void InfiniteLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    const QRectF rect = boundingRect();
    QPen drawPen = currentPen_;
    drawPen.setJoinStyle(Qt::MiterJoin);
    painter->setPen(drawPen);
    painter->drawLine(QPointF(rect.left(), 0.0), QPointF(rect.right(), 0.0));
}

void InfiniteLine::hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event != nullptr && !event->isExit() && movable_ && event->acceptDrags(Qt::LeftButton, this)) {
        setMouseHover(true);
    } else {
        setMouseHover(false);
    }
}

void InfiniteLine::mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event)
{
    emit sigClicked(this, event);
    if (event != nullptr && moving_ && event->button() == Qt::RightButton) {
        event->accept(this);
        setPos(startPosition_);
        moving_ = false;
        emit sigDragged(this);
        emit sigPositionChangeFinished(this);
    }
}

void InfiniteLine::mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event)
{
    if (event == nullptr || !movable_ || event->button() != Qt::LeftButton) {
        return;
    }

    if (event->isStart()) {
        moving_ = true;
        cursorOffset_ = position_ - mapToParent(event->buttonDownPos());
        startPosition_ = position_;
    }
    event->accept(this);

    if (!moving_) {
        return;
    }

    setPos(cursorOffset_ + mapToParent(event->pos()));
    emit sigDragged(this);
    if (event->isFinish()) {
        moving_ = false;
        emit sigPositionChangeFinished(this);
    }
}

QPointF InfiniteLine::clampPosition(QPointF pos) const
{
    const bool vertical = std::fmod(std::abs(angle_), 180.0) == 90.0;
    const bool horizontal = std::fmod(std::abs(angle_), 180.0) == 0.0;
    qreal* coordinate = vertical ? &pos.rx() : horizontal ? &pos.ry() : nullptr;
    if (coordinate != nullptr) {
        if (bounds_.first.has_value()) {
            *coordinate = std::max(*coordinate, *bounds_.first);
        }
        if (bounds_.second.has_value()) {
            *coordinate = std::min(*coordinate, *bounds_.second);
        }
    }
    return pos;
}

void InfiniteLine::invalidateBounds()
{
    prepareGeometryChange();
    update();
}

void InfiniteLine::refreshViewGeometry()
{
    invalidateBounds();
}

LinearRegionItem::LinearRegionItem(std::pair<qreal, qreal> values,
                                   Orientation orientation,
                                   bool movable,
                                   std::optional<InfiniteLine::Bounds> bounds,
                                   QGraphicsItem* parent)
    : GraphicsObject(parent)
    , orientation_(orientation)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    const qreal angle = orientation_ == Orientation::Vertical ? 90.0 : 0.0;
    lines_[0] = new InfiniteLine(valuePoint(values.first), angle, movable, bounds, this);
    lines_[1] = new InfiniteLine(valuePoint(values.second), angle, movable, bounds, this);

    for (auto* regionLine : lines_) {
        regionLine->setParentItem(this);
        connect(regionLine, &InfiniteLine::sigPositionChangeFinished, this, [this](InfiniteLine*) { lineMoveFinished(); });
    }
    connect(lines_[0], &InfiniteLine::sigPositionChanged, this, [this](InfiniteLine*) { lineMoved(0); });
    connect(lines_[1], &InfiniteLine::sigPositionChanged, this, [this](InfiniteLine*) { lineMoved(1); });

    setBrush(QBrush(QColor(0, 0, 255, 50)));
    QColor hover = brush_.color();
    hover.setAlpha(std::min(hover.alpha() * 2, 255));
    setHoverBrush(QBrush(hover));
    setMovable(movable);
    connectViewBoxUpdates();
}

void LinearRegionItem::connectViewBoxUpdates()
{
    QObject::disconnect(viewRangeConnection_);
    QObject::disconnect(viewTransformConnection_);
    viewRangeConnection_ = {};
    viewTransformConnection_ = {};

    ViewBox* viewBox = findViewBox(parentItem());
    if (viewBox == nullptr) {
        return;
    }

    viewRangeConnection_ = QObject::connect(
        viewBox,
        &ViewBox::sigRangeChanged,
        this,
        [this](ViewBox*, ViewBox::Range2D, std::array<bool, 2>) { invalidateViewGeometry(); });
    viewTransformConnection_ = QObject::connect(
        viewBox, &ViewBox::sigTransformChanged, this, [this](ViewBox*) { invalidateViewGeometry(); });
    invalidateViewGeometry();
}

void LinearRegionItem::invalidateViewGeometry()
{
    prepareGeometryChange();
    for (auto* regionLine : lines_) {
        regionLine->refreshViewGeometry();
    }
    update();
}

QVariant LinearRegionItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemParentHasChanged || change == QGraphicsItem::ItemSceneHasChanged) {
        connectViewBoxUpdates();
    }
    return GraphicsObject::itemChange(change, value);
}

LinearRegionItem::~LinearRegionItem() = default;

std::pair<qreal, qreal> LinearRegionItem::getRegion() const
{
    std::pair<qreal, qreal> region{lines_[0]->value(), lines_[1]->value()};
    if (swapMode_ == SwapMode::Sort && region.first > region.second) {
        std::swap(region.first, region.second);
    }
    return region;
}

void LinearRegionItem::setRegion(std::pair<qreal, qreal> region)
{
    if (qFuzzyCompare(lines_[0]->value() + 1.0, region.first + 1.0) &&
        qFuzzyCompare(lines_[1]->value() + 1.0, region.second + 1.0)) {
        return;
    }

    blockLineSignal_ = true;
    lines_[0]->setValue(region.first);
    lines_[1]->setValue(region.second);
    lineMoved(0);
    blockLineSignal_ = false;
    lineMoved(1);
    lineMoveFinished();
}

void LinearRegionItem::setBounds(std::optional<InfiniteLine::Bounds> bounds)
{
    for (auto* regionLine : lines_) {
        regionLine->setBounds(bounds);
    }
}

void LinearRegionItem::setBounds(std::pair<qreal, qreal> bounds)
{
    setBounds(InfiniteLine::Bounds{bounds.first, bounds.second});
}

void LinearRegionItem::setMovable(bool movable)
{
    movable_ = movable;
    for (auto* regionLine : lines_) {
        regionLine->setMovable(movable_);
    }
    setAcceptHoverEvents(movable_);
}

bool LinearRegionItem::movable() const noexcept
{
    return movable_;
}

void LinearRegionItem::setSpan(qreal min, qreal max)
{
    const std::pair<qreal, qreal> next{min, max};
    if (span_ == next) {
        return;
    }
    prepareGeometryChange();
    span_ = next;
    for (auto* regionLine : lines_) {
        regionLine->setSpan(min, max);
    }
    update();
}

std::pair<qreal, qreal> LinearRegionItem::span() const noexcept
{
    return span_;
}

void LinearRegionItem::setSwapMode(SwapMode mode)
{
    swapMode_ = mode;
}

LinearRegionItem::SwapMode LinearRegionItem::swapMode() const noexcept
{
    return swapMode_;
}

void LinearRegionItem::setBrush(const QBrush& brush)
{
    brush_ = brush;
    if (!mouseHovering_) {
        currentBrush_ = brush_;
    }
    update();
}

QBrush LinearRegionItem::brush() const
{
    return brush_;
}

void LinearRegionItem::setHoverBrush(const QBrush& brush)
{
    hoverBrush_ = brush;
    if (mouseHovering_) {
        currentBrush_ = hoverBrush_;
    }
    update();
}

QBrush LinearRegionItem::hoverBrush() const
{
    return hoverBrush_;
}

InfiniteLine* LinearRegionItem::line(int index) const
{
    if (index < 0 || index >= static_cast<int>(lines_.size())) {
        throw std::out_of_range("LinearRegionItem::line index out of range");
    }
    return lines_[static_cast<std::size_t>(index)];
}

LinearRegionItem::Orientation LinearRegionItem::orientation() const noexcept
{
    return orientation_;
}

void LinearRegionItem::lineMoved(int index)
{
    if (blockLineSignal_) {
        return;
    }

    if (lines_[0]->value() > lines_[1]->value()) {
        if (swapMode_ == SwapMode::Block) {
            lines_[index]->setValue(lines_[1 - index]->value());
        } else if (swapMode_ == SwapMode::Push) {
            lines_[1 - index]->setValue(lines_[index]->value());
        }
    }

    prepareGeometryChange();
    update();
    emit sigRegionChanged(this);
}

void LinearRegionItem::lineMoveFinished()
{
    emit sigRegionChangeFinished(this);
}

void LinearRegionItem::setMouseHover(bool hover)
{
    if (mouseHovering_ == hover) {
        return;
    }
    mouseHovering_ = hover;
    currentBrush_ = mouseHovering_ ? hoverBrush_ : brush_;
    update();
}

bool LinearRegionItem::mouseHovering() const noexcept
{
    return mouseHovering_;
}

std::optional<QRectF> LinearRegionItem::autoRangeBoundsRect() const
{
    return std::nullopt;
}

QRectF LinearRegionItem::boundingRect() const
{
    QRectF view = localViewRect(this, findViewBox(parentItem()));

    const auto region = getRegion();
    if (orientation_ == Orientation::Vertical) {
        view.setLeft(region.first);
        view.setRight(region.second);
        const qreal top = view.top();
        const qreal height = view.height();
        view.setTop(top + height * span_.first);
        view.setBottom(top + height * span_.second);
    } else {
        view.setTop(region.first);
        view.setBottom(region.second);
        const qreal left = view.left();
        const qreal width = view.width();
        view.setLeft(left + width * span_.first);
        view.setRight(left + width * span_.second);
    }
    return view.normalized();
}

void LinearRegionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(Qt::NoPen);
    painter->setBrush(currentBrush_);
    painter->drawRect(boundingRect());
}

void LinearRegionItem::hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event != nullptr && movable_ && !event->isExit() && event->acceptDrags(Qt::LeftButton, this)) {
        setMouseHover(true);
    } else {
        setMouseHover(false);
    }
}

void LinearRegionItem::mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (event != nullptr && moving_ && event->button() == Qt::RightButton) {
        event->accept(this);
        blockLineSignal_ = true;
        for (std::size_t i = 0; i < lines_.size(); ++i) {
            lines_[i]->setPos(startPositions_[i]);
        }
        blockLineSignal_ = false;
        moving_ = false;
        prepareGeometryChange();
        emit sigRegionChanged(this);
        emit sigRegionChangeFinished(this);
    }
}

void LinearRegionItem::mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event)
{
    if (event == nullptr || !movable_ || event->button() != Qt::LeftButton) {
        return;
    }
    event->accept(this);

    if (event->isStart()) {
        const QPointF down = event->buttonDownPos();
        for (std::size_t i = 0; i < lines_.size(); ++i) {
            cursorOffsets_[i] = lines_[i]->pos() - down;
            startPositions_[i] = lines_[i]->pos();
        }
        moving_ = true;
    }

    if (!moving_) {
        return;
    }

    blockLineSignal_ = true;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        lines_[i]->setPos(cursorOffsets_[i] + event->pos());
    }
    blockLineSignal_ = false;
    prepareGeometryChange();
    update();

    if (event->isFinish()) {
        moving_ = false;
        emit sigRegionChangeFinished(this);
    } else {
        emit sigRegionChanged(this);
    }
}

QPointF LinearRegionItem::valuePoint(qreal value) const
{
    return orientation_ == Orientation::Vertical ? QPointF(value, 0.0) : QPointF(0.0, value);
}

qreal LinearRegionItem::axisDelta(const QPointF& point) const
{
    return orientation_ == Orientation::Vertical ? point.x() : point.y();
}

TargetItem::TargetItem(const QPointF& pos, qreal size, bool movable, QGraphicsItem* parent)
    : GraphicsObject(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setPen(QPen(QColor(255, 255, 0), 1.0));
    setHoverPen(QPen(QColor(255, 0, 255), 1.0));
    setBrush(QBrush(QColor(0, 0, 255, 50)));
    setHoverBrush(QBrush(QColor(0, 255, 255, 100)));
    setSymbol(QStringLiteral("crosshair"));
    setSize(size);
    setMovable(movable);
    setPos(pos);
}

TargetItem::~TargetItem() = default;

void TargetItem::setMovable(bool movable)
{
    movable_ = movable;
    setAcceptHoverEvents(movable_);
}

bool TargetItem::movable() const noexcept
{
    return movable_;
}

void TargetItem::setPos(const QPointF& pos)
{
    if (qFuzzyCompare(position_.x() + 1.0, pos.x() + 1.0) && qFuzzyCompare(position_.y() + 1.0, pos.y() + 1.0)) {
        return;
    }
    position_ = pos;
    GraphicsObject::setPos(position_);
    emit sigPositionChanged(this);
}

void TargetItem::setPos(qreal x, qreal y)
{
    setPos(QPointF(x, y));
}

QPointF TargetItem::pos() const
{
    return position_;
}

void TargetItem::setSize(qreal size)
{
    if (qFuzzyCompare(size_ + 1.0, size + 1.0)) {
        return;
    }
    prepareGeometryChange();
    size_ = size;
    update();
}

qreal TargetItem::size() const noexcept
{
    return size_;
}

void TargetItem::setSymbol(const QString& symbol)
{
    if (symbol != QStringLiteral("crosshair")) {
        throw std::invalid_argument("TargetItem currently supports the crosshair symbol");
    }
    prepareGeometryChange();
    symbol_ = symbol;
    unitPath_ = crosshairPath();
    update();
}

void TargetItem::setSymbol(const QPainterPath& path)
{
    prepareGeometryChange();
    symbol_ = QStringLiteral("custom");
    unitPath_ = path;
    update();
}

QString TargetItem::symbol() const
{
    return symbol_;
}

void TargetItem::setPen(const QPen& pen)
{
    pen_ = pen;
    if (!mouseHovering_) {
        currentPen_ = pen_;
    }
    update();
}

QPen TargetItem::pen() const
{
    return pen_;
}

void TargetItem::setHoverPen(const QPen& pen)
{
    hoverPen_ = pen;
    if (mouseHovering_) {
        currentPen_ = hoverPen_;
    }
    update();
}

QPen TargetItem::hoverPen() const
{
    return hoverPen_;
}

void TargetItem::setBrush(const QBrush& brush)
{
    brush_ = brush;
    if (!mouseHovering_) {
        currentBrush_ = brush_;
    }
    update();
}

QBrush TargetItem::brush() const
{
    return brush_;
}

void TargetItem::setHoverBrush(const QBrush& brush)
{
    hoverBrush_ = brush;
    if (mouseHovering_) {
        currentBrush_ = hoverBrush_;
    }
    update();
}

QBrush TargetItem::hoverBrush() const
{
    return hoverBrush_;
}

void TargetItem::setMouseHover(bool hover)
{
    if (mouseHovering_ == hover) {
        return;
    }
    mouseHovering_ = hover;
    currentPen_ = mouseHovering_ ? hoverPen_ : pen_;
    currentBrush_ = mouseHovering_ ? hoverBrush_ : brush_;
    update();
}

bool TargetItem::mouseHovering() const noexcept
{
    return mouseHovering_;
}

std::optional<QRectF> TargetItem::autoRangeBoundsRect() const
{
    return std::nullopt;
}

QPainterPath TargetItem::shape() const
{
    return scaledPath();
}

QRectF TargetItem::boundingRect() const
{
    const qreal pad = std::max<qreal>(1.0, currentPen_.widthF()) * 0.5 + 1.0;
    return scaledPath().boundingRect().adjusted(-pad, -pad, pad, pad);
}

void TargetItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(currentPen_);
    painter->setBrush(currentBrush_);
    painter->drawPath(scaledPath());
}

void TargetItem::hoverEvent(cppqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event != nullptr && movable_ && !event->isExit() && event->acceptDrags(Qt::LeftButton, this)) {
        setMouseHover(true);
    } else {
        setMouseHover(false);
    }
}

void TargetItem::mouseClickEvent(cppqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (event != nullptr && moving_ && event->button() == Qt::RightButton) {
        event->accept(this);
        moving_ = false;
        emit sigPositionChanged(this);
        emit sigPositionChangeFinished(this);
    }
}

void TargetItem::mouseDragEvent(cppqtgraph::GraphicsScene::MouseDragEvent* event)
{
    if (event == nullptr || !movable_ || event->button() != Qt::LeftButton) {
        return;
    }
    event->accept(this);

    if (event->isStart()) {
        symbolOffset_ = position_ - mapToParent(event->buttonDownPos());
        moving_ = true;
    }

    if (!moving_) {
        return;
    }

    setPos(symbolOffset_ + mapToParent(event->pos()));

    if (event->isFinish()) {
        moving_ = false;
        emit sigPositionChangeFinished(this);
    }
}

QPainterPath TargetItem::scaledPath() const
{
    QTransform transform;
    transform.scale(size_, size_);
    return transform.map(unitPath_);
}

} // namespace cppqtgraph::graphicsItems
