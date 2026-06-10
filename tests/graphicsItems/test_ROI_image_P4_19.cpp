#include <cppqtgraph/core/ArrayView.hpp>
#include <cppqtgraph/graphicsItems/ImageItem.hpp>
#include <cppqtgraph/graphicsItems/ROI.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P4_19_ARTIFACT_DIR
#define CPPQTGRAPH_P4_19_ARTIFACT_DIR "artifacts/P4.19"
#endif

#ifndef CPPQTGRAPH_P4_19_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P4_19_VISUAL_DIFF_DIR "reports/visual-diffs/ROI-image-extraction"
#endif

#ifndef CPPQTGRAPH_P4_19_GPT_REVIEW_REPORT
#define CPPQTGRAPH_P4_19_GPT_REVIEW_REPORT "reports/issues/P4.19/gpt5_vision_review.md"
#endif

#ifndef CPPQTGRAPH_P4_19_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_19_REPOSITORY_REPORT_DIR "reports/issues/P4.19"
#endif

using cppqtgraph::core::ArrayView;
using cppqtgraph::graphicsItems::ImageItem;
using cppqtgraph::graphicsItems::ROI;
using cppqtgraph::graphicsItems::ROIArrayRegion;
using cppqtgraph::graphicsItems::ROIArraySlice;
using cppqtgraph::graphicsItems::ROIAffineSliceParams;

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

bool nearly(double actual, double expected, double tolerance = 1.0e-9)
{
    return std::abs(actual - expected) <= tolerance;
}

bool samePoint(const QPointF& actual, const QPointF& expected, double tolerance = 1.0e-9)
{
    return nearly(actual.x(), expected.x(), tolerance) && nearly(actual.y(), expected.y(), tolerance);
}

QJsonArray pointJson(const QPointF& point)
{
    QJsonArray array;
    array.append(point.x());
    array.append(point.y());
    return array;
}

QJsonArray shapeJson(const std::array<std::size_t, 2>& shape)
{
    QJsonArray array;
    array.append(static_cast<int>(shape[0]));
    array.append(static_cast<int>(shape[1]));
    return array;
}

QJsonArray boundsJson(const ROIArraySlice& slice)
{
    QJsonArray result;
    for (const auto& bound : slice.bounds) {
        QJsonArray axis;
        axis.append(static_cast<int>(bound.first));
        axis.append(static_cast<int>(bound.second));
        result.append(axis);
    }
    return result;
}

QJsonObject affineJson(const ROIAffineSliceParams& params)
{
    QJsonArray vectors;
    vectors.append(pointJson(params.vectors[0]));
    vectors.append(pointJson(params.vectors[1]));
    return QJsonObject{{QStringLiteral("shape"), pointJson(params.shape)},
                       {QStringLiteral("origin"), pointJson(params.origin)},
                       {QStringLiteral("vectors"), vectors}};
}

QJsonArray valuesJson(const ROIArrayRegion& region)
{
    QJsonArray rows;
    for (std::size_t axis0 = 0; axis0 < region.shape[0]; ++axis0) {
        QJsonArray row;
        for (std::size_t axis1 = 0; axis1 < region.shape[1]; ++axis1) {
            row.append(region(axis0, axis1));
        }
        rows.append(row);
    }
    return rows;
}

bool writeJson(const QString& directory, const QString& filename, const QJsonObject& report)
{
    if (!QDir().mkpath(directory)) {
        return false;
    }
    QFile file(directory + QLatin1Char('/') + filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    return true;
}

QImage regionImage(const ROIArrayRegion& region)
{
    QImage image(static_cast<int>(region.shape[1]), static_cast<int>(region.shape[0]), QImage::Format_Grayscale8);
    for (std::size_t row = 0; row < region.shape[0]; ++row) {
        auto* scanLine = image.scanLine(static_cast<int>(row));
        for (std::size_t col = 0; col < region.shape[1]; ++col) {
            const auto value = static_cast<int>(std::lround(std::clamp(region(row, col), 0.0, 255.0)));
            scanLine[col] = static_cast<uchar>(value);
        }
    }
    return image;
}

QImage scaledImage(const QImage& source, int scale)
{
    QImage result(source.width() * scale, source.height() * scale, QImage::Format_ARGB32);
    result.fill(Qt::black);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.scale(scale, scale);
    painter.drawImage(QPointF(0.0, 0.0), source);
    painter.end();
    return result;
}

struct PixelMetrics {
    int changedPixels = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    diff = QImage(reference.size(), QImage::Format_ARGB32);
    diff.fill(Qt::black);
    PixelMetrics metrics;
    std::uint64_t totalDelta = 0;
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected(reference.pixel(x, y));
            const QColor observed(actual.pixel(x, y));
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            if (delta != 0) {
                ++metrics.changedPixels;
                diff.setPixelColor(x, y, QColor(255, 0, 0, 255));
            }
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
            totalDelta += static_cast<std::uint64_t>(delta);
        }
    }
    const auto totalPixels = static_cast<double>(reference.width() * reference.height());
    metrics.meanDelta = static_cast<double>(totalDelta) / totalPixels;
    metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
    return metrics;
}

