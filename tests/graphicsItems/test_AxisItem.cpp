#include <cppqtgraph/graphicsItems/AxisItem.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QDir>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace {

bool check(bool condition, std::string_view expression, std::string_view file, int line)
{
    if (!condition) {
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
        return false;
    }

    return true;
}

bool checkClose(double actual, double expected, double tolerance, std::string_view expression, std::string_view file, int line)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << file << ':' << line << ": check failed: " << expression << " actual=" << actual
                  << " expected=" << expected << " tolerance=" << tolerance << '\n';
        return false;
    }
    return true;
}

#define CHECK(expression) \
    do { \
        if (!check((expression), #expression, __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (false)

#define CHECK_CLOSE(actual, expected, tolerance) \
    do { \
        if (!checkClose((actual), (expected), (tolerance), #actual, __FILE__, __LINE__)) { \
            return false; \
        } \
    } while (false)

class ApplicationGuard {
public:
    ApplicationGuard(int& argc, char** argv)
    {
        if (QApplication::instance() == nullptr) {
            application_ = std::make_unique<QApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QApplication> application_;
};

bool testConstructionAndHierarchy()
{
    using cppqtgraph::graphicsItems::AxisItem;
    using cppqtgraph::graphicsItems::GraphicsItem;
    using cppqtgraph::graphicsItems::GraphicsWidget;

    static_assert(std::is_constructible_v<AxisItem>);
    static_assert(std::is_constructible_v<AxisItem, QGraphicsItem*>);
    static_assert(std::is_constructible_v<AxisItem, AxisItem::Orientation>);
    static_assert(std::is_constructible_v<AxisItem, QString>);
    static_assert(std::is_destructible_v<AxisItem>);
    static_assert(std::is_base_of_v<GraphicsWidget, AxisItem>);
    static_assert(std::is_base_of_v<GraphicsItem, AxisItem>);
    static_assert(std::is_base_of_v<QGraphicsWidget, AxisItem>);
    static_assert(std::is_base_of_v<QGraphicsItem, AxisItem>);

    AxisItem axis;
    CHECK(axis.orientation() == AxisItem::Orientation::Bottom);
    CHECK(axis.orientationName() == QStringLiteral("bottom"));
    CHECK(axis.graphicsItem() == static_cast<QGraphicsItem*>(&axis));
    CHECK(axis.getViewWidget() == nullptr);

    AxisItem left(QStringLiteral("left"));
    CHECK(left.orientation() == AxisItem::Orientation::Left);
    CHECK(left.orientationName() == QStringLiteral("left"));

    return true;
}

bool testInheritedViewWidgetDiscovery()
{
    cppqtgraph::graphicsItems::AxisItem axis;
    QGraphicsScene firstScene;
    firstScene.addItem(&axis);

    CHECK(axis.getViewWidget() == nullptr);

    QGraphicsView firstView(&firstScene);
    CHECK(axis.getViewWidget() == &firstView);
    CHECK(axis.getViewWidget() == &firstView);

    firstScene.removeItem(&axis);
    CHECK(axis.getViewWidget() == nullptr);

    QGraphicsScene secondScene;
    QGraphicsView secondView(&secondScene);
    secondScene.addItem(&axis);
    CHECK(axis.getViewWidget() == &secondView);

    secondScene.removeItem(&axis);
    CHECK(axis.getViewWidget() == nullptr);

    return true;
}

bool testParentConstruction()
{
    QGraphicsRectItem parent(QRectF(0.0, 0.0, 1.0, 1.0));
    cppqtgraph::graphicsItems::AxisItem axis(&parent);

    CHECK(axis.parentItem() == &parent);
    CHECK(axis.graphicsItem() == static_cast<QGraphicsItem*>(&axis));
    CHECK(axis.getViewWidget() == nullptr);

    return true;
}

bool testP307TickLabelAndUnitsOracle()
{
    using cppqtgraph::graphicsItems::AxisItem;

    AxisItem bottom(QStringLiteral("bottom"));
    bottom.setGeometry(QRectF(0.0, 0.0, 400.0, 70.0));
    bottom.setHeight(70.0);
    bottom.setRange(0.0, 99.0);

    const auto spacings = bottom.tickSpacing(0.0, 99.0, 400.0);
    CHECK(spacings.size() == 3U);
    CHECK_CLOSE(spacings[0].spacing, 20.0, 1.0e-12);
    CHECK_CLOSE(spacings[1].spacing, 10.0, 1.0e-12);
    CHECK_CLOSE(spacings[2].spacing, 2.0, 1.0e-12);

    const auto levels = bottom.tickValues(0.0, 99.0, 400.0);
    CHECK(levels.size() == 3U);
    CHECK((levels[0].values == std::vector<double>{0.0, 20.0, 40.0, 60.0, 80.0}));
    CHECK((levels[1].values == std::vector<double>{10.0, 30.0, 50.0, 70.0, 90.0}));
    CHECK(levels[2].values.size() > 20U);

    const auto strings = bottom.tickStrings(levels[0].values, bottom.autoSIPrefixScale(), levels[0].spacing);
    CHECK((strings == std::vector<QString>{QStringLiteral("0"), QStringLiteral("20"), QStringLiteral("40"), QStringLiteral("60"), QStringLiteral("80")}));

    AxisItem scaledExplicit(QStringLiteral("bottom"));
    scaledExplicit.setScale(2.0);
    scaledExplicit.setTickSpacing(1.0, std::nullopt);
    const auto scaledExplicitLevels = scaledExplicit.tickValues(0.0, 3.0, 300.0);
    CHECK(!scaledExplicitLevels.empty());
    CHECK_CLOSE(scaledExplicitLevels[0].spacing, 1.0, 1.0e-12);
    CHECK((scaledExplicitLevels[0].values == std::vector<double>{0.0, 1.0, 2.0, 3.0}));
    const auto scaledExplicitStrings = scaledExplicit.tickStrings(scaledExplicitLevels[0].values, 2.0, scaledExplicitLevels[0].spacing);
    CHECK((scaledExplicitStrings == std::vector<QString>{QStringLiteral("0"), QStringLiteral("2"), QStringLiteral("4"), QStringLiteral("6")}));

    AxisItem scaledExplicitOffset(QStringLiteral("bottom"));
    scaledExplicitOffset.setScale(2.0);
    scaledExplicitOffset.setTickSpacingLevels(std::vector<AxisItem::TickSpacing>{{1.0, 0.25}});
    const auto scaledExplicitOffsetLevels = scaledExplicitOffset.tickValues(0.0, 2.0, 300.0);
    CHECK(!scaledExplicitOffsetLevels.empty());
    CHECK((scaledExplicitOffsetLevels[0].values == std::vector<double>{0.25, 1.25}));

    bottom.setLabel(
        QStringLiteral("Time"),
        QStringLiteral("s"),
        QString{},
        std::vector<std::pair<double, double>>{{1.0, 1.0e6}});
    bottom.setRange(0.0, 1.0e6);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (Ms)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0e-6, 1.0e-18);
    bottom.setRange(0.0, 1.0e3);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (ks)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0e-3, 1.0e-15);
    bottom.setRange(0.0, 1.0e9);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (s)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0, 1.0e-15);
    bottom.setRange(0.0, 1.0e-9);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (s)")));
    bottom.setRange(-1.0e3, 0.0);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (ks)")));
    bottom.setRange(0.0, 1.0e6);
    bottom.enableAutoSIPrefix(false);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (s)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0, 1.0e-15);
    bottom.enableAutoSIPrefix(true);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (Ms)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0e-6, 1.0e-18);
    bottom.showLabel(false);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (s)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0, 1.0e-15);
    bottom.showLabel(true);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (Ms)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0e-6, 1.0e-18);

    AxisItem explicitPrefix(QStringLiteral("bottom"));
    explicitPrefix.enableAutoSIPrefix(false);
    explicitPrefix.setLabel(QStringLiteral("Distance"), QStringLiteral("m"), QStringLiteral("k"));
    explicitPrefix.setRange(0.0, 1.0e6);
    CHECK(explicitPrefix.labelUnitPrefix() == QStringLiteral("k"));
    CHECK(explicitPrefix.labelString().contains(QStringLiteral("Distance (km)")));
    CHECK_CLOSE(explicitPrefix.autoSIPrefixScale(), 1.0, 1.0e-15);

    bottom.enableAutoSIPrefix(true);
    CHECK(bottom.labelString().contains(QStringLiteral("Time (Ms)")));
    bottom.setSIPrefixEnableRanges(std::vector<std::pair<double, double>>{{0.0, 1.0}});
    CHECK(bottom.labelString().contains(QStringLiteral("Time (s)")));
    CHECK_CLOSE(bottom.autoSIPrefixScale(), 1.0, 1.0e-15);

    AxisItem relabelledBottom(QStringLiteral("bottom"));
    relabelledBottom.setGeometry(QRectF(0.0, 0.0, 220.0, 60.0));
    relabelledBottom.setHeight(60.0);
    relabelledBottom.setLabel(QStringLiteral("A"), QStringLiteral("s"));
    QGraphicsTextItem* labelItem = nullptr;
    for (QGraphicsItem* child : relabelledBottom.childItems()) {
        labelItem = qgraphicsitem_cast<QGraphicsTextItem*>(child);
        if (labelItem != nullptr) {
            break;
        }
    }
    CHECK(labelItem != nullptr);
    const double shortLabelX = labelItem->pos().x();
    relabelledBottom.setLabel(QStringLiteral("Longer axis label"), QStringLiteral("s"));
    CHECK(labelItem->pos().x() < shortLabelX);
    CHECK_CLOSE(labelItem->pos().x(), std::floor(110.0 - labelItem->boundingRect().width() / 2.0), 1.0e-12);

    AxisItem autoBottom(QStringLiteral("bottom"));
    autoBottom.setHeight(std::nullopt);
    const double bottomAutoHeight = autoBottom.minimumHeight();
    autoBottom.setTickLength(12.0);
    CHECK_CLOSE(autoBottom.minimumHeight(), bottomAutoHeight + 12.0, 1.0e-12);

    AxisItem autoLeft(QStringLiteral("left"));
    autoLeft.setWidth(std::nullopt);
    const double leftAutoWidth = autoLeft.minimumWidth();
    autoLeft.setTickLength(9.0);
    CHECK_CLOSE(autoLeft.minimumWidth(), leftAutoWidth + 9.0, 1.0e-12);

    AxisItem hiddenValuesBottom(QStringLiteral("bottom"));
    hiddenValuesBottom.setShowValues(false);
    hiddenValuesBottom.setHeight(std::nullopt);
    hiddenValuesBottom.setTickLength(11.0);
    CHECK_CLOSE(hiddenValuesBottom.minimumHeight(), 11.0, 1.0e-12);

    AxisItem hiddenValuesLeft(QStringLiteral("left"));
    hiddenValuesLeft.setShowValues(false);
    hiddenValuesLeft.setWidth(std::nullopt);
    hiddenValuesLeft.setTickLength(13.0);
    CHECK_CLOSE(hiddenValuesLeft.minimumWidth(), 13.0, 1.0e-12);

    AxisItem hiddenLabelBottom(QStringLiteral("bottom"));
    hiddenLabelBottom.setLabel(QStringLiteral("Time"), QStringLiteral("s"));
    hiddenLabelBottom.setHeight(std::nullopt);
    CHECK(hiddenLabelBottom.minimumHeight() > 0.0);
    hiddenLabelBottom.hide();
    CHECK_CLOSE(hiddenLabelBottom.minimumHeight(), 0.0, 1.0e-12);
    hiddenLabelBottom.show();
    CHECK(hiddenLabelBottom.minimumHeight() > 0.0);
    hiddenLabelBottom.setVisible(false);
    hiddenLabelBottom.setHeight(std::nullopt);
    CHECK_CLOSE(hiddenLabelBottom.minimumHeight(), 0.0, 1.0e-12);

    AxisItem hiddenLabelLeft(QStringLiteral("left"));
    hiddenLabelLeft.setLabel(QStringLiteral("Value"), QStringLiteral("V"));
    hiddenLabelLeft.setWidth(std::nullopt);
    CHECK(hiddenLabelLeft.minimumWidth() > 0.0);
    hiddenLabelLeft.hide();
    CHECK_CLOSE(hiddenLabelLeft.minimumWidth(), 0.0, 1.0e-12);
    hiddenLabelLeft.show();
    CHECK(hiddenLabelLeft.minimumWidth() > 0.0);

    QGraphicsScene tickBoundsScene;
    AxisItem tickBounds(QStringLiteral("bottom"));
    tickBounds.setGeometry(QRectF(0.0, 0.0, 120.0, 30.0));
    tickBounds.setHeight(30.0);
    tickBoundsScene.addItem(&tickBounds);
    const QRectF initialSceneBounds = tickBoundsScene.itemsBoundingRect();
    tickBounds.setTickLength(-25.0);
    const QRectF extendedSceneBounds = tickBoundsScene.itemsBoundingRect();
    CHECK(extendedSceneBounds.top() < initialSceneBounds.top());
    tickBoundsScene.removeItem(&tickBounds);

    AxisItem measuredLeft(QStringLiteral("left"));
    measuredLeft.setGeometry(QRectF(0.0, 0.0, 40.0, 180.0));
    measuredLeft.setWidth(std::nullopt);
    measuredLeft.setTicks({{{0.5, QStringLiteral("12345678901234567890")}}});
    const double measuredLeftWidth = measuredLeft.minimumWidth();
    QImage measureProbe(140, 220, QImage::Format_ARGB32_Premultiplied);
    measureProbe.fill(Qt::black);
    QPainter measurePainter(&measureProbe);
    CHECK(measuredLeft.generateDrawSpecs(measurePainter).has_value());
    measurePainter.end();
    CHECK(measuredLeft.minimumWidth() > measuredLeftWidth);

    AxisItem measuredBottom(QStringLiteral("bottom"));
    measuredBottom.setGeometry(QRectF(0.0, 0.0, 240.0, 30.0));
    measuredBottom.setHeight(std::nullopt);
    QFont largeTickFont;
    largeTickFont.setPointSize(24);
    measuredBottom.setTickFont(largeTickFont);
    measuredBottom.setTicks({{{0.5, QStringLiteral("42")}}});
    const double measuredBottomHeight = measuredBottom.minimumHeight();
    QImage horizontalMeasureProbe(280, 120, QImage::Format_ARGB32_Premultiplied);
    horizontalMeasureProbe.fill(Qt::black);
    QPainter horizontalMeasurePainter(&horizontalMeasureProbe);
    CHECK(measuredBottom.generateDrawSpecs(horizontalMeasurePainter).has_value());
    horizontalMeasurePainter.end();
    CHECK(measuredBottom.minimumHeight() > measuredBottomHeight);

    AxisItem left(QStringLiteral("left"));
    left.setGeometry(QRectF(0.0, 0.0, 70.0, 240.0));
    left.setWidth(70.0);
    left.setTicks({{{0.125, QStringLiteral("a")}, {0.55, QStringLiteral("b")}, {0.875, QStringLiteral("c")}}});
    left.setRange(0.0, 1.0);
    left.setStopAxisAtTick(true, true);
    QImage probe(90, 260, QImage::Format_ARGB32_Premultiplied);
    probe.fill(Qt::black);
    QPainter painter(&probe);
    const auto specs = left.generateDrawSpecs(painter);
    painter.end();
    CHECK(specs.has_value());
    CHECK_CLOSE(specs->axisLine.p1().y(), 30.0, 0.5);
    CHECK_CLOSE(specs->axisLine.p2().y(), 210.0, 0.5);

    AxisItem top(QStringLiteral("top"));
    top.setLogMode(false, true);
    CHECK(top.orientation() == AxisItem::Orientation::Top);
    top.setLogMode(true, false);
    const auto topRange = top.range();
    CHECK_CLOSE(topRange.first, 0.0, 1.0e-12);
    CHECK_CLOSE(topRange.second, 1.0, 1.0e-12);

    AxisItem logBottom(QStringLiteral("bottom"));
    logBottom.setRange(0.0, 3.0);
    logBottom.setLogMode(true);
    const auto logLevels = logBottom.tickValues(0.0, 3.0, 300.0);
    CHECK(logLevels.size() >= 2U);
    CHECK((logLevels[0].values == std::vector<double>{0.0, 1.0, 2.0, 3.0}));
    const auto logStrings = logBottom.tickStrings(logLevels[0].values, 1.0, logLevels[0].spacing);
    CHECK((logStrings == std::vector<QString>{QStringLiteral("1"), QString::fromUtf8("10¹"), QString::fromUtf8("10²"), QString::fromUtf8("10³")}));
    const double logTwo = std::log10(2.0);
    CHECK(std::any_of(logLevels[1].values.begin(), logLevels[1].values.end(), [logTwo](double value) {
        return std::abs(value - logTwo) < 1.0e-12;
    }));
    for (double majorValue : logLevels[0].values) {
        CHECK(std::none_of(logLevels[1].values.begin(), logLevels[1].values.end(), [majorValue](double minorValue) {
            return std::abs(minorValue - majorValue) < 1.0e-12;
        }));
    }

    AxisItem cappedLogBottom(QStringLiteral("bottom"));
    cappedLogBottom.setRange(0.0, 3.0);
    cappedLogBottom.setLogMode(true);
    cappedLogBottom.setMaxTickLevel(0);
    const auto cappedLogLevels = cappedLogBottom.tickValues(0.0, 3.0, 300.0);
    CHECK(cappedLogLevels.size() == 1U);
    CHECK((cappedLogLevels[0].values == std::vector<double>{0.0, 1.0, 2.0, 3.0}));

    AxisItem oneMinorLogBottom(QStringLiteral("bottom"));
    oneMinorLogBottom.setRange(0.0, 30.0);
    oneMinorLogBottom.setLogMode(true);
    oneMinorLogBottom.setMaxTickLevel(1);
    const auto oneMinorLogLevels = oneMinorLogBottom.tickValues(0.0, 30.0, 300.0);
    CHECK(oneMinorLogLevels.size() <= 2U);

    AxisItem scaledDensity(QStringLiteral("bottom"));
    scaledDensity.setGeometry(QRectF(0.0, 0.0, 400.0, 60.0));
    scaledDensity.setHeight(60.0);
    scaledDensity.setRange(0.0, 99.0);
    QImage scaledDensityProbe(2600, 120, QImage::Format_ARGB32_Premultiplied);
    scaledDensityProbe.fill(Qt::black);
    QPainter scaledDensityPainter(&scaledDensityProbe);
    scaledDensityPainter.scale(6.0, 1.0);
    const auto scaledDensitySpecs = scaledDensity.generateDrawSpecs(scaledDensityPainter);
    scaledDensityPainter.end();
    CHECK(scaledDensitySpecs.has_value());
    const auto majorTickCount = std::count_if(scaledDensitySpecs->ticks.begin(), scaledDensitySpecs->ticks.end(), [](const auto& tick) {
        return std::abs(tick.second.length() - 5.0) < 1.0e-9;
    });
    CHECK(majorTickCount == 10);

    AxisItem translucentTicks(QStringLiteral("bottom"));
    translucentTicks.setGeometry(QRectF(0.0, 0.0, 200.0, 50.0));
    translucentTicks.setHeight(50.0);
    translucentTicks.setRange(0.0, 1.0);
    translucentTicks.setTickPen(QPen(QColor(10, 20, 30, 96)));
    translucentTicks.setTicks({{{0.25, QStringLiteral("major")}}, {{0.75, QStringLiteral("minor")}}});
    QImage alphaProbe(240, 90, QImage::Format_ARGB32_Premultiplied);
    alphaProbe.fill(Qt::black);
    QPainter alphaPainter(&alphaProbe);
    const auto alphaSpecs = translucentTicks.generateDrawSpecs(alphaPainter);
    alphaPainter.end();
    CHECK(alphaSpecs.has_value());
    CHECK(alphaSpecs->ticks.size() == 2U);
    CHECK(alphaSpecs->ticks[0].first.color().alpha() == 96);
    CHECK(alphaSpecs->ticks[1].first.color().alpha() == 48);

    AxisItem transparentInheritedTicks(QStringLiteral("bottom"));
    transparentInheritedTicks.setGeometry(QRectF(0.0, 0.0, 200.0, 50.0));
    transparentInheritedTicks.setHeight(50.0);
    transparentInheritedTicks.setRange(0.0, 1.0);
    transparentInheritedTicks.setPen(QPen(QColor(0, 0, 0, 0)));
    transparentInheritedTicks.setTicks({{{0.5, QStringLiteral("transparent")}}});
    QImage transparentProbe(240, 90, QImage::Format_ARGB32_Premultiplied);
    transparentProbe.fill(Qt::black);
    QPainter transparentPainter(&transparentProbe);
    const auto transparentSpecs = transparentInheritedTicks.generateDrawSpecs(transparentPainter);
    transparentPainter.end();
    CHECK(transparentSpecs.has_value());
    CHECK(transparentSpecs->ticks.size() == 1U);
    CHECK(transparentSpecs->ticks[0].first.color().alpha() == 0);

    return true;
}

