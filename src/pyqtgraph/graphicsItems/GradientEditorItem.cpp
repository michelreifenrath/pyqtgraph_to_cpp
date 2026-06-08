// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GradientEditorItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GradientEditorItem.hpp"

#include <QtCore/QMetaObject>
#include <QtCore/QRectF>
#include <QtGui/QBrush>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsRectItem>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pyqtgraph::graphicsItems {

namespace {

constexpr qreal kSqrt3 = 1.7320508075688772;

QColor interpolateRgb(const QColor& first, const QColor& second, qreal fraction)
{
    const qreal inverse = 1.0 - fraction;
    return QColor(static_cast<int>(first.red() * inverse + second.red() * fraction),
                  static_cast<int>(first.green() * inverse + second.green() * fraction),
                  static_cast<int>(first.blue() * inverse + second.blue() * fraction),
                  static_cast<int>(first.alpha() * inverse + second.alpha() * fraction));
}

} // namespace

Tick::Tick(const QPointF& pos,
           const QColor& color,
           bool movable,
           qreal scale,
           const QPen& pen,
           bool removeAllowed,
           TickSliderItem* sliderItem)
    : GraphicsWidget(sliderItem)
    , sliderItem_(sliderItem)
    , color_(color)
    , pen_(pen)
    , hoverPen_(QColor(255, 255, 0))
    , currentPen_(pen)
    , scale_(scale)
    , movable_(movable)
    , removeAllowed_(removeAllowed)
{
    path_.moveTo(0.0, 0.0);
    path_.lineTo(QPointF(-scale_ / kSqrt3, scale_));
    path_.lineTo(QPointF(scale_ / kSqrt3, scale_));
    path_.closeSubpath();
    setPos(pos);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setZValue(movable_ ? 1.0 : 0.0);
}

void Tick::setColor(const QColor& color)
{
    color_ = color;
    update();
}

QRectF Tick::boundingRect() const
{
    return path_.boundingRect();
}

QPainterPath Tick::shape() const
{
    return path_;
}

void Tick::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillPath(path_, color_);
    painter->setPen(currentPen_);
    painter->drawPath(path_);
}

void Tick::hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event != nullptr && !event->isExit() && movable_ && event->acceptDrags(Qt::LeftButton, this)) {
        event->acceptClicks(Qt::LeftButton, this);
        event->acceptClicks(Qt::RightButton, this);
        currentPen_ = hoverPen_;
    } else {
        currentPen_ = pen_;
    }
    update();
}

void Tick::mouseClickEvent(pyqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (event == nullptr) {
        return;
    }
    event->accept(this);
    if (event->button() == Qt::RightButton && moving_) {
        setPos(startPosition_);
        moving_ = false;
        emit sigMoving(this, startPosition_);
        emit sigMoved(this);
        return;
    }
    emit sigClicked(this, event);
}

void Tick::mouseDragEvent(pyqtgraph::GraphicsScene::MouseDragEvent* event)
{
    if (event == nullptr || !movable_ || event->button() != Qt::LeftButton) {
        return;
    }

    if (event->isStart()) {
        moving_ = true;
        cursorOffset_ = pos() - mapToParent(QPointF(event->buttonDownPos().x(), event->buttonDownPos().y()));
        startPosition_ = pos();
    }
    event->accept(this);

    if (!moving_) {
        return;
    }

    QPointF newPos = cursorOffset_ + mapToParent(QPointF(event->pos().x(), event->pos().y()));
    newPos.setY(pos().y());
    setPos(newPos);
    emit sigMoving(this, newPos);
    if (event->isFinish()) {
        moving_ = false;
        emit sigMoved(this);
    }
}

TickSliderItem::TickSliderItem(const QString& orientation, bool allowAdd, bool allowRemove, QGraphicsItem* parent)
    : GraphicsWidget(parent)
    , orientation_(orientation)
    , allowAdd_(allowAdd)
    , allowRemove_(allowRemove)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setFixedHeight(maxDim_);
    setMinimumWidth(40.0);
}

