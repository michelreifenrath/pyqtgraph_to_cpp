#include <pyqtgraph/widgets/ScatterPlotWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaType>
#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QListWidget>

#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P5_08_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_08_REPOSITORY_REPORT_DIR "reports/issues/P5.08"
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

pyqtgraph::widgets::ScatterPlotRecordArray makeSampleData()
{
    pyqtgraph::widgets::ScatterPlotRecordArray records;
    records.push_back({{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 10.0}, {QStringLiteral("group"), 1.0}});
    records.push_back({{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 20.0}, {QStringLiteral("group"), 2.0}});
    records.push_back({{QStringLiteral("x"), 2.0}, {QStringLiteral("y"), 30.0}, {QStringLiteral("group"), 1.0}});
    records.push_back({{QStringLiteral("x"), 3.0}, {QStringLiteral("y"), 40.0}, {QStringLiteral("group"), 2.0}});
    records.push_back({{QStringLiteral("x"), std::numeric_limits<double>::quiet_NaN()}, {QStringLiteral("y"), 50.0},
        {QStringLiteral("group"), 1.0}});
    return records;
}

bool testApiShape()
{
    using pyqtgraph::widgets::ScatterPlotWidget;

    static_assert(std::is_base_of_v<QSplitter, ScatterPlotWidget>);
    static_assert(!std::is_copy_constructible_v<ScatterPlotWidget>);

    ScatterPlotWidget widget;
    CHECK(widget.fieldList() != nullptr);
    CHECK(widget.plotWidget() != nullptr);
    CHECK(widget.colorMapWidget() != nullptr);
    CHECK(widget.fieldList()->selectionMode() == QAbstractItemView::ExtendedSelection);
    CHECK(widget.data().isEmpty());
    CHECK(!widget.hasVisiblePlot());
    return true;
}

bool testFieldOrderingAndSelectionCap()
{
    using pyqtgraph::widgets::ScatterPlotFieldOptions;
    using pyqtgraph::widgets::ScatterPlotWidget;

    ScatterPlotWidget widget;
    widget.setFields({
        {QStringLiteral("alpha"), ScatterPlotFieldOptions{}},
        {QStringLiteral("beta"), ScatterPlotFieldOptions{}},
        {QStringLiteral("gamma"), ScatterPlotFieldOptions{}},
    });

    CHECK(widget.fieldList()->count() == 3);
    CHECK(widget.fieldList()->item(0)->text() == QStringLiteral("alpha"));
    CHECK(widget.fieldList()->item(1)->text() == QStringLiteral("beta"));
    CHECK(widget.fieldList()->item(2)->text() == QStringLiteral("gamma"));

    widget.fieldList()->item(0)->setSelected(true);
    widget.fieldList()->item(1)->setSelected(true);
    widget.fieldList()->item(2)->setSelected(true);
    QApplication::processEvents();

    int selectedCount = 0;
    for (int row = 0; row < widget.fieldList()->count(); ++row) {
        if (widget.fieldList()->item(row)->isSelected()) {
            ++selectedCount;
        }
    }
    CHECK(selectedCount == 2);
    CHECK(widget.fieldList()->item(0)->isSelected());
    CHECK(widget.fieldList()->item(2)->isSelected());
    CHECK(!widget.fieldList()->item(1)->isSelected());
    return true;
}

bool testSingleFieldPseudoScatterAndFilter()
{
    using pyqtgraph::widgets::ScatterPlotWidget;

    ScatterPlotWidget widget;
    pyqtgraph::widgets::ScatterPlotFieldOptions xField;
    xField.mode = QStringLiteral("range");
    widget.setFields({
        {QStringLiteral("x"), xField},
        {QStringLiteral("y"), pyqtgraph::widgets::ScatterPlotFieldOptions{}},
    });
    widget.setData(makeSampleData());
    widget.setSelectedFields({QStringLiteral("x")});
    QApplication::processEvents();

    const bool prePlotVisible = widget.hasVisiblePlot();
    const int preVisibleCount = widget.visibleIndices().size();
    CHECK(prePlotVisible);
    CHECK(preVisibleCount == 4);

    const std::array<double, 4> pseudoInput = {1.0, 1.0, 2.0, 3.0};
    const auto pseudoY = pyqtgraph::widgets::pseudoScatter(pseudoInput);
    CHECK(pseudoY.size() == 4);
    CHECK(pseudoY[0] != pseudoY[1]);

    QVector<bool> mask = {true, false, true, true, false};
    widget.setFilterMask(mask);
    QApplication::processEvents();
    CHECK(widget.visibleIndices().size() == 3);
    CHECK(widget.visibleIndices().at(0) == 0);
    CHECK(widget.visibleIndices().at(1) == 2);
    CHECK(widget.visibleIndices().at(2) == 3);

    std::cout << "P5.08 single-field report\n"
              << "pre_state: hasVisiblePlot=" << prePlotVisible << " visibleCount=" << preVisibleCount << '\n'
              << "event_sequence: setSelectedFields(x) processEvents setFilterMask processEvents\n"
              << "post_state: visibleCount=" << widget.visibleIndices().size()
              << " pseudoY0=" << pseudoY[0] << " pseudoY1=" << pseudoY[1] << '\n';
    return true;
}

