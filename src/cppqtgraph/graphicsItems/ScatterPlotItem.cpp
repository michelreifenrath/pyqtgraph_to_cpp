// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ScatterPlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/cppqtgraph/graphicsItems/ScatterPlotItem.hpp"

#include "../../../include/cppqtgraph/functions.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmap>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace cppqtgraph::graphicsItems {
namespace {

constexpr qreal spotDiagonalPadding = 0.7072;

QPen defaultScatterPen()
{
    QPen pen(QColor(255, 255, 255), 1.0);
    pen.setCosmetic(true);
    return pen;
}

QBrush defaultScatterBrush()
{
    return QBrush(QColor(100, 100, 150));
}

bool isFinitePoint(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

void validateSymbolName(const QString& symbol, const char* context)
{
    if (symbol.isEmpty()) {
        throw std::invalid_argument(std::string(context) + " requires a non-empty symbol name");
    }
    const auto& symbols = cppqtgraph::symbolPaths();
    if (symbols.find(symbol) == symbols.end()) {
        throw std::invalid_argument(std::string(context) + " received unknown scatter symbol \"" + symbol.toStdString() + "\"");
    }
}

void validateSymbolSize(qreal size, const char* context)
{
    if (!std::isfinite(static_cast<double>(size)) || size < 0.0) {
        throw std::invalid_argument(std::string(context) + " requires a finite non-negative size");
    }
}

int penPixelWidth(const QPen& pen)
{
    if (pen.style() == Qt::NoPen) {
        return 1;
    }
    return std::max(static_cast<int>(std::ceil(pen.widthF())), 1);
}

QString colorKey(const QColor& color)
{
    return QString::number(color.rgba64().red()) + QStringLiteral(",") + QString::number(color.rgba64().green())
        + QStringLiteral(",") + QString::number(color.rgba64().blue()) + QStringLiteral(",")
        + QString::number(color.rgba64().alpha());
}

QString penKey(const QPen& pen)
{
    return colorKey(pen.color()) + QStringLiteral(";") + QString::number(static_cast<int>(pen.style()))
        + QStringLiteral(";") + QString::number(pen.widthF(), 'g', 17) + QStringLiteral(";")
        + QString::number(pen.isCosmetic() ? 1 : 0) + QStringLiteral(";")
        + QString::number(static_cast<int>(pen.capStyle())) + QStringLiteral(";")
        + QString::number(static_cast<int>(pen.joinStyle()));
}

QString brushKey(const QBrush& brush)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << brush;
    return QString::fromLatin1(bytes.toHex());
}

QString styleKey(const QString& symbol, qreal size, const QPen& pen, const QBrush& brush)
{
    return symbol + QStringLiteral("\x1f") + QString::number(size, 'g', 17) + QStringLiteral("\x1f") + penKey(pen)
        + QStringLiteral("\x1f") + brushKey(brush);
}

void drawSymbolPath(QPainter& painter, const QPainterPath& symbol, qreal size, const QPen& pen, const QBrush& brush)
{
    if (symbol.isEmpty()) {
        return;
    }
    painter.scale(size, size);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawPath(symbol);
}

QImage renderSymbolPath(const QPainterPath& symbol, qreal size, const QPen& pen, const QBrush& brush, qreal devicePixelRatio)
{
    validateSymbolSize(size, "renderSymbol");
    const qreal dpr = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    const int side = static_cast<int>(std::ceil(dpr * (size + penPixelWidth(pen))));
    QImage image(std::max(side, 1), std::max(side, 1), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(image.width() / dpr * 0.5, image.height() / dpr * 0.5);
    drawSymbolPath(painter, symbol, size, pen, brush);
    painter.end();
    return image;
}

} // namespace

void drawSymbol(QPainter& painter, const QString& symbol, qreal size, const QPen& pen, const QBrush& brush)
{
    drawSymbolPath(painter, cppqtgraph::symbolPath(symbol), size, pen, brush);
}

void drawSymbol(QPainter& painter, const QPainterPath& symbol, qreal size, const QPen& pen, const QBrush& brush)
{
    drawSymbolPath(painter, symbol, size, pen, brush);
}

QImage renderSymbol(const QString& symbol, qreal size, const QPen& pen, const QBrush& brush, qreal devicePixelRatio)
{
    return renderSymbolPath(cppqtgraph::symbolPath(symbol), size, pen, brush, devicePixelRatio);
}

QImage renderSymbol(const QPainterPath& symbol, qreal size, const QPen& pen, const QBrush& brush, qreal devicePixelRatio)
{
    return renderSymbolPath(symbol, size, pen, brush, devicePixelRatio);
}

class SymbolAtlas::Private {
public:
    void clear()
    {
        coords.clear();
        image = QImage();
        pixmap = QPixmap();
        maxWidthPixels = 0;
    }

