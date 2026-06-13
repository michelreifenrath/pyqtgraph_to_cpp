// Source note: translated/adapted from PyQtGraph examples/MultiDataPlot.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QColor>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cppqtgraph::examples {

namespace {

constexpr const char* kPinnedCommit = "a20028b98294b9cc8770f2015a92eb342224b788";

std::vector<double> jsonToDoubleVector(const QJsonArray& array)
{
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& value : array) {
        values.push_back(value.toDouble());
    }
    return values;
}

std::vector<std::vector<double>> jsonToMatrix(const QJsonArray& rows)
{
    std::vector<std::vector<double>> matrix;
    matrix.reserve(static_cast<std::size_t>(rows.size()));
    for (const QJsonValue& rowValue : rows) {
        matrix.push_back(jsonToDoubleVector(rowValue.toArray()));
    }
    return matrix;
}

QVariantList makeValueChoiceList(const QStringList& valueKeys)
{
    QVariantList choices;
    choices.reserve(valueKeys.size() + 1);
    choices.append(QStringLiteral("random"));
    for (const QString& key : valueKeys) {
        choices.append(key);
    }
    return choices;
}

QVariantList makeSymbolChoiceList()
{
    QVariantList choices;
    for (const auto& entry : cppqtgraph::symbolPaths()) {
        choices.append(entry.first);
    }
    return choices;
}

std::vector<double> makeIndexVector(std::size_t count)
{
    std::vector<double> indices;
    indices.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        indices.push_back(static_cast<double>(index));
    }
    return indices;
}

} // namespace

struct MultiDataPlotFixture {
    QStringList valueKeys;
    std::vector<double> singleCurveValues;
    std::vector<std::vector<double>> containerValues;
    std::vector<std::vector<double>> matrix2d;
    struct Selection {
        QString xtype;
        QString ytype;
    };
    std::vector<Selection> randomSelections;
};

struct MultiDataPlotOptions {
    QString dataFixturePath;
    bool plotFirstSelection = false;
};

struct NextPlotInvocation {
    QString xtype;
    QString ytype;
    QString symbol;
    QColor symbolBrush;
};

using NextPlotCallback = std::function<void(const NextPlotInvocation&)>;

class MultiDataPlotRunBinding final : public QObject {
public:
    static std::unique_ptr<MultiDataPlotRunBinding> install(const std::shared_ptr<parametertree::Parameter>& root,
                                                            NextPlotCallback callback)
    {
        if (root == nullptr || !callback) {
            return nullptr;
        }

        auto* nextPlot = root->child(QStringLiteral("next_plot"));
        if (nextPlot == nullptr) {
            return nullptr;
        }

        auto binding = std::unique_ptr<MultiDataPlotRunBinding>(new MultiDataPlotRunBinding(std::move(callback)));
        binding->bindChild(nextPlot, QStringLiteral("xtype"), &NextPlotInvocation::xtype);
        binding->bindChild(nextPlot, QStringLiteral("ytype"), &NextPlotInvocation::ytype);
        binding->bindChild(nextPlot, QStringLiteral("symbol"), &NextPlotInvocation::symbol);
        binding->bindColorChild(nextPlot, QStringLiteral("symbolBrush"), &NextPlotInvocation::symbolBrush);

        auto* runAction = dynamic_cast<parametertree::ActionParameter*>(nextPlot->child(QStringLiteral("Run")));
        if (runAction == nullptr) {
            return nullptr;
        }

        binding->connections_.push_back(QObject::connect(runAction, &parametertree::ActionParameter::sigActivated,
                                                         binding.get(), [binding = binding.get()](parametertree::Parameter*) {
                                                             binding->onRun();
                                                         }));
        return binding;
    }

private:
    explicit MultiDataPlotRunBinding(NextPlotCallback callback)
        : callback_(std::move(callback))
    {
    }

    void bindChild(parametertree::Parameter* parent, const QString& name, QString NextPlotInvocation::*field)
    {
        auto* child = parent->child(name);
        if (child == nullptr) {
            return;
        }
        cache_.*field = child->value().toString();
        connections_.push_back(QObject::connect(child, &parametertree::Parameter::sigValueChanged, this,
                                                [this, field](parametertree::Parameter*, const QVariant& value) {
                                                    cache_.*field = value.toString();
                                                }));
    }

    void bindColorChild(parametertree::Parameter* parent, const QString& name, QColor NextPlotInvocation::*field)
    {
        auto* child = parent->child(name);
        if (child == nullptr) {
            return;
        }
        cache_.*field = child->value().value<QColor>();
        connections_.push_back(QObject::connect(child, &parametertree::Parameter::sigValueChanged, this,
                                                [this, field](parametertree::Parameter*, const QVariant& value) {
                                                    cache_.*field = value.value<QColor>();
                                                }));
    }

    void onRun()
    {
        if (callback_) {
            callback_(cache_);
        }
    }

