#include <cppqtgraph/core/ArrayView.hpp>
#include <cppqtgraph/graphicsItems/BoxplotItem.hpp>
#include <cppqtgraph/graphicsItems/PColorMeshItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P4_04_FIXTURE
#define CPPQTGRAPH_P4_04_FIXTURE "oracle/fixtures/P4_04/boxplot_pcolormesh_oracle.json"
#endif

#ifndef CPPQTGRAPH_P4_04_ARTIFACT_DIR
#define CPPQTGRAPH_P4_04_ARTIFACT_DIR "artifacts/P4.04"
#endif

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

bool nearly(double actual, double expected, double tolerance = 1.0e-9)
{
    return std::abs(actual - expected) <= tolerance;
}

bool throwsInvalidArgument(auto&& callable)
{
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << "unexpected exception: " << error.what() << '\n';
        return false;
    }
    return false;
}

QJsonObject readFixture()
{
    QFile file(QStringLiteral(CPPQTGRAPH_P4_04_FIXTURE));
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("could not open P4.04 oracle fixture");
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object();
}

QJsonArray pairJson(const std::pair<double, double>& values)
{
    QJsonArray out;
    out.append(values.first);
    out.append(values.second);
    return out;
}

QJsonArray rectJson(const QRectF& rect)
{
    QJsonArray out;
    out.append(rect.left());
    out.append(rect.top());
    out.append(rect.right());
    out.append(rect.bottom());
    return out;
}

QImage blankImage(int width = 240, int height = 180)
{
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

QTransform viewTransform(const QRectF& view, const QSize& size)
{
    QTransform transform;
    transform.translate(0.0, size.height() - 1.0);
    transform.scale(size.width() / view.width(), -size.height() / view.height());
    transform.translate(-view.left(), -view.top());
    return transform;
}

QJsonObject imageDiffMetrics(const QImage& actual, const QImage& reference, QImage* diff)
{
    std::uint64_t changed = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    for (int y = 0; y < actual.height(); ++y) {
        for (int x = 0; x < actual.width(); ++x) {
            const QColor a = actual.pixelColor(x, y);
            const QColor r = reference.pixelColor(x, y);
            const int delta = std::abs(a.red() - r.red()) + std::abs(a.green() - r.green()) + std::abs(a.blue() - r.blue())
                + std::abs(a.alpha() - r.alpha());
            if (delta != 0) {
                ++changed;
            }
            totalDelta += static_cast<std::uint64_t>(delta);
            maxDelta = std::max(maxDelta, delta);
            diff->setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0, 255) : QColor(255, 0, 0, 255));
        }
    }
    return QJsonObject{{QStringLiteral("changedPixels"), QString::number(changed)},
                       {QStringLiteral("totalDelta"), QString::number(totalDelta)},
                       {QStringLiteral("maxDelta"), maxDelta}};
}

