#include <pyqtgraph/core/ArrayView.hpp>
#include <pyqtgraph/graphicsItems/IsocurveItem.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#ifndef PYQTGRAPH_CPP_P4_07_FIXTURE
#define PYQTGRAPH_CPP_P4_07_FIXTURE "oracle/fixtures/P4_07/isocurve_oracle.json"
#endif

#ifndef PYQTGRAPH_CPP_P4_07_ARTIFACT_DIR
#define PYQTGRAPH_CPP_P4_07_ARTIFACT_DIR "artifacts/P4.07"
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

QJsonObject readFixture()
{
    QFile file(QStringLiteral(PYQTGRAPH_CPP_P4_07_FIXTURE));
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("could not open P4.07 oracle fixture");
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::vector<double> dataFromJson(const QJsonArray& rows, std::size_t* rowCount, std::size_t* colCount)
{
    *rowCount = static_cast<std::size_t>(rows.size());
    *colCount = rows.isEmpty() ? 0U : static_cast<std::size_t>(rows.first().toArray().size());
    std::vector<double> values;
    values.reserve((*rowCount) * (*colCount));
    for (const QJsonValue& rowValue : rows) {
        const QJsonArray row = rowValue.toArray();
        for (const QJsonValue& value : row) {
            values.push_back(value.toDouble());
        }
    }
    return values;
}

std::vector<std::vector<QPointF>> linesFromJson(const QJsonArray& lines)
{
    std::vector<std::vector<QPointF>> out;
    for (const QJsonValue& lineValue : lines) {
        std::vector<QPointF> line;
        for (const QJsonValue& pointValue : lineValue.toArray()) {
            const QJsonArray point = pointValue.toArray();
            line.emplace_back(point[0].toDouble(), point[1].toDouble());
        }
        out.push_back(std::move(line));
    }
    return out;
}

std::vector<std::pair<long long, long long>> canonicalLine(std::vector<QPointF> line)
{
    constexpr double tolerance = 1.0e-9;
    auto quantize = [](double value) { return static_cast<long long>(std::llround(value * 1.0e9)); };
    const bool closed = line.size() > 1 && nearly(line.front().x(), line.back().x(), tolerance) && nearly(line.front().y(), line.back().y(), tolerance);
    if (closed) {
        line.pop_back();
    }
    std::vector<std::vector<std::pair<long long, long long>>> candidates;
    auto appendCandidate = [&](const std::vector<QPointF>& candidate) {
        std::vector<std::pair<long long, long long>> quantized;
        for (const QPointF& point : candidate) {
            quantized.emplace_back(quantize(point.x()), quantize(point.y()));
        }
        if (closed && !quantized.empty()) {
            quantized.push_back(quantized.front());
        }
        candidates.push_back(std::move(quantized));
    };
    if (closed && !line.empty()) {
        for (std::size_t shift = 0; shift < line.size(); ++shift) {
            std::vector<QPointF> rotated;
            for (std::size_t i = 0; i < line.size(); ++i) {
                rotated.push_back(line[(shift + i) % line.size()]);
            }
            appendCandidate(rotated);
            std::reverse(rotated.begin(), rotated.end());
            appendCandidate(rotated);
        }
    } else {
        appendCandidate(line);
        std::reverse(line.begin(), line.end());
        appendCandidate(line);
    }
    return *std::min_element(candidates.begin(), candidates.end());
}

std::vector<std::vector<std::pair<long long, long long>>> canonicalLines(const std::vector<std::vector<QPointF>>& lines)
{
    std::vector<std::vector<std::pair<long long, long long>>> out;
    out.reserve(lines.size());
    for (const auto& line : lines) {
        out.push_back(canonicalLine(line));
    }
    std::sort(out.begin(), out.end());
    return out;
}

QPainterPath pathFromLines(const std::vector<std::vector<QPointF>>& lines)
{
    QPainterPath path;
    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }
        path.moveTo(line.front());
        for (std::size_t i = 1; i < line.size(); ++i) {
            path.lineTo(line[i]);
        }
    }
    return path;
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