    NextPlotCallback callback_;
    NextPlotInvocation cache_;
    std::vector<QMetaObject::Connection> connections_;
};

struct MultiDataPlotExample {
    std::unique_ptr<QWidget> window;
    widgets::PlotWidget* plotWidget = nullptr;
    parametertree::ParameterTree* parameterTree = nullptr;
    std::shared_ptr<parametertree::Parameter> root;
    std::shared_ptr<MultiDataPlotFixture> fixture;
    std::unique_ptr<MultiDataPlotRunBinding> runBinding;
};

bool populateMultiDataPlotFixtureFromJson(const QJsonObject& root, MultiDataPlotFixture& fixture)
{
    const QJsonArray valueKeys = root.value(QStringLiteral("value_keys")).toArray();
    if (valueKeys.isEmpty()) {
        return false;
    }

    fixture.valueKeys.clear();
    fixture.valueKeys.reserve(valueKeys.size());
    for (const QJsonValue& value : valueKeys) {
        fixture.valueKeys.append(value.toString());
    }

    const QJsonObject values = root.value(QStringLiteral("values")).toObject();
    fixture.singleCurveValues = jsonToDoubleVector(values.value(QStringLiteral("single_curve_values")).toArray());
    fixture.containerValues = jsonToMatrix(values.value(QStringLiteral("container")).toArray());
    fixture.matrix2d = jsonToMatrix(values.value(QStringLiteral("matrix_2d")).toArray());

    const QJsonArray selections = root.value(QStringLiteral("random_selections")).toArray();
    fixture.randomSelections.clear();
    fixture.randomSelections.reserve(static_cast<std::size_t>(selections.size()));
    for (const QJsonValue& selectionValue : selections) {
        const QJsonObject selection = selectionValue.toObject();
        fixture.randomSelections.push_back({
            .xtype = selection.value(QStringLiteral("xtype")).toString(),
            .ytype = selection.value(QStringLiteral("ytype")).toString(),
        });
    }

    return fixture.valueKeys.size() == 4 && fixture.singleCurveValues.size() == 15 && fixture.containerValues.size() == 5
        && fixture.matrix2d.size() == 6 && !fixture.randomSelections.empty();
}

bool loadMultiDataPlotFixture(const QString& fixturePath, MultiDataPlotFixture& fixture)
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
    if (root.value(QStringLiteral("pinned_commit")).toString() != QString::fromUtf8(kPinnedCommit)) {
        return false;
    }

    return populateMultiDataPlotFixtureFromJson(root, fixture);
}

const std::vector<double>* fixtureVectorForType(const MultiDataPlotFixture& fixture, const QString& typeName)
{
    if (typeName == QStringLiteral("Single curve values")) {
        return &fixture.singleCurveValues;
    }
    return nullptr;
}

const std::vector<std::vector<double>>* fixtureMatrixForType(const MultiDataPlotFixture& fixture, const QString& typeName)
{
    if (typeName == QStringLiteral("2D matrix")) {
        return &fixture.matrix2d;
    }
    if (typeName == QStringLiteral("container of (optionally) mixed-size curve values")) {
        return &fixture.containerValues;
    }
    return nullptr;
}

void plotSelectionOnWidget(widgets::PlotWidget& plotWidget, const MultiDataPlotFixture& fixture,
                           const MultiDataPlotFixture::Selection& selection)
{
    plotWidget.clear();

    const auto* yVector = fixtureVectorForType(fixture, selection.ytype);
    const auto* yMatrix = fixtureMatrixForType(fixture, selection.ytype);
    const auto* xVector = fixtureVectorForType(fixture, selection.xtype);
    const auto* xMatrix = fixtureMatrixForType(fixture, selection.xtype);

    if (yMatrix != nullptr) {
        for (const std::vector<double>& row : *yMatrix) {
            std::vector<double> xValues;
            if (selection.xtype == QStringLiteral("None (replaced by integer indices)")) {
                xValues = makeIndexVector(row.size());
            } else if (xVector != nullptr) {
                xValues = *xVector;
            } else if (xMatrix != nullptr && !xMatrix->empty()) {
                xValues = xMatrix->front();
            } else {
                xValues = makeIndexVector(row.size());
            }
            plotWidget.plot(xValues, row);
        }
        return;
    }

    if (yVector == nullptr || yVector->empty()) {
        return;
    }

    std::vector<double> xValues;
    if (selection.xtype == QStringLiteral("None (replaced by integer indices)")) {
        xValues = makeIndexVector(yVector->size());
    } else if (xVector != nullptr) {
        xValues = *xVector;
    } else if (xMatrix != nullptr && !xMatrix->empty()) {
        xValues = xMatrix->front();
    } else {
        xValues = makeIndexVector(yVector->size());
    }
    plotWidget.plot(xValues, *yVector);
}