Tick* TickSliderItem::addTick(double fraction, const QColor& color, bool movable, bool finish)
{
    QColor tickColor = color;
    if (!tickColor.isValid()) {
        tickColor = Qt::white;
    }

    const qreal x = fraction * length_;
    auto tick = std::make_unique<Tick>(QPointF(x, 0.0), tickColor, movable, tickSize_, tickPen_, allowRemove_, this);
    Tick* tickPtr = tick.get();
    ticks_.emplace_back(std::move(tick), fraction);
    tickPtr->setParentItem(this);
    connectTick(tickPtr);

    emit sigTicksChanged(this);
    if (finish) {
        emit sigTicksChangeFinished(this);
    }
    return tickPtr;
}

void TickSliderItem::removeTick(Tick* tick, bool finish)
{
    if (tick == nullptr) {
        return;
    }

    const auto iterator = std::find_if(ticks_.begin(), ticks_.end(), [tick](const auto& entry) {
        return entry.first.get() == tick;
    });
    if (iterator == ticks_.end()) {
        return;
    }

    std::unique_ptr<Tick> removed = std::move(iterator->first);
    ticks_.erase(iterator);

    removed->setParentItem(nullptr);
    removed->hide();
    pendingRemovedTicks_.push_back(std::move(removed));
    schedulePendingRemovalFlush();

    emit sigTicksChanged(this);
    if (finish) {
        emit sigTicksChangeFinished(this);
    }
}

void TickSliderItem::schedulePendingRemovalFlush()
{
    if (pendingRemovalFlushScheduled_) {
        return;
    }
    pendingRemovalFlushScheduled_ = true;
    QMetaObject::invokeMethod(
        this,
        [this]() {
            pendingRemovalFlushScheduled_ = false;
            flushPendingRemovedTicks();
        },
        Qt::QueuedConnection);
}

void TickSliderItem::flushPendingRemovedTicks()
{
    pendingRemovedTicks_.clear();
}

std::vector<std::pair<Tick*, double>> TickSliderItem::listTicks() const
{
    std::vector<std::pair<Tick*, double>> entries;
    entries.reserve(ticks_.size());
    for (const auto& entry : ticks_) {
        entries.emplace_back(entry.first.get(), entry.second);
    }
    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second < rhs.second;
    });
    return entries;
}

double TickSliderItem::tickValue(Tick* tick) const
{
    for (const auto& entry : ticks_) {
        if (entry.first.get() == tick) {
            return entry.second;
        }
    }
    return 0.0;
}

Tick* TickSliderItem::tickAt(std::size_t index) const
{
    const auto sorted = listTicks();
    if (index >= sorted.size()) {
        return nullptr;
    }
    return sorted[index].first;
}

void TickSliderItem::setTickValue(Tick* tick, double value)
{
    value = std::clamp(value, 0.0, 1.0);
    const qreal x = value * length_;
    tick->setPos(QPointF(x, tick->pos().y()));
    setTickFraction(tick, value);
    update();
    emit sigTicksChanged(this);
    emit sigTicksChangeFinished(this);
}

void TickSliderItem::setLength(qreal newLen)
{
    for (auto& entry : ticks_) {
        entry.first->setPos(QPointF(entry.second * newLen, entry.first->pos().y()));
    }
    length_ = newLen;
}

void TickSliderItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void TickSliderItem::hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event)
{
    if (event != nullptr && !event->isExit()) {
        event->acceptClicks(Qt::LeftButton, this);
        event->acceptClicks(Qt::RightButton, this);
    }
}

void TickSliderItem::mouseClickEvent(pyqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (event == nullptr) {
        return;
    }

    if (event->button() == Qt::LeftButton && allowAdd_) {
        QPointF clickPos(event->pos().x(), event->pos().y());
        if (clickPos.x() < 0.0 || clickPos.x() > length_ || clickPos.y() < 0.0 || clickPos.y() > tickSize_) {
            return;
        }
        clickPos.setX(std::clamp(clickPos.x(), 0.0, length_));
        addTick(clickPos.x() / length_, QColor(), true, true);
        event->accept(this);
        return;
    }

    if (event->button() == Qt::RightButton) {
        event->accept(this);
    }
}

