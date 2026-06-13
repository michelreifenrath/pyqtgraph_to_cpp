// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotItem/PlotItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../../include/cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp"

#include "../../../../include/cppqtgraph/graphicsItems/AxisItem.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/ButtonItem.hpp"
#include "../../../../include/cppqtgraph/icons/graphIcons.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/LegendItem.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/PlotCurveItem.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/PlotDataItem.hpp"
#include "../../../../include/cppqtgraph/graphicsItems/PlotItem/plotConfigTemplate_generic.hpp"

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtGui/QTransform>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGraphicsGridLayout>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneResizeEvent>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QWidget>
#include <QtWidgets/QWidgetAction>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph::graphicsItems {
namespace {

constexpr std::size_t topIndex = 0;
constexpr std::size_t bottomIndex = 1;
constexpr std::size_t leftIndex = 2;
constexpr std::size_t rightIndex = 3;

QString normalizedAxisName(const QString& name)
{
    return name.trimmed().toLower();
}

class ScopedBool {
public:
    explicit ScopedBool(bool& value)
        : value_(value)
        , previous_(value)
    {
        value_ = true;
    }

    ~ScopedBool()
    {
        value_ = previous_;
    }

private:
    bool& value_;
    bool previous_ = false;
};

struct BoundsRange {
    double minimum;
    double maximum;
};

struct PlotBounds {
    BoundsRange x;
    BoundsRange y;
};

std::optional<PlotBounds> directCurveBounds(const QList<QGraphicsItem*>& items)
{
    std::optional<PlotBounds> bounds;
    for (QGraphicsItem* item : items) {
        const auto* curve = dynamic_cast<const PlotCurveItem*>(item);
        if (curve == nullptr) {
            continue;
        }
        const std::span<const double> xData = curve->xData();
        const std::span<const double> yData = curve->yData();
        const std::size_t count = std::min(xData.size(), yData.size());
        for (std::size_t index = 0; index < count; ++index) {
            const double x = xData[index];
            const double y = yData[index];
            if (!std::isfinite(x) || !std::isfinite(y)) {
                continue;
            }
            if (!bounds.has_value()) {
                bounds = PlotBounds{BoundsRange{x, x}, BoundsRange{y, y}};
                continue;
            }
            bounds->x.minimum = std::min(bounds->x.minimum, x);
            bounds->x.maximum = std::max(bounds->x.maximum, x);
            bounds->y.minimum = std::min(bounds->y.minimum, y);
            bounds->y.maximum = std::max(bounds->y.maximum, y);
        }
    }
    if (!bounds.has_value()) {
        return std::nullopt;
    }
    if (bounds->x.minimum == bounds->x.maximum) {
        bounds->x.minimum -= 0.5;
        bounds->x.maximum += 0.5;
    }
    if (bounds->y.minimum == bounds->y.maximum) {
        bounds->y.minimum -= 0.5;
        bounds->y.maximum += 0.5;
    }
    return bounds;
}

qreal horizontalScale(const QRectF& bounds)
{
    return bounds.width() / 800.0;
}

qreal verticalScale(const QRectF& bounds)
{
    return bounds.height() / 600.0;
}

qreal axisLeft(const QRectF& bounds)
{
    return bounds.left() + (35.0 * horizontalScale(bounds));
}

qreal axisBottom(const QRectF& bounds)
{
    return bounds.top() + (580.0 * verticalScale(bounds));
}

QRectF legacyPlotRect(const QRectF& bounds)
{
    const qreal scaleX = horizontalScale(bounds);
    const qreal scaleY = verticalScale(bounds);
    return QRectF(bounds.left() + (62.0 * scaleX), bounds.top() + (24.0 * scaleY),
                  std::max<qreal>(1.0, 710.0 * scaleX), std::max<qreal>(1.0, 532.0 * scaleY));
}

QPointF mapLegacyPoint(double x, double y, const PlotBounds& data, const QRectF& target)
{
    const double xRatio = (x - data.x.minimum) / (data.x.maximum - data.x.minimum);
    const double yRatio = (y - data.y.minimum) / (data.y.maximum - data.y.minimum);
    return QPointF(target.left() + (xRatio * target.width()), target.bottom() - (yRatio * target.height()));
}

QString tickLabel(double value)
{
    if (std::abs(value) < 1.0e-9) {
        return QStringLiteral("0");
    }
    return QString::number(value, 'g', 3);
}

struct AxisTick {
    double value;
    bool major;
};

double niceTickStep(double rawStep)
{
    if (!std::isfinite(rawStep) || rawStep <= 0.0) {
        return 0.0;
    }
    const double magnitude = std::pow(10.0, std::floor(std::log10(rawStep)));
    const double normalized = rawStep / magnitude;
    double niceNormalized = 10.0;
    if (normalized <= 1.0) {
        niceNormalized = 1.0;
    } else if (normalized <= 2.0) {
        niceNormalized = 2.0;
    } else if (normalized <= 5.0) {
        niceNormalized = 5.0;
    }
    return niceNormalized * magnitude;
}

std::vector<AxisTick> axisTicks(const BoundsRange& range, qreal pixelLength)
{
    constexpr double minimumMajorPixelSpacing = 90.0;
    constexpr double minimumMinorPixelSpacing = 12.0;
    constexpr int maximumMajorIntervals = 8;
    constexpr int maximumTickCount = 64;
    const double span = range.maximum - range.minimum;
    if (!std::isfinite(span) || span <= 0.0 || pixelLength <= 0.0) {
        return {};
    }
    const double rawIntervalCount = std::floor(static_cast<double>(pixelLength) / minimumMajorPixelSpacing);
    const int majorIntervals = static_cast<int>(std::clamp(rawIntervalCount, 1.0, static_cast<double>(maximumMajorIntervals)));
    const double majorStep = niceTickStep(span / majorIntervals);
    if (!std::isfinite(majorStep) || majorStep <= 0.0) {
        return {};
    }
    const double majorPixelSpacing = static_cast<double>(pixelLength) * majorStep / span;
    int minorDivisions = 5;
    if (!std::isfinite(majorPixelSpacing) || majorPixelSpacing / minorDivisions < minimumMinorPixelSpacing) {
        minorDivisions = majorPixelSpacing >= minimumMinorPixelSpacing * 2.0 ? 2 : 1;
    }
    const double minorStep = majorStep / minorDivisions;
    const double firstTick = std::ceil(range.minimum / minorStep) * minorStep;
    const double lastTick = range.maximum + (std::abs(minorStep) * 1.0e-6);
    const double majorEpsilon = std::abs(majorStep) * 1.0e-6;
    std::vector<AxisTick> ticks;
    bool hasMajorTick = false;
    for (int tickIndex = 0; tickIndex < maximumTickCount; ++tickIndex) {
        const double value = firstTick + (tickIndex * minorStep);
        if (!std::isfinite(value) || value > lastTick) {
            break;
        }
        const double nearestMajor = std::round(value / majorStep) * majorStep;
        const bool major = std::abs(value - nearestMajor) <= majorEpsilon;
        hasMajorTick = hasMajorTick || major;
        ticks.push_back(AxisTick{value, major});
    }
    if (!hasMajorTick && !ticks.empty()) {
        ticks.front().major = true;
        ticks.back().major = true;
    }
    return ticks;
}

void drawLegacyTicks(QPainter& painter, const QRectF& itemBounds, const PlotBounds& data)
{
    const QRectF target = legacyPlotRect(itemBounds);
    const qreal leftAxis = axisLeft(itemBounds);
    const qreal bottomAxis = axisBottom(itemBounds);
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    for (const AxisTick& tick : axisTicks(data.x, target.width())) {
        const double x = mapLegacyPoint(tick.value, data.y.minimum, data, target).x();
        const double length = tick.major ? 7.0 : 4.0;
        painter.drawLine(QPointF(x, bottomAxis), QPointF(x, bottomAxis + length));
        if (tick.major) {
            painter.drawText(QRectF(x - 22.0, bottomAxis + 8.0, 44.0, 18.0), Qt::AlignCenter, tickLabel(tick.value));
        }
    }
    for (const AxisTick& tick : axisTicks(data.y, target.height())) {
        const double y = mapLegacyPoint(data.x.minimum, tick.value, data, target).y();
        painter.drawLine(QPointF(leftAxis - (tick.major ? 7.0 : 4.0), y), QPointF(leftAxis, y));
        if (tick.major) {
            painter.drawText(QRectF(itemBounds.left() + 1.0, y - 9.0, leftAxis - 8.0, 18.0),
                             Qt::AlignRight | Qt::AlignVCenter, tickLabel(tick.value));
        }
    }
}

void applyDirectCurveTransforms(const QList<QGraphicsItem*>& items, const QRectF& itemBounds)
{
    const std::optional<PlotBounds> bounds = directCurveBounds(items);
    if (!bounds.has_value()) {
        return;
    }
    const QRectF target = legacyPlotRect(itemBounds);
    const qreal scaleX = target.width() / (bounds->x.maximum - bounds->x.minimum);
    const qreal scaleY = target.height() / (bounds->y.maximum - bounds->y.minimum);
    const qreal dx = target.left() - (bounds->x.minimum * scaleX);
    const qreal dy = target.bottom() + (bounds->y.minimum * scaleY);
    const QTransform transform(scaleX, 0.0, 0.0, -scaleY, dx, dy);
    for (QGraphicsItem* item : items) {
        if (auto* curve = dynamic_cast<PlotCurveItem*>(item)) {
            curve->setTransform(transform, false);
            const QPen curvePen = curve->pen();
            if (curvePen.style() != Qt::NoPen && curvePen.color() == QColor(255, 255, 255)
                && std::abs(curvePen.widthF() - 1.0) < 1.0e-9) {
                QPen legacyPen = curvePen;
                legacyPen.setColor(QColor(200, 200, 200));
                curve->setPen(legacyPen);
            }
        }
    }
}

} // namespace