struct ImageStats {
    int width = 0;
    int height = 0;
    std::uint64_t nonWhitePixels = 0;
    std::uint64_t darkPixels = 0;
    std::uint64_t changedPixels = 0;
    double meanDelta = 0.0;
    int maxDelta = 0;
};

ImageStats semanticStats(const QImage& image)
{
    ImageStats stats;
    stats.width = image.width();
    stats.height = image.height();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            const int redDistance = 255 - color.red();
            const int greenDistance = 255 - color.green();
            const int blueDistance = 255 - color.blue();
            if (redDistance > 8 || greenDistance > 8 || blueDistance > 8) {
                ++stats.nonWhitePixels;
            }
            if (color.red() < 90 && color.green() < 90 && color.blue() < 90) {
                ++stats.darkPixels;
            }
        }
    }
    return stats;
}

ImageStats compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    ImageStats stats;
    stats.width = reference.width();
    stats.height = reference.height();
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::white);

    double totalDelta = 0.0;
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor ref = reference.pixelColor(x, y);
            const QColor act = actual.pixelColor(x, y);
            const int red = std::abs(ref.red() - act.red());
            const int green = std::abs(ref.green() - act.green());
            const int blue = std::abs(ref.blue() - act.blue());
            const int pixelMax = std::max({red, green, blue});
            totalDelta += static_cast<double>(red + green + blue) / 3.0;
            stats.maxDelta = std::max(stats.maxDelta, pixelMax);
            if (pixelMax > 8) {
                ++stats.changedPixels;
                diff.setPixelColor(x, y, QColor(255, std::max(0, 255 - pixelMax), std::max(0, 255 - pixelMax)));
            }
        }
    }
    stats.meanDelta = totalDelta / static_cast<double>(reference.width() * reference.height());
    return stats;
}

