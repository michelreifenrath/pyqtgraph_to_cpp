#include <cppqtgraph/graphicsItems/DateAxisItem.hpp>

#include <QtCore/QDir>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsItem>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
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

bool containsClose(const std::vector<double>& values, double expected)
{
    return std::any_of(values.begin(), values.end(), [expected](double value) {
        return std::abs(value - expected) <= 1.0e-9;
    });
}

bool testConstructionAndApi()
{
    using cppqtgraph::graphicsItems::AxisItem;
    using cppqtgraph::graphicsItems::DateAxisItem;

    static_assert(std::is_base_of_v<AxisItem, DateAxisItem>);
    static_assert(std::is_constructible_v<DateAxisItem>);
    static_assert(std::is_constructible_v<DateAxisItem, QString>);
    static_assert(std::is_constructible_v<DateAxisItem, AxisItem::Orientation>);
    static_assert(std::is_destructible_v<DateAxisItem>);

    DateAxisItem axis;
    CHECK(axis.orientation() == AxisItem::Orientation::Bottom);
    CHECK(!axis.utcOffset().has_value());
    CHECK(axis.autoSIPrefixScale() == 1.0);

    DateAxisItem left(AxisItem::Orientation::Left, -3600);
    CHECK(left.orientation() == AxisItem::Orientation::Left);
    CHECK(left.utcOffset().has_value());
    CHECK(*left.utcOffset() == -3600);
    left.setUtcOffset(std::nullopt);
    CHECK(!left.utcOffset().has_value());

    return true;
}

bool testPinnedOracleTickStrings()
{
    using cppqtgraph::graphicsItems::DateAxisItem;

    DateAxisItem axis(QStringLiteral("bottom"), 0);
    const std::vector<double> values{0.0, 978307200.0, 978328800.0, 978350400.0, 978372000.0};

    CHECK((axis.tickStrings(values, 1.0, 31'536'000.0) == std::vector<QString>{
        QStringLiteral("1970"), QStringLiteral("2001"), QStringLiteral("2001"), QStringLiteral("2001"), QStringLiteral("2001")}));
    CHECK((axis.tickStrings(values, 1.0, 2'592'000.0) == std::vector<QString>{
        QStringLiteral("Jan"), QStringLiteral("Jan"), QStringLiteral("Jan"), QStringLiteral("Jan"), QStringLiteral("Jan")}));
    CHECK((axis.tickStrings(values, 1.0, 86'400.0) == std::vector<QString>{
        QStringLiteral("Thu 01"), QStringLiteral("Mon 01"), QStringLiteral("Mon 01"), QStringLiteral("Mon 01"), QStringLiteral("Mon 01")}));
    CHECK((axis.tickStrings(values, 1.0, 3'600.0) == std::vector<QString>{
        QStringLiteral("00:00"), QStringLiteral("00:00"), QStringLiteral("06:00"), QStringLiteral("12:00"), QStringLiteral("18:00")}));
    CHECK((axis.tickStrings(values, 1.0, 1.0) == std::vector<QString>{
        QStringLiteral("00:00:00"), QStringLiteral("00:00:00"), QStringLiteral("06:00:00"), QStringLiteral("12:00:00"), QStringLiteral("18:00:00")}));

    const std::vector<double> msValues{0.0, 0.001, 0.005, 0.123, 59.999};
    CHECK((axis.tickStrings(msValues, 1.0, 0.001) == std::vector<QString>{
        QStringLiteral("00.000"), QStringLiteral("00.001"), QStringLiteral("00.005"), QStringLiteral("00.123"), QStringLiteral("59.999")}));

    DateAxisItem offsetAxis(QStringLiteral("bottom"), -3600);
    CHECK((offsetAxis.tickStrings({978307200.0, 978310800.0, 978314400.0, 978318000.0, 978321600.0}, 1.0, 3'600.0)
        == std::vector<QString>{QStringLiteral("01:00"), QStringLiteral("02:00"), QStringLiteral("03:00"), QStringLiteral("04:00"), QStringLiteral("05:00")}));

    CHECK((axis.tickStrings({1.0e20, -1.0e20}, 1.0, 31'536'000.0)
        == std::vector<QString>{QStringLiteral("3.16881e+12"), QStringLiteral("-3.16881e+12")}));

    return true;
}

bool testPinnedOracleTickValues()
{
    using cppqtgraph::graphicsItems::DateAxisItem;

    DateAxisItem axis(QStringLiteral("bottom"), 0);
    const auto levels = axis.tickValues(978307199.0, 978372001.0, 324.0);
    CHECK(levels.size() >= 2U);
    CHECK_CLOSE(levels[0].spacing, 86'400.0, 1.0e-9);
    CHECK(containsClose(levels[0].values, 978307200.0));

    const auto hourLevel = std::find_if(levels.begin(), levels.end(), [](const auto& level) {
        return std::abs(level.spacing - 3'600.0) <= 1.0e-9;
    });
    CHECK(hourLevel != levels.end());
    CHECK(containsClose(hourLevel->values, 978328800.0));
    CHECK(containsClose(hourLevel->values, 978350400.0));
    CHECK(containsClose(hourLevel->values, 978372000.0));

    const auto msLevels = axis.tickValues(0.0, 0.010, 500.0);
    CHECK(!msLevels.empty());
    CHECK_CLOSE(msLevels.back().spacing, 0.001, 1.0e-12);
    CHECK(containsClose(msLevels.back().values, 0.005));
    CHECK(containsClose(msLevels.back().values, 0.010));

    return true;
}

bool testSupplementalAxisVisualArtifact()
{
    using cppqtgraph::graphicsItems::DateAxisItem;

    DateAxisItem axis(QStringLiteral("bottom"), 0);
    axis.setGeometry(QRectF(0.0, 0.0, 640.0, 80.0));
    axis.setRange(978307199.0, 978372001.0);
    axis.setPen(QColor(230, 230, 230));
    axis.setTextPen(QColor(230, 230, 230));
    axis.setTickPen(QColor(230, 230, 230));

    QImage image(680, 120, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(20, 20, 20));
    QPainter painter(&image);
    painter.translate(20.0, 20.0);
    axis.paint(&painter, nullptr, nullptr);
    painter.end();

    int brightPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.red() > 80 || color.green() > 80 || color.blue() > 80) {
                ++brightPixels;
            }
        }
    }
    CHECK(brightPixels > 250);

#ifdef CPPQTGRAPH_P3_08_ARTIFACT_DIR
    const std::filesystem::path artifactDir = CPPQTGRAPH_P3_08_ARTIFACT_DIR;
    std::filesystem::create_directories(artifactDir);
    const std::filesystem::path imagePath = artifactDir / "dateaxis_bottom_day_hour.png";
    CHECK(image.save(QString::fromStdString(imagePath.string())));
    std::ofstream metrics(artifactDir / "metrics.txt");
    metrics << "issue=P3.08\n"
            << "artifact=dateaxis_bottom_day_hour.png\n"
            << "bright_pixels=" << brightPixels << "\n"
            << "oracle=pyqtgraph-0.14.0 a20028b98294b9cc8770f2015a92eb342224b788\n";
#endif

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ApplicationGuard app(argc, argv);
    if (!testConstructionAndApi()) {
        return 1;
    }
    if (!testPinnedOracleTickStrings()) {
        return 1;
    }
    if (!testPinnedOracleTickValues()) {
        return 1;
    }
    if (!testSupplementalAxisVisualArtifact()) {
        return 1;
    }
    return 0;
}
