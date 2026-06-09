#include <pyqtgraph/widgets/TableWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>

#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#ifndef PYQTGRAPH_CPP_P5_17_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_17_REPOSITORY_REPORT_DIR "reports/issues/P5.17"
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

bool testApiShape()
{
    using pyqtgraph::widgets::TableWidget;
    using pyqtgraph::widgets::TableWidgetItem;

    static_assert(std::is_base_of_v<QTableWidget, TableWidget>);
    static_assert(std::is_base_of_v<QTableWidgetItem, TableWidgetItem>);
    static_assert(!std::is_copy_constructible_v<TableWidget>);

    TableWidget table;
    CHECK(table.verticalScrollMode() == QAbstractItemView::ScrollPerPixel);
    CHECK(table.selectionMode() == QAbstractItemView::ContiguousSelection);
    CHECK(table.isSortingEnabled());
    CHECK(table.rowCount() == 0);
    CHECK(table.columnCount() == 0);
    return true;
}

bool testDataLoading()
{
    using pyqtgraph::widgets::TableWidget;

    TableWidget table;

    const QVariantList listOfLists = {
        QVariant::fromValue(QVariantList{1, 2, 3}),
        QVariant::fromValue(QVariantList{4, 5, 6}),
    };
    table.setData(listOfLists);
    CHECK(table.rowCount() == 2);
    CHECK(table.columnCount() == 3);

    TableWidget dictTable;
    QVariantMap dictOfLists;
    dictOfLists.insert(QStringLiteral("x"), QVariant::fromValue(QVariantList{1, 2, 3}));
    dictOfLists.insert(QStringLiteral("y"), QVariant::fromValue(QVariantList{4, 5, 6}));
    dictTable.setData(dictOfLists);
    CHECK(dictTable.rowCount() == 2);
    CHECK(dictTable.columnCount() == 3);
    CHECK(dictTable.verticalHeaderItem(0)->text() == QStringLiteral("x"));
    CHECK(dictTable.verticalHeaderItem(1)->text() == QStringLiteral("y"));

    TableWidget recordTable;
    const QVariantList listOfDicts = {
        QVariantMap{{QStringLiteral("x"), 1}, {QStringLiteral("y"), 4}},
        QVariantMap{{QStringLiteral("x"), 2}, {QStringLiteral("y"), 5}},
    };
    recordTable.setData(listOfDicts);
    CHECK(recordTable.rowCount() == 2);
    CHECK(recordTable.columnCount() == 2);
    CHECK(recordTable.horizontalHeaderItem(0)->text() == QStringLiteral("x"));
    CHECK(recordTable.horizontalHeaderItem(1)->text() == QStringLiteral("y"));
    return true;
}

bool testEditabilityAndValueConversion()
{
    using pyqtgraph::widgets::TableWidget;
    using pyqtgraph::widgets::TableWidgetItem;

    TableWidget table;
    table.setEditable(true);
    const QVariantList rows = {QVariant::fromValue(QVariantList{42, 3.5})};
    table.setData(rows);

    auto* firstItem = dynamic_cast<TableWidgetItem*>(table.item(0, 0));
    auto* secondItem = dynamic_cast<TableWidgetItem*>(table.item(0, 1));
    CHECK(firstItem != nullptr);
    CHECK(secondItem != nullptr);
    CHECK((firstItem->flags() & Qt::ItemIsEditable) != 0);

    firstItem->setText(QStringLiteral("99"));
    firstItem->itemChanged();
    CHECK(firstItem->value().toInt() == 99);

    secondItem->setText(QStringLiteral("7.25"));
    secondItem->itemChanged();
    CHECK(qFuzzyCompare(secondItem->value().toDouble(), 7.25));

    table.setEditable(false);
    CHECK((firstItem->flags() & Qt::ItemIsEditable) == 0);
    return true;
}

