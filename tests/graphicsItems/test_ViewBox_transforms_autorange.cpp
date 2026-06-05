#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>
#include <pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QtGlobal>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {

using GraphicsScene = pyqtgraph::GraphicsScene::GraphicsScene;
using ViewBox = pyqtgraph::graphicsItems::ViewBox;

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

class CountingRectItem : public QGraphicsRectItem {
public:
    explicit CountingRectItem(const QRectF& rect)
        : QGraphicsRectItem(rect)
    {
    }

    ~CountingRectItem() override
    {
        ++destructionCount;
    }

    static int destructionCount;
};

int CountingRectItem::destructionCount = 0;

bool nearlyEqual(qreal lhs, qreal rhs)
{
    return std::abs(lhs - rhs) <= 1.0e-9;
}

bool checkPoint(const QPointF& point, qreal x, qreal y)
{
    CHECK(nearlyEqual(point.x(), x));
    CHECK(nearlyEqual(point.y(), y));
    return true;
}

bool checkRange(const ViewBox::Range2D& range, qreal xMin, qreal xMax, qreal yMin, qreal yMax)
{
    CHECK(nearlyEqual(range[0][0], xMin));
    CHECK(nearlyEqual(range[0][1], xMax));
    CHECK(nearlyEqual(range[1][0], yMin));
    CHECK(nearlyEqual(range[1][1], yMax));
    return true;
}

bool testCoordinateMappingAndInversion()
{
    ViewBox viewBox;
    CHECK(viewBox.flags().testFlag(QGraphicsItem::ItemClipsChildrenToShape));
    viewBox.resize(200.0, 100.0);
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);

    CHECK(checkPoint(viewBox.mapFromView(QPointF(0.0, 0.0)), 0.0, 100.0));
    CHECK(checkPoint(viewBox.mapFromView(QPointF(10.0, 10.0)), 200.0, 0.0));
    CHECK(checkPoint(viewBox.mapToView(QPointF(200.0, 0.0)), 10.0, 10.0));
    CHECK(checkPoint(viewBox.mapToView(viewBox.mapFromView(QPointF(2.5, 7.5))), 2.5, 7.5));

    const auto pixelSize = viewBox.viewPixelSize();
    CHECK(nearlyEqual(pixelSize.width(), 0.05));
    CHECK(nearlyEqual(pixelSize.height(), 0.1));

    QGraphicsScene scaledScene;
    QGraphicsRectItem scaledParent;
    scaledParent.setScale(2.0);
    scaledScene.addItem(&scaledParent);
    auto scaledViewBox = std::make_unique<ViewBox>(&scaledParent);
    scaledViewBox->resize(200.0, 100.0);
    scaledViewBox->setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    const auto scaledPixelSize = scaledViewBox->viewPixelSize();
    CHECK(nearlyEqual(scaledPixelSize.width(), 0.025));
    CHECK(nearlyEqual(scaledPixelSize.height(), 0.05));
    scaledViewBox->setParentItem(nullptr);
    scaledViewBox.reset();
    scaledScene.removeItem(&scaledParent);

    viewBox.invertY(true);
    CHECK(viewBox.yInverted());
    CHECK(checkRange(viewBox.viewRange(), 0.0, 10.0, 0.0, 10.0));
    CHECK(checkPoint(viewBox.mapFromView(QPointF(0.0, 0.0)), 0.0, 0.0));
    CHECK(checkPoint(viewBox.mapFromView(QPointF(10.0, 10.0)), 200.0, 100.0));

    viewBox.invertX(true);
    CHECK(viewBox.xInverted());
    CHECK(checkPoint(viewBox.mapFromView(QPointF(0.0, 0.0)), 200.0, 0.0));
    CHECK(checkPoint(viewBox.mapFromView(QPointF(10.0, 10.0)), 0.0, 100.0));

    ViewBox limitedViewBox;
    limitedViewBox.resize(100.0, 100.0);
    limitedViewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0);
    CHECK(checkPoint(limitedViewBox.mapFromView(QPointF(10.0, 10.0)), 100.0, 0.0));
    ViewBox::Limits limits;
    limits.xMin = 0.0;
    limits.xMax = 5.0;
    limitedViewBox.setLimits(limits);
    CHECK(checkRange(limitedViewBox.viewRange(), 0.0, 5.0, 0.0, 10.0));
    CHECK(checkPoint(limitedViewBox.mapFromView(QPointF(5.0, 10.0)), 100.0, 0.0));

    ViewBox deferredViewBox;
    deferredViewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0, true, false);
    deferredViewBox.setRange(ViewBox::AxisRange{20.0, 30.0}, std::nullopt, 0.0, false, false);
    CHECK(checkRange(deferredViewBox.viewRange(), 0.0, 10.0, 0.0, 10.0));
    ViewBox::Limits deferredLimits;
    deferredLimits.yMin = -100.0;
    deferredViewBox.setLimits(deferredLimits);
    CHECK(checkRange(deferredViewBox.viewRange(), 0.0, 10.0, 0.0, 10.0));
    CHECK(checkRange(deferredViewBox.targetRange(), 20.0, 30.0, 0.0, 10.0));

    return true;
}