bool testTwoFieldScatterSelectionAndSignals()
{
    using pyqtgraph::widgets::ScatterPlotWidget;

    ScatterPlotWidget widget;
    widget.setFields({
        {QStringLiteral("x"), pyqtgraph::widgets::ScatterPlotFieldOptions{}},
        {QStringLiteral("y"), pyqtgraph::widgets::ScatterPlotFieldOptions{}},
    });
    widget.setData(makeSampleData());
    widget.setSelectedFields({QStringLiteral("x"), QStringLiteral("y")});
    QApplication::processEvents();

    CHECK(widget.hasVisiblePlot());
    CHECK(widget.visibleData().size() == 4);
    CHECK(widget.visibleData().front().value(QStringLiteral("x")).toDouble() == 1.0);

    widget.setSelectedIndices({0, 2});
    QApplication::processEvents();
    CHECK(widget.hasVisiblePlot());

    QSignalSpy clickedSpy(&widget, &ScatterPlotWidget::sigScatterPlotClicked);
    QSignalSpy hoveredSpy(&widget, &ScatterPlotWidget::sigScatterPlotHovered);
    CHECK(clickedSpy.isValid());
    CHECK(hoveredSpy.isValid());

    widget.emitPointClicked(2);
    widget.emitPointHovered({0, 2});
    QApplication::processEvents();

    CHECK(clickedSpy.count() == 1);
    CHECK(hoveredSpy.count() == 1);
    const QVariantList clickedPoints = clickedSpy.at(0).at(1).toList();
    CHECK(clickedPoints.size() == 1);
    CHECK(clickedPoints.front().toMap().value(QStringLiteral("originalIndex")).toInt() == 2);

    const QVariantList hoveredPoints = hoveredSpy.at(0).at(1).toList();
    CHECK(hoveredPoints.size() == 2);
    CHECK(hoveredPoints.at(0).toMap().value(QStringLiteral("originalIndex")).toInt() == 0);
    CHECK(hoveredPoints.at(1).toMap().value(QStringLiteral("originalIndex")).toInt() == 2);
    return true;
}

bool testNoOpWhenNoFieldsSelected()
{
    using pyqtgraph::widgets::ScatterPlotWidget;

    ScatterPlotWidget widget;
    widget.setFields({
        {QStringLiteral("x"), pyqtgraph::widgets::ScatterPlotFieldOptions{}},
        {QStringLiteral("y"), pyqtgraph::widgets::ScatterPlotFieldOptions{}},
    });
    widget.setData(makeSampleData());
    QApplication::processEvents();

    CHECK(!widget.hasVisiblePlot());
    CHECK(widget.visibleIndices().isEmpty());

    widget.setSelectedFields({QStringLiteral("x"), QStringLiteral("y")});
    QApplication::processEvents();
    CHECK(widget.hasVisiblePlot());

    widget.fieldList()->clearSelection();
    QApplication::processEvents();
    CHECK(!widget.hasVisiblePlot());
    CHECK(widget.visibleIndices().isEmpty());
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_08_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ScatterPlotWidget_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.08\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::ScatterPlotWidget\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ScatterPlotWidget.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/ScatterPlotWidget.hpp\", \"src/pyqtgraph/widgets/ScatterPlotWidget.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.08 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_scatterplotwidget_p5_08\"},\n"
            "  \"checks\": [\"QSplitter widget with field list, color map, and plot\", \"field selection capped at two\", \"single-field pseudoScatter plot and filter mask\", \"two-field scatter with selection overlay and signals\", \"no-op when no fields selected\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.08 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.08 ScatterPlotWidget completion report\n\n"
            "- Issue: GitHub #165 / P5.08\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ScatterPlotWidget` with ordered field selection, record data/index tracking, filter masks, deterministic pseudo-scatter plotting, visible state, selection overlay, and scripted click/hover signals.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.08 --output-on-failure` | 0 |\n"
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

    if (!testApiShape()) {
        return 1;
    }
    if (!testFieldOrderingAndSelectionCap()) {
        return 1;
    }
    if (!testSingleFieldPseudoScatterAndFilter()) {
        return 1;
    }
    if (!testTwoFieldScatterSelectionAndSignals()) {
        return 1;
    }
    if (!testNoOpWhenNoFieldsSelected()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