    qreal dpr = 1.0;
    QHash<QString, QRect> coords;
    QImage image;
    mutable QPixmap pixmap;
    int maxWidthPixels = 0;
};

SymbolAtlas::SymbolAtlas()
    : d_(std::make_unique<Private>())
{
}

SymbolAtlas::~SymbolAtlas() = default;
SymbolAtlas::SymbolAtlas(SymbolAtlas&&) noexcept = default;
SymbolAtlas& SymbolAtlas::operator=(SymbolAtlas&&) noexcept = default;

void SymbolAtlas::clear()
{
    d_->clear();
}

void SymbolAtlas::setDevicePixelRatio(qreal devicePixelRatio)
{
    const qreal normalized = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    if (qFuzzyCompare(d_->dpr, normalized)) {
        return;
    }
    d_->dpr = normalized;
    clear();
}

qreal SymbolAtlas::devicePixelRatio() const noexcept
{
    return d_->dpr;
}

QRect SymbolAtlas::sourceRect(const QString& symbol, qreal size, const QPen& pen, const QBrush& brush)
{
    const QString key = styleKey(symbol, size, pen, brush);
    if (const auto existing = d_->coords.constFind(key); existing != d_->coords.constEnd()) {
        return *existing;
    }

    const QImage rendered = renderSymbol(symbol, size, pen, brush, d_->dpr);
    const int oldWidth = d_->image.isNull() ? 0 : d_->image.width();
    const int oldHeight = d_->image.isNull() ? 0 : d_->image.height();
    const int newWidth = oldWidth + rendered.width();
    const int newHeight = std::max(oldHeight, rendered.height());

    QImage combined(newWidth, newHeight, QImage::Format_ARGB32_Premultiplied);
    combined.setDevicePixelRatio(d_->dpr);
    combined.fill(Qt::transparent);
    QPainter painter(&combined);
    if (!d_->image.isNull()) {
        painter.drawImage(QPoint(0, 0), d_->image);
    }
    painter.drawImage(QPoint(oldWidth, 0), rendered);
    painter.end();

    d_->image = std::move(combined);
    d_->pixmap = QPixmap();
    d_->maxWidthPixels = std::max(d_->maxWidthPixels, rendered.width());

    const QRect rect(oldWidth, 0, rendered.width(), rendered.height());
    d_->coords.insert(key, rect);
    return rect;
}

const QPixmap& SymbolAtlas::pixmap() const
{
    if (d_->pixmap.isNull() && !d_->image.isNull()) {
        d_->pixmap = QPixmap::fromImage(d_->image);
        d_->pixmap.setDevicePixelRatio(d_->dpr);
    }
    return d_->pixmap;
}

qreal SymbolAtlas::maxWidth() const noexcept
{
    return static_cast<qreal>(d_->maxWidthPixels) / d_->dpr;
}

std::size_t SymbolAtlas::size() const noexcept
{
    return static_cast<std::size_t>(d_->coords.size());
}

SpotItem::SpotItem(std::size_t index, QPointF position, qreal size, QString symbol, QPen pen, QBrush brush, QVariant data)
    : index_(index)
    , position_(position)
    , size_(size)
    , symbol_(std::move(symbol))
    , pen_(std::move(pen))
    , brush_(std::move(brush))
    , data_(std::move(data))
{
}

std::size_t SpotItem::index() const noexcept { return index_; }
QPointF SpotItem::pos() const noexcept { return position_; }
qreal SpotItem::size() const noexcept { return size_; }
QString SpotItem::symbol() const { return symbol_; }
QPen SpotItem::pen() const { return pen_; }
QBrush SpotItem::brush() const { return brush_; }
QVariant SpotItem::data() const { return data_; }

struct ScatterPlotItem::Spot {
    qreal size = -1.0;
    QString symbol;
    QPen pen;
    bool hasPen = false;
    QBrush brush;
    bool hasBrush = false;
    bool visible = true;
    QVariant data;
    QRect sourceRect;
};

ScatterPlotItem::ScatterPlotItem(QGraphicsItem* parent)
    : GraphicsObject(parent)
    , defaultPen_(defaultScatterPen())
    , defaultBrush_(defaultScatterBrush())
{
}

ScatterPlotItem::ScatterPlotItem(std::span<const double> y, QGraphicsItem* parent)
    : ScatterPlotItem(parent)
{
    setData(y);
}

ScatterPlotItem::ScatterPlotItem(std::span<const double> x, std::span<const double> y, QGraphicsItem* parent)
    : ScatterPlotItem(parent)
{
    setData(x, y);
}

ScatterPlotItem::~ScatterPlotItem() = default;

void ScatterPlotItem::setData()
{
    clear();
}

void ScatterPlotItem::setData(std::span<const double> y)
{
    std::vector<double> x(y.size());
    std::iota(x.begin(), x.end(), 0.0);
    setData(x, y);
}

void ScatterPlotItem::setData(std::span<const double> x, std::span<const double> y)
{
    if (x.size() != y.size()) {
        throw std::invalid_argument("ScatterPlotItem::setData requires x and y to have the same length");
    }
    std::vector<double> newX(x.begin(), x.end());
    std::vector<double> newY(y.begin(), y.end());
    clear();
    addPoints(newX, newY);
}

void ScatterPlotItem::addPoints(std::span<const double> x, std::span<const double> y)
{
    if (x.size() != y.size()) {
        throw std::invalid_argument("ScatterPlotItem::addPoints requires x and y to have the same length");
    }

    std::vector<double> newX(x.begin(), x.end());
    std::vector<double> newY(y.begin(), y.end());
    if (!newX.empty()) {
        prepareGeometryChange();
    }
    const std::size_t oldSize = xData_.size();
    xData_.insert(xData_.end(), newX.begin(), newX.end());
    yData_.insert(yData_.end(), newY.begin(), newY.end());
    spots_.resize(oldSize + newX.size());
    updateSpots();
    refreshBounds();
    update();
    Q_EMIT sigPlotChanged(this);
}

void ScatterPlotItem::addPoints(std::span<const QPointF> points)
{
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(points.size());
    y.reserve(points.size());
    for (const QPointF& point : points) {
        x.push_back(point.x());
        y.push_back(point.y());
    }
    addPoints(x, y);
}

void ScatterPlotItem::clear()
{
    if (!spots_.empty() || !xData_.empty() || !yData_.empty()) {
        prepareGeometryChange();
    }
    xData_.clear();
    yData_.clear();
    spots_.clear();
    bounds_ = QRectF();
    maxSpotWidth_ = 0.0;
    maxSpotPxWidth_ = 0.0;
    update();
}

bool ScatterPlotItem::hasData() const noexcept
{
    return !xData_.empty() && !yData_.empty();
}

std::span<const double> ScatterPlotItem::xData() const noexcept
{
    return xData_;
}

std::span<const double> ScatterPlotItem::yData() const noexcept
{
    return yData_;
}

std::pair<std::span<const double>, std::span<const double>> ScatterPlotItem::getData() const noexcept
{
    return {xData_, yData_};
}

void ScatterPlotItem::setPen(const QPen& pen)
{
    defaultPen_ = pen;
    markSpotsDirty();
    updateSpots();
    refreshBounds();
    update();
}

void ScatterPlotItem::setPen(std::nullptr_t)
{
    setPen(QPen(Qt::NoPen));
}

QPen ScatterPlotItem::pen() const
{
    return defaultPen_;
}

void ScatterPlotItem::setPens(std::span<const QPen> pens)
{
    if (pens.size() != spots_.size()) {
        throw std::invalid_argument("ScatterPlotItem::setPens requires one pen per point");
    }
    prepareGeometryChange();
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        spots_[index].pen = pens[index];
        spots_[index].hasPen = true;
        spots_[index].sourceRect = QRect();
    }
    updateSpots();
    refreshBounds();
    update();
}