bool testAspectLockedRangeUsesWidgetGeometry()
{
    ViewBox viewBox;
    viewBox.resize(20.0, 10.0);
    bool rejectedNegativeRatio = false;
    try {
        viewBox.setAspectLocked(true, -1.0);
    } catch (const std::invalid_argument&) {
        rejectedNegativeRatio = true;
    }
    CHECK(rejectedNegativeRatio);
    viewBox.setAspectLocked(true, 1.0);
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 15.0), 0.0);

    const auto viewRange = viewBox.viewRange();
    const qreal viewWidth = viewRange[0][1] - viewRange[0][0];
    const qreal viewHeight = viewRange[1][1] - viewRange[1][0];
    CHECK(nearlyEqual(viewWidth, 2.0 * viewHeight));
    CHECK(checkRange(viewBox.targetRange(), 0.0, 10.0, 0.0, 15.0));

    viewBox.resize(10.0, 20.0);
    const auto tallRange = viewBox.viewRange();
    CHECK(nearlyEqual(tallRange[1][1] - tallRange[1][0], 2.0 * (tallRange[0][1] - tallRange[0][0])));

    QGraphicsScene emptyBoundsScene;
    ViewBox emptyBoundsViewBox;
    emptyBoundsScene.addItem(&emptyBoundsViewBox);
    emptyBoundsViewBox.resize(20.0, 10.0);
    emptyBoundsViewBox.setDefaultPadding(0.0);
    emptyBoundsViewBox.setAspectLocked(true, 1.0);
    emptyBoundsViewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0, true, false);
    auto emptyItem = std::make_unique<QGraphicsRectItem>(QRectF(0.0, 0.0, 0.0, 0.0));
    emptyItem->setPen(QPen(Qt::NoPen));
    emptyBoundsViewBox.addItem(emptyItem.get());
    emptyBoundsViewBox.resize(10.0, 20.0);
    const auto noBoundsTallRange = emptyBoundsViewBox.viewRange();
    CHECK(nearlyEqual(noBoundsTallRange[1][1] - noBoundsTallRange[1][0], 2.0 * (noBoundsTallRange[0][1] - noBoundsTallRange[0][0])));
    emptyBoundsViewBox.removeItem(emptyItem.get());
    emptyBoundsScene.removeItem(&emptyBoundsViewBox);

    return true;
}