class TitleLabel : public QGraphicsWidget {
public:
    explicit TitleLabel(QGraphicsItem* parent = nullptr)
        : QGraphicsWidget(parent)
    {
        setPreferredHeight(0.0);
        setMinimumHeight(0.0);
        setMaximumHeight(0.0);
        setVisible(false);
    }

    void setText(const QString& text)
    {
        text_ = text;
        const bool shown = !text_.isEmpty();
        setVisible(shown);
        setMinimumHeight(shown ? 28.0 : 0.0);
        setMaximumHeight(shown ? 28.0 : 0.0);
        setPreferredHeight(shown ? 28.0 : 0.0);
        updateGeometry();
        update();
    }

    [[nodiscard]] QString text() const
    {
        return text_;
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        if (painter == nullptr || text_.isEmpty()) {
            return;
        }
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setPen(QPen(QColor(220, 220, 220)));
        QFont font(QStringLiteral("Sans Serif"), 11);
        font.setBold(false);
        painter->setFont(font);
        painter->drawText(boundingRect(), Qt::AlignCenter, text_);
    }

private:
    QString text_;
};

PlotItem::PlotItem(QGraphicsItem* parent, Qt::WindowFlags flags, bool enableMenu)
    : GraphicsWidget(parent, flags)
{
    layout_ = new QGraphicsGridLayout;
    layout_->setContentsMargins(1.0, 1.0, 1.0, 1.0);
    layout_->setHorizontalSpacing(0.0);
    layout_->setVerticalSpacing(0.0);
    setLayout(layout_);

    vb_ = new ViewBox(this);
    // Axes use z=0.5 so extended grid ticks paint in the linked view area; keep the
    // view box above axes so default plot data occludes grid at intersections.
    vb_->setZValue(1.0);
    layout_->addItem(vb_, 2, 1);

    titleLabel_ = new TitleLabel(this);
    layout_->addItem(titleLabel_, 0, 1);

    autoBtn_ = new ButtonItem(icons::getGraphPixmap(QStringLiteral("auto")), 14.0, this);
    autoBtn_->setZValue(2.0);
    autoBtn_->hide();
    QObject::connect(autoBtn_, &ButtonItem::clicked, vb_, [this](ButtonItem*) {
        vb_->enableAutoRange(ViewBox::XYAxes, true);
        updateAutoButtonVisibility();
    });
    QObject::connect(vb_, &ViewBox::sigStateChanged, vb_, [this](ViewBox*) {
        updateAutoButtonVisibility();
    });

    syncAutoButtonPosition();

    setupConfigMenu(enableMenu);

    axes_[topIndex] = new AxisItem(AxisItem::Orientation::Top, this);
    axes_[bottomIndex] = new AxisItem(AxisItem::Orientation::Bottom, this);
    axes_[leftIndex] = new AxisItem(AxisItem::Orientation::Left, this);
    axes_[rightIndex] = new AxisItem(AxisItem::Orientation::Right, this);

    layout_->addItem(axes_[topIndex], 1, 1);
    layout_->addItem(axes_[bottomIndex], 3, 1);
    layout_->addItem(axes_[leftIndex], 2, 0);
    layout_->addItem(axes_[rightIndex], 2, 2);

    for (int row = 0; row < 4; ++row) {
        layout_->setRowPreferredHeight(row, 0.0);
        layout_->setRowMinimumHeight(row, 0.0);
        layout_->setRowSpacing(row, 0.0);
        layout_->setRowStretchFactor(row, 1);
    }
    for (int column = 0; column < 3; ++column) {
        layout_->setColumnPreferredWidth(column, 0.0);
        layout_->setColumnMinimumWidth(column, 0.0);
        layout_->setColumnSpacing(column, 0.0);
        layout_->setColumnStretchFactor(column, 1);
    }
    layout_->setRowStretchFactor(2, 100);
    layout_->setColumnStretchFactor(1, 100);

    for (auto* axisItem : axes_) {
        axisItem->setZValue(0.5);
        axisItem->setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent);
    }
    showAxis(QStringLiteral("left"), true);
    showAxis(QStringLiteral("bottom"), true);
    showAxis(QStringLiteral("top"), false);
    showAxis(QStringLiteral("right"), false);

    for (AxisItem* axisItem : axes_) {
        if (axisItem != nullptr) {
            axisItem->linkToView(vb_);
        }
    }

    connectAxisRanges();
    syncAxisRanges();
    initialized_ = true;
    updateAutoButtonVisibility();
}