bool writeVisualArtifacts(const QString& directory, const ROIArrayRegion& referenceRegion, const ROIArrayRegion& actualRegion, QJsonObject& report)
{
    CHECK(QDir().mkpath(directory));
    const QImage reference = scaledImage(regionImage(referenceRegion), 24);
    const QImage actual = scaledImage(regionImage(actualRegion), 24);
    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(reference.save(directory + QStringLiteral("/reference.png")));
    CHECK(actual.save(directory + QStringLiteral("/actual.png")));
    CHECK(diff.save(directory + QStringLiteral("/diff.png")));

    QJsonObject metricsJson{{QStringLiteral("changed_pixels"), metrics.changedPixels},
                            {QStringLiteral("max_delta"), metrics.maxDelta},
                            {QStringLiteral("mean_delta"), metrics.meanDelta},
                            {QStringLiteral("passed"), metrics.passed},
                            {QStringLiteral("reference"), QStringLiteral("reference.png")},
                            {QStringLiteral("actual"), QStringLiteral("actual.png")},
                            {QStringLiteral("diff"), QStringLiteral("diff.png")},
                            {QStringLiteral("gpt5_vision_review"), QJsonObject{{QStringLiteral("required_for_pr"), true},
                                                                                 {QStringLiteral("path"), QStringLiteral("gpt5_vision_review.md")},
                                                                                 {QStringLiteral("generated_by_test"), false}}}};
    CHECK(writeJson(directory, QStringLiteral("metrics.json"), metricsJson));
    report.insert(QStringLiteral("visualMetrics"), metricsJson);
    return metrics.passed;
}

bool hasExternalGptReview(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "missing required external GPT visual review: " << path.toStdString() << '\n';
        return false;
    }
    const QString content = QString::fromUtf8(file.readAll()).toLower();
    if (content.trimmed().isEmpty()) {
        std::cerr << "GPT visual review evidence is empty\n";
        return false;
    }
    const bool citesVisualPacket = content.contains(QStringLiteral("reference.png"))
        && content.contains(QStringLiteral("actual.png")) && content.contains(QStringLiteral("diff.png"))
        && content.contains(QStringLiteral("metrics.json"));
    if (!citesVisualPacket) {
        std::cerr << "GPT visual review evidence must cite the generated image and metrics artifacts\n";
    }
    return citesVisualPacket;
}