bool testAutorangeAndStateControls()
{
    QGraphicsScene scene;
    ViewBox viewBox;
    scene.addItem(&viewBox);
    viewBox.resize(100.0, 100.0);
    viewBox.setDefaultPadding(0.0);
    viewBox.setRange(QRectF(0.0, 0.0, 1.0, 1.0), 0.0, true, false);

    CHECK(viewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(viewBox.autoRangeEnabled()[ViewBox::YAxis]);

    viewBox.setRange(QRectF(10.0, 10.0, 1.0, 1.0), 0.0, true, false);
    auto emptyItem = std::make_unique<QGraphicsRectItem>(QRectF(0.0, 0.0, 0.0, 0.0));
    emptyItem->setPen(QPen(Qt::NoPen));
    viewBox.addItem(emptyItem.get());
    CHECK(checkRange(viewBox.viewRange(), 10.0, 11.0, 10.0, 11.0));
    const auto emptyBounds = viewBox.childrenBounds();
    CHECK(!emptyBounds[ViewBox::XAxis].has_value());
    CHECK(!emptyBounds[ViewBox::YAxis].has_value());
    viewBox.removeItem(emptyItem.get());
    viewBox.setRange(QRectF(0.0, 0.0, 1.0, 1.0), 0.0, true, false);

    auto ignored = std::make_unique<QGraphicsRectItem>(QRectF(-100.0, -100.0, 10.0, 10.0));
    ignored->setPen(QPen(Qt::NoPen));
    viewBox.addItem(ignored.get(), true);
    viewBox.autoRange(0.5);
    CHECK(checkRange(viewBox.viewRange(), 0.0, 1.0, 0.0, 1.0));
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(ignored->scene() == &scene);
    viewBox.clear();
    CHECK(ignored->scene() == nullptr);
    viewBox.autoRange(0.5);
    CHECK(checkRange(viewBox.viewRange(), 0.0, 1.0, 0.0, 1.0));

    QGraphicsScene unrelatedScene;
    QGraphicsRectItem unrelatedItem(QRectF(0.0, 0.0, 1.0, 1.0));
    unrelatedScene.addItem(&unrelatedItem);
    viewBox.removeItem(&unrelatedItem);
    CHECK(unrelatedItem.scene() == &unrelatedScene);
    unrelatedScene.removeItem(&unrelatedItem);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(-2.0, 3.0, 5.0, 7.0));
    item->setPen(QPen(Qt::NoPen));
    viewBox.addItem(item.get());
    viewBox.autoRange(0.0);
    CHECK(checkRange(viewBox.viewRange(), -2.0, 3.0, 3.0, 10.0));

    CHECK(!viewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    item->setRect(QRectF(-2.0, 3.0, 5.0, 20.0));
    QApplication::processEvents();
    CHECK(checkRange(viewBox.viewRange(), -2.0, 3.0, 3.0, 10.0));
    viewBox.enableAutoRange(ViewBox::YAxis);
    CHECK(checkRange(viewBox.viewRange(), -2.0, 3.0, 3.0, 23.0));
    item->setRect(QRectF(-2.0, 3.0, 5.0, 7.0));
    viewBox.disableAutoRange(ViewBox::YAxis);
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(checkRange(viewBox.viewRange(), -2.0, 3.0, 3.0, 10.0));
    viewBox.enableAutoRange(ViewBox::YAxis);

    viewBox.setXRange(-1.0, 1.0, 0.0);
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    viewBox.autoRange(0.0);
    CHECK(checkRange(viewBox.viewRange(), -2.0, 3.0, 3.0, 10.0));
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::XAxis]);
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    viewBox.setXRange(-1.0, 1.0, 0.0);
    viewBox.enableAutoRange(ViewBox::YAxis);

    auto widerItem = std::make_unique<QGraphicsRectItem>(QRectF(50.0, 20.0, 10.0, 5.0));
    widerItem->setPen(QPen(Qt::NoPen));
    viewBox.addItem(widerItem.get());
    CHECK(checkRange(viewBox.viewRange(), -1.0, 1.0, 3.0, 25.0));

    viewBox.removeItem(widerItem.get());
    CHECK(checkRange(viewBox.viewRange(), -1.0, 1.0, 3.0, 10.0));

    viewBox.disableAutoRange(ViewBox::YAxis);
    CHECK(!viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    viewBox.setYRange(100.0, 101.0, 0.0);
    CHECK(checkRange(viewBox.viewRange(), -1.0, 1.0, 100.0, 101.0));
    viewBox.enableAutoRange(ViewBox::YAxis);
    CHECK(viewBox.autoRangeEnabled()[ViewBox::YAxis]);
    CHECK(checkRange(viewBox.viewRange(), -1.0, 1.0, 3.0, 10.0));

    viewBox.removeItem(item.get());
    scene.removeItem(&viewBox);

    QGraphicsScene parentedScene;
    QGraphicsRectItem parentItem;
    parentedScene.addItem(&parentItem);
    auto parentedViewBox = std::make_unique<ViewBox>(&parentItem);
    parentedViewBox->resize(100.0, 100.0);
    parentedViewBox->setDefaultPadding(0.0);
    parentedViewBox->setRange(QRectF(0.0, 0.0, 1.0, 1.0), 0.0, true, false);
    CHECK(parentedViewBox->scene() == &parentedScene);

    auto parentedChild = std::make_unique<QGraphicsRectItem>(QRectF(2.0, 4.0, 3.0, 5.0));
    parentedChild->setPen(QPen(Qt::NoPen));
    parentedViewBox->addItem(parentedChild.get());
    CHECK(checkRange(parentedViewBox->viewRange(), 2.0, 5.0, 4.0, 9.0));

    parentedChild->setRect(QRectF(-10.0, -20.0, 30.0, 40.0));
    QApplication::processEvents();
    CHECK(checkRange(parentedViewBox->viewRange(), -10.0, 20.0, -20.0, 20.0));

    parentedViewBox->removeItem(parentedChild.get());
    parentedViewBox->setParentItem(nullptr);
    parentedScene.removeItem(&parentItem);
    return true;
}