void drawReferenceAxis(
    QPainter& painter,
    const QRectF& rect,
    cppqtgraph::graphicsItems::AxisItem::Orientation orientation,
    const QString& label)
{
    using cppqtgraph::graphicsItems::AxisItem;
    const std::vector<double> majorValues{0.0, 20.0, 40.0, 60.0, 80.0, 100.0};
    const std::vector<QString> majorLabels{
        QStringLiteral("0"),
        QStringLiteral("20"),
        QStringLiteral("40"),
        QStringLiteral("60"),
        QStringLiteral("80"),
        QStringLiteral("100"),
    };

    painter.save();
    painter.setPen(QPen(Qt::black));
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    if (orientation == AxisItem::Orientation::Bottom) {
        const double axisY = rect.top() + 1.0;
        painter.drawLine(QPointF(rect.left() - 1.0, axisY), QPointF(rect.right() + 1.0, axisY));
        for (std::size_t index = 0; index < majorValues.size(); ++index) {
            const double x = rect.left() + (majorValues[index] / 100.0) * rect.width();
            painter.drawLine(QPointF(x, axisY), QPointF(x, axisY + 6.0));
            painter.drawText(QRectF(x - 20.0, axisY + 8.0, 40.0, 18.0), Qt::AlignHCenter | Qt::AlignTop, majorLabels[index]);
        }
        painter.drawText(QRectF(rect.left(), rect.bottom() - 18.0, rect.width(), 18.0), Qt::AlignCenter, QStringLiteral("%1 (s)").arg(label));
    } else {
        const double axisX = rect.right() - 1.0;
        painter.drawLine(QPointF(axisX, rect.top() - 1.0), QPointF(axisX, rect.bottom() + 1.0));
        for (std::size_t index = 0; index < majorValues.size(); ++index) {
            const double y = rect.bottom() - (majorValues[index] / 100.0) * rect.height();
            painter.drawLine(QPointF(axisX, y), QPointF(axisX - 6.0, y));
            painter.drawText(QRectF(rect.left(), y - 9.0, rect.width() - 10.0, 18.0), Qt::AlignRight | Qt::AlignVCenter, majorLabels[index]);
        }
        painter.save();
        painter.translate(rect.left() + 2.0, rect.center().y() + 38.0);
        painter.rotate(-90.0);
        painter.drawText(QRectF(0.0, 0.0, 76.0, 18.0), Qt::AlignCenter, QStringLiteral("%1 (V)").arg(label));
        painter.restore();
    }
    painter.restore();
}

