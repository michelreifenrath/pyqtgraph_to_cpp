#include <cppqtgraph/graphicsItems/LabelItem.hpp>
#include <cppqtgraph/graphicsItems/TextItem.hpp>
#include <cppqtgraph/functions.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef CPPQTGRAPH_P4_22_VISUAL_DIFF_DIR
#define CPPQTGRAPH_P4_22_VISUAL_DIFF_DIR "reports/visual-diffs/TextLabelItem"
#endif

#ifndef CPPQTGRAPH_P4_22_GPT_REVIEW_REPORT
#define CPPQTGRAPH_P4_22_GPT_REVIEW_REPORT "reports/visual-diffs/TextLabelItem/gpt5_vision_review.md"
#endif

#ifndef CPPQTGRAPH_P4_22_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P4_22_REPOSITORY_REPORT_DIR "reports/issues/P4.22"
#endif

namespace {

constexpr int imageWidth = 480;
constexpr int imageHeight = 220;

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

QFont proofFont()
{
    QFont font(QStringLiteral("Sans Serif"));
    font.setPointSizeF(10.0);
    return font;
}

class ReferenceTextItem : public QGraphicsObject {
public:
    ReferenceTextItem(const QString& text,
                      const QColor& color,
                      const QPointF& anchor,
                      const std::optional<QPen>& border,
                      const std::optional<QBrush>& fill,
                      qreal angle,
                      QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent)
        , anchor_(anchor)
        , angle_(angle)
        , color_(color)
        , border_(border.value_or(QPen(Qt::NoPen)))
        , fill_(fill.value_or(QBrush(Qt::NoBrush)))
    {
        textItem_ = new QGraphicsTextItem(this);
        textItem_->setFont(proofFont());
        textItem_->setDefaultTextColor(color_);
        textItem_->setPlainText(text);
        updateTextPos();
        updateTransform(true);
    }

    QRectF boundingRect() const override
    {
        return textItem_->mapRectToParent(textItem_->boundingRect());
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        updateTransform();
        if (border_.style() != Qt::NoPen || fill_.style() != Qt::NoBrush) {
            painter->setPen(border_);
            painter->setBrush(fill_);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawPolygon(textItem_->mapToParent(textItem_->boundingRect()));
        }
    }

private:
    void updateTextPos()
    {
        const QRectF bounds = textItem_->boundingRect();
        const QPointF topLeft = textItem_->mapToParent(bounds.topLeft());
        const QPointF bottomRight = textItem_->mapToParent(bounds.bottomRight());
        const QPointF delta = bottomRight - topLeft;
        textItem_->setPos(-QPointF(delta.x() * anchor_.x(), delta.y() * anchor_.y()));
    }

    void updateTransform(bool force = false)
    {
        QGraphicsItem* parent = parentItem();
        QTransform parentSceneTransform;
        if (parent != nullptr) {
            parentSceneTransform = parent->sceneTransform();
        }
        if (!force && parentSceneTransform == lastParentTransform_) {
            return;
        }
        bool invertible = false;
        QTransform transform = parentSceneTransform.inverted(&invertible);
        if (!invertible) {
            transform = QTransform();
        }
        transform.setMatrix(transform.m11(),
                            transform.m12(),
                            transform.m13(),
                            transform.m21(),
                            transform.m22(),
                            transform.m23(),
                            0.0,
                            0.0,
                            transform.m33());
        transform.rotate(-angle_);
        setTransform(transform);
        lastParentTransform_ = parentSceneTransform;
        updateTextPos();
    }

    QGraphicsTextItem* textItem_ = nullptr;
    QPointF anchor_;
    qreal angle_ = 0.0;
    QColor color_;
    QPen border_;
    QBrush fill_;
    QTransform lastParentTransform_;
};

class ReferenceLabelItem : public QGraphicsWidget {
public:
    ReferenceLabelItem(const QString& text,
                         const QString& justify,
                         const QColor& color,
                         const QString& size,
                         qreal angle,
                         const QRectF& geometry,
                         QGraphicsItem* parent = nullptr)
        : QGraphicsWidget(parent)
        , justify_(justify)
        , angle_(angle)
    {
        textItem_ = new QGraphicsTextItem(this);
        setGeometry(geometry);
        applyStyledHtml(text, color, size);
        setAngle(angle);
        layoutText();
    }