bool testDeletedTrackedItemDoesNotLeaveDanglingPointer()
{
    QGraphicsScene scene;
    ViewBox viewBox;
    scene.addItem(&viewBox);
    viewBox.resize(100.0, 100.0);
    viewBox.setDefaultPadding(0.0);
    viewBox.setRange(QRectF(0.0, 0.0, 1.0, 1.0), 0.0, true, false);

    auto externallyOwnedItem = std::make_unique<QGraphicsRectItem>(QRectF(-5.0, -6.0, 3.0, 4.0));
    externallyOwnedItem->setPen(QPen(Qt::NoPen));
    viewBox.addItem(externallyOwnedItem.get());
    CHECK(checkRange(viewBox.viewRange(), -5.0, -2.0, -6.0, -2.0));

    externallyOwnedItem.reset();
    viewBox.clear();
    QApplication::processEvents();
    const auto boundsAfterDelete = viewBox.childrenBounds();
    CHECK(!boundsAfterDelete[ViewBox::XAxis].has_value());
    CHECK(!boundsAfterDelete[ViewBox::YAxis].has_value());
    viewBox.autoRange(0.0);
    viewBox.enableAutoRange();

    auto replacementItem = std::make_unique<QGraphicsRectItem>(QRectF(7.0, 8.0, 2.0, 3.0));
    replacementItem->setPen(QPen(Qt::NoPen));
    viewBox.addItem(replacementItem.get());
    CHECK(checkRange(viewBox.viewRange(), 7.0, 9.0, 8.0, 11.0));

    viewBox.removeItem(replacementItem.get());
    scene.removeItem(&viewBox);
    return true;
}

bool testViewBoxDestructorDetachesNonOwnedItems()
{
    const int before = CountingRectItem::destructionCount;
    auto* item = new CountingRectItem(QRectF(0.0, 0.0, 1.0, 1.0));

    {
        ViewBox viewBox;
        viewBox.addItem(item);
        CHECK(item->parentItem() != nullptr);
    }

    CHECK(CountingRectItem::destructionCount == before);
    CHECK(item->parentItem() == nullptr);
    CHECK(item->scene() == nullptr);

    delete item;
    CHECK(CountingRectItem::destructionCount == before + 1);
    return true;
}