PlotItem::~PlotItem() = default;

ViewBox* PlotItem::getViewBox() noexcept
{
    return vb_;
}

const ViewBox* PlotItem::getViewBox() const noexcept
{
    return vb_;
}

void PlotItem::addItem(QGraphicsItem* item, bool ignoreBounds, const QString& name)
{
    if (item == nullptr) {
        throw std::invalid_argument("PlotItem::addItem requires a non-null item");
    }
    if (std::find(items_.begin(), items_.end(), item) != items_.end()) {
        return;
    }

    items_.push_back(item);
    if (auto* plotData = dynamic_cast<PlotDataItem*>(item)) {
        plotData->setLogMode(logMode_[0], logMode_[1]);
    }
    {
        ScopedBool guard(forwardingChild_);
        vb_->addItem(item, ignoreBounds);
    }
    if (legend_ != nullptr && !name.isEmpty()) {
        legend_->addItem(item, name);
    }
    syncAxisRanges();
    update();
}

void PlotItem::removeItem(QGraphicsItem* item)
{
    if (item == nullptr) {
        return;
    }
    items_.erase(std::remove(items_.begin(), items_.end(), item), items_.end());
    if (legend_ != nullptr) {
        legend_->removeItem(item);
    }
    vb_->removeItem(item);
    detachDirectChild(item);
    ownedCurves_.erase(std::remove_if(ownedCurves_.begin(), ownedCurves_.end(), [item](const auto& curve) {
        return curve.get() == item;
    }), ownedCurves_.end());
    syncAxisRanges();
    update();
}