bool exerciseBoxplot(QJsonObject& report)
{
    using cppqtgraph::graphicsItems::BoxplotItem;
    using cppqtgraph::graphicsItems::BoxplotItemOptions;

    const std::vector<std::vector<double>> data{{1.0, 2.0, 3.0, 4.0, 100.0}, {-5.0, 0.0, 5.0}};
    BoxplotItem item;
    item.setData(data);
    const auto stats = item.statistics();
    CHECK(stats.size() == 2);
    CHECK(nearly(stats[0].p25, 2.0));
    CHECK(nearly(stats[0].median, 3.0));
    CHECK(nearly(stats[0].p75, 4.0));
    CHECK(nearly(stats[0].lowerWhisker, 1.0));
    CHECK(nearly(stats[0].upperWhisker, 4.0));
    CHECK(stats[0].outliers.size() == 1 && nearly(stats[0].outliers[0], 100.0));
    CHECK(nearly(stats[1].p25, -2.5));
    CHECK(nearly(stats[1].median, 0.0));
    CHECK(nearly(stats[1].p75, 2.5));
    CHECK(nearly(stats[1].lowerWhisker, -5.0));
    CHECK(nearly(stats[1].upperWhisker, 5.0));
    CHECK(stats[1].outliers.empty());
    CHECK(nearly(item.width(), 0.8));
    CHECK(nearly(item.dataBounds(0).first, -0.4));
    CHECK(nearly(item.dataBounds(0).second, 1.4));
    CHECK(nearly(item.dataBounds(1).first, -5.0));
    CHECK(nearly(item.dataBounds(1).second, 100.0));
    const qreal boxPadding = item.pixelPadding();
    CHECK(boxPadding > 0.0);
    const QRectF boxBoundingRect = item.boundingRect();
    CHECK(nearly(boxBoundingRect.left(), item.dataBounds(0).first - boxPadding));
    CHECK(nearly(boxBoundingRect.right(), item.dataBounds(0).second + boxPadding));
    CHECK(nearly(boxBoundingRect.top(), item.dataBounds(1).first - boxPadding));
    CHECK(nearly(boxBoundingRect.bottom(), item.dataBounds(1).second + boxPadding));

    BoxplotItemOptions noOutlier;
    noOutlier.data = data;
    noOutlier.outlier = false;
    BoxplotItem noOutlierItem(noOutlier);
    CHECK(nearly(noOutlierItem.dataBounds(1).first, -5.0));
    CHECK(nearly(noOutlierItem.dataBounds(1).second, 5.0));
    CHECK(noOutlierItem.statistics()[0].outliers.empty());

    BoxplotItemOptions horizontal;
    horizontal.data = data;
    horizontal.locAsX = false;
    BoxplotItem horizontalItem(horizontal);
    CHECK(nearly(horizontalItem.dataBounds(0).first, -5.0));
    CHECK(nearly(horizontalItem.dataBounds(0).second, 100.0));
    CHECK(nearly(horizontalItem.dataBounds(1).first, -0.4));
    CHECK(nearly(horizontalItem.dataBounds(1).second, 1.4));

    BoxplotItemOptions customLoc;
    customLoc.data = data;
    customLoc.loc = {10.0, 20.0};
    BoxplotItem customLocItem(customLoc);
    CHECK(nearly(customLocItem.dataBounds(0).first, 9.6));
    CHECK(nearly(customLocItem.dataBounds(0).second, 20.4));

    BoxplotItem minMaxItem;
    minMaxItem.setWhiskerFunc([](std::span<const double> values) {
        return std::make_pair(*std::min_element(values.begin(), values.end()), *std::max_element(values.begin(), values.end()));
    });
    minMaxItem.setData(data);
    CHECK(nearly(minMaxItem.statistics()[0].lowerWhisker, 1.0));
    CHECK(nearly(minMaxItem.statistics()[0].upperWhisker, 100.0));
    CHECK(minMaxItem.statistics()[0].outliers.empty());

    BoxplotItemOptions hidden;
    hidden.data = data;
    hidden.pen = QPen(Qt::NoPen);
    hidden.brush = QBrush(Qt::NoBrush);
    hidden.medianPen = QPen(Qt::NoPen);
    BoxplotItem hiddenItem(hidden);
    CHECK(nearly(hiddenItem.width(), 0.0));
    CHECK(throwsInvalidArgument([&] { hiddenItem.setData(std::vector<std::vector<double>>{{}}); }));

    report.insert(QStringLiteral("boxplot"), QJsonObject{{QStringLiteral("boundsX"), pairJson(item.dataBounds(0))},
                                                 {QStringLiteral("boundsY"), pairJson(item.dataBounds(1))},
                                                 {QStringLiteral("boundingRect"), rectJson(item.boundingRect())},
                                                 {QStringLiteral("statsCount"), static_cast<int>(stats.size())}});
    return true;
}

std::vector<QColor> fiveColorLut()
{
    return {QColor(0, 0, 0), QColor(64, 0, 0), QColor(128, 0, 0), QColor(192, 0, 0), QColor(255, 0, 0)};
}