bool exerciseCases(const QJsonObject& fixture, QJsonObject& report)
{
    using pyqtgraph::core::ArrayView;
    using pyqtgraph::graphicsItems::IsocurveItem;

    QJsonArray caseReports;
    for (const QJsonValue& caseValue : fixture.value(QStringLiteral("cases")).toArray()) {
        const QJsonObject caseObject = caseValue.toObject();
        std::size_t rows = 0;
        std::size_t cols = 0;
        std::vector<double> data = dataFromJson(caseObject.value(QStringLiteral("data")).toArray(), &rows, &cols);
        IsocurveItem item(ArrayView<const double, 2>(data.data(), {rows, cols}), caseObject.value(QStringLiteral("level")).toDouble());
        if (caseObject.value(QStringLiteral("axis_order")).toString() == QStringLiteral("row-major")) {
            item.setAxisOrder(IsocurveItem::AxisOrder::RowMajor);
        }
        const auto expectedLines = linesFromJson(caseObject.value(QStringLiteral("lines")).toArray());
        CHECK(canonicalLines(item.isocurves()) == canonicalLines(expectedLines));
        CHECK(canonicalLines(item.pathLines()) == canonicalLines(expectedLines));
        const QJsonArray expectedBounds = caseObject.value(QStringLiteral("bounds")).toArray();
        const QRectF rect = item.boundingRect();
        CHECK(nearly(rect.left(), expectedBounds[0].toDouble()));
        CHECK(nearly(rect.top(), expectedBounds[1].toDouble()));
        CHECK(nearly(rect.right(), expectedBounds[2].toDouble()));
        CHECK(nearly(rect.bottom(), expectedBounds[3].toDouble()));
        CHECK(canonicalLines(item.isocurves()) == canonicalLines(linesFromJson(caseObject.value(QStringLiteral("lines")).toArray())));
        caseReports.append(QJsonObject{{QStringLiteral("name"), caseObject.value(QStringLiteral("name")).toString()},
                                       {QStringLiteral("lineCount"), static_cast<int>(item.isocurves().size())},
                                       {QStringLiteral("boundingRect"), rectJson(rect)}});
    }

    IsocurveItem empty;
    CHECK(!empty.hasData());
    CHECK(empty.boundingRect().isNull());
    CHECK(empty.isocurves().empty());

    const double values[] = {0.0, 1.0, 1.0, 2.0};
    IsocurveItem mutableItem(ArrayView<const double, 2>(values, {2, 2}), 1.0);
    const QRectF firstRect = mutableItem.boundingRect();
    mutableItem.setLevel(0.5);
    const QRectF secondRect = mutableItem.boundingRect();
    CHECK(!nearly(firstRect.left(), secondRect.left()) || !nearly(firstRect.top(), secondRect.top()) || !nearly(firstRect.right(), secondRect.right())
        || !nearly(firstRect.bottom(), secondRect.bottom()));
    mutableItem.clear();
    CHECK(!mutableItem.hasData());
    CHECK(mutableItem.boundingRect().isNull());

    report.insert(QStringLiteral("cases"), caseReports);
    return true;
}