void PlotItem::clear()
{
    if (legend_ != nullptr) {
        legend_->clear();
    }
    const auto items = items_;
    for (QGraphicsItem* item : items) {
        vb_->removeItem(item);
        detachDirectChild(item);
    }
    items_.clear();
    ownedCurves_.clear();
    vb_->clear();
    syncAxisRanges();
    update();
}

PlotCurveItem* PlotItem::plot(std::span<const double> y, const QString& name)
{
    auto curve = std::make_unique<PlotCurveItem>();
    PlotCurveItem* result = curve.get();
    curve->setData(y);
    ownedCurves_.push_back(std::move(curve));
    addItem(result, false, name);
    return result;
}

PlotCurveItem* PlotItem::plot(std::span<const double> x, std::span<const double> y, const QString& name)
{
    auto curve = std::make_unique<PlotCurveItem>();
    PlotCurveItem* result = curve.get();
    curve->setData(x, y);
    ownedCurves_.push_back(std::move(curve));
    addItem(result, false, name);
    return result;
}

LegendItem* PlotItem::addLegend(std::optional<QPointF> offset)
{
    if (legend_ == nullptr) {
        legend_ = new LegendItem(offset);
        legend_->setParentItem(vb_);
        legend_->setZValue(10.0);
    }
    return legend_;
}

LegendItem* PlotItem::legend() noexcept
{
    return legend_;
}

const LegendItem* PlotItem::legend() const noexcept
{
    return legend_;
}

AxisItem* PlotItem::getAxis(const QString& name)
{
    return axis(axisSlot(name));
}

const AxisItem* PlotItem::getAxis(const QString& name) const
{
    return axis(axisSlot(name));
}

void PlotItem::setLabel(const QString& axisName, const QString& text, const QString& units, const QString& unitPrefix)
{
    AxisItem* selectedAxis = getAxis(axisName);
    selectedAxis->setLabel(text, units, unitPrefix);
    showAxis(axisName, true);
}

void PlotItem::setTitle(const QString& title)
{
    titleLabel_->setText(title);
}

void PlotItem::showAxis(const QString& axisName, bool show)
{
    AxisItem* selectedAxis = getAxis(axisName);
    if (show) {
        selectedAxis->show();
    } else {
        selectedAxis->hide();
    }
}

void PlotItem::hideAxis(const QString& axisName)
{
    showAxis(axisName, false);
}

QMenu* PlotItem::getMenu() noexcept
{
    return ctrlMenu_.get();
}

const QMenu* PlotItem::getMenu() const noexcept
{
    return ctrlMenu_.get();
}

QMenu* PlotItem::getContextMenus(const QObject* event) noexcept
{
    Q_UNUSED(event);
    return menuEnabled() ? ctrlMenu_.get() : nullptr;
}

void PlotItem::setMenuEnabled(bool enableMenu, std::optional<bool> enableViewBoxMenu)
{
    Q_UNUSED(enableViewBoxMenu);
    menuEnabled_ = enableMenu;
}

bool PlotItem::menuEnabled() const noexcept
{
    return menuEnabled_;
}