void ScatterPlotItem::setBrush(const QBrush& brush)
{
    defaultBrush_ = brush;
    markSpotsDirty();
    updateSpots();
    update();
}

void ScatterPlotItem::setBrush(std::nullptr_t)
{
    setBrush(QBrush(Qt::NoBrush));
}

QBrush ScatterPlotItem::brush() const
{
    return defaultBrush_;
}

void ScatterPlotItem::setBrushes(std::span<const QBrush> brushes)
{
    if (brushes.size() != spots_.size()) {
        throw std::invalid_argument("ScatterPlotItem::setBrushes requires one brush per point");
    }
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        spots_[index].brush = brushes[index];
        spots_[index].hasBrush = true;
        spots_[index].sourceRect = QRect();
    }
    updateSpots();
    update();
}

void ScatterPlotItem::setSymbol(const QString& symbol)
{
    validateSymbolName(symbol, "ScatterPlotItem::setSymbol");
    defaultSymbol_ = symbol;
    markSpotsDirty();
    updateSpots();
    update();
}

QString ScatterPlotItem::symbol() const
{
    return defaultSymbol_;
}

void ScatterPlotItem::setSymbols(std::span<const QString> symbols)
{
    if (symbols.size() != spots_.size()) {
        throw std::invalid_argument("ScatterPlotItem::setSymbols requires one symbol per point");
    }
    for (const QString& symbol : symbols) {
        validateSymbolName(symbol, "ScatterPlotItem::setSymbols");
    }
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        spots_[index].symbol = symbols[index];
        spots_[index].sourceRect = QRect();
    }
    updateSpots();
    update();
}

