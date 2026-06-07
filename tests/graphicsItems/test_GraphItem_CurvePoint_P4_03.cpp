#include <pyqtgraph/graphicsItems/CurvePoint.hpp>
#include <pyqtgraph/graphicsItems/GraphItem.hpp>

#include <pyqtgraph/graphicsItems/GraphicsObject.hpp>
#include <pyqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <pyqtgraph/graphicsItems/ScatterPlotItem.hpp>

#include <QtCore/QPointer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QtMath>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsPathItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
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

#define CHECK(expression) \
    do { \
        if (!check((expression), #expression, __FILE__, __LINE__)) { \
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

QColor pixelAt(const QImage& image, int x, int y)
{
    return QColor::fromRgba(image.pixel(x, y));
}

bool colorNear(const QColor& actual, const QColor& expected)
{
    constexpr int tolerance = 24;
    return std::abs(actual.red() - expected.red()) <= tolerance
        && std::abs(actual.green() - expected.green()) <= tolerance
        && std::abs(actual.blue() - expected.blue()) <= tolerance
        && std::abs(actual.alpha() - expected.alpha()) <= tolerance;
}

bool pointNear(const QPointF& actual, const QPointF& expected)
{
    return qAbs(actual.x() - expected.x()) < 1e-6 && qAbs(actual.y() - expected.y()) < 1e-6;
}

bool doubleNear(qreal actual, qreal expected, qreal tolerance = 1e-6)
{
    return qAbs(actual - expected) <= tolerance;
}

bool testGraphItemConstructionAndLifetime()
{
    using pyqtgraph::graphicsItems::GraphItem;
    using pyqtgraph::graphicsItems::GraphicsObject;
    using pyqtgraph::graphicsItems::ScatterPlotItem;

    static_assert(std::is_base_of_v<GraphicsObject, GraphItem>);
    static_assert(std::is_base_of_v<QGraphicsObject, GraphItem>);
    static_assert(!std::is_copy_constructible_v<GraphItem>);
    static_assert(!std::is_copy_assignable_v<GraphItem>);

    auto graph = std::make_unique<GraphItem>();
    ScatterPlotItem* scatter = graph->scatter();
    QPointer<ScatterPlotItem> scatterPointer(scatter);

    CHECK(scatter != nullptr);
    CHECK(scatter->parentItem() == graph.get());
    CHECK(graph->boundingRect().isNull());
    CHECK(graph->pixelPadding() == scatter->pixelPadding());

    graph.reset();
    CHECK(scatterPointer.isNull());

    return true;
}

bool testGraphItemExampleDataAndEdges()
{
    using pyqtgraph::graphicsItems::GraphEdge;
    using pyqtgraph::graphicsItems::GraphItem;

    GraphItem graph;
    const std::vector<QPointF> positions = {
        {10.0, 10.0}, {90.0, 10.0}, {10.0, 25.0}, {90.0, 25.0}, {50.0, 60.0}, {110.0, 60.0}};
    const std::vector<GraphEdge> adjacency = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {1, 5}, {3, 5}};
    std::vector<QPen> pens;
    pens.emplace_back(QColor(255, 0, 0, 255), 3.0);
    pens.emplace_back(QColor(255, 0, 255, 255), 2.0);
    pens.emplace_back(QColor(255, 0, 255, 255), 3.0);
    pens.emplace_back(QColor(255, 255, 0, 255), 2.0);
    pens.emplace_back(QColor(255, 0, 0, 255), 1.0);
    pens.emplace_back(QColor(255, 255, 255, 255), 4.0);
    const std::vector<QString> symbols = {
        QStringLiteral("o"), QStringLiteral("o"), QStringLiteral("o"), QStringLiteral("o"), QStringLiteral("t"), QStringLiteral("+")};

    graph.setData(positions, adjacency, pens);
    graph.scatter()->setSize(1.0);
    graph.scatter()->setSymbols(symbols);
    graph.scatter()->setPxMode(false);
    graph.setPos(QPointF(3.0, 4.0));

    CHECK(pointNear(graph.pos(), QPointF(3.0, 4.0)));
    CHECK(graph.positions().size() == positions.size());
    CHECK(graph.adjacency().size() == adjacency.size());
    CHECK(graph.pens().size() == pens.size());
    CHECK(graph.scatter()->points().size() == positions.size());
    CHECK(graph.scatter()->symbol() == QStringLiteral("o"));
    CHECK(graph.scatter()->size() == 1.0);
    CHECK(!graph.scatter()->pxMode());
    CHECK(graph.dataBounds(0).first <= 10.0);
    CHECK(graph.dataBounds(0).second >= 110.0);

    QImage image(128, 80, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QStyleOptionGraphicsItem option;
    graph.paint(&painter, &option, nullptr);
    painter.end();

    CHECK(colorNear(pixelAt(image, 50, 10), QColor(255, 0, 0, 255)));
    CHECK(colorNear(pixelAt(image, 50, 25), QColor(255, 0, 255, 255)));

    return true;
}

bool testGraphItemEmptyAndInvalidAdjacency()
{
    using pyqtgraph::graphicsItems::GraphEdge;
    using pyqtgraph::graphicsItems::GraphItem;

    GraphItem graph;
    const std::vector<QPointF> positions = {{10.0, 10.0}, {50.0, 10.0}};
    const std::vector<GraphEdge> noEdges;
    graph.setData(positions, noEdges, QPen(QColor(255, 0, 0), 3.0));
    CHECK(graph.adjacency().empty());
    CHECK(graph.scatter()->points().size() == positions.size());

    QImage image(64, 24, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QStyleOptionGraphicsItem option;
    graph.paint(&painter, &option, nullptr);
    painter.end();
    CHECK(pixelAt(image, 30, 10).alpha() == 0);

    const std::vector<GraphEdge> negativeEdge = {{0, -1}};
    const std::vector<GraphEdge> outOfRangeEdge = {{0, 2}};
    const std::vector<GraphEdge> oneEdge = {{0, 1}};
    const std::vector<QPen> mismatchedPens = {QPen(Qt::red), QPen(Qt::green)};
    bool threwNegative = false;
    bool threwOutOfRange = false;
    bool threwMismatchedPens = false;
    try {
        graph.setData(positions, negativeEdge, QPen(Qt::white));
    } catch (const std::out_of_range&) {
        threwNegative = true;
    }
    try {
        graph.setData(positions, outOfRangeEdge, QPen(Qt::white));
    } catch (const std::out_of_range&) {
        threwOutOfRange = true;
    }
    try {
        graph.setData(positions, oneEdge, mismatchedPens);
    } catch (const std::invalid_argument&) {
        threwMismatchedPens = true;
    }
    CHECK(threwNegative);
    CHECK(threwOutOfRange);
    CHECK(threwMismatchedPens);

    return true;
}

bool testGraphItemDynamicSceneBounds()
{
    using pyqtgraph::graphicsItems::GraphItem;

    QGraphicsScene scene;
    GraphItem graph;
    const std::vector<QPointF> initialPositions = {{0.0, 0.0}, {10.0, 10.0}};
    const std::vector<QPointF> movedPositions = {{80.0, 80.0}, {120.0, 120.0}};

    graph.setData(initialPositions);
    scene.addItem(&graph);
    const QRectF initialBounds = graph.boundingRect();
    graph.setData(movedPositions);
    const QRectF movedBounds = graph.boundingRect();
    const QRectF sceneBounds = scene.itemsBoundingRect();

    CHECK(movedBounds != initialBounds);
    CHECK(movedBounds.contains(QPointF(80.0, 80.0)));
    CHECK(movedBounds.contains(QPointF(120.0, 120.0)));
    CHECK(sceneBounds.contains(graph.mapToScene(QPointF(80.0, 80.0))));
    CHECK(sceneBounds.contains(graph.mapToScene(QPointF(120.0, 120.0))));

    scene.removeItem(&graph);
    return true;
}

bool testCurvePointPositioningAnimationAndLifetime()
{
    using pyqtgraph::graphicsItems::CurvePoint;
    using pyqtgraph::graphicsItems::GraphicsObject;
    using pyqtgraph::graphicsItems::PlotCurveItem;

    static_assert(std::is_base_of_v<GraphicsObject, CurvePoint>);
    static_assert(std::is_base_of_v<QGraphicsObject, CurvePoint>);
    static_assert(!std::is_copy_constructible_v<CurvePoint>);
    static_assert(!std::is_copy_assignable_v<CurvePoint>);

    PlotCurveItem curve;
    const std::vector<double> x = {0.0, 10.0, 20.0};
    const std::vector<double> y = {0.0, 10.0, 10.0};
    curve.setData(x, y);

    CurvePoint point(&curve, 1);
    CHECK(point.parentItem() == &curve);
    CHECK(point.flags().testFlag(QGraphicsItem::ItemHasNoContents));
    CHECK(pointNear(point.pos(), QPointF(10.0, 10.0)));
    CHECK(point.index() == 1);
    CHECK(point.curve() == &curve);
    CHECK(doubleNear(point.rotation(), 180.0 + qRadiansToDegrees(std::atan2(10.0, 20.0))));
    CHECK(point.boundingRect().isNull());

    CurvePoint midpoint(&curve, 0.5);
    CHECK(pointNear(midpoint.pos(), QPointF(10.0, 10.0)));
    CHECK(doubleNear(midpoint.position(), 0.5));

    point.setPosition(0.25);
    CHECK(doubleNear(point.position(), 0.25));
    CHECK(pointNear(point.pos(), QPointF(5.0, 5.0)));

    point.setPosition(-4.0);
    CHECK(pointNear(point.pos(), QPointF(0.0, 0.0)));
    point.setPosition(4.0);
    CHECK(pointNear(point.pos(), QPointF(20.0, 10.0)));

    CurvePoint nonRotating(&curve, 1, false);
    CHECK(pointNear(nonRotating.pos(), QPointF(10.0, 10.0)));
    CHECK(doubleNear(nonRotating.rotation(), 0.0));

    CHECK(!doubleNear(point.rotation(), 0.0));
    point.setRotate(false);
    CHECK(!point.rotate());
    CHECK(doubleNear(point.rotation(), 0.0));

    std::unique_ptr<QPropertyAnimation> animation(point.makeAnimation("position", 0.1, 0.9, 1234, 3));
    CHECK(animation->targetObject() == &point);
    CHECK(animation->propertyName() == QByteArray("position"));
    CHECK(animation->duration() == 1234);
    CHECK(doubleNear(animation->startValue().toDouble(), 0.1));
    CHECK(doubleNear(animation->endValue().toDouble(), 0.9));
    CHECK(animation->loopCount() == 3);

    auto orphanCurve = std::make_unique<PlotCurveItem>();
    orphanCurve->setData(x, y);
    auto orphanPoint = std::make_unique<CurvePoint>(orphanCurve.get(), 1);
    orphanPoint->setParentItem(nullptr);
    orphanCurve.reset();
    CHECK(orphanPoint->curve() == nullptr);
    orphanPoint->setPosition(0.5);
    orphanPoint->setIndex(0);

    return true;
}

bool testCurveArrowExampleChildAndStyle()
{
    using pyqtgraph::graphicsItems::CurveArrow;
    using pyqtgraph::graphicsItems::CurveArrowStyle;
    using pyqtgraph::graphicsItems::CurvePoint;
    using pyqtgraph::graphicsItems::PlotCurveItem;

    static_assert(std::is_base_of_v<CurvePoint, CurveArrow>);
    static_assert(!std::is_copy_constructible_v<CurveArrow>);
    static_assert(!std::is_copy_assignable_v<CurveArrow>);

    PlotCurveItem curve;
    const std::vector<double> x = {0.0, 10.0, 20.0};
    const std::vector<double> y = {0.0, 10.0, 0.0};
    curve.setData(x, y);

    CurveArrow arrow(&curve);
    CHECK(arrow.parentItem() == &curve);
    CHECK(arrow.flags().testFlag(QGraphicsItem::ItemIgnoresTransformations));
    CHECK(arrow.arrow() != nullptr);
    CHECK(arrow.arrow()->parentItem() == &arrow);
    CHECK(!arrow.arrow()->path().isEmpty());
    CHECK(!arrow.arrow()->flags().testFlag(QGraphicsItem::ItemIgnoresTransformations));
    CHECK(pointNear(arrow.pos(), QPointF(0.0, 0.0)));

    const QRectF defaultBounds = arrow.arrow()->boundingRect();
    CurveArrowStyle style = arrow.style();
    style.headLen = 40.0;
    style.tailLen = 30.0;
    style.tailWidth = 8.0;
    style.brush = QBrush(Qt::yellow);
    style.pen = QPen(Qt::white, 3.0);
    arrow.setStyle(style);
    CHECK(arrow.arrow()->boundingRect().width() > defaultBounds.width());
    CHECK(arrow.arrow()->brush().color() == QColor(Qt::yellow));
    CHECK(arrow.arrow()->pen().color() == QColor(Qt::white));

    style.pxMode = false;
    arrow.setStyle(style);
    CHECK(!arrow.flags().testFlag(QGraphicsItem::ItemIgnoresTransformations));

    CurveArrow positionedArrow(&curve, 0.5);
    CHECK(pointNear(positionedArrow.pos(), QPointF(10.0, 10.0)));

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testGraphItemConstructionAndLifetime()) {
        return 1;
    }
    if (!testGraphItemExampleDataAndEdges()) {
        return 1;
    }
    if (!testGraphItemEmptyAndInvalidAdjacency()) {
        return 1;
    }
    if (!testGraphItemDynamicSceneBounds()) {
        return 1;
    }
    if (!testCurvePointPositioningAnimationAndLifetime()) {
        return 1;
    }
    if (!testCurveArrowExampleChildAndStyle()) {
        return 1;
    }

    return 0;
}
