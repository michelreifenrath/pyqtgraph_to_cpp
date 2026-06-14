#include <cppqtgraph/graphicsItems/PlotDataItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidgetAction>

#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
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

template <typename Widget>
Widget* findControl(QMenu* menu, const QString& objectName)
{
    for (QAction* action : menu->actions()) {
        if (action == nullptr || action->menu() == nullptr) {
            continue;
        }
        for (QAction* subAction : action->menu()->actions()) {
            if (auto* widgetAction = qobject_cast<QWidgetAction*>(subAction)) {
                if (QWidget* root = widgetAction->defaultWidget(); root != nullptr) {
                    if (root->objectName() == objectName) {
                        return qobject_cast<Widget*>(root);
                    }
                    if (auto* control = root->findChild<Widget*>(objectName)) {
                        return control;
                    }
                }
            }
        }
    }
    return nullptr;
}

bool actionVisible(QMenu* menu, const QString& actionText)
{
    for (QAction* action : menu->actions()) {
        if (action != nullptr && action->text() == actionText) {
            return action->isVisible();
        }
    }
    throw std::runtime_error("menu action not found");
}

bool controlHidden(QMenu* menu, const QString& objectName)
{
    if (auto* checkbox = findControl<QCheckBox>(menu, objectName); checkbox != nullptr) {
        return !checkbox->isEnabled();
    }
    if (auto* spin = findControl<QSpinBox>(menu, objectName); spin != nullptr) {
        return !spin->isEnabled();
    }
    if (auto* list = findControl<QListWidget>(menu, objectName); list != nullptr) {
        return !list->isEnabled();
    }
    if (auto* group = findControl<QGroupBox>(menu, objectName); group != nullptr) {
        return !group->isEnabled();
    }
    return false;
}

bool controlSupported(QMenu* menu, const QString& objectName)
{
    if (auto* checkbox = findControl<QCheckBox>(menu, objectName); checkbox != nullptr) {
        return checkbox->isEnabled();
    }
    if (auto* spin = findControl<QSpinBox>(menu, objectName); spin != nullptr) {
        return spin->isEnabled();
    }
    if (auto* slider = findControl<QSlider>(menu, objectName); slider != nullptr) {
        return slider->isEnabled();
    }
    if (auto* group = findControl<QGroupBox>(menu, objectName); group != nullptr) {
        return group->isEnabled();
    }
    return false;
}

bool nearlyEqual(double lhs, double rhs, double tolerance = 1.0e-9)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool testMenuTreePolicy()
{
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    QMenu* menu = plot.getMenu();
    CHECK(menu != nullptr);

    const std::vector<QString> upstreamSubmenus{QStringLiteral("Transforms"), QStringLiteral("Downsample"),
                                                QStringLiteral("Average"), QStringLiteral("Alpha"),
                                                QStringLiteral("Grid"), QStringLiteral("Points")};
    const std::vector<QString> supportedVisibleSubmenus{QStringLiteral("Transforms"), QStringLiteral("Downsample"),
                                                        QStringLiteral("Alpha"), QStringLiteral("Grid")};

    std::vector<QString> actualActions;
    for (QAction* action : menu->actions()) {
        actualActions.push_back(action->text());
    }
    CHECK(actualActions == upstreamSubmenus);

    for (const QString& submenu : supportedVisibleSubmenus) {
        CHECK(actionVisible(menu, submenu));
    }
    CHECK(!actionVisible(menu, QStringLiteral("Average")));
    CHECK(!actionVisible(menu, QStringLiteral("Points")));

    const std::vector<QString> hiddenControls{QStringLiteral("fftCheck"),
                                                QStringLiteral("subtractMeanCheck"),
                                                QStringLiteral("derivativeCheck"),
                                                QStringLiteral("phasemapCheck"),
                                                QStringLiteral("maxTracesCheck"),
                                                QStringLiteral("maxTracesSpin"),
                                                QStringLiteral("forgetTracesCheck"),
                                                QStringLiteral("averageGroup"),
                                                QStringLiteral("avgParamList"),
                                                QStringLiteral("pointsGroup")};
    for (const QString& objectName : hiddenControls) {
        CHECK(controlHidden(menu, objectName));
    }

    CHECK(controlSupported(menu, QStringLiteral("logXCheck")));
    CHECK(controlSupported(menu, QStringLiteral("logYCheck")));
    CHECK(controlSupported(menu, QStringLiteral("downsampleCheck")));
    CHECK(controlSupported(menu, QStringLiteral("clipToViewCheck")));
    CHECK(controlSupported(menu, QStringLiteral("alphaGroup")));
    CHECK(controlSupported(menu, QStringLiteral("xGridCheck")));
    CHECK(controlHidden(menu, QStringLiteral("pointsGroup")));

    return true;
}