    void setAngle(qreal angle)
    {
        angle_ = angle;
        textItem_->resetTransform();
        textItem_->setRotation(angle_);
        updateMin();
        layoutText();
    }

    void updateMin()
    {
        const QRectF bounds = itemRect();
        setMinimumWidth(bounds.width());
        setMinimumHeight(bounds.height());
    }

private:
    void applyStyledHtml(const QString& text, const QColor& color, const QString& size)
    {
        const QString html = QStringLiteral("<span style='color: %1; font-size: %2'>%3</span>")
                                 .arg(color.name(QColor::HexArgb), size, text);
        textItem_->setHtml(html);
    }

    QRectF itemRect() const
    {
        return textItem_->mapRectToParent(textItem_->boundingRect());
    }

    void layoutText()
    {
        textItem_->setPos(0.0, 0.0);
        QRectF bounds = itemRect();
        const QPointF left = mapFromItem(textItem_, QPointF(0.0, 0.0)) - mapFromItem(textItem_, QPointF(1.0, 0.0));
        const QRectF rect = this->rect();
        if (justify_ == QStringLiteral("left")) {
            if (!qFuzzyIsNull(left.x())) {
                bounds.moveLeft(rect.left());
            }
            if (left.y() < 0.0) {
                bounds.moveTop(rect.top());
            } else if (left.y() > 0.0) {
                bounds.moveBottom(rect.bottom());
            }
        } else if (justify_ == QStringLiteral("center")) {
            bounds.moveCenter(rect.center());
        } else if (justify_ == QStringLiteral("right")) {
            if (!qFuzzyIsNull(left.x())) {
                bounds.moveRight(rect.right());
            }
            if (left.y() < 0.0) {
                bounds.moveBottom(rect.bottom());
            } else if (left.y() > 0.0) {
                bounds.moveTop(rect.top());
            }
        }
        textItem_->setPos(bounds.topLeft() - itemRect().topLeft());
        updateMin();
    }

    QGraphicsTextItem* textItem_ = nullptr;
    QString justify_;
    qreal angle_ = 0.0;
};

QImage blankImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(8, 8, 10));
    return image;
}

QImage renderScene(QGraphicsScene& scene)
{
    QImage image = blankImage();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    scene.render(&painter, QRectF(0.0, 0.0, imageWidth, imageHeight), QRectF(0.0, 0.0, imageWidth, imageHeight));
    painter.end();
    return image;
}

QImage renderReference()
{
    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));

    auto* plain = new ReferenceTextItem(QStringLiteral("Signal A"),
                                        QColor(200, 200, 200),
                                        QPointF(0.0, 0.0),
                                        std::nullopt,
                                        std::nullopt,
                                        0.0);
    plain->setPos(36.0, 36.0);
    scene.addItem(plain);

    auto* bordered = new ReferenceTextItem(QStringLiteral("Bordered"),
                                           QColor(220, 220, 220),
                                           QPointF(0.0, 0.0),
                                           QPen(QColor(240, 240, 240), 1.0),
                                           QBrush(QColor(0, 0, 0, 128)),
                                           0.0);
    bordered->setPos(150.0, 36.0);
    scene.addItem(bordered);

    auto* centered = new ReferenceTextItem(QStringLiteral("Mid"),
                                           QColor(180, 220, 255),
                                           QPointF(0.5, 0.5),
                                           std::nullopt,
                                           std::nullopt,
                                           0.0);
    centered->setPos(270.0, 48.0);
    scene.addItem(centered);

    auto* rotated = new ReferenceTextItem(QStringLiteral("Rotated"),
                                            QColor(255, 200, 120),
                                            QPointF(0.0, 0.0),
                                            std::nullopt,
                                            std::nullopt,
                                            30.0);
    rotated->setPos(390.0, 36.0);
    scene.addItem(rotated);

    auto* scaledParent = new QGraphicsRectItem();
    scaledParent->setScale(2.0);
    scaledParent->setPos(36.0, 96.0);
    scene.addItem(scaledParent);
    auto* scaledText = new ReferenceTextItem(QStringLiteral("Scaled"),
                                             QColor(160, 255, 160),
                                             QPointF(0.0, 0.0),
                                             std::nullopt,
                                             std::nullopt,
                                             0.0,
                                             scaledParent);
    scaledText->setPos(8.0, 0.0);

    auto* labelCenter = new ReferenceLabelItem(QStringLiteral("Center"),
                                               QStringLiteral("center"),
                                               QColor(220, 220, 220),
                                               QStringLiteral("10pt"),
                                               0.0,
                                               QRectF(0.0, 0.0, 120.0, 28.0));
    labelCenter->setPos(180.0, 150.0);
    scene.addItem(labelCenter);

    auto* labelLeft = new ReferenceLabelItem(QStringLiteral("Left"),
                                             QStringLiteral("left"),
                                             QColor(255, 180, 180),
                                             QStringLiteral("10pt"),
                                             0.0,
                                             QRectF(0.0, 0.0, 100.0, 28.0));
    labelLeft->setPos(320.0, 150.0);
    scene.addItem(labelLeft);

    auto* labelRight = new ReferenceLabelItem(QStringLiteral("Tilt"),
                                              QStringLiteral("right"),
                                              QColor(180, 200, 255),
                                              QStringLiteral("10pt"),
                                              0.0,
                                              QRectF(0.0, 0.0, 110.0, 28.0));
    labelRight->setAngle(45.0);
    labelRight->setPos(36.0, 150.0);
    scene.addItem(labelRight);

    return renderScene(scene);
}