bool testViewBoxUsesGraphicsSceneItemSignals()
{
    GraphicsScene firstScene;
    GraphicsScene scene;
    ViewBox viewBox;
    scene.addItem(&viewBox);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(1.0, 2.0, 3.0, 4.0));
    int addedToScene = 0;
    int removedFromScene = 0;
    int removedFromFirstScene = 0;
    const auto addedConnection = QObject::connect(&scene, &GraphicsScene::sigItemAdded, [&addedToScene, item = item.get()](QGraphicsItem* addedItem) {
        if (addedItem == item) {
            ++addedToScene;
        }
    });
    const auto removedConnection = QObject::connect(&scene, &GraphicsScene::sigItemRemoved, [&removedFromScene, item = item.get()](QGraphicsItem* removedItem) {
        if (removedItem == item) {
            ++removedFromScene;
        }
    });
    const auto firstRemovedConnection = QObject::connect(&firstScene,
                                                         &GraphicsScene::sigItemRemoved,
                                                         [&removedFromFirstScene, item = item.get()](QGraphicsItem* removedItem) {
                                                             if (removedItem == item) {
                                                                 ++removedFromFirstScene;
                                                             }
                                                         });

    viewBox.addItem(item.get());
    CHECK(addedToScene == 1);
    CHECK(item->scene() == &scene);

    viewBox.removeItem(item.get());
    CHECK(removedFromScene == 1);
    CHECK(item->scene() == nullptr);

    firstScene.addItem(item.get());
    CHECK(item->scene() == &firstScene);
    viewBox.addItem(item.get());
    CHECK(removedFromFirstScene == 1);
    CHECK(addedToScene == 2);
    CHECK(item->scene() == &scene);

    viewBox.removeItem(item.get());
    CHECK(removedFromScene == 2);
    CHECK(item->scene() == nullptr);

    QObject::disconnect(firstRemovedConnection);
    QObject::disconnect(removedConnection);
    QObject::disconnect(addedConnection);
    scene.removeItem(&viewBox);
    return true;
}

bool testSceneRenderRefreshesPendingAutoRange()
{
    QGraphicsScene scene;
    ViewBox viewBox;
    scene.addItem(&viewBox);
    viewBox.resize(100.0, 100.0);
    viewBox.setDefaultPadding(0.0);
    viewBox.setRange(QRectF(0.0, 0.0, 1.0, 1.0), 0.0, true, false);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(0.0, 0.0, 1.0, 1.0));
    item->setPen(QPen(Qt::NoPen));
    viewBox.addItem(item.get());
    CHECK(checkRange(viewBox.viewRange(), 0.0, 1.0, 0.0, 1.0));

    item->setRect(QRectF(-4.0, 2.0, 12.0, 9.0));
    QImage image(128, 128, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    CHECK(checkRange(viewBox.viewRange(), -4.0, 8.0, 2.0, 11.0));

    viewBox.removeItem(item.get());
    scene.removeItem(&viewBox);
    return true;
}

bool testSceneRenderAppliesPendingChildTransform()
{
    QGraphicsScene scene;
    ViewBox viewBox;
    scene.addItem(&viewBox);
    viewBox.resize(100.0, 100.0);
    viewBox.setRange(QRectF(0.0, 0.0, 10.0, 10.0), 0.0, true, false);

    auto item = std::make_unique<QGraphicsRectItem>(QRectF(0.0, 0.0, 1.0, 1.0));
    viewBox.addItem(item.get(), true);
    CHECK(nearlyEqual(item->sceneTransform().m11(), 1.0));

    QApplication::processEvents();
    int changedRegions = 0;
    const auto connection = QObject::connect(&scene, &QGraphicsScene::changed, [&changedRegions](const auto&) {
        ++changedRegions;
    });
    viewBox.setRange(QRectF(0.0, 0.0, 20.0, 20.0), 0.0, true, false);
    QApplication::processEvents();
    CHECK(changedRegions > 0);
    QObject::disconnect(connection);

    QImage image(128, 128, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    CHECK(nearlyEqual(item->sceneTransform().m11(), 5.0));
    CHECK(nearlyEqual(item->sceneTransform().m22(), -5.0));

    viewBox.removeItem(item.get());
    scene.removeItem(&viewBox);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testCoordinateMappingAndInversion()) {
        return 1;
    }
    if (!testAspectLockedRangeUsesWidgetGeometry()) {
        return 1;
    }
    if (!testAutorangeAndStateControls()) {
        return 1;
    }
    if (!testDeletedTrackedItemDoesNotLeaveDanglingPointer()) {
        return 1;
    }
    if (!testViewBoxDestructorDetachesNonOwnedItems()) {
        return 1;
    }
    if (!testViewBoxUsesGraphicsSceneItemSignals()) {
        return 1;
    }
    if (!testSceneRenderRefreshesPendingAutoRange()) {
        return 1;
    }
    if (!testSceneRenderAppliesPendingChildTransform()) {
        return 1;
    }

    return 0;
}