bool spanEquals(std::span<const double> values, const std::vector<double>& expected)
{
    if (values.size() != expected.size()) {
        return false;
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (!nearlyEqual(values[index], expected[index])) {
            return false;
        }
    }
    return true;
}

bool testSupportedControlsAffectPlotDataItem()
{
    using cppqtgraph::graphicsItems::PlotDataItem;
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    std::vector<double> x;
    std::vector<double> y;
    for (int index = 0; index < 10; ++index) {
        x.push_back(static_cast<double>(index));
        y.push_back(static_cast<double>(index * index));
    }
    auto data = std::make_unique<PlotDataItem>(x, y);
    plot.addItem(data.get());

    QMenu* menu = plot.getMenu();
    auto* subsampleRadio = findControl<QRadioButton>(menu, QStringLiteral("subsampleRadio"));
    auto* downsampleSpin = findControl<QSpinBox>(menu, QStringLiteral("downsampleSpin"));
    auto* downsampleCheck = findControl<QCheckBox>(menu, QStringLiteral("downsampleCheck"));
    auto* clipToView = findControl<QCheckBox>(menu, QStringLiteral("clipToViewCheck"));
    auto* alphaSlider = findControl<QSlider>(menu, QStringLiteral("alphaSlider"));
    CHECK(subsampleRadio != nullptr && downsampleSpin != nullptr && downsampleCheck != nullptr);
    CHECK(clipToView != nullptr && alphaSlider != nullptr);

    CHECK(spanEquals(data->xData(), x));
    CHECK(spanEquals(data->yData(), y));
    CHECK(data->curve()->xData().size() == x.size());

    subsampleRadio->setChecked(true);
    downsampleSpin->setValue(2);
    downsampleCheck->setChecked(true);
    CHECK(data->curve()->xData().size() == 5U);
    CHECK(spanEquals(data->xData(), x));

    downsampleCheck->setChecked(false);
    plot.setXRange(3.0, 7.0, 0.0);
    clipToView->setChecked(true);
    const auto clippedX = data->curve()->xData();
    CHECK(clippedX.size() == 6U);
    CHECK(nearlyEqual(clippedX.front(), 2.0));
    CHECK(nearlyEqual(clippedX.back(), 7.0));
    for (double value : clippedX) {
        CHECK(value >= 2.0 && value <= 7.0);
    }

    alphaSlider->setValue(500);
    CHECK(data->curve()->pen().color().alpha() == 128);

    plot.removeItem(data.get());
    return true;
}

bool testLogMenuToggleUpdatesPlotDataItem()
{
    using cppqtgraph::graphicsItems::PlotDataItem;
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    auto data = std::make_unique<PlotDataItem>(std::vector<double>{1.0, 10.0}, std::vector<double>{2.0, 3.0});
    plot.addItem(data.get());

    QMenu* menu = plot.getMenu();
    auto* logX = findControl<QCheckBox>(menu, QStringLiteral("logXCheck"));
    auto* logY = findControl<QCheckBox>(menu, QStringLiteral("logYCheck"));
    CHECK(logX != nullptr && logY != nullptr);

    logX->setChecked(true);
    logY->setChecked(false);
    CHECK((data->logMode() == std::array<bool, 2>{true, false}));
    CHECK(nearlyEqual(data->curve()->xData()[0], 0.0));
    CHECK(nearlyEqual(data->curve()->xData()[1], 1.0));

    plot.removeItem(data.get());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testMenuTreePolicy()) {
        return 1;
    }
    if (!testSupportedControlsAffectPlotDataItem()) {
        return 1;
    }
    if (!testLogMenuToggleUpdatesPlotDataItem()) {
        return 1;
    }
    return 0;
}