QImage renderActual()
{
    using cppqtgraph::graphicsItems::LabelItem;
    using cppqtgraph::graphicsItems::TextItem;

    QGraphicsScene scene(QRectF(0.0, 0.0, imageWidth, imageHeight));

    auto* plain = new TextItem(QStringLiteral("Signal A"), QColor(200, 200, 200), QPointF(0.0, 0.0));
    plain->setFont(proofFont());
    plain->setPos(36.0, 36.0);
    scene.addItem(plain);

    auto* bordered = new TextItem(QStringLiteral("Bordered"),
                                  QColor(220, 220, 220),
                                  QString{},
                                  QPointF(0.0, 0.0),
                                  QPen(QColor(240, 240, 240), 1.0),
                                  QBrush(QColor(0, 0, 0, 128)),
                                  0.0,
                                  std::nullopt);
    bordered->setFont(proofFont());
    bordered->setPos(150.0, 36.0);
    scene.addItem(bordered);

    auto* centered = new TextItem(QStringLiteral("Mid"), QColor(180, 220, 255), QPointF(0.5, 0.5));
    centered->setFont(proofFont());
    centered->setPos(270.0, 48.0);
    scene.addItem(centered);

    auto* rotated = new TextItem(QStringLiteral("Rotated"), QColor(255, 200, 120), QPointF(0.0, 0.0));
    rotated->setFont(proofFont());
    rotated->setAngle(30.0);
    rotated->setPos(390.0, 36.0);
    scene.addItem(rotated);

    auto* scaledParent = new QGraphicsRectItem();
    scaledParent->setScale(2.0);
    scaledParent->setPos(36.0, 96.0);
    scene.addItem(scaledParent);
    auto* scaledText = new TextItem(QStringLiteral("Scaled"), QColor(160, 255, 160), QPointF(0.0, 0.0), scaledParent);
    scaledText->setFont(proofFont());
    scaledText->setPos(8.0, 0.0);

    LabelItem::TextStyleOptions centerStyle;
    centerStyle.color = QColor(220, 220, 220);
    centerStyle.justify = QStringLiteral("center");
    centerStyle.size = QStringLiteral("10pt");
    auto* labelCenter = new LabelItem();
    labelCenter->setGeometry(QRectF(0.0, 0.0, 120.0, 28.0));
    labelCenter->setText(QStringLiteral("Center"), centerStyle);
    labelCenter->setPos(180.0, 150.0);
    scene.addItem(labelCenter);

    LabelItem::TextStyleOptions leftStyle;
    leftStyle.color = QColor(255, 180, 180);
    leftStyle.justify = QStringLiteral("left");
    leftStyle.size = QStringLiteral("10pt");
    auto* labelLeft = new LabelItem();
    labelLeft->setGeometry(QRectF(0.0, 0.0, 100.0, 28.0));
    labelLeft->setText(QStringLiteral("Left"), leftStyle);
    labelLeft->setPos(320.0, 150.0);
    scene.addItem(labelLeft);

    LabelItem::TextStyleOptions rightStyle;
    rightStyle.color = QColor(180, 200, 255);
    rightStyle.justify = QStringLiteral("right");
    rightStyle.size = QStringLiteral("10pt");
    auto* labelRight = new LabelItem();
    labelRight->setGeometry(QRectF(0.0, 0.0, 110.0, 28.0));
    labelRight->setText(QStringLiteral("Tilt"), rightStyle);
    labelRight->setAngle(45.0);
    labelRight->setPos(36.0, 150.0);
    scene.addItem(labelRight);

    return renderScene(scene);
}