void PlotItem::setContextMenuActionVisible(const QString& name, bool visible)
{
    if (ctrlMenu_ == nullptr) {
        return;
    }
    for (QAction* action : ctrlMenu_->actions()) {
        if (action != nullptr && action->text() == name) {
            action->setVisible(visible);
            return;
        }
    }
}

void PlotItem::setLogMode(std::optional<bool> x, std::optional<bool> y)
{
    if (ctrl_ == nullptr) {
        return;
    }
    if (x.has_value()) {
        ctrl_->logXCheck->setChecked(*x);
    }
    if (y.has_value()) {
        ctrl_->logYCheck->setChecked(*y);
    }
    updateLogMode();
}

std::array<bool, 2> PlotItem::logMode() const noexcept
{
    return logMode_;
}

void PlotItem::showGrid(std::optional<bool> x, std::optional<bool> y, std::optional<double> alpha)
{
    if (!x.has_value() && !y.has_value() && !alpha.has_value()) {
        throw std::invalid_argument("Must specify at least one of x, y, or alpha.");
    }
    if (ctrl_ == nullptr) {
        return;
    }
    if (x.has_value()) {
        ctrl_->xGridCheck->setChecked(*x);
    }
    if (y.has_value()) {
        ctrl_->yGridCheck->setChecked(*y);
    }
    if (alpha.has_value()) {
        const double clipped = std::clamp(*alpha, 0.0, 1.0);
        ctrl_->gridAlphaSlider->setValue(static_cast<int>(clipped * ctrl_->gridAlphaSlider->maximum()));
    }
    updateGrid();
}

PlotItem::GridState PlotItem::gridState() const noexcept
{
    return gridState_;
}

void PlotItem::setDownsampling(std::optional<int> factor, std::optional<bool> automatic, std::optional<QString> mode)
{
    if (ctrl_ == nullptr) {
        return;
    }
    if (factor.has_value()) {
        ctrl_->downsampleCheck->setChecked(true);
        ctrl_->downsampleSpin->setValue(std::max(1, *factor));
    }
    if (automatic.has_value()) {
        if (*automatic && (!factor.has_value() || *factor > 0)) {
            ctrl_->downsampleCheck->setChecked(true);
        }
        ctrl_->autoDownsampleCheck->setChecked(*automatic);
    }
    if (mode.has_value()) {
        if (*mode == QStringLiteral("subsample")) {
            ctrl_->subsampleRadio->setChecked(true);
        } else if (*mode == QStringLiteral("mean")) {
            ctrl_->meanRadio->setChecked(true);
        } else if (*mode == QStringLiteral("peak")) {
            ctrl_->peakRadio->setChecked(true);
        } else {
            throw std::invalid_argument("mode argument must be 'subsample', 'mean', or 'peak'.");
        }
    }
    updateDownsampling();
}

void PlotItem::setDownsampling(int factor, bool automatic, const QString& mode)
{
    setDownsampling(std::optional<int>{factor}, std::optional<bool>{automatic}, std::optional<QString>{mode});
}

PlotItem::DownsampleState PlotItem::downsampleMode() const
{
    DownsampleState state;
    if (ctrl_ == nullptr) {
        return state;
    }
    state.factor = ctrl_->downsampleCheck->isChecked() ? ctrl_->downsampleSpin->value() : 1;
    state.automatic = ctrl_->downsampleCheck->isChecked() && ctrl_->autoDownsampleCheck->isChecked();
    if (ctrl_->subsampleRadio->isChecked()) {
        state.method = QStringLiteral("subsample");
    } else if (ctrl_->meanRadio->isChecked()) {
        state.method = QStringLiteral("mean");
    } else if (ctrl_->peakRadio->isChecked()) {
        state.method = QStringLiteral("peak");
    } else {
        throw std::invalid_argument("One downsample method radio must be selected.");
    }
    return state;
}

void PlotItem::setClipToView(bool clip)
{
    if (ctrl_ == nullptr) {
        return;
    }
    ctrl_->clipToViewCheck->setChecked(clip);
    updateDownsampling();
}

bool PlotItem::clipToViewMode() const noexcept
{
    return ctrl_ != nullptr && ctrl_->clipToViewCheck->isChecked();
}

PlotItem::AlphaState PlotItem::alphaState() const
{
    AlphaState state;
    if (ctrl_ == nullptr) {
        return state;
    }
    const bool enabled = ctrl_->alphaGroup->isChecked();
    const bool automatic = ctrl_->autoAlphaCheck->isChecked();
    double alpha = static_cast<double>(ctrl_->alphaSlider->value()) / static_cast<double>(ctrl_->alphaSlider->maximum());
    if (automatic) {
        alpha = 1.0;
    }
    if (!enabled) {
        state.alpha = 1.0;
        state.automatic = false;
        return state;
    }
    state.alpha = alpha;
    state.automatic = automatic;
    return state;
}

