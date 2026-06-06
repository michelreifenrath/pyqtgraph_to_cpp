// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GridItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/graphicsItems/GridItem.hpp"

#include "../../../include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp"

#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pyqtgraph::graphicsItems {
namespace {

QPen defaultGridPen()
{
    QPen pen(QColor(200, 200, 200));
    pen.setCosmetic(true);
    return pen;
}

bool finite(qreal value)
{
    return std::isfinite(static_cast<double>(value));
}

qreal niceAutomaticSpacing(qreal span, qreal targetLines)
{
    const qreal raw = std::abs(span / targetLines);
    if (!finite(raw) || raw <= 0.0) {
        return 1.0;
    }
    return std::pow(10.0, std::floor(std::log10(raw) + 0.5));
}

qreal clippedAlpha(qreal pixelsPerLine)
{
    return std::clamp(5.0 * (pixelsPerLine - 3.0), 0.0, 50.0);
}

std::optional<qreal> spacingAt(const std::vector<std::optional<qreal>>& spacings, int level)
{
    if (level < 0 || static_cast<std::size_t>(level) >= spacings.size()) {
        return std::nullopt;
    }
    return spacings[static_cast<std::size_t>(level)];
}

} // namespace

GridItem::GridItem(QGraphicsItem* parent)
    : GridItem(defaultGridPen(), defaultGridPen(), parent)
{
}

GridItem::GridItem(const QPen& pen, std::optional<QPen> textPen, QGraphicsItem* parent)
    : GraphicsObject(parent)
    , pen_(pen)
    , textPen_(std::move(textPen))
{
    pen_.setCosmetic(true);
    if (textPen_.has_value()) {
        textPen_->setCosmetic(true);
    }
}

GridItem::~GridItem() = default;

QPen GridItem::pen() const
{
    return pen_;
}

void GridItem::setPen(const QPen& pen)
{
    pen_ = pen;
    pen_.setCosmetic(true);
    update();
}

std::optional<QPen> GridItem::textPen() const
{
    return textPen_;
}

void GridItem::setTextPen(std::optional<QPen> pen)
{
    textPen_ = std::move(pen);
    if (textPen_.has_value()) {
        textPen_->setCosmetic(true);
    }
    update();
}

GridItem::TickSpacing GridItem::tickSpacing() const
{
    return tickSpacing_;
}

void GridItem::setTickSpacing(std::optional<std::vector<std::optional<qreal>>> x,
                              std::optional<std::vector<std::optional<qreal>>> y)
{
    if (x.has_value()) {
        tickSpacing_.x = std::move(*x);
    }
    if (y.has_value()) {
        tickSpacing_.y = std::move(*y);
    }
    if (tickSpacing_.x.empty() && tickSpacing_.y.empty()) {
        throw std::invalid_argument("GridItem requires at least one tick-spacing level");
    }
    update();
}

void GridItem::setTickSpacing(const TickSpacing& spacing)
{
    if (spacing.x.empty() && spacing.y.empty()) {
        throw std::invalid_argument("GridItem requires at least one tick-spacing level");
    }
    tickSpacing_ = spacing;
    update();
}

QRectF GridItem::boundingRect() const
{
    if (const ViewBox* vb = viewBox(); vb != nullptr) {
        return vb->viewRect().normalized();
    }
    return QRectF{};
}

void GridItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }
    const ViewBox* vb = viewBox();
    if (vb == nullptr) {
        return;
    }
    const QRectF view = vb->viewRect().normalized();
    if (!view.isValid() || view.isEmpty()) {
        return;
    }
    const QSizeF pixels = const_cast<ViewBox*>(vb)->viewPixelSize();
    const std::array<qreal, 2> unitsPerPixel{{pixels.width() > 0.0 ? pixels.width() : view.width() / 800.0,
                                               pixels.height() > 0.0 ? pixels.height() : view.height() / 600.0}};
    const std::array<qreal, 2> dimensions{{std::max<qreal>(1.0, view.width() / unitsPerPixel[0]),
                                            std::max<qreal>(1.0, view.height() / unitsPerPixel[1])}};
    const std::array<qreal, 2> lower{{view.left(), view.top()}};
    const std::array<qreal, 2> upper{{view.right(), view.bottom()}};

    struct TextLabel {
        QPointF position;
        QString text;
        QColor color;
    };
    std::vector<TextLabel> labels;

    for (int level = gridDepth() - 1; level >= 0; --level) {
        const qreal targetLines = std::pow(10.0, static_cast<qreal>(level));
        std::array<qreal, 2> spacing{{niceAutomaticSpacing(view.width(), targetLines), niceAutomaticSpacing(view.height(), targetLines)}};
        if (auto explicitSpacing = spacingAt(tickSpacing_.x, level); explicitSpacing.has_value() && *explicitSpacing > 0.0) {
            spacing[0] = *explicitSpacing;
        }
        if (auto explicitSpacing = spacingAt(tickSpacing_.y, level); explicitSpacing.has_value() && *explicitSpacing > 0.0) {
            spacing[1] = *explicitSpacing;
        }

        for (int axis = 0; axis < 2; ++axis) {
            const auto& axisSpacing = axis == 0 ? tickSpacing_.x : tickSpacing_.y;
            if (static_cast<std::size_t>(level) >= axisSpacing.size() || spacing[axis] <= 0.0 || !finite(spacing[axis])) {
                continue;
            }
            const int crossAxis = axis == 0 ? 1 : 0;
            const qreal first = std::floor(lower[axis] / spacing[axis]) * spacing[axis];
            const qreal last = std::ceil(upper[axis] / spacing[axis]) * spacing[axis];
            const qreal intervals = std::max<qreal>(1.0, (last - first) / spacing[axis] + 0.5);
            const qreal alpha = clippedAlpha(dimensions[axis] / intervals);
            if (alpha <= 0.0) {
                continue;
            }
            QPen linePen = pen_;
            QColor lineColor = linePen.color();
            lineColor.setAlpha(std::clamp(static_cast<int>(std::lround(alpha)), 0, 255));
            linePen.setColor(lineColor);
            linePen.setCosmetic(true);
            painter->setPen(linePen);

            const int maximumTicks = 2048;
            for (int tickIndex = 0; tickIndex < maximumTicks; ++tickIndex) {
                const qreal value = first + static_cast<qreal>(tickIndex) * spacing[axis];
                if (value > last + spacing[axis] * 1.0e-9) {
                    break;
                }
                if (value < lower[axis] - spacing[axis] * 1.0e-9 || value > upper[axis] + spacing[axis] * 1.0e-9) {
                    continue;
                }
                std::array<qreal, 2> p1{{0.0, 0.0}};
                std::array<qreal, 2> p2{{0.0, 0.0}};
                p1[axis] = value;
                p2[axis] = value;
                p1[crossAxis] = lower[crossAxis];
                p2[crossAxis] = upper[crossAxis];
                painter->drawLine(QPointF(p1[0], p1[1]), QPointF(p2[0], p2[1]));
                if (level < 2 && textPen_.has_value()) {
                    QColor textColor = textPen_->color();
                    textColor.setAlpha(std::clamp(static_cast<int>(std::lround(alpha * 2.0)), 0, 255));
                    const QPointF textPosition = axis == 0
                        ? QPointF(value + unitsPerPixel[0], lower[1] + unitsPerPixel[1] * 8.0)
                        : QPointF(lower[0] + unitsPerPixel[0] * 3.0, value + unitsPerPixel[1]);
                    labels.push_back(TextLabel{textPosition, QString::number(value, 'g', 6), textColor});
                }
            }
        }
    }

    if (!labels.empty()) {
        const QTransform itemToDevice = painter->worldTransform();
        painter->save();
        painter->resetTransform();
        QPen textPen = *textPen_;
        for (const TextLabel& label : labels) {
            textPen.setColor(label.color);
            painter->setPen(textPen);
            painter->drawText(itemToDevice.map(label.position) + QPointF(0.5, 0.5), label.text);
        }
        painter->restore();
    }
}

ViewBox* GridItem::viewBox() const
{
    for (QGraphicsItem* item = parentItem(); item != nullptr; item = item->parentItem()) {
        if (auto* vb = dynamic_cast<ViewBox*>(item)) {
            return vb;
        }
    }
    return nullptr;
}

int GridItem::gridDepth() const
{
    return static_cast<int>(std::max(tickSpacing_.x.size(), tickSpacing_.y.size()));
}

} // namespace pyqtgraph::graphicsItems