std::shared_ptr<parametertree::Parameter> buildMultiDataPlotParameterShell(const MultiDataPlotFixture& fixture)
{
    const QVariantList valueChoices = makeValueChoiceList(fixture.valueKeys);

    return parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("params")},
        {QStringLiteral("type"), QStringLiteral("group")},
        {QStringLiteral("children"),
         QVariantList{
             QVariant::fromValue(QVariantMap{
                 {QStringLiteral("name"), QStringLiteral("next_plot")},
                 {QStringLiteral("type"), QStringLiteral("group")},
                 {QStringLiteral("children"),
                  QVariantList{
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("xtype")},
                                                      {QStringLiteral("type"), QStringLiteral("list")},
                                                      {QStringLiteral("values"), valueChoices},
                                                      {QStringLiteral("value"), QStringLiteral("random")},
                                                      {QStringLiteral("default"), QStringLiteral("random")}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("ytype")},
                                                      {QStringLiteral("type"), QStringLiteral("list")},
                                                      {QStringLiteral("values"), valueChoices},
                                                      {QStringLiteral("value"), QStringLiteral("random")},
                                                      {QStringLiteral("default"), QStringLiteral("random")}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("symbol")},
                                                      {QStringLiteral("type"), QStringLiteral("list")},
                                                      {QStringLiteral("values"), makeSymbolChoiceList()},
                                                      {QStringLiteral("value"), QStringLiteral("o")},
                                                      {QStringLiteral("default"), QStringLiteral("o")}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("symbolBrush")},
                                                      {QStringLiteral("type"), QStringLiteral("color")},
                                                      {QStringLiteral("value"), QStringLiteral("#f00")},
                                                      {QStringLiteral("default"), QStringLiteral("#f00")}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("Run")},
                                                      {QStringLiteral("type"), QStringLiteral("action")}}),
                  }},
             }),
             QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("text")},
                                             {QStringLiteral("type"), QStringLiteral("text")},
                                             {QStringLiteral("readonly"), true},
                                             {QStringLiteral("value"), QString{}}}),
         }},
    });
}

std::unique_ptr<MultiDataPlotRunBinding> connectMultiDataPlotRunAction(const std::shared_ptr<parametertree::Parameter>& root,
                                                                       NextPlotCallback callback)
{
    return MultiDataPlotRunBinding::install(root, std::move(callback));
}

MultiDataPlotExample createMultiDataPlotExample(const MultiDataPlotOptions& options = {})
{
    auto fixture = std::make_shared<MultiDataPlotFixture>();
    if (!options.dataFixturePath.isEmpty()) {
        if (!loadMultiDataPlotFixture(options.dataFixturePath, *fixture)) {
            throw std::runtime_error(
                ("failed to load MultiDataPlot data fixture: " + options.dataFixturePath.toStdString()).c_str());
        }
    } else {
        fixture->valueKeys = {
            QStringLiteral("None (replaced by integer indices)"),
            QStringLiteral("Single curve values"),
            QStringLiteral("container of (optionally) mixed-size curve values"),
            QStringLiteral("2D matrix"),
        };
    }

    auto window = std::make_unique<QWidget>();
    window->setWindowTitle(QStringLiteral("pyqtgraph example: Plotting Datasets"));

    auto* layout = new QHBoxLayout(window.get());
    auto* plotWidget = new widgets::PlotWidget(window.get());
    auto* tree = new parametertree::ParameterTree(window.get());
    tree->setMinimumWidth(150);

    layout->addWidget(plotWidget);
    layout->addWidget(tree);

    auto root = buildMultiDataPlotParameterShell(*fixture);
    tree->setParameters(root, true);

    auto* textParam = root->child(QStringLiteral("text"));
    auto runBinding = connectMultiDataPlotRunAction(root, [textParam](const NextPlotInvocation& invocation) {
        if (textParam != nullptr) {
            textParam->setValue(QStringLiteral("x=%1\ny=%2").arg(invocation.xtype, invocation.ytype));
        }
    });

    if (options.plotFirstSelection && !fixture->randomSelections.empty()) {
        plotSelectionOnWidget(*plotWidget, *fixture, fixture->randomSelections.front());
    }

    return {.window = std::move(window),
            .plotWidget = plotWidget,
            .parameterTree = tree,
            .root = std::move(root),
            .fixture = std::move(fixture),
            .runBinding = std::move(runBinding)};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_MULTIDATAPLOT_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    const bool smokeMode = argc > 1 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--smoke");
    cppqtgraph::examples::MultiDataPlotOptions options;
    if (smokeMode) {
        const QString fixturePath = QString::fromLocal8Bit(
#ifdef CPPQTGRAPH_P458_FIXTURE
            CPPQTGRAPH_P458_FIXTURE
#else
            ""
#endif
        );
        if (!fixturePath.isEmpty()) {
            options.dataFixturePath = fixturePath;
            options.plotFirstSelection = true;
        }
    }

    auto example = cppqtgraph::examples::createMultiDataPlotExample(options);
    example.window->show();
    if (smokeMode) {
        QApplication::processEvents();
        example.window->close();
        return 0;
    }
    return QApplication::exec();
}
#endif