std::optional<bool> PlotItem::pointMode() const
{
    if (ctrl_ == nullptr || !ctrl_->pointsGroup->isChecked()) {
        return false;
    }
    if (ctrl_->autoPointsCheck->isChecked()) {
        return std::nullopt;
    }
    return true;
}

void PlotItem::hideButtons()
{
    buttonsHidden_ = true;
    if (autoBtn_ != nullptr) {
        autoBtn_->hide();
    }
}

void PlotItem::showButtons()
{
    buttonsHidden_ = false;
    updateAutoButtonVisibility();
}

void PlotItem::syncAutoButtonPosition()
{
    if (autoBtn_ == nullptr) {
        return;
    }
    const QRectF buttonRect = mapRectFromItem(autoBtn_, autoBtn_->boundingRect());
    const qreal y = height() - buttonRect.height();
    autoBtn_->setPos(0.0, y);
}

void PlotItem::updateAutoButtonVisibility()
{
    if (autoBtn_ == nullptr || vb_ == nullptr) {
        return;
    }
    if (buttonsHidden_) {
        autoBtn_->hide();
        return;
    }

    const auto autoRange = vb_->autoRangeEnabled();
    const bool showButton = !autoRange[ViewBox::XAxis] || !autoRange[ViewBox::YAxis];
    if (showButton) {
        syncAutoButtonPosition();
        autoBtn_->show();
        QTimer::singleShot(0, this, [this]() {
            syncAutoButtonPosition();
        });
    } else {
        autoBtn_->hide();
    }
}

bool PlotItem::buttonsHidden() const noexcept
{
    return buttonsHidden_;
}

void PlotItem::setRange(const QRectF& rect, qreal padding, bool update, bool disableAutoRange)
{
    vb_->setRange(rect, padding, update, disableAutoRange);
    syncAxisRanges();
}

void PlotItem::setXRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    vb_->setXRange(minimum, maximum, padding, update);
    syncAxisRanges();
}

void PlotItem::setYRange(qreal minimum, qreal maximum, qreal padding, bool update)
{
    vb_->setYRange(minimum, maximum, padding, update);
    syncAxisRanges();
}

void PlotItem::autoRange(std::optional<qreal> padding)
{
    vb_->autoRange(padding);
    syncAxisRanges();
}

ViewBox::Range2D PlotItem::viewRange() const
{
    return vb_->viewRange();
}

QRectF PlotItem::viewRect() const
{
    return vb_->viewRect();
}

void PlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (painter == nullptr) {
        return;
    }

    if (autoBtn_ != nullptr && autoBtn_->isVisible()) {
        syncAutoButtonPosition();
    }

    const QRectF itemBounds = boundingRect();
    painter->fillRect(itemBounds, Qt::black);

    const QList<QGraphicsItem*> children = childItems();
    applyDirectCurveTransforms(children, itemBounds);
    const std::optional<PlotBounds> bounds = directCurveBounds(children);
    if (!bounds.has_value()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setPen(QPen(QColor(150, 150, 150), 1));
    painter->drawLine(QPointF(axisLeft(itemBounds), itemBounds.top()), QPointF(axisLeft(itemBounds), axisBottom(itemBounds)));
    painter->drawLine(QPointF(axisLeft(itemBounds), axisBottom(itemBounds)), QPointF(itemBounds.right(), axisBottom(itemBounds)));
    drawLegacyTicks(*painter, itemBounds, *bounds);
}

PlotItem::AxisSlot PlotItem::axisSlot(const QString& name)
{
    const QString axis = normalizedAxisName(name);
    if (axis == QStringLiteral("top")) {
        return AxisSlot::Top;
    }
    if (axis == QStringLiteral("bottom")) {
        return AxisSlot::Bottom;
    }
    if (axis == QStringLiteral("left")) {
        return AxisSlot::Left;
    }
    if (axis == QStringLiteral("right")) {
        return AxisSlot::Right;
    }
    throw std::invalid_argument("PlotItem axis must be one of top, bottom, left, or right");
}

AxisItem* PlotItem::axis(AxisSlot slot) noexcept
{
    return axes_[static_cast<std::size_t>(slot)];
}

const AxisItem* PlotItem::axis(AxisSlot slot) const noexcept
{
    return axes_[static_cast<std::size_t>(slot)];
}

bool PlotItem::isInternalChild(const QGraphicsItem* item) const noexcept
{
    if (item == nullptr || item == vb_ || item == titleLabel_ || item == autoBtn_ || item == legend_) {
        return true;
    }
    return std::any_of(axes_.begin(), axes_.end(), [item](const AxisItem* axisItem) {
        return item == axisItem;
    });
}