void TickSliderItem::tickMoved(Tick* tick, const QPointF& pos)
{
    const qreal newX = std::clamp(pos.x(), 0.0, length_);
    tick->setPos(QPointF(newX, tick->pos().y()));
    setTickFraction(tick, newX / length_);
    emit sigTicksChanged(this);
}

void TickSliderItem::tickMoveFinished(Tick* tick)
{
    Q_UNUSED(tick);
    emit sigTicksChangeFinished(this);
}

void TickSliderItem::tickClicked(Tick* tick, pyqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (event != nullptr && event->button() == Qt::RightButton && tick != nullptr && allowRemove_ && tick->removeAllowed()
        && ticks_.size() >= 3) {
        removeTick(tick, true);
        event->accept(this);
    }
}

Tick* TickSliderItem::getTick(std::variant<Tick*, int> tick) const
{
    if (std::holds_alternative<Tick*>(tick)) {
        return std::get<Tick*>(tick);
    }
    return tickAt(static_cast<std::size_t>(std::get<int>(tick)));
}

void TickSliderItem::connectTick(Tick* tick)
{
    QObject::connect(tick, &Tick::sigMoving, this, [this](Tick* movedTick, const QPointF& pos) {
        tickMoved(movedTick, pos);
    });
    QObject::connect(tick, &Tick::sigMoved, this, [this](Tick* movedTick) { tickMoveFinished(movedTick); });
    QObject::connect(tick, &Tick::sigClicked, this, [this](Tick* clickedTick, pyqtgraph::GraphicsScene::MouseClickEvent* event) {
        tickClicked(clickedTick, event);
    });
}

void TickSliderItem::setTickFraction(Tick* tick, double fraction) noexcept
{
    for (auto& entry : ticks_) {
        if (entry.first.get() == tick) {
            entry.second = fraction;
            return;
        }
    }
}

GradientEditorItem::GradientEditorItem(const QString& orientation, bool allowAdd, bool allowRemove, QGraphicsItem* parent)
    : TickSliderItem(orientation, allowAdd, allowRemove, parent)
{
    backgroundRect_ = new QGraphicsRectItem(QRectF(0.0, -rectSize_, length_, rectSize_), this);
    backgroundRect_->setBrush(QBrush(Qt::DiagCrossPattern));
    gradRect_ = new QGraphicsRectItem(QRectF(1.0, -rectSize_, length_, rectSize_), this);

    setFixedHeight(rectSize_ + tickSize());
    initializeDefaultTicks();

    QObject::connect(this, &TickSliderItem::sigTicksChanged, this, [this](TickSliderItem*) { updateGradient(); });
    QObject::connect(this, &TickSliderItem::sigTicksChangeFinished, this, [this](TickSliderItem*) {
        emit sigGradientChangeFinished(this);
    });
}

void GradientEditorItem::setColorMode(const QString& mode)
{
    if (mode != QStringLiteral("rgb") && mode != QStringLiteral("hsv")) {
        throw std::invalid_argument("Unknown color mode");
    }
    colorMode_ = mode;
    updateGradient();
    emit sigGradientChangeFinished(this);
}

QColor GradientEditorItem::getColor(double fraction, bool toQColor) const
{
    const auto ticks = listTicks();
    if (ticks.empty()) {
        return toQColor ? QColor() : QColor();
    }
    if (fraction <= ticks.front().second) {
        return ticks.front().first->color();
    }
    if (fraction >= ticks.back().second) {
        return ticks.back().first->color();
    }

    for (std::size_t index = 1; index < ticks.size(); ++index) {
        const double x1 = ticks[index - 1].second;
        const double x2 = ticks[index].second;
        if (x1 <= fraction && x2 >= fraction) {
            const double dx = x2 - x1;
            const double blend = dx == 0.0 ? 0.0 : (fraction - x1) / dx;
            if (colorMode_ == QStringLiteral("rgb")) {
                return interpolateRgb(ticks[index - 1].first->color(), ticks[index].first->color(), blend);
            }
            break;
        }
    }
    return ticks.back().first->color();
}

