#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidgetAction>

#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef CPPQTGRAPH_P4_14_ARTIFACT_DIR
#define CPPQTGRAPH_P4_14_ARTIFACT_DIR "artifacts/P4.14"
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

std::string quote(const QString& value)
{
    std::string result = "\"";
    const QByteArray utf8 = value.toUtf8();
    for (char ch : utf8) {
        if (ch == '\\' || ch == '\"') {
            result.push_back('\\');
        }
        result.push_back(ch);
    }
    result.push_back('\"');
    return result;
}

QWidget* defaultWidgetFor(QMenu* menu, const QString& actionText)
{
    if (menu == nullptr) {
        return nullptr;
    }
    for (QAction* action : menu->actions()) {
        if (action != nullptr && action->text() == actionText && action->menu() != nullptr) {
            for (QAction* subAction : action->menu()->actions()) {
                if (auto* widgetAction = qobject_cast<QWidgetAction*>(subAction)) {
                    return widgetAction->defaultWidget();
                }
            }
        }
    }
    return nullptr;
}

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

void writeReport(const std::vector<QString>& actionTexts,
                 const std::vector<QString>& events,
                 const cppqtgraph::graphicsItems::PlotItem::DownsampleState& downsample,
                 const cppqtgraph::graphicsItems::PlotItem::GridState& grid,
                 const cppqtgraph::graphicsItems::PlotItem::AlphaState& alpha,
                 const std::array<bool, 2>& logMode,
                 int callbackCount,
                 bool showGridNoArgsThrows,
                 bool invalidModeThrows,
                 bool disabledMenuNoop)
{
    QDir().mkpath(QString::fromUtf8(CPPQTGRAPH_P4_14_ARTIFACT_DIR));
    QFile file(QString::fromUtf8(CPPQTGRAPH_P4_14_ARTIFACT_DIR) + QStringLiteral("/plotitem_menu_config_interaction_report.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("unable to write P4.14 interaction report");
    }

    QTextStream out(&file);
    out << "{\n";
    out << "  \"issue\": \"P4.14\",\n";
    out << "  \"upstream_reference\": \"pyqtgraph-0.14.0 pyqtgraph/graphicsItems/PlotItem/PlotItem.py menu/config and plotConfigTemplate_generic.py defaults\",\n";
    out << "  \"pre_state\": {\n";
    out << "    \"menu_enabled\": true,\n";
    out << "    \"menu_title\": \"Plot Options\",\n";
    out << "    \"actions\": [";
    for (qsizetype index = 0; index < static_cast<qsizetype>(actionTexts.size()); ++index) {
        if (index != 0) {
            out << ", ";
        }
        out << QString::fromStdString(quote(actionTexts[static_cast<std::size_t>(index)]));
    }
    out << "]\n";
    out << "  },\n";
    out << "  \"event_sequence\": [";
    for (qsizetype index = 0; index < static_cast<qsizetype>(events.size()); ++index) {
        if (index != 0) {
            out << ", ";
        }
        out << QString::fromStdString(quote(events[static_cast<std::size_t>(index)]));
    }
    out << "],\n";
    out << "  \"post_state\": {\n";
    out << "    \"log_x\": " << (logMode[0] ? "true" : "false") << ",\n";
    out << "    \"log_y\": " << (logMode[1] ? "true" : "false") << ",\n";
    out << "    \"grid_x\": " << (grid.x ? "true" : "false") << ",\n";
    out << "    \"grid_y\": " << (grid.y ? "true" : "false") << ",\n";
    out << "    \"grid_alpha\": " << grid.alpha << ",\n";
    out << "    \"downsample_factor\": " << downsample.factor << ",\n";
    out << "    \"downsample_auto\": " << (downsample.automatic ? "true" : "false") << ",\n";
    out << "    \"downsample_method\": " << QString::fromStdString(quote(downsample.method)) << ",\n";
    out << "    \"clip_to_view\": true,\n";
    out << "    \"alpha\": " << alpha.alpha << ",\n";
    out << "    \"alpha_auto\": " << (alpha.automatic ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"signals_callbacks\": {\"widget_signal_count\": " << callbackCount << "},\n";
    out << "  \"negative_noop_cases\": {\n";
    out << "    \"show_grid_without_arguments_throws\": " << (showGridNoArgsThrows ? "true" : "false") << ",\n";
    out << "    \"invalid_downsample_mode_throws\": " << (invalidModeThrows ? "true" : "false") << ",\n";
    out << "    \"set_menu_disabled_keeps_menu_but_context_returns_null\": " << (disabledMenuNoop ? "true" : "false") << "\n";
    out << "  }\n";
    out << "}\n";
}

bool testMenuConfigInteraction()
{
    using cppqtgraph::graphicsItems::PlotItem;

    PlotItem plot;
    QMenu* menu = plot.getMenu();
    CHECK(menu != nullptr);
    CHECK(menu->title() == QStringLiteral("Plot Options"));
    CHECK(plot.menuEnabled());
    CHECK(plot.getContextMenus(nullptr) == menu);

    const std::vector<QString> expectedActions{QStringLiteral("Transforms"), QStringLiteral("Downsample"),
                                               QStringLiteral("Average"), QStringLiteral("Alpha"),
                                               QStringLiteral("Grid"), QStringLiteral("Points")};
    std::vector<QString> actualActions;
    for (QAction* action : menu->actions()) {
        actualActions.push_back(action->text());
    }
    CHECK(actualActions == expectedActions);
    for (const QString& actionText : expectedActions) {
        if (actionText == QStringLiteral("Average")) {
            CHECK(!actionVisible(menu, actionText));
            continue;
        }
        CHECK(defaultWidgetFor(menu, actionText) != nullptr);
    }

    const std::vector<QString> hiddenControls{QStringLiteral("fftCheck"),
                                                QStringLiteral("subtractMeanCheck"),
                                                QStringLiteral("derivativeCheck"),
                                                QStringLiteral("phasemapCheck"),
                                                QStringLiteral("maxTracesCheck"),
                                                QStringLiteral("maxTracesSpin"),
                                                QStringLiteral("forgetTracesCheck")};
    for (const QString& objectName : hiddenControls) {
        if (auto* checkbox = findControl<QCheckBox>(menu, objectName); checkbox != nullptr) {
            CHECK(!checkbox->isEnabled());
        } else if (auto* spin = findControl<QSpinBox>(menu, objectName); spin != nullptr) {
            CHECK(!spin->isEnabled());
        }
    }

    auto* logX = findControl<QCheckBox>(menu, QStringLiteral("logXCheck"));
    auto* logY = findControl<QCheckBox>(menu, QStringLiteral("logYCheck"));
    auto* xGrid = findControl<QCheckBox>(menu, QStringLiteral("xGridCheck"));
    auto* yGrid = findControl<QCheckBox>(menu, QStringLiteral("yGridCheck"));
    auto* gridAlpha = findControl<QSlider>(menu, QStringLiteral("gridAlphaSlider"));
    auto* downsampleCheck = findControl<QCheckBox>(menu, QStringLiteral("downsampleCheck"));
    auto* downsampleSpin = findControl<QSpinBox>(menu, QStringLiteral("downsampleSpin"));
    auto* autoDownsample = findControl<QCheckBox>(menu, QStringLiteral("autoDownsampleCheck"));
    auto* meanRadio = findControl<QRadioButton>(menu, QStringLiteral("meanRadio"));
    auto* peakRadio = findControl<QRadioButton>(menu, QStringLiteral("peakRadio"));
    auto* clipToView = findControl<QCheckBox>(menu, QStringLiteral("clipToViewCheck"));
    auto* alphaGroup = findControl<QGroupBox>(menu, QStringLiteral("alphaGroup"));
    auto* autoAlpha = findControl<QCheckBox>(menu, QStringLiteral("autoAlphaCheck"));
    auto* alphaSlider = findControl<QSlider>(menu, QStringLiteral("alphaSlider"));
    auto* pointsGroup = findControl<QGroupBox>(menu, QStringLiteral("pointsGroup"));
    auto* autoPoints = findControl<QCheckBox>(menu, QStringLiteral("autoPointsCheck"));

    CHECK(logX != nullptr && logY != nullptr);
    CHECK(xGrid != nullptr && yGrid != nullptr && gridAlpha != nullptr);
    CHECK(downsampleCheck != nullptr && downsampleSpin != nullptr && autoDownsample != nullptr);
    CHECK(meanRadio != nullptr && peakRadio != nullptr && clipToView != nullptr);
    CHECK(alphaGroup != nullptr && autoAlpha != nullptr && alphaSlider != nullptr);
    CHECK(pointsGroup != nullptr && autoPoints != nullptr);
    CHECK(!pointsGroup->isEnabled());
    CHECK(!autoPoints->isEnabled());
    CHECK(!logY->isChecked());
    CHECK(!downsampleCheck->isChecked());
    CHECK(autoDownsample->isChecked());
    CHECK(peakRadio->isChecked());
    CHECK(downsampleSpin->minimum() == 1);
    CHECK(downsampleSpin->maximum() == 100000);
    CHECK(downsampleSpin->value() == 1);
    CHECK(gridAlpha->maximum() == 255);
    CHECK(gridAlpha->value() == 128);
    CHECK(alphaGroup->isChecked());
    CHECK(!autoAlpha->isChecked());
    CHECK(alphaSlider->maximum() == 1000);
    CHECK(alphaSlider->value() == 1000);

    int callbackCount = 0;
    QObject::connect(logX, &QCheckBox::toggled, [&callbackCount](bool) { ++callbackCount; });
    QObject::connect(logY, &QCheckBox::toggled, [&callbackCount](bool) { ++callbackCount; });
    QObject::connect(gridAlpha, &QSlider::valueChanged, [&callbackCount](int) { ++callbackCount; });
    QObject::connect(downsampleSpin, qOverload<int>(&QSpinBox::valueChanged), [&callbackCount](int) { ++callbackCount; });
    QObject::connect(meanRadio, &QRadioButton::toggled, [&callbackCount](bool checked) {
        if (checked) {
            ++callbackCount;
        }
    });
    QObject::connect(clipToView, &QCheckBox::toggled, [&callbackCount](bool) { ++callbackCount; });

    std::vector<QString> events;
    logX->setChecked(true);
    events.push_back(QStringLiteral("toggle logXCheck true"));
    logY->setChecked(true);
    events.push_back(QStringLiteral("toggle logYCheck true"));
    plot.showGrid(true, false, 0.5);
    events.push_back(QStringLiteral("showGrid x=true y=false alpha=0.5"));
    gridAlpha->setValue(200);
    events.push_back(QStringLiteral("gridAlphaSlider value=200"));
    plot.setDownsampling(5, false, QStringLiteral("mean"));
    events.push_back(QStringLiteral("setDownsampling factor=5 auto=false mode=mean"));
    plot.setClipToView(true);
    events.push_back(QStringLiteral("setClipToView true"));
    plot.setContextMenuActionVisible(QStringLiteral("Grid"), false);
    events.push_back(QStringLiteral("setContextMenuActionVisible Grid false"));
    plot.hideButtons();
    events.push_back(QStringLiteral("hideButtons"));
    plot.showButtons();
    events.push_back(QStringLiteral("showButtons"));

    const auto logMode = plot.logMode();
    const auto grid = plot.gridState();
    const auto downsample = plot.downsampleMode();
    const auto alpha = plot.alphaState();
    CHECK(logMode[0] && logMode[1]);
    CHECK(grid.x && !grid.y);
    CHECK(grid.alphaSliderValue == 200);
    CHECK(downsample.factor == 5);
    CHECK(!downsample.automatic);
    CHECK(downsample.method == QStringLiteral("mean"));
    CHECK(meanRadio->isChecked());
    CHECK(plot.clipToViewMode());
    CHECK(!actionVisible(menu, QStringLiteral("Grid")));
    CHECK(!plot.buttonsHidden());
    CHECK(alpha.alpha == 1.0);
    CHECK(!alpha.automatic);
    CHECK(!plot.pointMode().has_value());
    CHECK(callbackCount >= 6);

    bool showGridNoArgsThrows = false;
    try {
        plot.showGrid();
    } catch (const std::invalid_argument&) {
        showGridNoArgsThrows = true;
    }
    CHECK(showGridNoArgsThrows);

    bool invalidModeThrows = false;
    try {
        plot.setDownsampling(2, false, QStringLiteral("invalid"));
    } catch (const std::invalid_argument&) {
        invalidModeThrows = true;
    }
    CHECK(invalidModeThrows);

    plot.setMenuEnabled(false);
    const bool disabledMenuNoop = !plot.menuEnabled() && plot.getMenu() == menu && plot.getContextMenus(nullptr) == nullptr;
    CHECK(disabledMenuNoop);

    writeReport(actualActions, events, downsample, grid, alpha, logMode, callbackCount, showGridNoArgsThrows, invalidModeThrows,
                disabledMenuNoop);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    return testMenuConfigInteraction() ? 0 : 1;
}