bool runChecks()
{
    QGraphicsScene scene;
    std::array<std::uint8_t, 6 * 8> colMajorData{};
    for (std::size_t axis0 = 0; axis0 < 6; ++axis0) {
        for (std::size_t axis1 = 0; axis1 < 8; ++axis1) {
            colMajorData[axis0 * 8 + axis1] = static_cast<std::uint8_t>(axis0 * 20 + axis1);
        }
    }
    const ArrayView<const std::uint8_t, 2> colMajorView(colMajorData.data(), {6, 8});

    ImageItem image;
    image.setImage(colMajorView);
    ROI roi(QPointF(1.0, 2.0), QPointF(3.0, 2.0), 0.0);
    scene.addItem(&image);
    scene.addItem(&roi);

    QJsonObject report;
    QJsonArray checks;
    report.insert(QStringLiteral("issue"), QStringLiteral("P4.19"));
    report.insert(QStringLiteral("reference"), QStringLiteral("pyqtgraph-0.14.0 pyqtgraph/graphicsItems/ROI.py:1079-1265 getArraySlice/getArrayRegion/getAffineSliceParams; pyqtgraph/functions.py:789-904 affineSlice and 925-1084 interpolateArray; tests/graphicsItems/test_ROI.py:14-176"));

    const ROIAffineSliceParams identityParams = roi.getAffineSliceParams(colMajorView.shape(), image);
    CHECK(samePoint(identityParams.shape, QPointF(3.0, 2.0)));
    CHECK(samePoint(identityParams.origin, QPointF(1.0, 2.0)));
    CHECK(samePoint(identityParams.vectors[0], QPointF(1.0, 0.0)));
    CHECK(samePoint(identityParams.vectors[1], QPointF(0.0, 1.0)));
    const std::optional<ROIArraySlice> identitySlice = roi.getArraySlice(colMajorView.shape(), image);
    CHECK(identitySlice.has_value());
    CHECK((identitySlice->bounds[0] == std::pair<std::size_t, std::size_t>{1, 5}));
    CHECK((identitySlice->bounds[1] == std::pair<std::size_t, std::size_t>{2, 5}));
    const ROIArrayRegion identityRegion = roi.getArrayRegion(colMajorView, image);
    CHECK(identityRegion.shape == (std::array<std::size_t, 2>{3, 2}));
    CHECK(nearly(identityRegion(0, 0), 22.0));
    CHECK(nearly(identityRegion(2, 1), 63.0));
    checks.append(QStringLiteral("identity-col-major-affine-slice-region"));

    ROI halfPixel(QPointF(1.5, 2.5), QPointF(2.0, 2.0), 0.0);
    scene.addItem(&halfPixel);
    const ROIArrayRegion halfPixelRegion = halfPixel.getArrayRegion(colMajorView, image);
    CHECK(halfPixelRegion.shape == (std::array<std::size_t, 2>{2, 2}));
    CHECK(nearly(halfPixelRegion(0, 0), 32.5));
    CHECK(nearly(halfPixelRegion(0, 1), 33.5));
    CHECK(nearly(halfPixelRegion(1, 0), 52.5));
    CHECK(nearly(halfPixelRegion(1, 1), 53.5));
    checks.append(QStringLiteral("half-pixel-bilinear-interpolation"));

    ROI rotated(QPointF(3.0, 1.0), QPointF(2.0, 3.0), 90.0);
    scene.addItem(&rotated);
    const ROIAffineSliceParams rotatedParams = rotated.getAffineSliceParams(colMajorView.shape(), image);
    CHECK(samePoint(rotatedParams.shape, QPointF(2.0, 3.0)));
    CHECK(samePoint(rotatedParams.origin, QPointF(3.0, 1.0)));
    CHECK(samePoint(rotatedParams.vectors[0], QPointF(0.0, 1.0)));
    CHECK(samePoint(rotatedParams.vectors[1], QPointF(-1.0, 0.0)));
    const ROIArrayRegion rotatedRegion = rotated.getArrayRegion(colMajorView, image);
    CHECK(rotatedRegion.shape == (std::array<std::size_t, 2>{2, 3}));
    CHECK(nearly(rotatedRegion(0, 0), 61.0));
    CHECK(nearly(rotatedRegion(0, 2), 21.0));
    CHECK(nearly(rotatedRegion(1, 0), 62.0));
    checks.append(QStringLiteral("rotated-roi-affine-region"));

    std::array<std::uint8_t, 8 * 6> rowMajorData{};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 6; ++col) {
            rowMajorData[row * 6 + col] = static_cast<std::uint8_t>(row * 20 + col);
        }
    }
    const ArrayView<const std::uint8_t, 2> rowMajorView(rowMajorData.data(), {8, 6});
    ImageItem rowMajorImage;
    rowMajorImage.setAxisOrder(ImageItem::AxisOrder::RowMajor);
    rowMajorImage.setImage(rowMajorView);
    ROI rowMajorRoi(QPointF(1.0, 2.0), QPointF(3.0, 2.0), 0.0);
    scene.addItem(&rowMajorImage);
    scene.addItem(&rowMajorRoi);
    const ROIAffineSliceParams rowMajorParams = rowMajorRoi.getAffineSliceParams(rowMajorView.shape(), rowMajorImage);
    CHECK(samePoint(rowMajorParams.shape, QPointF(2.0, 3.0)));
    CHECK(samePoint(rowMajorParams.origin, QPointF(2.0, 1.0)));
    CHECK(samePoint(rowMajorParams.vectors[0], QPointF(1.0, 0.0)));
    CHECK(samePoint(rowMajorParams.vectors[1], QPointF(0.0, 1.0)));
    const std::optional<ROIArraySlice> rowMajorSlice = rowMajorRoi.getArraySlice(rowMajorView.shape(), rowMajorImage);
    CHECK(rowMajorSlice.has_value());
    CHECK((rowMajorSlice->bounds[0] == std::pair<std::size_t, std::size_t>{2, 5}));
    CHECK((rowMajorSlice->bounds[1] == std::pair<std::size_t, std::size_t>{1, 5}));
    const ROIArrayRegion rowMajorRegion = rowMajorRoi.getArrayRegion(rowMajorView, rowMajorImage);
    CHECK(rowMajorRegion.shape == (std::array<std::size_t, 2>{2, 3}));
    CHECK(nearly(rowMajorRegion(0, 0), 41.0));
    CHECK(nearly(rowMajorRegion(1, 2), 63.0));
    checks.append(QStringLiteral("row-major-axis-order-transposed-shape-bounds"));

    ImageItem scaledImage;
    scaledImage.setImage(colMajorView);
    scaledImage.setTransform(QTransform::fromScale(1.0, 0.5));
    ROI scaledImageRoi(QPointF(1.0, 1.0), QPointF(2.0, 1.0), 0.0);
    scene.addItem(&scaledImage);
    scene.addItem(&scaledImageRoi);
    const ROIAffineSliceParams scaledImageParams = scaledImageRoi.getAffineSliceParams(colMajorView.shape(), scaledImage);
    CHECK(samePoint(scaledImageParams.shape, QPointF(2.0, 2.0)));
    CHECK(samePoint(scaledImageParams.origin, QPointF(1.0, 2.0)));
    CHECK(samePoint(scaledImageParams.vectors[0], QPointF(1.0, 0.0)));
    CHECK(samePoint(scaledImageParams.vectors[1], QPointF(0.0, 1.0)));
    const ROIArrayRegion scaledImageRegion = scaledImageRoi.getArrayRegion(colMajorView, scaledImage);
    CHECK(scaledImageRegion.shape == (std::array<std::size_t, 2>{2, 2}));
    CHECK(nearly(scaledImageRegion(0, 0), 22.0));
    CHECK(nearly(scaledImageRegion(1, 1), 43.0));
    checks.append(QStringLiteral("transformed-image-affine-region"));

    ROIArrayRegion halfPixelReference;
    halfPixelReference.shape = {2, 2};
    halfPixelReference.values = {32.5, 33.5, 52.5, 53.5};
    const QString visualDir = QStringLiteral(CPPQTGRAPH_P4_19_VISUAL_DIFF_DIR);
    CHECK(writeVisualArtifacts(visualDir, halfPixelReference, halfPixelRegion, report));
    CHECK(hasExternalGptReview(QStringLiteral(CPPQTGRAPH_P4_19_GPT_REVIEW_REPORT)));

    report.insert(QStringLiteral("checks"), checks);
    report.insert(QStringLiteral("identityAffine"), affineJson(identityParams));
    report.insert(QStringLiteral("identitySliceBounds"), boundsJson(identitySlice.value()));
    report.insert(QStringLiteral("identityRegion"), QJsonObject{{QStringLiteral("shape"), shapeJson(identityRegion.shape)},
                                                       {QStringLiteral("values"), valuesJson(identityRegion)}});
    report.insert(QStringLiteral("halfPixelRegion"), QJsonObject{{QStringLiteral("shape"), shapeJson(halfPixelRegion.shape)},
                                                        {QStringLiteral("values"), valuesJson(halfPixelRegion)}});
    report.insert(QStringLiteral("rotatedAffine"), affineJson(rotatedParams));
    report.insert(QStringLiteral("rowMajorAffine"), affineJson(rowMajorParams));
    report.insert(QStringLiteral("rowMajorSliceBounds"), boundsJson(rowMajorSlice.value()));
    report.insert(QStringLiteral("scaledImageAffine"), affineJson(scaledImageParams));

    CHECK(writeJson(QStringLiteral(CPPQTGRAPH_P4_19_ARTIFACT_DIR), QStringLiteral("ROI_image_extraction.json"), report));
    CHECK(writeJson(QStringLiteral(CPPQTGRAPH_P4_19_REPOSITORY_REPORT_DIR), QStringLiteral("ROI_image_extraction.json"), report));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (std::getenv("QT_QPA_PLATFORM") == nullptr) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);
    return runChecks() ? 0 : 1;
}