ColorMap GradientEditorItem::colorMap() const
{
    if (colorMode_ == QStringLiteral("hsv")) {
        throw std::runtime_error("hsv colormaps not yet supported");
    }
    std::vector<double> positions;
    std::vector<QColor> colors;
    for (const auto& [tick, fraction] : listTicks()) {
        positions.push_back(fraction);
        colors.push_back(tick->color());
    }
    return ColorMap(std::move(positions), std::move(colors));
}

QLinearGradient GradientEditorItem::getGradient() const
{
    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(length(), 0.0));
    QGradientStops stops;
    for (const auto& [tick, fraction] : listTicks()) {
        stops.append(qMakePair(fraction, tick->color()));
    }
    gradient.setStops(stops);
    return gradient;
}

GradientEditorState GradientEditorItem::saveState() const
{
    GradientEditorState state;
    state.mode = colorMode_;
    for (const auto& [tick, fraction] : listTicks()) {
        state.ticks.emplace_back(fraction, tick->color());
    }
    state.ticksVisible = tickCount() == 0 ? true : tickAt(0)->isVisible();
    return state;
}

void GradientEditorItem::restoreState(const GradientEditorState& state)
{
    const bool blocked = blockSignals(true);
    colorMode_ = state.mode;
    const auto existingTicks = listTicks();
    for (const auto& [tick, fraction] : existingTicks) {
        Q_UNUSED(fraction);
        removeTick(tick, false);
    }
    for (const auto& [fraction, color] : state.ticks) {
        addTick(fraction, color, true, false);
    }
    showTicks(state.ticksVisible);
    blockSignals(blocked);
    updateGradient();
    emit sigTicksChanged(this);
    emit sigGradientChangeFinished(this);
}

void GradientEditorItem::showTicks(bool show)
{
    if (show) {
        if (allowAddBackup_) {
            setAllowAdd(true);
        }
        for (const auto& [tick, fraction] : listTicks()) {
            Q_UNUSED(fraction);
            tick->show();
        }
    } else {
        allowAddBackup_ = allowAdd();
        setAllowAdd(false);
        for (const auto& [tick, fraction] : listTicks()) {
            Q_UNUSED(fraction);
            tick->hide();
        }
    }
}

Tick* GradientEditorItem::addTick(double fraction, const QColor& color, bool movable, bool finish)
{
    QColor tickColor = color;
    if (!tickColor.isValid()) {
        tickColor = getColor(fraction);
    }
    Tick* tick = TickSliderItem::addTick(fraction, tickColor, movable, finish);
    if (tick != nullptr) {
        tick->setColor(tickColor);
    }
    updateGradient();
    return tick;
}

void GradientEditorItem::setLength(qreal newLen)
{
    TickSliderItem::setLength(newLen);
    if (backgroundRect_ != nullptr) {
        backgroundRect_->setRect(1.0, -rectSize_, newLen, rectSize_);
    }
    if (gradRect_ != nullptr) {
        gradRect_->setRect(1.0, -rectSize_, newLen, rectSize_);
    }
    updateGradient();
    emit sigTicksChanged(this);
}

void GradientEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    TickSliderItem::paint(painter, option, widget);
}

void GradientEditorItem::tickClicked(Tick* tick, pyqtgraph::GraphicsScene::MouseClickEvent* event)
{
    if (event == nullptr || tick == nullptr) {
        return;
    }
    if (event->button() == Qt::RightButton && allowRemove() && tick->removeAllowed() && tickCount() >= 3) {
        removeTick(tick, true);
        event->accept(this);
        updateGradient();
        emit sigGradientChangeFinished(this);
    }
}

void GradientEditorItem::updateGradient()
{
    if (gradRect_ != nullptr) {
        gradRect_->setBrush(QBrush(getGradient()));
    }
    emit sigGradientChanged(this);
}

void GradientEditorItem::initializeDefaultTicks()
{
    while (tickCount() > 0) {
        removeTick(tickAt(0), false);
    }
    addTick(0.0, QColor(0, 0, 0), true, false);
    addTick(1.0, QColor(255, 0, 0), true, false);
    setColorMode(QStringLiteral("rgb"));
    updateGradient();
}

} // namespace pyqtgraph::graphicsItems