void ScatterPlotItem::setSize(qreal size)
{
    validateSymbolSize(size, "ScatterPlotItem::setSize");
    defaultSize_ = size;
    markSpotsDirty();
    updateSpots();
    refreshBounds();
    update();
}

qreal ScatterPlotItem::size() const noexcept
{
    return defaultSize_;
}

void ScatterPlotItem::setSizes(std::span<const qreal> sizes)
{
    if (sizes.size() != spots_.size()) {
        throw std::invalid_argument("ScatterPlotItem::setSizes requires one size per point");
    }
    for (const qreal size : sizes) {
        validateSymbolSize(size, "ScatterPlotItem::setSizes");
    }
    prepareGeometryChange();
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        spots_[index].size = sizes[index];
        spots_[index].sourceRect = QRect();
    }
    updateSpots();
    refreshBounds();
    update();
}

void ScatterPlotItem::setPointData(std::span<const QVariant> data)
{
    if (data.size() != spots_.size()) {
        throw std::invalid_argument("ScatterPlotItem::setPointData requires one data value per point");
    }
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        spots_[index].data = data[index];
    }
}

void ScatterPlotItem::setPxMode(bool enabled)
{
    if (pxMode_ == enabled) {
        return;
    }
    prepareGeometryChange();
    pxMode_ = enabled;
    markSpotsDirty();
    updateSpots();
    refreshBounds();
    update();
}

bool ScatterPlotItem::pxMode() const noexcept
{
    return pxMode_;
}

void ScatterPlotItem::setUseCache(bool enabled)
{
    if (useCache_ == enabled) {
        return;
    }
    prepareGeometryChange();
    useCache_ = enabled;
    markSpotsDirty();
    updateSpots();
    refreshBounds();
    update();
}

bool ScatterPlotItem::useCache() const noexcept
{
    return useCache_;
}

void ScatterPlotItem::setAntialias(bool enabled)
{
    if (antialias_ == enabled) {
        return;
    }
    antialias_ = enabled;
    update();
}

bool ScatterPlotItem::antialias() const noexcept
{
    return antialias_;
}

void ScatterPlotItem::setCompositionMode(QPainter::CompositionMode mode)
{
    compositionMode_ = mode;
    hasCompositionMode_ = true;
    update();
}

void ScatterPlotItem::clearCompositionMode()
{
    hasCompositionMode_ = false;
    update();
}

void ScatterPlotItem::setName(const QString& name)
{
    name_ = name;
}

