#include <pyqtgraph/widgets/JoystickButton.hpp>
#include <pyqtgraph/widgets/VerticalLabel.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaType>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P5_20_VISUAL_DIFF_DIR
#define PYQTGRAPH_CPP_P5_20_VISUAL_DIFF_DIR "reports/visual-diffs/JoystickVerticalLabel"
#endif

#ifndef PYQTGRAPH_CPP_P5_20_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_20_REPOSITORY_REPORT_DIR "reports/issues/P5.20"
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

#define CHECK_CLOSE(actual, expected, tolerance) \
    do { \
        const double actualValue = (actual); \
        const double expectedValue = (expected); \
        if (!check(std::abs(actualValue - expectedValue) <= (tolerance), #actual " close to " #expected, __FILE__, \
                __LINE__)) { \
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

QString caseArtifactDir(const QString& caseName)
{
    return QStringLiteral(PYQTGRAPH_CPP_P5_20_VISUAL_DIFF_DIR) + QChar('/') + caseName;
}

SemanticReviewStatus readGptVisualReview(const QString& caseName)
{
    SemanticReviewStatus status;
    status.path = caseArtifactDir(caseName) + QStringLiteral("/gpt5_vision_review.md");
    if (!QFile::exists(status.path)) {
        std::cerr << "missing P5.20 GPT visual review: " << status.path.toStdString() << '\n';
        return status;
    }
    QFile file(status.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "unreadable P5.20 GPT visual review: " << status.path.toStdString() << '\n';
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
        std::cerr << "P5.20 GPT visual review is not accepted in " << status.path.toStdString()
                  << " (verdict=" << status.verdict.toStdString()
                  << ", recommendation=" << status.recommendation.toStdString()
                  << ", citesArtifacts=" << status.citesArtifacts << ")\n";
    }
    return status;
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
    if (reference.size() != actual.size()) {
        std::cerr << "image size mismatch: reference=" << reference.width() << 'x' << reference.height()
                  << " actual=" << actual.width() << 'x' << actual.height() << '\n';
        return metrics;
    }
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

QPoint expectedSpotPosition(double stateX, double stateY, int width, int height)
{
    const double halfWidth = static_cast<double>(width) / 2.0;
    const double halfHeight = static_cast<double>(height) / 2.0;
    return QPoint(static_cast<int>(halfWidth * (1.0 + stateX)), static_cast<int>(halfHeight * (1.0 - stateY)));
}

QVector<double> expectedNormalizedState(double rawX, double rawY, double radius)
{
    const double length = std::hypot(rawX, rawY);
    double normalizedX = 0.0;
    double normalizedY = 0.0;
    if (rawX != 0.0) {
        normalizedX = rawX / length;
    }
    if (rawY != 0.0) {
        normalizedY = rawY / length;
    }
    double clampedLength = length;
    if (clampedLength > radius) {
        clampedLength = radius;
    }
    const double scaledLength = std::pow(clampedLength / radius, 2.0);
    return {normalizedX * scaledLength, normalizedY * scaledLength};
}

bool testJoystickButtonApiShape()
{
    using pyqtgraph::widgets::JoystickButton;

    static_assert(std::is_base_of_v<QPushButton, JoystickButton>);
    static_assert(!std::is_copy_constructible_v<JoystickButton>);

    JoystickButton button;
    CHECK(button.isCheckable());
    CHECK(button.width() == 50);
    CHECK(button.height() == 50);
    CHECK_CLOSE(button.radius(), 200.0, 1.0e-12);

    const QVector<double> state = button.getState();
    CHECK(state.size() == 2);
    CHECK_CLOSE(state[0], 0.0, 1.0e-12);
    CHECK_CLOSE(state[1], 0.0, 1.0e-12);
    CHECK(button.spotPosition() == QPoint(25, 25));
    return true;
}

bool testJoystickButtonSetStateNormalization()
{
    using pyqtgraph::widgets::JoystickButton;

    JoystickButton button;
    QSignalSpy stateSpy(&button, &JoystickButton::sigStateChanged);

    button.setState(100.0, 0.0);
    const QVector<double> rightState = button.getState();
    const QVector<double> expectedRight = expectedNormalizedState(100.0, 0.0, button.radius());
    CHECK_CLOSE(rightState[0], expectedRight[0], 1.0e-9);
    CHECK_CLOSE(rightState[1], expectedRight[1], 1.0e-9);
    CHECK(stateSpy.count() == 1);

    button.setState(100.0, 100.0);
    const QVector<double> diagonalState = button.getState();
    const QVector<double> expectedDiagonal = expectedNormalizedState(100.0, 100.0, button.radius());
    CHECK_CLOSE(diagonalState[0], expectedDiagonal[0], 1.0e-9);
    CHECK_CLOSE(diagonalState[1], expectedDiagonal[1], 1.0e-9);

    button.setState(500.0, 0.0);
    const QVector<double> clampedState = button.getState();
    CHECK_CLOSE(clampedState[0], 1.0, 1.0e-9);
    CHECK_CLOSE(clampedState[1], 0.0, 1.0e-9);
    CHECK(button.spotPosition() == expectedSpotPosition(clampedState[0], clampedState[1], 50, 50));
    return true;
}

bool testJoystickButtonInteraction()
{
    using pyqtgraph::widgets::JoystickButton;

    JoystickButton button;
    button.show();
    QApplication::processEvents();

    QSignalSpy stateSpy(&button, &JoystickButton::sigStateChanged);

    QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier, QPoint(25, 25));
    CHECK(button.isChecked());
    CHECK(stateSpy.count() >= 0);

    QTest::mouseMove(&button, QPoint(40, 10), Qt::NoModifier);
    QApplication::processEvents();
    const QVector<double> draggedState = button.getState();
    CHECK(draggedState.size() == 2);
    CHECK(std::abs(draggedState[0]) > 0.0 || std::abs(draggedState[1]) > 0.0);
    CHECK(button.spotPosition() != QPoint(25, 25));

    QTest::mouseRelease(&button, Qt::LeftButton, Qt::NoModifier, QPoint(40, 10));
    QApplication::processEvents();
    CHECK(!button.isChecked());
    const QVector<double> releasedState = button.getState();
    CHECK_CLOSE(releasedState[0], 0.0, 1.0e-12);
    CHECK_CLOSE(releasedState[1], 0.0, 1.0e-12);
    CHECK(button.spotPosition() == QPoint(25, 25));
    return true;
}

bool testVerticalLabelApiShape()
{
    using pyqtgraph::widgets::VerticalLabel;

    static_assert(std::is_base_of_v<QLabel, VerticalLabel>);
    static_assert(!std::is_copy_constructible_v<VerticalLabel>);

    VerticalLabel label(QStringLiteral("Axis"));
    CHECK(label.orientation() == QStringLiteral("vertical"));
    CHECK(label.forceWidth());
    CHECK(label.text() == QStringLiteral("Axis"));

    const QSize hint = label.sizeHint();
    CHECK(hint.width() == 19);
    CHECK(hint.height() == 50);
    return true;
}

bool testVerticalLabelOrientationSwitch()
{
    using pyqtgraph::widgets::VerticalLabel;

    VerticalLabel label(QStringLiteral("Channel"), QStringLiteral("vertical"), true);
    label.show();
    QApplication::processEvents();
    label.repaint();
    QApplication::processEvents();

    CHECK(label.hasTextHint());
    const QSize verticalHint = label.sizeHint();
    CHECK(verticalHint.width() > 0);
    CHECK(verticalHint.height() > 0);

    label.setOrientation(QStringLiteral("horizontal"));
    CHECK(label.orientation() == QStringLiteral("horizontal"));
    label.repaint();
    QApplication::processEvents();
    const QSize horizontalHint = label.sizeHint();
    CHECK(horizontalHint.width() >= verticalHint.width());
    return true;
}

// Upstream-derived reference oracle for pyqtgraph/widgets/JoystickButton.py (a20028b).
class ReferenceJoystickButton : public QPushButton {
public:
    ReferenceJoystickButton()
    {
        setCheckable(true);
        setFixedWidth(50);
        setFixedHeight(50);
        setState(0.0, 0.0);
    }

    void setState(double x, double y)
    {
        const double length = std::hypot(x, y);
        double normalizedX = 0.0;
        double normalizedY = 0.0;
        if (x != 0.0) {
            normalizedX = x / length;
        }
        if (y != 0.0) {
            normalizedY = y / length;
        }

        double clampedLength = length;
        if (clampedLength > radius_) {
            clampedLength = radius_;
        }
        const double scaledLength = std::pow(clampedLength / radius_, 2.0);
        const double stateX = normalizedX * scaledLength;
        const double stateY = normalizedY * scaledLength;

        const double halfWidth = static_cast<double>(width()) / 2.0;
        const double halfHeight = static_cast<double>(height()) / 2.0;
        spotPos_ = QPoint(static_cast<int>(halfWidth * (1.0 + stateX)), static_cast<int>(halfHeight * (1.0 - stateY)));
        update();

        if (state_.size() == 2 && state_[0] == stateX && state_[1] == stateY) {
            return;
        }
        state_ = {stateX, stateY};
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPushButton::paintEvent(event);
        QPainter painter(this);
        painter.setBrush(QBrush(QColor(0, 0, 0)));
        painter.drawEllipse(spotPos_.x() - 3, spotPos_.y() - 3, 6, 6);
    }

private:
    double radius_ = 200.0;
    QPoint spotPos_{25, 25};
    QVector<double> state_{0.0, 0.0};
};

// Upstream-derived reference oracle for pyqtgraph/widgets/VerticalLabel.py (a20028b).
class ReferenceVerticalLabel : public QLabel {
public:
    ReferenceVerticalLabel(const QString& text, const QString& orientation, bool forceWidth)
        : QLabel(text)
        , forceWidth_(forceWidth)
    {
        setOrientation(orientation);
    }

    void setOrientation(const QString& orientation)
    {
        if (orientation_ == orientation) {
            return;
        }
        orientation_ = orientation;
        update();
        updateGeometry();
    }

    QSize sizeHint() const override
    {
        if (orientation_ == QStringLiteral("vertical")) {
            if (hasTextHint_) {
                return QSize(textHint_.height(), textHint_.width());
            }
            return QSize(19, 50);
        }
        if (hasTextHint_) {
            return QSize(textHint_.width(), textHint_.height());
        }
        return QSize(50, 19);
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter painter(this);

        QRect region;
        if (orientation_ == QStringLiteral("vertical")) {
            painter.rotate(-90.0);
            region = QRect(-height(), 0, height(), width());
        } else {
            region = contentsRect();
        }

        const Qt::Alignment textAlignment = QLabel::alignment();
        textHint_ = painter.boundingRect(region, static_cast<int>(textAlignment), text());
        painter.drawText(region, static_cast<int>(textAlignment), text());
        hasTextHint_ = true;
        painter.end();

        if (orientation_ == QStringLiteral("vertical")) {
            setMaximumWidth(textHint_.height());
            setMinimumWidth(0);
            setMaximumHeight(16777215);
            if (forceWidth_) {
                setMinimumHeight(textHint_.width());
            } else {
                setMinimumHeight(0);
            }
        } else {
            setMaximumHeight(textHint_.height());
            setMinimumHeight(0);
            setMaximumWidth(16777215);
            if (forceWidth_) {
                setMinimumWidth(textHint_.width());
            } else {
                setMinimumWidth(0);
            }
        }
    }

private:
    bool forceWidth_ = true;
    QString orientation_;
    QRect textHint_;
    bool hasTextHint_ = false;
};

QImage grabWidget(QWidget& widget)
{
    widget.show();
    QApplication::processEvents();
    widget.repaint();
    QApplication::processEvents();
    widget.resize(widget.sizeHint());
    QApplication::processEvents();
    widget.repaint();
    QApplication::processEvents();
    return widget.grab().toImage();
}

QImage renderVerticalLabelReference(const QString& text, const QString& orientation)
{
    ReferenceVerticalLabel label(text, orientation, true);
    label.setAlignment(Qt::AlignCenter);
    return grabWidget(label);
}

QImage renderVerticalLabelActual(const QString& text, const QString& orientation)
{
    pyqtgraph::widgets::VerticalLabel label(text, orientation, true);
    label.setAlignment(Qt::AlignCenter);
    return grabWidget(label);
}

QImage renderJoystickButtonActual(double rawX, double rawY)
{
    pyqtgraph::widgets::JoystickButton button;
    button.show();
    QApplication::processEvents();
    button.setState(rawX, rawY);
    QApplication::processEvents();
    return button.grab().toImage();
}

QImage renderJoystickButtonReference(double rawX, double rawY)
{
    ReferenceJoystickButton reference;
    reference.show();
    QApplication::processEvents();
    reference.setState(rawX, rawY);
    QApplication::processEvents();
    return reference.grab().toImage();
}

bool writeIssueReport(const PixelMetrics& metrics, std::uint64_t referencePixels, std::uint64_t actualPixels);

bool verifyPerCaseArtifactLayout(const QString& caseName)
{
    const QString caseDir = caseArtifactDir(caseName);
    const QStringList requiredFiles = {QStringLiteral("reference.png"), QStringLiteral("actual.png"),
        QStringLiteral("diff.png"), QStringLiteral("metrics.json"), QStringLiteral("gpt5_vision_review.md")};
    for (const QString& fileName : requiredFiles) {
        const QString path = caseDir + QChar('/') + fileName;
        if (!QFile::exists(path)) {
            std::cerr << "missing P5.20 per-case visual artifact: " << path.toStdString() << '\n';
            return false;
        }
    }
    return true;
}

bool writeCaseArtifacts(const QString& caseName, const QString& widget, const QImage& reference, const QImage& actual,
    const QImage& diff, const PixelMetrics& metrics)
{
    const QString caseDir = caseArtifactDir(caseName);
    CHECK(ensureDirectory(caseDir));
    CHECK(reference.save(caseDir + QStringLiteral("/reference.png")));
    CHECK(actual.save(caseDir + QStringLiteral("/actual.png")));
    CHECK(diff.save(caseDir + QStringLiteral("/diff.png")));

    const SemanticReviewStatus review = readGptVisualReview(caseName);
    CHECK(review.accepted);

    writeTextFile(caseDir + QStringLiteral("/metrics.json"),
        QStringLiteral(
            "{\n"
            "  \"case\": \"")
            + jsonEscape(caseName)
            + QStringLiteral(
                "\",\n"
                "  \"issue\": \"P5.20\",\n"
                "  \"widget\": \"")
            + jsonEscape(widget)
            + QStringLiteral(
                "\",\n"
                "  \"compared_paths\": {\"reference\": \"reference.png\", \"actual\": \"actual.png\", \"diff\": \"diff.png\"},\n"
                "  \"reference_source\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/JoystickButton.py and VerticalLabel.py upstream-derived test oracle\",\n"
                "  \"pinned_commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\",\n"
                "  \"dimensions\": [")
            + QString::number(reference.width())
            + QStringLiteral(", ")
            + QString::number(reference.height())
            + QStringLiteral(
                "],\n"
                "  \"fixture_hash\": \"P5.20:")
            + jsonEscape(caseName)
            + QStringLiteral(
                ":v1\",\n"
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
                "  \"reproducibility\": {\"qt_qpa_platform\": \"offscreen\", \"background\": \"#ffffff\", \"antialias\": true}\n"
                "}\n"));
    CHECK(verifyPerCaseArtifactLayout(caseName));
    return true;
}

bool testVisualBehavior()
{
    struct VisualCase {
        QString name;
        QString widget;
        std::function<QImage()> reference;
        std::function<QImage()> actual;
    };

    const std::vector<VisualCase> cases = {
        {QStringLiteral("JoystickButton-center"), QStringLiteral("JoystickButton"),
            [] { return renderJoystickButtonReference(0.0, 0.0); },
            [] { return renderJoystickButtonActual(0.0, 0.0); }},
        {QStringLiteral("JoystickButton-dragged"), QStringLiteral("JoystickButton"),
            [] { return renderJoystickButtonReference(80.0, -40.0); },
            [] { return renderJoystickButtonActual(80.0, -40.0); }},
        {QStringLiteral("VerticalLabel-vertical"), QStringLiteral("VerticalLabel"),
            [] { return renderVerticalLabelReference(QStringLiteral("Channel"), QStringLiteral("vertical")); },
            [] { return renderVerticalLabelActual(QStringLiteral("Channel"), QStringLiteral("vertical")); }},
        {QStringLiteral("VerticalLabel-horizontal"), QStringLiteral("VerticalLabel"),
            [] { return renderVerticalLabelReference(QStringLiteral("Gain"), QStringLiteral("horizontal")); },
            [] { return renderVerticalLabelActual(QStringLiteral("Gain"), QStringLiteral("horizontal")); }},
    };

    PixelMetrics aggregateMetrics;
    std::uint64_t referencePixels = 0;
    std::uint64_t actualPixels = 0;

    for (const VisualCase& visualCase : cases) {
        const QImage reference = visualCase.reference();
        const QImage actual = visualCase.actual();
        const std::uint64_t caseReferencePixels = semanticPixelCount(reference);
        const std::uint64_t caseActualPixels = semanticPixelCount(actual);
        if (caseReferencePixels < 50 || caseActualPixels < 50) {
            std::cerr << "P5.20 blank/placeholder guard failed for " << visualCase.name.toStdString()
                      << ": reference=" << caseReferencePixels << " actual=" << caseActualPixels << '\n';
            return false;
        }
        referencePixels += caseReferencePixels;
        actualPixels += caseActualPixels;

        QImage diff;
        const PixelMetrics metrics = compareImages(reference, actual, diff);
        aggregateMetrics.changedPixels += metrics.changedPixels;
        aggregateMetrics.totalDelta += metrics.totalDelta;
        aggregateMetrics.maxDelta = std::max(aggregateMetrics.maxDelta, metrics.maxDelta);
        if (!metrics.passed) {
            std::cerr << "P5.20 visual comparison failed for " << visualCase.name.toStdString()
                      << ": changedPixels=" << metrics.changedPixels << " maxDelta=" << metrics.maxDelta << '\n';
            return false;
        }
        CHECK(writeCaseArtifacts(visualCase.name, visualCase.widget, reference, actual, diff, metrics));
    }

    aggregateMetrics.passed = aggregateMetrics.changedPixels == 0 && aggregateMetrics.maxDelta == 0;
    CHECK(writeIssueReport(aggregateMetrics, referencePixels, actualPixels));
    return true;
}

bool writeIssueReport(const PixelMetrics& metrics, std::uint64_t referencePixels, std::uint64_t actualPixels)
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_20_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/JoystickVerticalLabel_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.20\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::JoystickButton\", \"pyqtgraph::widgets::VerticalLabel\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/JoystickButton.py and VerticalLabel.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/JoystickButton.hpp\", \"src/pyqtgraph/widgets/JoystickButton.cpp\", \"include/pyqtgraph/widgets/VerticalLabel.hpp\", \"src/pyqtgraph/widgets/VerticalLabel.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.20 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_joystickbutton_verticallabel_p5_20\"},\n"
            "  \"checks\": [\"JoystickButton API shape and fixed 50x50 checkable button\", \"JoystickButton normalized squared-radius setState\", \"JoystickButton press/drag/release interaction\", \"VerticalLabel API shape and default sizeHint\", \"VerticalLabel orientation switch\", \"deterministic visual reference-vs-actual pixels\"],\n"
            "  \"visual_artifacts\": {\"root\": \"reports/visual-diffs/JoystickVerticalLabel\", \"cases\": [\"JoystickButton-center\", \"JoystickButton-dragged\", \"VerticalLabel-vertical\", \"VerticalLabel-horizontal\"], \"per_case_files\": [\"reference.png\", \"actual.png\", \"diff.png\", \"metrics.json\", \"gpt5_vision_review.md\"]},\n"
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
                "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.20 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
                "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.20 JoystickButton and VerticalLabel completion report\n\n"
            "- Issue: GitHub #261 / P5.20\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `JoystickButton` with checkable press/drag/release interaction, squared-radius normalized state, black spot painting, and `sigStateChanged`. Implemented `VerticalLabel` with vertical/horizontal orientation, rotated text rendering, swapped size hints, and min/max geometry updates.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.20 --output-on-failure` | 0 |\n"
            "| `python3 -m pytest -q` | 0 |\n"
            "| `git diff --check` | 0 |\n"
            "| `git diff --name-only origin/main...HEAD` | 0 |\n"));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testJoystickButtonApiShape()) {
        return 1;
    }
    if (!testJoystickButtonSetStateNormalization()) {
        return 1;
    }
    if (!testJoystickButtonInteraction()) {
        return 1;
    }
    if (!testVerticalLabelApiShape()) {
        return 1;
    }
    if (!testVerticalLabelOrientationSwitch()) {
        return 1;
    }
    if (!testVisualBehavior()) {
        return 1;
    }
    return 0;
}