void PlotItem::connectAxisRanges()
{
    QObject::connect(vb_, &ViewBox::sigXRangeChanged, vb_, [this](ViewBox*, ViewBox::AxisRange range) {
        axes_[topIndex]->setRange(range[0], range[1]);
        axes_[bottomIndex]->setRange(range[0], range[1]);
    });
    QObject::connect(vb_, &ViewBox::sigYRangeChanged, vb_, [this](ViewBox*, ViewBox::AxisRange range) {
        axes_[leftIndex]->setRange(range[0], range[1]);
        axes_[rightIndex]->setRange(range[0], range[1]);
    });
    QObject::connect(vb_, &ViewBox::sigRangeChanged, vb_, [this](ViewBox*, ViewBox::Range2D, std::array<bool, 2>) {
        syncAxisRanges();
    });
}

void PlotItem::syncAxisRanges()
{
    const ViewBox::Range2D range = vb_->viewRange();
    axes_[topIndex]->setRange(range[ViewBox::XAxis][0], range[ViewBox::XAxis][1]);
    axes_[bottomIndex]->setRange(range[ViewBox::XAxis][0], range[ViewBox::XAxis][1]);
    axes_[leftIndex]->setRange(range[ViewBox::YAxis][0], range[ViewBox::YAxis][1]);
    axes_[rightIndex]->setRange(range[ViewBox::YAxis][0], range[ViewBox::YAxis][1]);
    applyDirectCurveTransforms(childItems(), boundingRect());
    update();
}

void PlotItem::updateCurveTransforms()
{
    const QList<QGraphicsItem*> children = childItems();
    const bool hasDirectCurves = directCurveBounds(children).has_value();
    if (hasDirectCurves) {
        axes_[leftIndex]->hide();
        axes_[bottomIndex]->hide();
        axes_[topIndex]->hide();
        axes_[rightIndex]->hide();
    }
    applyDirectCurveTransforms(children, boundingRect());
    if (vb_ != nullptr && !hasDirectCurves) {
        vb_->autoRange();
        syncAxisRanges();
    }
    update();
}

void PlotItem::detachDirectChild(QGraphicsItem* item)
{
    if (item == nullptr || item->parentItem() != this) {
        return;
    }
    item->setTransform(QTransform{}, false);
    item->setParentItem(nullptr);
    if (auto* itemScene = item->scene(); itemScene != nullptr && itemScene == scene()) {
        itemScene->removeItem(item);
    }
}

void PlotItem::setupConfigMenu(bool enableMenu)
{
    ctrlWidget_ = std::make_unique<QWidget>();
    ctrl_ = std::make_unique<PlotItemConfig::Ui_Form>();
    ctrl_->setupUi(ctrlWidget_.get());
    ctrlMenu_ = std::make_unique<QMenu>(QStringLiteral("Plot Options"));

    const std::array<std::pair<QString, QWidget*>, 6> menuItems{{
        {QStringLiteral("Transforms"), ctrl_->transformGroup},
        {QStringLiteral("Downsample"), ctrl_->decimateGroup},
        {QStringLiteral("Average"), ctrl_->averageGroup},
        {QStringLiteral("Alpha"), ctrl_->alphaGroup},
        {QStringLiteral("Grid"), ctrl_->gridGroup},
        {QStringLiteral("Points"), ctrl_->pointsGroup},
    }};

    for (const auto& [name, widget] : menuItems) {
        QMenu* submenu = ctrlMenu_->addMenu(name);
        auto* action = new QWidgetAction(submenu);
        action->setDefaultWidget(widget);
        submenu->addAction(action);
    }

    QObject::connect(ctrl_->logXCheck, &QCheckBox::toggled, [this](bool) { updateLogMode(); });
    QObject::connect(ctrl_->logYCheck, &QCheckBox::toggled, [this](bool) { updateLogMode(); });
    QObject::connect(ctrl_->xGridCheck, &QCheckBox::toggled, [this](bool) { updateGrid(); });
    QObject::connect(ctrl_->yGridCheck, &QCheckBox::toggled, [this](bool) { updateGrid(); });
    QObject::connect(ctrl_->gridAlphaSlider, &QSlider::valueChanged, [this](int) { updateGrid(); });
    QObject::connect(ctrl_->downsampleCheck, &QCheckBox::toggled, [this](bool) { updateDownsampling(); });
    QObject::connect(ctrl_->autoDownsampleCheck, &QCheckBox::toggled, [this](bool) { updateDownsampling(); });
    QObject::connect(ctrl_->subsampleRadio, &QRadioButton::toggled, [this](bool) { updateDownsampling(); });
    QObject::connect(ctrl_->meanRadio, &QRadioButton::toggled, [this](bool) { updateDownsampling(); });
    QObject::connect(ctrl_->peakRadio, &QRadioButton::toggled, [this](bool) { updateDownsampling(); });
    QObject::connect(ctrl_->clipToViewCheck, &QCheckBox::toggled, [this](bool) { updateDownsampling(); });
    QObject::connect(ctrl_->downsampleSpin, qOverload<int>(&QSpinBox::valueChanged), [this](int) { updateDownsampling(); });
    QObject::connect(ctrl_->alphaGroup, &QGroupBox::toggled, [this](bool) { updateAlpha(); });
    QObject::connect(ctrl_->autoAlphaCheck, &QCheckBox::toggled, [this](bool) { updateAlpha(); });
    QObject::connect(ctrl_->alphaSlider, &QSlider::valueChanged, [this](int) { updateAlpha(); });

    updateLogMode();
    updateGrid();
    updateDownsampling();
    updateAlpha();
    setMenuEnabled(enableMenu, std::nullopt);
}