QString ScatterPlotItem::name() const
{
    return name_;
}

std::vector<SpotItem> ScatterPlotItem::points() const
{
    std::vector<SpotItem> result;
    result.reserve(spots_.size());
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        result.push_back(makeSpotItem(index));
    }
    return result;
}

std::vector<SpotItem> ScatterPlotItem::pointsAt(const QPointF& position) const
{
    return pointsAt(QRectF(position, position));
}

std::vector<SpotItem> ScatterPlotItem::pointsAt(const QRectF& rect) const
{
    std::vector<SpotItem> result;
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        if (maskContains(spots_[index], rect.normalized())) {
            result.push_back(makeSpotItem(index));
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

qreal ScatterPlotItem::pixelPadding() const noexcept
{
    return maxSpotPxWidth_ * spotDiagonalPadding;
}

std::pair<qreal, qreal> ScatterPlotItem::dataBounds(int axis) const
{
    if (axis != 0 && axis != 1) {
        throw std::invalid_argument("ScatterPlotItem::dataBounds axis must be 0 or 1");
    }
    bool haveBounds = false;
    qreal minimum = 0.0;
    qreal maximum = 0.0;
    for (std::size_t index = 0; index < xData_.size() && index < yData_.size(); ++index) {
        if (!isFinitePoint(xData_[index], yData_[index])) {
            continue;
        }
        const double value = axis == 0 ? xData_[index] : yData_[index];
        if (!haveBounds) {
            minimum = value;
            maximum = value;
            haveBounds = true;
        } else {
            minimum = std::min<qreal>(minimum, value);
            maximum = std::max<qreal>(maximum, value);
        }
    }
    if (!haveBounds) {
        const qreal quietNaN = std::numeric_limits<qreal>::quiet_NaN();
        return {quietNaN, quietNaN};
    }
    if (pxMode_) {
        return {minimum, maximum};
    }
    const qreal pad = maxSpotWidth_ * spotDiagonalPadding;
    return {minimum - pad, maximum + pad};
}

std::optional<QRectF> ScatterPlotItem::autoRangeBoundsRect() const
{
    if (xData_.empty() || yData_.empty()) {
        return std::nullopt;
    }

    bool havePoint = false;
    qreal xMin = 0.0;
    qreal xMax = 0.0;
    qreal yMin = 0.0;
    qreal yMax = 0.0;
    for (std::size_t index = 0; index < xData_.size() && index < yData_.size(); ++index) {
        if (!isFinitePoint(xData_[index], yData_[index])) {
            continue;
        }
        if (!havePoint) {
            xMin = xMax = xData_[index];
            yMin = yMax = yData_[index];
            havePoint = true;
            continue;
        }
        xMin = std::min(xMin, static_cast<qreal>(xData_[index]));
        xMax = std::max(xMax, static_cast<qreal>(xData_[index]));
        yMin = std::min(yMin, static_cast<qreal>(yData_[index]));
        yMax = std::max(yMax, static_cast<qreal>(yData_[index]));
    }

    if (!havePoint) {
        return std::nullopt;
    }

    constexpr qreal kMinimumBoundsSpan = 1.0e-12;
    const qreal width = std::max(xMax - xMin, kMinimumBoundsSpan);
    const qreal height = std::max(yMax - yMin, kMinimumBoundsSpan);
    const qreal xCenter = (xMin + xMax) * 0.5;
    const qreal yCenter = (yMin + yMax) * 0.5;
    return QRectF(xCenter - width * 0.5, yCenter - height * 0.5, width, height);
}

QRectF ScatterPlotItem::boundingRect() const
{
    return bounds_;
}

void ScatterPlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    if (painter == nullptr || spots_.empty()) {
        return;
    }

    if (hasCompositionMode_) {
        painter->setCompositionMode(compositionMode_);
    }

    if (pxMode_) {
        const qreal dpr = devicePixelRatioFor(widget, painter);
        if (!qFuzzyCompare(fragmentAtlas_.devicePixelRatio(), dpr)) {
            fragmentAtlas_.setDevicePixelRatio(dpr);
            markSpotsDirty();
            updateSpots();
        }

        const QTransform world = painter->worldTransform();
        painter->resetTransform();
        painter->setRenderHint(QPainter::Antialiasing, antialias_);
        if (useCache_) {
            const QPixmap& atlas = fragmentAtlas_.pixmap();
            for (std::size_t index = 0; index < spots_.size(); ++index) {
                const Spot& spot = spots_[index];
                if (!spot.visible || !isFinitePoint(xData_[index], yData_[index]) || spot.sourceRect.isEmpty()) {
                    continue;
                }
                const QPointF point = world.map(QPointF(xData_[index], yData_[index]));
                const QRectF source(spot.sourceRect);
                const QRectF target(point.x() - source.width() / dpr * 0.5, point.y() - source.height() / dpr * 0.5,
                    source.width() / dpr, source.height() / dpr);
                painter->drawPixmap(target, atlas, source);
            }
        } else {
            for (std::size_t index = 0; index < spots_.size(); ++index) {
                const Spot& spot = spots_[index];
                if (!spot.visible || !isFinitePoint(xData_[index], yData_[index])) {
                    continue;
                }
                painter->save();
                painter->translate(world.map(QPointF(xData_[index], yData_[index])));
                drawSymbol(*painter, effectiveSymbol(spot), effectiveSize(spot), effectivePen(spot), effectiveBrush(spot));
                painter->restore();
            }
        }
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, antialias_);
    for (std::size_t index = 0; index < spots_.size(); ++index) {
        const Spot& spot = spots_[index];
        if (!spot.visible || !isFinitePoint(xData_[index], yData_[index])) {
            continue;
        }
        painter->save();
        painter->translate(xData_[index], yData_[index]);
        drawSymbol(*painter, effectiveSymbol(spot), effectiveSize(spot), effectivePen(spot), effectiveBrush(spot));
        painter->restore();
    }
}

qreal ScatterPlotItem::effectiveSize(const Spot& spot) const noexcept
{
    return spot.size >= 0.0 ? spot.size : defaultSize_;
}

QString ScatterPlotItem::effectiveSymbol(const Spot& spot) const
{
    return spot.symbol.isEmpty() ? defaultSymbol_ : spot.symbol;
}

QPen ScatterPlotItem::effectivePen(const Spot& spot) const
{
    return spot.hasPen ? spot.pen : defaultPen_;
}

QBrush ScatterPlotItem::effectiveBrush(const Spot& spot) const
{
    return spot.hasBrush ? spot.brush : defaultBrush_;
}

SpotItem ScatterPlotItem::makeSpotItem(std::size_t index) const
{
    const Spot& spot = spots_[index];
    return SpotItem(index, QPointF(xData_[index], yData_[index]), effectiveSize(spot), effectiveSymbol(spot),
        effectivePen(spot), effectiveBrush(spot), spot.data);
}

void ScatterPlotItem::resetPerSpotStyles()
{
    for (Spot& spot : spots_) {
        spot = Spot{};
    }
}

void ScatterPlotItem::markSpotsDirty()
{
    for (Spot& spot : spots_) {
        spot.sourceRect = QRect();
    }
}

void ScatterPlotItem::updateSpots()
{
    qreal maxWidth = 0.0;
    qreal maxPxWidth = 0.0;

    if (pxMode_ && useCache_) {
        const qreal dpr = fragmentAtlas_.devicePixelRatio();
        for (Spot& spot : spots_) {
            if (!spot.visible) {
                continue;
            }
            if (spot.sourceRect.isEmpty()) {
                spot.sourceRect = fragmentAtlas_.sourceRect(
                    effectiveSymbol(spot), effectiveSize(spot), effectivePen(spot), effectiveBrush(spot));
            }
            maxPxWidth = std::max(maxPxWidth, static_cast<qreal>(spot.sourceRect.width()) / dpr);
        }
    } else {
        for (const Spot& spot : spots_) {
            if (!spot.visible) {
                continue;
            }
            const qreal size = effectiveSize(spot);
            const QPen pen = effectivePen(spot);
            const qreal penWidth = pen.style() == Qt::NoPen ? 0.0 : pen.widthF();
            if (pxMode_) {
                maxPxWidth = std::max(maxPxWidth, size + penWidth);
            } else if (pen.isCosmetic()) {
                maxWidth = std::max(maxWidth, size);
                maxPxWidth = std::max(maxPxWidth, penWidth);
            } else {
                maxWidth = std::max(maxWidth, size + penWidth);
            }
        }
    }

    maxSpotWidth_ = maxWidth;
    maxSpotPxWidth_ = maxPxWidth;
}

void ScatterPlotItem::refreshBounds()
{
    QRectF newBounds;
    if (spots_.empty() || xData_.empty() || yData_.empty()) {
        newBounds = QRectF();
    } else {
        const auto [xMin, xMax] = dataBounds(0);
        const auto [yMin, yMax] = dataBounds(1);
        if (std::isfinite(static_cast<double>(xMin)) && std::isfinite(static_cast<double>(xMax))
            && std::isfinite(static_cast<double>(yMin)) && std::isfinite(static_cast<double>(yMax))) {
            if (pxMode_) {
                newBounds = QRectF(QPointF(xMin, yMin), QPointF(xMax, yMax)).normalized();
                if (newBounds.width() <= 0.0 || newBounds.height() <= 0.0) {
                    constexpr qreal kPointBoundsSpan = 1.0e-12;
                    const qreal xCenter = (xMin + xMax) * 0.5;
                    const qreal yCenter = (yMin + yMax) * 0.5;
                    newBounds = QRectF(xCenter - kPointBoundsSpan * 0.5, yCenter - kPointBoundsSpan * 0.5,
                        kPointBoundsSpan, kPointBoundsSpan);
                }
            } else {
                const qreal pxPad = pixelPadding();
                newBounds = QRectF(QPointF(xMin - pxPad, yMin - pxPad), QPointF(xMax + pxPad, yMax + pxPad))
                                .normalized();
            }
        } else {
            newBounds = QRectF();
        }
    }
    if (newBounds != bounds_) {
        prepareGeometryChange();
        bounds_ = newBounds;
    }
}

bool ScatterPlotItem::maskContains(const Spot& spot, const QRectF& rect) const
{
    const auto index = static_cast<std::size_t>(&spot - spots_.data());
    if (!spot.visible || index >= xData_.size() || !isFinitePoint(xData_[index], yData_[index])) {
        return false;
    }
    qreal width = effectiveSize(spot);
    qreal height = effectiveSize(spot);
    if (pxMode_ && useCache_ && !spot.sourceRect.isEmpty()) {
        const qreal dpr = fragmentAtlas_.devicePixelRatio();
        width = static_cast<qreal>(spot.sourceRect.width()) / dpr;
        height = static_cast<qreal>(spot.sourceRect.height()) / dpr;
    }
    width *= 0.5;
    height *= 0.5;
    const QPointF point(xData_[index], yData_[index]);
    if (pxMode_) {
        if (const QGraphicsView* view = getViewWidget(); view != nullptr) {
            bool invertible = false;
            const QTransform itemToDevice = deviceTransform(view->viewportTransform());
            const QTransform deviceToItem = itemToDevice.inverted(&invertible);
            if (invertible) {
                const QPointF devicePoint = itemToDevice.map(point);
                const QRectF deviceSpotRect(
                    devicePoint.x() - width, devicePoint.y() - height, width * 2.0, height * 2.0);
                const QRectF itemSpotRect = deviceToItem.mapRect(deviceSpotRect).normalized();
                return itemSpotRect.right() > rect.left() && itemSpotRect.left() < rect.right()
                    && itemSpotRect.bottom() > rect.top() && itemSpotRect.top() < rect.bottom();
            }
        }
    }
    return point.x() + width > rect.left() && point.x() - width < rect.right()
        && point.y() + height > rect.top() && point.y() - height < rect.bottom();
}

qreal ScatterPlotItem::devicePixelRatioFor(QWidget* widget, const QPainter* painter) const
{
    if (widget != nullptr && widget->devicePixelRatioF() > 0.0) {
        return widget->devicePixelRatioF();
    }
    if (painter != nullptr && painter->device() != nullptr && painter->device()->devicePixelRatioF() > 0.0) {
        return painter->device()->devicePixelRatioF();
    }
    return 1.0;
}

} // namespace cppqtgraph::graphicsItems