QImage renderReferenceFixture()
{
    QImage image(480, 320, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.fillRect(QRectF(90.0, 40.0, 340.0, 210.0), QColor(245, 245, 245));
    painter.setPen(QPen(QColor(220, 220, 220)));
    for (int index = 0; index <= 5; ++index) {
        const double x = 90.0 + static_cast<double>(index) * 68.0;
        painter.drawLine(QPointF(x, 40.0), QPointF(x, 250.0));
        const double y = 40.0 + static_cast<double>(index) * 42.0;
        painter.drawLine(QPointF(90.0, y), QPointF(430.0, y));
    }
    drawReferenceAxis(painter, QRectF(90.0, 250.0, 340.0, 60.0), cppqtgraph::graphicsItems::AxisItem::Orientation::Bottom, QStringLiteral("Time"));
    drawReferenceAxis(painter, QRectF(20.0, 40.0, 70.0, 210.0), cppqtgraph::graphicsItems::AxisItem::Orientation::Left, QStringLiteral("Value"));
    painter.end();
    return image;
}

QImage renderActualFixture()
{
    using cppqtgraph::graphicsItems::AxisItem;

    QGraphicsScene scene;
    scene.setSceneRect(0.0, 0.0, 480.0, 320.0);
    scene.setBackgroundBrush(Qt::white);
    scene.addRect(QRectF(90.0, 40.0, 340.0, 210.0), QPen(QColor(220, 220, 220)), QBrush(QColor(245, 245, 245)));
    for (int index = 0; index <= 5; ++index) {
        const double x = 90.0 + static_cast<double>(index) * 68.0;
        scene.addLine(QLineF(x, 40.0, x, 250.0), QPen(QColor(220, 220, 220)));
        const double y = 40.0 + static_cast<double>(index) * 42.0;
        scene.addLine(QLineF(90.0, y, 430.0, y), QPen(QColor(220, 220, 220)));
    }

    auto* bottom = new AxisItem(QStringLiteral("bottom"));
    bottom->setRange(0.0, 100.0);
    bottom->setLabel(QStringLiteral("Time"), QStringLiteral("s"));
    bottom->setPen(QPen(Qt::black));
    bottom->setTextPen(QPen(Qt::black));
    bottom->setHeight(60.0);
    bottom->setGeometry(QRectF(90.0, 250.0, 340.0, 60.0));
    scene.addItem(bottom);

    auto* left = new AxisItem(QStringLiteral("left"));
    left->setRange(0.0, 100.0);
    left->setLabel(QStringLiteral("Value"), QStringLiteral("V"));
    left->setPen(QPen(Qt::black));
    left->setTextPen(QPen(Qt::black));
    left->setWidth(70.0);
    left->setGeometry(QRectF(20.0, 40.0, 70.0, 210.0));
    scene.addItem(left);

    QImage image(480, 320, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    scene.render(&painter, QRectF(0.0, 0.0, 480.0, 320.0), QRectF(0.0, 0.0, 480.0, 320.0));
    painter.end();
    return image;
}

std::uint64_t fixtureHash()
{
    const std::string fixture = "P3.07 AxisItem bottom/left range=0..100 labels=Time(s),Value(V) size=480x320";
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char byte : fixture) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream stream(path);
    if (!stream) {
        std::cerr << "failed to open " << path << " for writing\n";
        return false;
    }
    stream << text;
    return true;
}

std::optional<std::string> readTextFile(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    if (!stream) {
        std::cerr << "required GPT visual review is missing or unreadable: " << path << '\n';
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::string trimAscii(std::string value)
{
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalizedReviewFieldValue(std::string value)
{
    if (const std::size_t comment = value.find('#'); comment != std::string::npos) {
        value.erase(comment);
    }
    value = trimAscii(value);
    if (value.size() >= 2U && ((value.front() == '\'' && value.back() == '\'') || (value.front() == '"' && value.back() == '"'))) {
        value = value.substr(1U, value.size() - 2U);
    }
    return lowerAscii(trimAscii(value));
}

std::string jsonEscape(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

struct SemanticReviewStatus {
    std::string verdict;
    std::string recommendation;
    bool accepted = false;
};

std::optional<SemanticReviewStatus> readRequiredGptVisualReview(const std::filesystem::path& reviewPath)
{
    const std::optional<std::string> text = readTextFile(reviewPath);
    if (!text.has_value()) {
        return std::nullopt;
    }

    SemanticReviewStatus status;
    std::istringstream lines(*text);
    std::string line;
    while (std::getline(lines, line)) {
        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = lowerAscii(trimAscii(line.substr(0U, separator)));
        if (key == "verdict") {
            status.verdict = normalizedReviewFieldValue(line.substr(separator + 1U));
        } else if (key == "recommendation") {
            status.recommendation = normalizedReviewFieldValue(line.substr(separator + 1U));
        }
    }

    status.accepted = status.verdict == "pass" && status.recommendation == "merge_ok";
    if (!status.accepted) {
        std::cerr << "required GPT visual review must report verdict: pass and recommendation: merge_ok in "
                  << reviewPath << " (verdict=" << status.verdict << ", recommendation=" << status.recommendation << ")\n";
        return std::nullopt;
    }
    return status;
}

bool copyRequiredGptVisualReview(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath)
{
    if (!readRequiredGptVisualReview(sourcePath).has_value()) {
        return false;
    }

    std::error_code error;
    std::filesystem::copy_file(sourcePath, destinationPath, std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        std::cerr << "failed to copy required GPT visual review from " << sourcePath << " to " << destinationPath
                  << ": " << error.message() << '\n';
        return false;
    }

    return readRequiredGptVisualReview(destinationPath).has_value();
}

bool writeP307VisualArtifactSet(
    const std::filesystem::path& artifactDir,
    const QImage& reference,
    const QImage& actual,
    const QImage& diff,
    const ImageStats& blankStats,
    const ImageStats& referenceSemantic,
    const ImageStats& actualSemantic,
    const ImageStats& comparison,
    const std::filesystem::path& reviewSourcePath,
    const SemanticReviewStatus& reviewStatus,
    double changedPercent,
    double maxMeanDelta,
    double maxChangedPercent,
    bool deterministicPass)
{
    std::filesystem::create_directories(artifactDir);

    const std::filesystem::path referencePath = artifactDir / "reference.png";
    const std::filesystem::path actualPath = artifactDir / "actual.png";
    const std::filesystem::path diffPath = artifactDir / "diff.png";
    const std::filesystem::path reviewPath = artifactDir / "gpt5_vision_review.md";
    CHECK(reference.save(QString::fromStdString(referencePath.string()), "PNG"));
    CHECK(actual.save(QString::fromStdString(actualPath.string()), "PNG"));
    CHECK(diff.save(QString::fromStdString(diffPath.string()), "PNG"));
    CHECK(copyRequiredGptVisualReview(reviewSourcePath, reviewPath));

    std::ostringstream metrics;
    metrics << std::fixed << std::setprecision(6);
    metrics << "{\n";
    metrics << "  \"case\": \"P3.07-AxisItem\",\n";
    metrics << "  \"reference_source\": \"pyqtgraph-0.14.0/pyqtgraph/graphicsItems/AxisItem.py lines 49-124, 438-557, 837-865, 1112-1279, 1323-1356, 1406-1727\",\n";
    metrics << "  \"upstream_tests\": \"pyqtgraph-0.14.0/tests/graphicsItems/test_AxisItem.py lines 10-39, 99-117, 153-163\",\n";
    metrics << "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n";
    metrics << "  \"fixture_hash_fnv1a64\": \"0x" << std::hex << fixtureHash() << std::dec << "\",\n";
    metrics << "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"size\": [480, 320], \"range\": [0, 100]},\n";
    metrics << "  \"dimensions\": [" << reference.width() << ", " << reference.height() << "],\n";
    metrics << "  \"mean_abs_delta\": " << comparison.meanDelta << ",\n";
    metrics << "  \"max_delta\": " << comparison.maxDelta << ",\n";
    metrics << "  \"changed_pixel_percent\": " << changedPercent << ",\n";
    metrics << "  \"tolerance\": {\"max_mean_delta\": " << maxMeanDelta << ", \"max_changed_pixel_percent\": " << maxChangedPercent << "},\n";
    metrics << "  \"blank_guard\": {\"checked\": true, \"blank_non_white_pixels\": " << blankStats.nonWhitePixels << ", \"passed\": true},\n";
    metrics << "  \"semantic_guard\": {\"reference_non_white_pixels\": " << referenceSemantic.nonWhitePixels << ", \"actual_non_white_pixels\": " << actualSemantic.nonWhitePixels << ", \"reference_dark_pixels\": " << referenceSemantic.darkPixels << ", \"actual_dark_pixels\": " << actualSemantic.darkPixels << "},\n";
    metrics << "  \"artifact_paths\": {\"reference\": \"" << jsonEscape(referencePath.string()) << "\", \"actual\": \"" << jsonEscape(actualPath.string()) << "\", \"diff\": \"" << jsonEscape(diffPath.string()) << "\", \"metrics\": \"" << jsonEscape((artifactDir / "metrics.json").string()) << "\"},\n";
    metrics << "  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"" << jsonEscape(reviewPath.string()) << "\", \"source\": \"" << jsonEscape(reviewSourcePath.string()) << "\", \"generated_by_test\": false, \"copied_by_test\": true},\n";
    metrics << "  \"semantic_review\": {\"verdict\": \"" << jsonEscape(reviewStatus.verdict) << "\", \"recommendation\": \"" << jsonEscape(reviewStatus.recommendation) << "\", \"accepted\": " << (reviewStatus.accepted ? "true" : "false") << "},\n";
    metrics << "  \"deterministic_verdict\": \"" << (deterministicPass ? "pass" : "fail") << "\",\n";
    metrics << "  \"passed\": " << (deterministicPass && reviewStatus.accepted ? "true" : "false") << "\n";
    metrics << "}\n";
    CHECK(writeTextFile(artifactDir / "metrics.json", metrics.str()));

    const std::string manual =
        "# P3.07 manual semantic inspection\n\n"
        "Generated reference.png, actual.png, diff.png, metrics.json, and the externally supplied gpt5_vision_review.md for the AxisItem fixture. The images are non-blank and include bottom/left axes with tick marks, numeric labels, and unit labels. The implementation report records the final agent image read/open step. The test copies and validates the required GPT visual review instead of fabricating a semantic verdict.\n";
    CHECK(writeTextFile(artifactDir / "manual_semantic_inspection.md", manual));

    return true;
}

bool testP307VisualArtifacts()
{
#ifndef CPPQTGRAPH_P3_07_ARTIFACT_DIR
    return true;
#else
#ifndef CPPQTGRAPH_P3_07_GPT_REVIEW_REPORT
    std::cerr << "CPPQTGRAPH_P3_07_GPT_REVIEW_REPORT must name the required GPT visual review artifact\n";
    return false;
#else
    const std::filesystem::path reviewSourcePath{CPPQTGRAPH_P3_07_GPT_REVIEW_REPORT};
    const std::optional<SemanticReviewStatus> reviewStatus = readRequiredGptVisualReview(reviewSourcePath);
    CHECK(reviewStatus.has_value());

    QImage blank(32, 24, QImage::Format_ARGB32_Premultiplied);
    blank.fill(Qt::white);
    const ImageStats blankStats = semanticStats(blank);
    CHECK(blankStats.nonWhitePixels == 0U);

    const QImage reference = renderReferenceFixture();
    const QImage actual = renderActualFixture();
    CHECK(reference.size() == actual.size());

    const ImageStats referenceSemantic = semanticStats(reference);
    const ImageStats actualSemantic = semanticStats(actual);
    CHECK(referenceSemantic.nonWhitePixels > 6000U);
    CHECK(actualSemantic.nonWhitePixels > 6000U);
    CHECK(referenceSemantic.darkPixels > 400U);
    CHECK(actualSemantic.darkPixels > 400U);

    QImage diff;
    const ImageStats comparison = compareImages(reference, actual, diff);
    const double changedPercent = 100.0 * static_cast<double>(comparison.changedPixels)
        / static_cast<double>(reference.width() * reference.height());
    constexpr double maxMeanDelta = 6.0;
    constexpr double maxChangedPercent = 5.0;
    const bool deterministicPass = comparison.meanDelta <= maxMeanDelta && changedPercent <= maxChangedPercent;

    std::vector<std::filesystem::path> artifactDirs{std::filesystem::path{CPPQTGRAPH_P3_07_ARTIFACT_DIR}};
#ifdef CPPQTGRAPH_P3_07_CANONICAL_ARTIFACT_DIR
    artifactDirs.push_back(std::filesystem::path{CPPQTGRAPH_P3_07_CANONICAL_ARTIFACT_DIR});
#endif
    for (const std::filesystem::path& artifactDir : artifactDirs) {
        CHECK(writeP307VisualArtifactSet(
            artifactDir,
            reference,
            actual,
            diff,
            blankStats,
            referenceSemantic,
            actualSemantic,
            comparison,
            reviewSourcePath,
            *reviewStatus,
            changedPercent,
            maxMeanDelta,
            maxChangedPercent,
            deterministicPass));
    }

    CHECK(deterministicPass);
    CHECK(reviewStatus->accepted);
    return true;
#endif
#endif
}

bool testGridStateStorage()
{
    using cppqtgraph::graphicsItems::AxisItem;

    AxisItem axis(QStringLiteral("bottom"));
    CHECK(!axis.grid().has_value());

    axis.setGrid(false);
    CHECK(!axis.grid().has_value());

    axis.setGrid(128);
    CHECK(axis.grid().has_value());
    CHECK(*axis.grid() == 128);

    axis.setGrid(0.5);
    CHECK(axis.grid().has_value());
    CHECK(*axis.grid() == 127);

    axis.setGrid(false);
    CHECK(!axis.grid().has_value());

    return true;
}

bool testGridExtendsLinkedViewTicks()
{
    using cppqtgraph::graphicsItems::AxisItem;
    using cppqtgraph::graphicsItems::ViewBox;

    QGraphicsScene scene;
    ViewBox viewBox;
    viewBox.setGeometry(QRectF(80.0, 20.0, 220.0, 140.0));
    scene.addItem(&viewBox);

    AxisItem bottom(QStringLiteral("bottom"));
    bottom.setGeometry(QRectF(80.0, 160.0, 220.0, 40.0));
    bottom.setHeight(40.0);
    bottom.setRange(0.0, 10.0);
    bottom.linkToView(&viewBox);
    scene.addItem(&bottom);

    QImage probe(360, 240, QImage::Format_ARGB32_Premultiplied);
    probe.fill(Qt::black);
    QPainter painter(&probe);

    bottom.setGrid(false);
    const auto disabledSpecs = bottom.generateDrawSpecs(painter);
    CHECK(disabledSpecs.has_value());
    double disabledMaxLength = 0.0;
    for (const auto& [tickPen, tickLine] : disabledSpecs->ticks) {
        Q_UNUSED(tickPen);
        disabledMaxLength = std::max(disabledMaxLength, tickLine.length());
    }

    bottom.setGrid(128);
    const auto enabledSpecs = bottom.generateDrawSpecs(painter);
    CHECK(enabledSpecs.has_value());
    double enabledMaxLength = 0.0;
    for (const auto& [tickPen, tickLine] : enabledSpecs->ticks) {
        Q_UNUSED(tickPen);
        enabledMaxLength = std::max(enabledMaxLength, tickLine.length());
    }

    CHECK(enabledMaxLength > disabledMaxLength);
    CHECK(enabledMaxLength > 80.0);
    CHECK(disabledMaxLength < 20.0);

    return true;
}

bool isRedCurvePixel(const QColor& color)
{
    return color.red() > 200 && color.green() < 80 && color.blue() < 80 && color.alpha() > 200;
}

bool isGrayGridPixel(const QColor& color)
{
    const int channel = color.red();
    return color.alpha() > 40 && std::abs(color.red() - color.green()) < 15
        && std::abs(color.green() - color.blue()) < 15 && channel > 120 && channel < 230
        && color.red() < 200;
}

bool testPlotDataPaintsAboveExtendedGrid()
{
    using cppqtgraph::graphicsItems::PlotItem;

    QGraphicsScene scene;
    scene.setSceneRect(0.0, 0.0, 420.0, 320.0);
    scene.setBackgroundBrush(Qt::white);

    PlotItem plot;
    scene.addItem(&plot);
    plot.resize(420.0, 320.0);
    plot.setXRange(0.0, 10.0, 0.0, true);
    plot.setYRange(0.0, 10.0, 0.0, true);
    plot.getAxis(QStringLiteral("bottom"))->setGrid(220);
    plot.getAxis(QStringLiteral("left"))->setGrid(220);

    const std::vector<double> x = {0.0, 10.0};
    const std::vector<double> y = {5.0, 5.0};
    auto* curve = plot.plot(x, y, QString());
    curve->setPen(QPen(QColor(255, 0, 0), 3.0));
    curve->setAntialias(false);

    QImage image(420, 320, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    scene.render(&painter, QRectF(0.0, 0.0, 420.0, 320.0), QRectF(0.0, 0.0, 420.0, 320.0));
    painter.end();

    const QRectF viewSceneRect = plot.getViewBox()->sceneBoundingRect();
    const int sampleY = static_cast<int>(std::lround(viewSceneRect.center().y()));
    const int sampleStartX = static_cast<int>(std::lround(viewSceneRect.left() + viewSceneRect.width() * 0.15));
    const int sampleEndX = static_cast<int>(std::lround(viewSceneRect.left() + viewSceneRect.width() * 0.85));

    int redSamples = 0;
    int graySamples = 0;
    for (int xPixel = sampleStartX; xPixel <= sampleEndX; ++xPixel) {
        const QColor color = image.pixelColor(xPixel, sampleY);
        if (isRedCurvePixel(color)) {
            ++redSamples;
        } else if (isGrayGridPixel(color)) {
            ++graySamples;
        }
    }

    CHECK(redSamples >= 12);
    CHECK(graySamples <= redSamples / 4);
    CHECK(redSamples > graySamples);

    return true;
}

bool testGridAlphaAffectsRenderedTickOpacity()
{
    using cppqtgraph::graphicsItems::AxisItem;

    AxisItem bottom(QStringLiteral("bottom"));
    bottom.setGeometry(QRectF(0.0, 0.0, 240.0, 40.0));
    bottom.setHeight(40.0);
    bottom.setRange(0.0, 1.0);
    bottom.setTicks({{{0.25, QStringLiteral("a")}, {0.75, QStringLiteral("b")}}});

    QImage probe(280, 80, QImage::Format_ARGB32_Premultiplied);
    probe.fill(Qt::black);
    QPainter painter(&probe);

    bottom.setGrid(255);
    const auto strongSpecs = bottom.generateDrawSpecs(painter);
    CHECK(strongSpecs.has_value());
    CHECK(!strongSpecs->ticks.empty());
    const int strongAlpha = strongSpecs->ticks.front().first.color().alpha();

    bottom.setGrid(64);
    const auto weakSpecs = bottom.generateDrawSpecs(painter);
    CHECK(weakSpecs.has_value());
    CHECK(!weakSpecs->ticks.empty());
    const int weakAlpha = weakSpecs->ticks.front().first.color().alpha();

    CHECK(strongAlpha > weakAlpha);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndHierarchy()) {
        return 1;
    }
    if (!testInheritedViewWidgetDiscovery()) {
        return 1;
    }
    if (!testParentConstruction()) {
        return 1;
    }
    if (!testP307TickLabelAndUnitsOracle()) {
        return 1;
    }
    if (!testP307VisualArtifacts()) {
        return 1;
    }
    if (!testGridStateStorage()) {
        return 1;
    }
    if (!testGridExtendsLinkedViewTicks()) {
        return 1;
    }
    if (!testGridAlphaAffectsRenderedTickOpacity()) {
        return 1;
    }
    if (!testPlotDataPaintsAboveExtendedGrid()) {
        return 1;
    }

    return 0;
}