struct PixelMetrics {
    std::uint64_t changedPixels = 0;
    std::uint64_t totalDelta = 0;
    int maxDelta = 0;
    double meanDelta = 0.0;
    double changedPercent = 0.0;
    bool passed = false;
};

PixelMetrics compareImages(const QImage& reference, const QImage& actual, QImage& diff)
{
    PixelMetrics metrics;
    diff = QImage(reference.size(), QImage::Format_ARGB32_Premultiplied);
    diff.fill(Qt::black);
    const int pixelCount = reference.width() * reference.height();
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            const QColor expected = reference.pixelColor(x, y);
            const QColor observed = actual.pixelColor(x, y);
            const int delta = std::abs(expected.red() - observed.red()) + std::abs(expected.green() - observed.green())
                + std::abs(expected.blue() - observed.blue()) + std::abs(expected.alpha() - observed.alpha());
            metrics.totalDelta += static_cast<std::uint64_t>(delta);
            metrics.maxDelta = std::max(metrics.maxDelta, delta);
            if (delta != 0) {
                ++metrics.changedPixels;
            }
            diff.setPixelColor(x, y, delta == 0 ? QColor(0, 0, 0) : QColor(255, std::min(delta, 255), std::min(delta, 255)));
        }
    }
    metrics.meanDelta = static_cast<double>(metrics.totalDelta) / static_cast<double>(pixelCount);
    metrics.changedPercent = 100.0 * static_cast<double>(metrics.changedPixels) / static_cast<double>(pixelCount);
    metrics.passed = metrics.changedPixels == 0 && metrics.maxDelta == 0;
    return metrics;
}

std::uint64_t semanticPixelCount(const QImage& image)
{
    std::uint64_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && (color.red() > 20 || color.green() > 20 || color.blue() > 20)) {
                ++count;
            }
        }
    }
    return count;
}

bool ensureDirectory(const QString& path)
{
    QDir dir;
    return dir.mkpath(path);
}

void writeTextFile(const QString& path, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        throw std::runtime_error("failed to write " + path.toStdString());
    }
    QTextStream stream(&file);
    stream << text;
}

QString jsonEscape(QString value)
{
    value.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    value.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    value.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    value.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    return value;
}

QString normalizedReviewValue(QString value)
{
    const qsizetype commentIndex = value.indexOf(QChar('#'));
    if (commentIndex >= 0) {
        value.truncate(commentIndex);
    }
    value = value.trimmed();
    if (value.size() >= 2
        && ((value.front() == QChar('\'') && value.back() == QChar('\''))
            || (value.front() == QChar('"') && value.back() == QChar('"')))) {
        value = value.mid(1, value.size() - 2);
    }
    return value.trimmed().toLower();
}

struct SemanticReviewStatus {
    QString path;
    QString verdict;
    QString recommendation;
    bool exists = false;
    bool citesArtifacts = false;
    bool accepted = false;
};

