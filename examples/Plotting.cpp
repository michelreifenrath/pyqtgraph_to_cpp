// Source note: translated/adapted from PyQtGraph examples/Plotting.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/configfile.hpp>
#include <cppqtgraph/graphicsItems/LinearRegionItem.hpp>
#include <cppqtgraph/graphicsItems/PlotCurveItem.hpp>
#include <cppqtgraph/graphicsItems/PlotDataItem.hpp>
#include <cppqtgraph/graphicsItems/PlotItem/PlotItem.hpp>
#include <cppqtgraph/graphicsItems/ViewBox/ViewBox.hpp>
#include <cppqtgraph/widgets/GraphicsLayoutWidget.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph::examples {

namespace {

constexpr std::uint32_t kPlottingDeterministicSeed = 0x504C5454U;
constexpr int kTimerIntervalMs = 50;
constexpr std::size_t kUpdateRowCount = 10;
constexpr std::size_t kUpdateColumnCount = 1000;
constexpr std::size_t kSincCount = 1000;

std::vector<double> makeNormalVector(std::size_t count, std::mt19937& generator)
{
    std::normal_distribution<double> distribution(0.0, 1.0);
    std::vector<double> values;
    values.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        values.push_back(distribution(generator));
    }
    return values;
}

std::vector<double> makeLinspace(double start, double end, std::size_t count)
{
    std::vector<double> values;
    values.reserve(count);
    if (count == 0) {
        return values;
    }
    if (count == 1) {
        values.push_back(start);
        return values;
    }
    const double step = (end - start) / static_cast<double>(count - 1);
    for (std::size_t index = 0; index < count; ++index) {
        values.push_back(start + step * static_cast<double>(index));
    }
    return values;
}

std::vector<double> makeSincData()
{
    const auto x = makeLinspace(-100.0, 100.0, kSincCount);
    std::vector<double> values;
    values.reserve(kSincCount);
    for (std::size_t index = 0; index < kSincCount; ++index) {
        const double value = x[index];
        values.push_back(value == 0.0 ? 1.0 : std::sin(value) / value);
    }
    return values;
}

QPen makeRgbPen(int red, int green, int blue, int alpha = 255)
{
    QPen pen(QColor(red, green, blue, alpha));
    pen.setCosmetic(true);
    return pen;
}

} // namespace

struct PlottingState {
    std::vector<double> p1Y;
    std::vector<double> p2Red;
    std::vector<double> p2Green;
    std::vector<double> p2Blue;
    std::vector<double> p3Y;
    std::vector<double> p4X;
    std::vector<double> p4Y;
    std::vector<double> p5X;
    std::vector<double> p5Y;
    std::vector<std::vector<double>> p6Rows;
    std::vector<double> p7Y;
    std::vector<double> sincData;
    std::size_t updatePtr = 0;
    bool updateAutoRangeDisabled = false;
    bool regionZoomSyncBlocked = false;
};

struct PlottingOptions {
    QString dataFixturePath;
    bool wrongSymbolP3 = false;
    bool disableGridP4 = false;
    bool hideRegionP8 = false;
};

struct PlottingExample {
    std::unique_ptr<widgets::GraphicsLayoutWidget> widget;
    std::array<graphicsItems::PlotItem*, 9> plots{};
    graphicsItems::PlotCurveItem* p1Curve = nullptr;
    graphicsItems::PlotCurveItem* p2RedCurve = nullptr;
    graphicsItems::PlotCurveItem* p2GreenCurve = nullptr;
    graphicsItems::PlotCurveItem* p2BlueCurve = nullptr;
    graphicsItems::PlotDataItem* p3Curve = nullptr;
    graphicsItems::PlotDataItem* p5Scatter = nullptr;
    graphicsItems::PlotCurveItem* p6Curve = nullptr;
    graphicsItems::PlotCurveItem* p7Curve = nullptr;
    graphicsItems::PlotCurveItem* p8Curve = nullptr;
    graphicsItems::PlotCurveItem* p9Curve = nullptr;
    graphicsItems::LinearRegionItem* region = nullptr;
    QTimer* timer = nullptr;
    std::shared_ptr<PlottingState> state;
};

std::size_t plottingPlotCount() noexcept
{
    return 9;
}

std::size_t plottingSincPointCount() noexcept
{
    return kSincCount;
}