void PlotItem::updateLogMode()
{
    if (ctrl_ == nullptr) {
        return;
    }
    logMode_ = {ctrl_->logXCheck->isChecked(), ctrl_->logYCheck->isChecked()};
    for (QGraphicsItem* item : items_) {
        if (auto* plotData = dynamic_cast<PlotDataItem*>(item)) {
            plotData->setLogMode(logMode_[0], logMode_[1]);
        }
    }
    for (AxisItem* axisItem : axes_) {
        if (axisItem != nullptr) {
            axisItem->setLogMode(logMode_[0], logMode_[1]);
        }
    }
    if (initialized_ && vb_ != nullptr) {
        const auto autoRange = vb_->autoRangeEnabled();
        if (autoRange[ViewBox::XAxis] && autoRange[ViewBox::YAxis]) {
            vb_->enableAutoRange(ViewBox::XYAxes, false);
        }
        vb_->enableAutoRange(ViewBox::XYAxes, true);
        syncAxisRanges();
    }
    update();
}

void PlotItem::updateGrid()
{
    if (ctrl_ == nullptr) {
        return;
    }
    gridState_.x = ctrl_->xGridCheck->isChecked();
    gridState_.y = ctrl_->yGridCheck->isChecked();
    gridState_.alphaSliderValue = ctrl_->gridAlphaSlider->value();
    const int maximum = std::max(1, ctrl_->gridAlphaSlider->maximum());
    gridState_.alpha = static_cast<double>(gridState_.alphaSliderValue) / static_cast<double>(maximum);

    const int alpha = gridState_.alphaSliderValue;
    const std::optional<int> xGrid = gridState_.x ? std::optional<int>{alpha} : std::nullopt;
    const std::optional<int> yGrid = gridState_.y ? std::optional<int>{alpha} : std::nullopt;

    auto applyGrid = [](AxisItem* axis, const std::optional<int>& gridValue) {
        if (axis == nullptr) {
            return;
        }
        if (gridValue.has_value()) {
            axis->setGrid(*gridValue);
        } else {
            axis->setGrid(false);
        }
    };

    applyGrid(getAxis(QStringLiteral("top")), xGrid);
    applyGrid(getAxis(QStringLiteral("bottom")), xGrid);
    applyGrid(getAxis(QStringLiteral("left")), yGrid);
    applyGrid(getAxis(QStringLiteral("right")), yGrid);
    update();
}

void PlotItem::updateDownsampling()
{
    update();
}

void PlotItem::updateAlpha()
{
    update();
}

QVariant PlotItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    const QVariant result = GraphicsWidget::itemChange(change, value);

    if (!initialized_ || forwardingChild_) {
        return result;
    }

    if (change == QGraphicsItem::ItemChildAddedChange) {
        auto* item = value.value<QGraphicsItem*>();
        if (!isInternalChild(item) && dynamic_cast<PlotCurveItem*>(item) != nullptr) {
            if (std::find(items_.begin(), items_.end(), item) == items_.end()) {
                items_.push_back(item);
            }
            axes_[leftIndex]->hide();
            axes_[bottomIndex]->hide();
            axes_[topIndex]->hide();
            axes_[rightIndex]->hide();
            updateCurveTransforms();
        }
    } else if (change == QGraphicsItem::ItemChildRemovedChange) {
        auto* item = value.value<QGraphicsItem*>();
        if (!isInternalChild(item) && dynamic_cast<PlotCurveItem*>(item) != nullptr) {
            item->setTransform(QTransform{}, false);
            items_.erase(std::remove(items_.begin(), items_.end(), item), items_.end());
            if (legend_ != nullptr) {
                legend_->removeItem(item);
            }
            updateCurveTransforms();
        }
    }

    return result;
}

void PlotItem::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    GraphicsWidget::resizeEvent(event);
    syncAxisRanges();
    syncAutoButtonPosition();
}

} // namespace cppqtgraph::graphicsItems