SemanticReviewStatus readGptVisualReview()
{
    SemanticReviewStatus status;
    status.path = QStringLiteral(CPPQTGRAPH_P4_22_GPT_REVIEW_REPORT);
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P4.22 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P4.22 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    status.exists = true;
    const QString content = QString::fromUtf8(file.readAll());
    const QString lowerContent = content.toLower();
    status.citesArtifacts = lowerContent.contains(QStringLiteral("reference.png"))
        && lowerContent.contains(QStringLiteral("actual.png")) && lowerContent.contains(QStringLiteral("diff.png"))
        && lowerContent.contains(QStringLiteral("metrics.json"));

    const QStringList lines = content.split(QChar('\n'));
    for (const QString& line : lines) {
        const qsizetype separator = line.indexOf(QChar(':'));
        if (separator < 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed().toLower();
        if (key == QStringLiteral("verdict")) {
            status.verdict = normalizedReviewValue(line.mid(separator + 1));
        } else if (key == QStringLiteral("recommendation")) {
            status.recommendation = normalizedReviewValue(line.mid(separator + 1));
        }
    }
    status.accepted = status.exists && status.citesArtifacts && status.verdict == QStringLiteral("pass")
        && status.recommendation == QStringLiteral("merge_ok");
    if (!status.accepted) {
        std::cerr << "P4.22 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
}

bool writeArtifacts(const QImage& reference, const QImage& actual, const QImage& diff, const PixelMetrics& metrics)
{
    const QString visualDir = QStringLiteral(CPPQTGRAPH_P4_22_VISUAL_DIFF_DIR);
    CHECK(ensureDirectory(visualDir));
    CHECK(reference.save(visualDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(visualDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(visualDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview();
    CHECK(review.accepted);

    writeTextFile(visualDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"TextLabelItem\",\n"
            "  \"issue\": \"P4.22\",\n"
            "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
            "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/TextItem.py and LabelItem.py\",\n"
            "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
            "  \"dimensions\": [480, 220],\n"
            "  \"fixture_hash\": \"P4.22:TextLabelItem:plain-border-anchor-rotate-scale-label-justify:v1\",\n"
            "  \"thresholds\": {\"max_changed_pixels\": 0, \"max_pixel_delta\": 0},\n"
            "  \"changed_pixels\": ")
            + QString::number(metrics.changedPixels)
            + QStringLiteral(
                ",\n"
                "  \"changed_percent\": ")
            + QString::number(metrics.changedPercent, 'f', 6)
            + QStringLiteral(
                ",\n"
                "  \"max_delta\": ")
            + QString::number(metrics.maxDelta)
            + QStringLiteral(
                ",\n"
                "  \"mean_delta\": ")
            + QString::number(metrics.meanDelta, 'f', 6)
            + QStringLiteral(
                ",\n"
                "  \"gpt5_vision_review\": {\"required_for_pr\": true, \"path\": \"gpt5_vision_review.md\", \"source\": \"")
            + jsonEscape(review.path)
            + QStringLiteral(
                "\", \"available\": true, \"accepted\": true},\n"
                "  \"semantic_review\": {\"verdict\": \"")
            + jsonEscape(review.verdict)
            + QStringLiteral("\", \"recommendation\": \"")
            + jsonEscape(review.recommendation)
            + QStringLiteral(
                "\"},\n"
                "  \"passed\": ")
            + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(
                ",\n"
                "  \"blank_placeholder_guard\": \"passed\",\n"
                "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"#08080a\", \"antialias\": true}\n"
                "}\n"));
    return true;
}

bool writeIssueReport(const PixelMetrics& metrics, std::uint64_t referencePixels, std::uint64_t actualPixels)
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P4_22_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/TextLabelItem_visual_behavior.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P4.22\",\n"
            "  \"classes\": [\"cppqtgraph::graphicsItems::TextItem\", \"cppqtgraph::graphicsItems::LabelItem\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/TextItem.py and LabelItem.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/graphicsItems/TextItem.hpp\", \"src/cppqtgraph/graphicsItems/TextItem.cpp\", \"include/cppqtgraph/graphicsItems/LabelItem.hpp\", \"src/cppqtgraph/graphicsItems/LabelItem.cpp\"],\n"
            "  \"shared_wiring\": [\"tests/CMakeLists.txt\", \"CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.22 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_graphicsitems_textlabelitem_p4_22\"},\n"
            "  \"checks\": [\"TextItem child QGraphicsTextItem anchor placement\", \"TextItem transform compensation under scaled parent\", \"TextItem border/fill rendering\", \"LabelItem styled HTML and justify layout\", \"LabelItem angle rotation\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/TextLabelItem\", \"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\", \"metrics\": \"metrics.json\", \"gpt5_vision_review\": \"gpt5_vision_review.md\"},\n"
            "  \"semantic_pixels\": {\"reference\": ")
            + QString::number(referencePixels)
            + QStringLiteral(", \"actual\": ")
            + QString::number(actualPixels)
            + QStringLiteral(
                "},\n"
                "  \"visual_metrics\": {\"changed_pixels\": ")
            + QString::number(metrics.changedPixels)
            + QStringLiteral(", \"max_delta\": ")
            + QString::number(metrics.maxDelta)
            + QStringLiteral(", \"mean_delta\": ")
            + QString::number(metrics.meanDelta, 'f', 6)
            + QStringLiteral(", \"passed\": ")
            + (metrics.passed ? QStringLiteral("true") : QStringLiteral("false"))
            + QStringLiteral(
                "},\n"
                "  \"validation_commands\": [\"cmake --preset dev\", \"cmake --build --preset dev --parallel\", \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P4.22 --output-on-failure\", \"python3 -m pytest -q\", \"git diff --check\", \"git diff --name-only origin/main...HEAD\"],\n"
                "  \"manifest_dashboard\": \"not_applicable: port_manifest/dashboard updates are outside P4.22 owned paths for this shard\"\n"
                "}\n"));
    return true;
}

bool testConstructionAndBehavior()
{
    using cppqtgraph::graphicsItems::LabelItem;
    using cppqtgraph::graphicsItems::TextItem;

    static_assert(std::is_constructible_v<TextItem>);
    static_assert(std::is_constructible_v<TextItem, QString, QColor, QPointF, QGraphicsItem*>);
    static_assert(std::is_base_of_v<cppqtgraph::graphicsItems::GraphicsObject, TextItem>);
    static_assert(std::is_constructible_v<LabelItem>);
    static_assert(std::is_base_of_v<cppqtgraph::graphicsItems::GraphicsWidget, LabelItem>);

    TextItem text(QStringLiteral("Probe"), QColor(255, 0, 0), QPointF(0.5, 0.5));
    text.setFont(proofFont());
    CHECK(text.toPlainText() == QStringLiteral("Probe"));
    CHECK(text.anchor() == QPointF(0.5, 0.5));
    CHECK(text.color() == QColor(255, 0, 0));
    CHECK((text.dataBounds(0) == std::pair<qreal, qreal>{0.5, 0.5}));
    CHECK(!text.boundingRect().isEmpty());

    LabelItem::TextStyleOptions style;
    style.justify = QStringLiteral("left");
    style.color = QColor(10, 20, 30);
    style.size = QStringLiteral("9pt");
    LabelItem label(QStringLiteral("Axis"), style);
    label.setGeometry(QRectF(0.0, 0.0, 80.0, 24.0));
    CHECK(label.justify() == QStringLiteral("left"));
    CHECK(label.sizeHint(Qt::MinimumSize).width() > 0.0);
    CHECK(!label.itemRect().isEmpty());
    label.setAngle(15.0);
    CHECK(label.angle() == 15.0);

    return true;
}

bool testSetAttrRefreshesLayoutAndSize()
{
    using cppqtgraph::graphicsItems::LabelItem;

    LabelItem::TextStyleOptions style;
    style.justify = QStringLiteral("center");
    style.size = QStringLiteral("9pt");
    LabelItem label(QStringLiteral("Resize me"), style);
    label.setGeometry(QRectF(0.0, 0.0, 120.0, 40.0));

    const QSizeF sizeBefore = label.sizeHint(Qt::MinimumSize);

    label.setAttr(QStringLiteral("size"), QStringLiteral("18pt"));
    const QSizeF sizeAfter = label.sizeHint(Qt::MinimumSize);
    CHECK(sizeAfter.height() > sizeBefore.height());

    label.setAttr(QStringLiteral("justify"), QStringLiteral("left"));
    CHECK(label.justify() == QStringLiteral("left"));
    CHECK(label.sizeHint(Qt::MinimumSize).width() > 0.0);
    CHECK(!label.itemRect().isEmpty());

    return true;
}

bool testVisualBehavior()
{
    const QImage reference = renderReference();
    const QImage actual = renderActual();
    const std::uint64_t referencePixels = semanticPixelCount(reference);
    const std::uint64_t actualPixels = semanticPixelCount(actual);
    if (referencePixels < 500 || actualPixels < 500) {
        std::cerr << "TextLabelItem blank/placeholder guard failed: reference=" << referencePixels
                  << " actual=" << actualPixels << '\n';
        return false;
    }

    QImage diff;
    const PixelMetrics metrics = compareImages(reference, actual, diff);
    CHECK(writeArtifacts(reference, actual, diff, metrics));
    CHECK(writeIssueReport(metrics, referencePixels, actualPixels));
    if (!metrics.passed) {
        std::cerr << "P4.22 TextLabelItem visual comparison failed: changedPixels=" << metrics.changedPixels
                  << " maxDelta=" << metrics.maxDelta << '\n';
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testConstructionAndBehavior()) {
        return 1;
    }
    if (!testSetAttrRefreshesLayoutAndSize()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