std::size_t plottingUpdateRowCount() noexcept
{
    return kUpdateRowCount;
}

std::size_t plottingUpdateColumnCount() noexcept
{
    return kUpdateColumnCount;
}

int plottingTimerIntervalMs() noexcept
{
    return kTimerIntervalMs;
}

std::vector<double> jsonToDoubleVector(const QJsonArray& array)
{
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& value : array) {
        values.push_back(value.toDouble());
    }
    return values;
}

bool populatePlottingStateFromFixture(const QString& fixturePath, PlottingState& state)
{
    QFile file(fixturePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonObject arrays = root.value(QStringLiteral("arrays")).toObject();
    if (arrays.isEmpty()) {
        return false;
    }

    state.p1Y = jsonToDoubleVector(arrays.value(QStringLiteral("p1_y")).toArray());
    state.p2Red = jsonToDoubleVector(arrays.value(QStringLiteral("p2_red")).toArray());
    state.p2Green = jsonToDoubleVector(arrays.value(QStringLiteral("p2_green")).toArray());
    state.p2Blue = jsonToDoubleVector(arrays.value(QStringLiteral("p2_blue")).toArray());
    state.p3Y = jsonToDoubleVector(arrays.value(QStringLiteral("p3_y")).toArray());
    state.p4X = jsonToDoubleVector(arrays.value(QStringLiteral("p4_x")).toArray());
    state.p4Y = jsonToDoubleVector(arrays.value(QStringLiteral("p4_y")).toArray());
    state.p5X = jsonToDoubleVector(arrays.value(QStringLiteral("p5_x")).toArray());
    state.p5Y = jsonToDoubleVector(arrays.value(QStringLiteral("p5_y")).toArray());
    state.p7Y = jsonToDoubleVector(arrays.value(QStringLiteral("p7_y")).toArray());
    state.sincData = jsonToDoubleVector(arrays.value(QStringLiteral("sinc_data")).toArray());

    state.p6Rows.clear();
    const QJsonArray p6Rows = arrays.value(QStringLiteral("p6_rows")).toArray();
    state.p6Rows.reserve(static_cast<std::size_t>(p6Rows.size()));
    for (const QJsonValue& rowValue : p6Rows) {
        state.p6Rows.push_back(jsonToDoubleVector(rowValue.toArray()));
    }

    return !state.p1Y.empty() && !state.p6Rows.empty() && !state.sincData.empty();
}

void populatePlottingStateFromGenerator(PlottingState& state)
{
    std::mt19937 generator(kPlottingDeterministicSeed);

    state.p1Y = makeNormalVector(100, generator);
    state.p2Red = makeNormalVector(100, generator);
    state.p2Green = makeNormalVector(110, generator);
    for (double& value : state.p2Green) {
        value += 5.0;
    }
    state.p2Blue = makeNormalVector(120, generator);
    for (double& value : state.p2Blue) {
        value += 10.0;
    }
    state.p3Y = makeNormalVector(100, generator);

    state.p4X.reserve(1000);
    state.p4Y.reserve(1000);
    for (int index = 0; index < 1000; ++index) {
        const double phase = static_cast<double>(index) / 999.0;
        state.p4X.push_back(std::cos(phase * 2.0 * M_PI));
        state.p4Y.push_back(std::sin(phase * 4.0 * M_PI));
    }

    {
        std::vector<double> x = makeNormalVector(1000, generator);
        std::vector<double> y = makeNormalVector(1000, generator);
        for (std::size_t index = 0; index < x.size(); ++index) {
            x[index] *= 1.0e-5;
            y[index] = x[index] * 1000.0 + 0.005 * y[index];
        }
        const double minimum = *std::min_element(y.begin(), y.end());
        for (double& value : y) {
            value -= minimum - 1.0;
        }
        state.p5X.clear();
        state.p5Y.clear();
        state.p5X.reserve(x.size());
        state.p5Y.reserve(y.size());
        for (std::size_t index = 0; index < x.size(); ++index) {
            if (x[index] > 1.0e-15) {
                state.p5X.push_back(x[index]);
                state.p5Y.push_back(y[index]);
            }
        }
    }

    state.p6Rows.reserve(kUpdateRowCount);
    for (std::size_t row = 0; row < kUpdateRowCount; ++row) {
        state.p6Rows.push_back(makeNormalVector(kUpdateColumnCount, generator));
    }

    {
        const auto x = makeLinspace(0.0, 10.0, 1000);
        std::vector<double> noise = makeNormalVector(1000, generator);
        state.p7Y.reserve(1000);
        for (std::size_t index = 0; index < 1000; ++index) {
            state.p7Y.push_back(std::sin(x[index]) + 0.1 * noise[index]);
        }
    }

    state.sincData = makeSincData();
}

PlottingExample createPlottingExample(const PlottingOptions& options = {})
{
    setConfigOptions({{"antialias", true}});

    auto state = std::make_shared<PlottingState>();
    if (!options.dataFixturePath.isEmpty()) {
        if (!populatePlottingStateFromFixture(options.dataFixturePath, *state)) {
            throw std::runtime_error(
                ("failed to load Plotting data fixture: " + options.dataFixturePath.toStdString()).c_str());
        }
    } else {
        populatePlottingStateFromGenerator(*state);
    }

    auto widget = std::make_unique<widgets::GraphicsLayoutWidget>(
        nullptr, true, QSize(1000, 600), QStringLiteral("Basic plotting examples"));
    widget->setWindowTitle(QStringLiteral("pyqtgraph example: Plotting"));

    PlottingExample example;
    example.widget = std::move(widget);
    example.state = std::move(state);

    auto* p1 = example.widget->addPlot();
    p1->setTitle(QStringLiteral("Basic array plotting"));
    example.p1Curve = p1->plot(example.state->p1Y);
    example.plots[0] = p1;

    auto* p2 = example.widget->addPlot();
    p2->setTitle(QStringLiteral("Multiple curves"));
    example.p2RedCurve = p2->plot(example.state->p2Red, QStringLiteral("Red curve"));
    example.p2RedCurve->setPen(makeRgbPen(255, 0, 0));
    example.p2GreenCurve = p2->plot(example.state->p2Green, QStringLiteral("Green curve"));
    example.p2GreenCurve->setPen(makeRgbPen(0, 255, 0));
    example.p2BlueCurve = p2->plot(example.state->p2Blue, QStringLiteral("Blue curve"));
    example.p2BlueCurve->setPen(makeRgbPen(0, 0, 255));
    example.plots[1] = p2;

    auto* p3 = example.widget->addPlot();
    p3->setTitle(QStringLiteral("Drawing with points"));
    auto* p3Data = new graphicsItems::PlotDataItem(example.state->p3Y);
    p3Data->setPen(makeRgbPen(200, 200, 200));
    p3Data->setSymbol(options.wrongSymbolP3 ? QStringLiteral("s") : QStringLiteral("o"));
    p3Data->setSymbolBrush(QBrush(QColor(255, 0, 0)));
    p3Data->setSymbolPen(QPen(Qt::white));
    p3->addItem(p3Data);
    example.p3Curve = p3Data;
    example.plots[2] = p3;

    example.widget->nextRow();

    auto* p4 = example.widget->addPlot();
    p4->setTitle(QStringLiteral("Parametric, grid enabled"));
    p4->plot(example.state->p4X, example.state->p4Y);
    p4->showGrid(!options.disableGridP4, !options.disableGridP4);
    example.plots[3] = p4;

    auto* p5 = example.widget->addPlot();
    p5->setTitle(QStringLiteral("Scatter plot, axis labels, log scale"));
    auto* scatter = new graphicsItems::PlotDataItem(example.state->p5X, example.state->p5Y);
    scatter->setPen(nullptr);
    scatter->setSymbol(QStringLiteral("t"));
    scatter->setSymbolPen(nullptr);
    scatter->setSymbolSize(10.0);
    scatter->setSymbolBrush(QBrush(QColor(100, 100, 255, 50)));
    p5->addItem(scatter);
    p5->setLabel(QStringLiteral("left"), QStringLiteral("Y Axis"), QStringLiteral("A"));
    p5->setLabel(QStringLiteral("bottom"), QStringLiteral("Y Axis"), QStringLiteral("s"));
    p5->setLogMode(true, false);
    example.p5Scatter = scatter;
    example.plots[4] = p5;

    auto* p6 = example.widget->addPlot();
    p6->setTitle(QStringLiteral("Updating plot"));
    example.p6Curve = new graphicsItems::PlotCurveItem();
    example.p6Curve->setPen(QPen(Qt::yellow));
    p6->addItem(example.p6Curve);
    example.plots[5] = p6;

    example.widget->nextRow();

    auto* p7 = example.widget->addPlot();
    p7->setTitle(QStringLiteral("Filled plot, axis disabled"));
    example.p7Curve = p7->plot(example.state->p7Y);
    example.p7Curve->setFillLevel(-0.3);
    example.p7Curve->setFillBrush(QBrush(QColor(50, 50, 200, 100)));
    p7->showAxis(QStringLiteral("bottom"), false);
    example.plots[6] = p7;

    auto* p8 = example.widget->addPlot();
    p8->setTitle(QStringLiteral("Region Selection"));
    example.p8Curve = p8->plot(example.state->sincData);
    example.p8Curve->setPen(makeRgbPen(255, 255, 255, 200));
    graphicsItems::LinearRegionItem* region = nullptr;
    if (!options.hideRegionP8) {
        region = new graphicsItems::LinearRegionItem(std::make_pair(400.0, 700.0));
        region->setZValue(-10.0);
        p8->addItem(region);
    }
    example.region = region;
    example.plots[7] = p8;

    auto* p9 = example.widget->addPlot();
    p9->setTitle(QStringLiteral("Zoom on selected region"));
    example.p9Curve = p9->plot(example.state->sincData);
    example.plots[8] = p9;

    const auto updatePlotFromRegion = [p9, region, state = example.state]() {
        if (region == nullptr || state->regionZoomSyncBlocked) {
            return;
        }
        state->regionZoomSyncBlocked = true;
        const auto [minimum, maximum] = region->getRegion();
        p9->setXRange(minimum, maximum, 0.0);
        state->regionZoomSyncBlocked = false;
    };
    const auto updateRegionFromPlot = [p9, region, state = example.state]() {
        if (region == nullptr || state->regionZoomSyncBlocked) {
            return;
        }
        state->regionZoomSyncBlocked = true;
        const auto xRange = p9->viewRange()[graphicsItems::ViewBox::XAxis];
        region->setRegion(std::make_pair(xRange[0], xRange[1]));
        state->regionZoomSyncBlocked = false;
    };

    if (region != nullptr) {
        QObject::connect(region, &graphicsItems::LinearRegionItem::sigRegionChanged, region, updatePlotFromRegion);
        QObject::connect(
            p9->getViewBox(), &graphicsItems::ViewBox::sigXRangeChanged, p9->getViewBox(), updateRegionFromPlot);
        updatePlotFromRegion();
    }

    const auto performTimerUpdate = [p6, curve = example.p6Curve, state = example.state]() {
        curve->setData(state->p6Rows[state->updatePtr % kUpdateRowCount]);
        if (!state->updateAutoRangeDisabled) {
            p6->getViewBox()->enableAutoRange(graphicsItems::ViewBox::XYAxes, false);
            state->updateAutoRangeDisabled = true;
        }
        state->updatePtr = (state->updatePtr + 1) % kUpdateRowCount;
    };

    auto* timer = new QTimer(example.widget.get());
    QObject::connect(timer, &QTimer::timeout, timer, performTimerUpdate);
    timer->start(kTimerIntervalMs);
    example.timer = timer;
    performTimerUpdate();

    return example;
}

void advancePlottingTimer(PlottingExample& example, int ticks = 1)
{
    if (example.timer == nullptr || example.state == nullptr || example.p6Curve == nullptr || ticks <= 0) {
        return;
    }
    for (int index = 0; index < ticks; ++index) {
        example.p6Curve->setData(example.state->p6Rows[example.state->updatePtr % kUpdateRowCount]);
        if (!example.state->updateAutoRangeDisabled && example.plots[5] != nullptr) {
            example.plots[5]->getViewBox()->enableAutoRange(graphicsItems::ViewBox::XYAxes, false);
            example.state->updateAutoRangeDisabled = true;
        }
        example.state->updatePtr = (example.state->updatePtr + 1) % kUpdateRowCount;
    }
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_PLOTTING_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    auto example = cppqtgraph::examples::createPlottingExample();
    return QApplication::exec();
}
#endif