bool exercisePColorMesh(QJsonObject& report)
{
    using cppqtgraph::core::ArrayView;
    using cppqtgraph::graphicsItems::PColorMeshItem;

    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::array<double, 6> z{{0.0, 1.0, 2.0, nan, 4.0, 4.0}};
    PColorMeshItem item(ArrayView<const double, 2>(z.data(), {2, 3}));
    item.setLookupTable(fiveColorLut());
    item.setLevels({0.0, 4.0});
    CHECK(item.zRows() == 2 && item.zCols() == 3);
    CHECK(nearly(item.width(), 2.0));
    CHECK(nearly(item.height(), 3.0));
    CHECK(nearly(item.dataBounds(0).first, 0.0));
    CHECK(nearly(item.dataBounds(0).second, 2.0));
    CHECK(nearly(item.dataBounds(1).first, 0.0));
    CHECK(nearly(item.dataBounds(1).second, 3.0));
    const auto cells = item.renderedCells();
    CHECK(cells.size() == 5);
    CHECK(cells[0].row == 0 && cells[0].column == 0 && cells[0].colorIndex == 0);
    CHECK(cells[1].row == 0 && cells[1].column == 1 && cells[1].colorIndex == 1);
    CHECK(cells[2].row == 0 && cells[2].column == 2 && cells[2].colorIndex == 2);
    CHECK(cells[2].polygon[0] == QPointF(0.0, 2.0));
    CHECK(cells[2].polygon[2] == QPointF(1.0, 3.0));
    CHECK(cells[3].row == 1 && cells[3].column == 1 && cells[3].colorIndex == 4);
    CHECK(cells[3].polygon.size() == 4);
    CHECK(cells[3].polygon[0] == QPointF(1.0, 1.0));
    CHECK(cells[3].polygon[1] == QPointF(2.0, 1.0));
    CHECK(cells[3].polygon[2] == QPointF(2.0, 2.0));
    CHECK(cells[3].polygon[3] == QPointF(1.0, 2.0));

    QPen thickEdge(Qt::black);
    thickEdge.setWidthF(4.0);
    item.setEdgePen(thickEdge);
    CHECK(nearly(item.pixelPadding(), 2.0));
    CHECK(nearly(item.boundingRect().left(), -2.0));
    CHECK(nearly(item.boundingRect().right(), 4.0));
    CHECK(nearly(item.boundingRect().top(), -2.0));
    CHECK(nearly(item.boundingRect().bottom(), 5.0));
    QGraphicsScene scene;
    auto* sceneItem = new PColorMeshItem(ArrayView<const double, 2>(z.data(), {2, 3}));
    scene.addItem(sceneItem);
    sceneItem->setEdgePen(thickEdge);
    CHECK(nearly(scene.itemsBoundingRect().left(), sceneItem->boundingRect().left()));
    CHECK(nearly(scene.itemsBoundingRect().right(), sceneItem->boundingRect().right()));

    item.setLevels({2.0, 2.0});
    const auto equalLevelCells = item.renderedCells();
    CHECK(equalLevelCells[2].colorIndex == 0);
    CHECK(equalLevelCells[3].colorIndex == 4);

    std::array<double, 9> x{{0.0, 0.5, 1.5, 2.0, 2.5, 3.5, 4.0, 4.5, 6.0}};
    std::array<double, 9> y{{0.0, 1.0, 2.0, 0.2, 1.3, 2.6, 0.4, 1.6, 3.0}};
    std::array<double, 4> z2{{10.0, 20.0, 30.0, 40.0}};
    PColorMeshItem explicitItem;
    explicitItem.setLookupTable({QColor(0, 0, 0), QColor(0, 85, 0), QColor(0, 170, 0), QColor(0, 255, 0)});
    explicitItem.setData(ArrayView<const double, 2>(x.data(), {3, 3}), ArrayView<const double, 2>(y.data(), {3, 3}),
        ArrayView<const double, 2>(z2.data(), {2, 2}));
    explicitItem.setLevels({10.0, 40.0});
    CHECK(nearly(explicitItem.dataBounds(0).first, 0.0));
    CHECK(nearly(explicitItem.dataBounds(0).second, 6.0));
    CHECK(nearly(explicitItem.dataBounds(1).first, 0.0));
    CHECK(nearly(explicitItem.dataBounds(1).second, 3.0));
    const auto explicitCells = explicitItem.renderedCells();
    CHECK(explicitCells.size() == 4);
    for (std::size_t index = 0; index < explicitCells.size(); ++index) {
        CHECK(explicitCells[index].colorIndex == index);
    }
    CHECK(explicitCells[1].polygon[0] == QPointF(0.5, 1.0));
    CHECK(explicitCells[1].polygon[1] == QPointF(2.5, 1.3));
    CHECK(explicitCells[1].polygon[2] == QPointF(3.5, 2.6));
    CHECK(explicitCells[1].polygon[3] == QPointF(1.5, 2.0));
    CHECK(throwsInvalidArgument([&] {
        explicitItem.setData(ArrayView<const double, 2>(x.data(), {3, 3}), ArrayView<const double, 2>(y.data(), {3, 3}),
            ArrayView<const double, 2>(z2.data(), {1, 2}));
    }));

    report.insert(QStringLiteral("pcolormesh"), QJsonObject{{QStringLiteral("zOnlyCellCount"), static_cast<int>(cells.size())},
                                                    {QStringLiteral("zOnlyBoundsX"), pairJson(item.dataBounds(0))},
                                                    {QStringLiteral("zOnlyBoundsY"), pairJson(item.dataBounds(1))},
                                                    {QStringLiteral("explicitBoundsX"), pairJson(explicitItem.dataBounds(0))},
                                                    {QStringLiteral("explicitBoundsY"), pairJson(explicitItem.dataBounds(1))}});
    return true;
}