bool renderVisual(const QJsonObject& fixture, const QString& artifactDir, QJsonObject& report)
{
    using pyqtgraph::core::ArrayView;
    using pyqtgraph::graphicsItems::IsocurveItem;

    const QString visualCase = fixture.value(QStringLiteral("visual_case")).toString();
    QJsonObject selected;
    for (const QJsonValue& caseValue : fixture.value(QStringLiteral("cases")).toArray()) {
        const QJsonObject caseObject = caseValue.toObject();
        if (caseObject.value(QStringLiteral("name")).toString() == visualCase) {
            selected = caseObject;
        }
    }
    CHECK(!selected.isEmpty());
    std::size_t rows = 0;
    std::size_t cols = 0;
    std::vector<double> data = dataFromJson(selected.value(QStringLiteral("data")).toArray(), &rows, &cols);
    IsocurveItem item(ArrayView<const double, 2>(data.data(), {rows, cols}), selected.value(QStringLiteral("level")).toDouble(), QPen(QColor(0, 0, 0), 1.0));
    const auto expectedLines = linesFromJson(selected.value(QStringLiteral("lines")).toArray());

    QDir().mkpath(artifactDir);
    QImage actual(180, 180, QImage::Format_ARGB32_Premultiplied);
    actual.fill(Qt::transparent);
    QPainter actualPainter(&actual);
    actualPainter.setRenderHint(QPainter::Antialiasing, false);
    actualPainter.setTransform(viewTransform(QRectF(0.5, 0.5, 2.0, 2.0), actual.size()));
    item.paint(&actualPainter, nullptr, nullptr);
    actualPainter.end();

    QImage reference(actual.size(), actual.format());
    reference.fill(Qt::transparent);
    QPainter referencePainter(&reference);
    referencePainter.setRenderHint(QPainter::Antialiasing, false);
    referencePainter.setTransform(viewTransform(QRectF(0.5, 0.5, 2.0, 2.0), reference.size()));
    referencePainter.setPen(QPen(QColor(0, 0, 0), 1.0));
    referencePainter.drawPath(pathFromLines(expectedLines));
    referencePainter.end();

    QImage diff(actual.size(), actual.format());
    diff.fill(Qt::transparent);
    const QJsonObject metrics = imageDiffMetrics(actual, reference, &diff);
    CHECK(metrics.value(QStringLiteral("changedPixels")).toString() == QStringLiteral("0"));
    CHECK(metrics.value(QStringLiteral("maxDelta")).toInt() == 0);
    CHECK(actual.save(artifactDir + QStringLiteral("/isocurve_actual.png")));
    CHECK(reference.save(artifactDir + QStringLiteral("/isocurve_reference.png")));
    CHECK(diff.save(artifactDir + QStringLiteral("/isocurve_diff.png")));

    IsocurveItem brushedItem(ArrayView<const double, 2>(data.data(), {rows, cols}), selected.value(QStringLiteral("level")).toDouble(), QPen(Qt::NoPen));
    const QColor brushColor(31, 119, 180, 255);
    brushedItem.setBrush(QBrush(brushColor));
    QImage brushActual(actual.size(), actual.format());
    brushActual.fill(Qt::transparent);
    QPainter brushActualPainter(&brushActual);
    brushActualPainter.setRenderHint(QPainter::Antialiasing, false);
    brushActualPainter.setTransform(viewTransform(QRectF(0.5, 0.5, 2.0, 2.0), brushActual.size()));
    brushedItem.paint(&brushActualPainter, nullptr, nullptr);
    brushActualPainter.end();

    QImage brushReference(actual.size(), actual.format());
    brushReference.fill(Qt::transparent);
    QPainter brushReferencePainter(&brushReference);
    brushReferencePainter.setRenderHint(QPainter::Antialiasing, false);
    brushReferencePainter.setTransform(viewTransform(QRectF(0.5, 0.5, 2.0, 2.0), brushReference.size()));
    brushReferencePainter.setPen(QPen(Qt::NoPen));
    brushReferencePainter.setBrush(QBrush(brushColor));
    brushReferencePainter.drawPath(pathFromLines(expectedLines));
    brushReferencePainter.end();

    QImage brushDiff(actual.size(), actual.format());
    brushDiff.fill(Qt::transparent);
    const QJsonObject brushMetrics = imageDiffMetrics(brushActual, brushReference, &brushDiff);
    CHECK(brushMetrics.value(QStringLiteral("changedPixels")).toString() == QStringLiteral("0"));
    CHECK(brushMetrics.value(QStringLiteral("maxDelta")).toInt() == 0);
    std::uint64_t filledPixels = 0;
    for (int y = 0; y < brushActual.height(); ++y) {
        for (int x = 0; x < brushActual.width(); ++x) {
            if (brushActual.pixelColor(x, y).alpha() > 0) {
                ++filledPixels;
            }
        }
    }
    CHECK(filledPixels > 1000);
    CHECK(brushActual.save(artifactDir + QStringLiteral("/isocurve_brush_actual.png")));
    CHECK(brushReference.save(artifactDir + QStringLiteral("/isocurve_brush_reference.png")));
    CHECK(brushDiff.save(artifactDir + QStringLiteral("/isocurve_brush_diff.png")));

    report.insert(QStringLiteral("visual"), QJsonObject{{QStringLiteral("case"), visualCase},
                                           {QStringLiteral("artifactDir"), artifactDir},
                                           {QStringLiteral("metrics"), metrics},
                                           {QStringLiteral("brushMetrics"), brushMetrics},
                                           {QStringLiteral("brushFilledPixels"), QString::number(filledPixels)}});
    return true;
}

bool writeReport(const QString& artifactDir, const QJsonObject& report)
{
    QDir().mkpath(artifactDir);
    QFile file(artifactDir + QStringLiteral("/isocurve_report.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << "failed to write P4.07 report\n";
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
    QJsonObject fixture;
    try {
        fixture = readFixture();
        report.insert(QStringLiteral("issue"), fixture.value(QStringLiteral("issue")));
        report.insert(QStringLiteral("reference"), fixture.value(QStringLiteral("reference")));
        report.insert(QStringLiteral("tolerance"), fixture.value(QStringLiteral("tolerance")));
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    const QString artifactDir = QStringLiteral(PYQTGRAPH_CPP_P4_07_ARTIFACT_DIR);
    if (!exerciseCases(fixture, report) || !renderVisual(fixture, artifactDir, report) || !writeReport(artifactDir, report)) {
        return 1;
    }
    return 0;
}