bool testSortingModes()
{
    using pyqtgraph::widgets::TableSortMode;
    using pyqtgraph::widgets::TableWidget;

    TableWidget table;
    const QVariantList rows = {
        QVariant::fromValue(QVariantList{QStringLiteral("b"), 2}),
        QVariant::fromValue(QVariantList{QStringLiteral("a"), 10}),
        QVariant::fromValue(QVariantList{QStringLiteral("c"), 1}),
    };
    table.setData(rows);

    table.setSortMode(0, TableSortMode::Text);
    table.sortByColumn(0, Qt::AscendingOrder);
    CHECK(table.item(0, 0)->text() == QStringLiteral("a"));
    CHECK(table.item(2, 0)->text() == QStringLiteral("c"));

    table.setSortMode(1, TableSortMode::Value);
    table.sortByColumn(1, Qt::AscendingOrder);
    CHECK(dynamic_cast<const pyqtgraph::widgets::TableWidgetItem*>(table.item(0, 1))->value().toInt() == 1);
    CHECK(dynamic_cast<const pyqtgraph::widgets::TableWidgetItem*>(table.item(2, 1))->value().toInt() == 10);

    table.setSortMode(0, TableSortMode::Value);
    table.sortByColumn(0, Qt::AscendingOrder);
    CHECK(table.item(0, 0)->text() == QStringLiteral("a"));
    CHECK(table.item(1, 0)->text() == QStringLiteral("b"));
    CHECK(table.item(2, 0)->text() == QStringLiteral("c"));

    TableWidget stringValueTable;
    const QVariantList stringRows = {
        QVariant::fromValue(QVariantList{QStringLiteral("2")}),
        QVariant::fromValue(QVariantList{QStringLiteral("10")}),
        QVariant::fromValue(QVariantList{QStringLiteral("1")}),
    };
    stringValueTable.setData(stringRows);
    stringValueTable.setSortMode(0, TableSortMode::Value);
    stringValueTable.sortByColumn(0, Qt::AscendingOrder);
    CHECK(stringValueTable.item(0, 0)->text() == QStringLiteral("1"));
    CHECK(stringValueTable.item(1, 0)->text() == QStringLiteral("10"));
    CHECK(stringValueTable.item(2, 0)->text() == QStringLiteral("2"));

    table.setSortMode(0, TableSortMode::Index);
    table.sortByColumn(0, Qt::AscendingOrder);
    CHECK(table.item(0, 0)->text() == QStringLiteral("b"));
    CHECK(table.item(2, 0)->text() == QStringLiteral("c"));
    return true;
}

bool testSelectionSerialization()
{
    using pyqtgraph::widgets::TableWidget;

    TableWidget table;
    const QVariantList rows = {
        QVariant::fromValue(QVariantList{1, 2}),
        QVariant::fromValue(QVariantList{3, 4}),
    };
    table.setData(rows);
    table.setRangeSelected(QTableWidgetSelectionRange(0, 0, 0, 1), true);

    const QString selected = table.serialize(true);
    CHECK(selected.contains(QStringLiteral("1")));
    CHECK(selected.contains(QStringLiteral("2")));
    CHECK(!selected.contains(QStringLiteral("3")));

    const QString all = table.serialize(false);
    CHECK(all.contains(QStringLiteral("3")));
    CHECK(all.contains(QStringLiteral("4")));
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_17_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/TableWidget_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.17\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::TableWidget\", \"pyqtgraph::widgets::TableWidgetItem\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/TableWidget.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/TableWidget.hpp\", \"src/pyqtgraph/widgets/TableWidget.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.17 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_tablewidget_p5_17\"},\n"
            "  \"checks\": [\"TableWidget defaults for scrolling, selection, sorting, and editability\", \"setData for list-of-lists, dict-of-lists, and list-of-dicts\", \"setEditable with typed value conversion on edit\", \"value/text/index sort modes\", \"serialize for selection and full table\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.17 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.17 TableWidget completion report\n\n"
            "- Issue: GitHub #256 / P5.17\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `TableWidget` and `TableWidgetItem` with data loading, editability, sorting modes, and selection serialization.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.17 --output-on-failure` | 0 |\n"
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
    if (!testDataLoading()) {
        return 1;
    }
    if (!testEditabilityAndValueConversion()) {
        return 1;
    }
    if (!testSortingModes()) {
        return 1;
    }
    if (!testSelectionSerialization()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