bool renderVisualArtifacts(const QString& artifactDir, QJsonObject& report)
{
    using cppqtgraph::core::ArrayView;
    using cppqtgraph::graphicsItems::BoxplotItem;
    using cppqtgraph::graphicsItems::PColorMeshItem;

    QDir().mkpath(artifactDir);

    BoxplotItem box;
    box.setData(std::vector<std::vector<double>>{{1.0, 2.0, 3.0, 4.0, 100.0}, {-5.0, 0.0, 5.0}});
    QImage boxActual = blankImage();
    QPainter boxPainter(&boxActual);
    boxPainter.setTransform(viewTransform(QRectF(-1.0, -10.0, 4.0, 120.0), boxActual.size()));
    box.paint(&boxPainter, nullptr, nullptr);
    boxPainter.end();
    CHECK(boxActual.save(artifactDir + QStringLiteral("/boxplot_actual.png")));

    std::array<double, 4> z{{10.0, 20.0, 30.0, 40.0}};
    PColorMeshItem mesh(ArrayView<const double, 2>(z.data(), {2, 2}));
    mesh.setLookupTable({QColor(0, 0, 0, 255), QColor(0, 85, 0, 255), QColor(0, 170, 0, 255), QColor(0, 255, 0, 255)});
    mesh.setLevels({10.0, 40.0});
    QImage meshActual = blankImage(200, 200);
    QPainter meshPainter(&meshActual);
    meshPainter.setTransform(viewTransform(QRectF(0.0, 0.0, 2.0, 2.0), meshActual.size()));
    mesh.paint(&meshPainter, nullptr, nullptr);
    meshPainter.end();

    QImage meshReference = blankImage(200, 200);
    QPainter referencePainter(&meshReference);
    referencePainter.setTransform(viewTransform(QRectF(0.0, 0.0, 2.0, 2.0), meshReference.size()));
    referencePainter.setPen(Qt::NoPen);
    for (const auto& cell : mesh.renderedCells()) {
        referencePainter.setBrush(cell.color);
        referencePainter.drawConvexPolygon(cell.polygon);
    }
    referencePainter.end();

    QImage diff = blankImage(200, 200);
    const QJsonObject metrics = imageDiffMetrics(meshActual, meshReference, &diff);
    CHECK(metrics.value(QStringLiteral("changedPixels")).toString() == QStringLiteral("0"));
    CHECK(meshActual.save(artifactDir + QStringLiteral("/pcolormesh_actual.png")));
    CHECK(meshReference.save(artifactDir + QStringLiteral("/pcolormesh_reference.png")));
    CHECK(diff.save(artifactDir + QStringLiteral("/pcolormesh_diff.png")));
    report.insert(QStringLiteral("visual"), QJsonObject{{QStringLiteral("artifactDir"), artifactDir}, {QStringLiteral("pcolormeshMetrics"), metrics}});
    return true;
}

bool writeReport(const QString& artifactDir, const QJsonObject& report)
{
    QDir().mkpath(artifactDir);
    QFile file(artifactDir + QStringLiteral("/boxplot_pcolormesh_report.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << "failed to write P4.04 report\n";
        return false;
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    ApplicationGuard app(argc, argv);

    QJsonObject report;
    try {
        const QJsonObject fixture = readFixture();
        report.insert(QStringLiteral("issue"), fixture.value(QStringLiteral("issue")));
        report.insert(QStringLiteral("reference"), fixture.value(QStringLiteral("reference")));
        report.insert(QStringLiteral("tolerance"), fixture.value(QStringLiteral("tolerance")));
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    const QString artifactDir = QStringLiteral(CPPQTGRAPH_P4_04_ARTIFACT_DIR);
    if (!exerciseBoxplot(report) || !exercisePColorMesh(report) || !renderVisualArtifacts(artifactDir, report) || !writeReport(artifactDir, report)) {
        return 1;
    }
    return 0;
}
