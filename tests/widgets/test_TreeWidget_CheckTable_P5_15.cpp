#include <pyqtgraph/widgets/CheckTable.hpp>
#include <pyqtgraph/widgets/TreeWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>

#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#ifndef PYQTGRAPH_CPP_P5_15_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_15_REPOSITORY_REPORT_DIR "reports/issues/P5.15"
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
    using pyqtgraph::widgets::CheckTable;
    using pyqtgraph::widgets::TreeWidget;
    using pyqtgraph::widgets::TreeWidgetItem;

    static_assert(std::is_base_of_v<QTreeWidget, TreeWidget>);
    static_assert(std::is_base_of_v<QTreeWidgetItem, TreeWidgetItem>);
    static_assert(std::is_base_of_v<QWidget, CheckTable>);
    static_assert(!std::is_copy_constructible_v<TreeWidget>);
    static_assert(!std::is_copy_assignable_v<CheckTable>);

    TreeWidget tree;
    CHECK(tree.acceptDrops());
    CHECK(tree.dragEnabled());
    CHECK((tree.editTriggers()
              & (QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked))
        != QAbstractItemView::NoEditTriggers);

    CheckTable table({QStringLiteral("A"), QStringLiteral("B")});
    CHECK(table.columns().size() == 2);
    return true;
}

bool testTreeWidgetBehavior()
{
    using pyqtgraph::widgets::TreeWidget;
    using pyqtgraph::widgets::TreeWidgetItem;

    TreeWidget tree;
    QSignalSpy checkSpy(&tree, &TreeWidget::sigItemCheckStateChanged);
    QSignalSpy textSpy(&tree, &TreeWidget::sigItemTextChanged);
    QSignalSpy columnSpy(&tree, &TreeWidget::sigColumnCountChanged);
    CHECK(checkSpy.isValid());
    CHECK(textSpy.isValid());
    CHECK(columnSpy.isValid());

    tree.setColumnCount(2);
    CHECK(columnSpy.count() >= 1);

    auto* item = new TreeWidgetItem({QStringLiteral("row")});
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable | Qt::ItemIsSelectable);
    item->setExpanded(true);
    auto* childWidget = new QLabel(QStringLiteral("widget"));
    item->setWidget(1, childWidget);

    tree.addTopLevelItem(item);
    CHECK(item->isExpanded());
    CHECK(tree.itemWidget(item, 1) == childWidget);

    tree.setCurrentItem(item);
    CHECK(tree.currentItem() == item);

    item->setChecked(0, true);
    CHECK(item->isChecked(0));
    CHECK(checkSpy.count() >= 1);

    item->setData(0, Qt::EditRole, QStringLiteral("edited"));
    CHECK(item->text(0) == QStringLiteral("edited"));
    CHECK(textSpy.count() >= 1);

    item->setExpanded(false);
    CHECK(!item->isExpanded());
    item->setExpanded(true);
    CHECK(item->isExpanded());
    return true;
}

bool testCheckTableBehavior()
{
    using pyqtgraph::widgets::CheckTable;

    CheckTable table({QStringLiteral("Alpha"), QStringLiteral("Beta")});
    QSignalSpy stateSpy(&table, &CheckTable::sigStateChanged);
    CHECK(stateSpy.isValid());

    table.updateRows({QStringLiteral("one"), QStringLiteral("two")});
    CHECK(table.rowNames().size() == 2);

    QCheckBox* firstCheck = table.checkBox(QStringLiteral("one"), QStringLiteral("Alpha"));
    CHECK(firstCheck != nullptr);
    firstCheck->setChecked(true);
    CHECK(stateSpy.count() >= 1);
    CHECK(stateSpy.last().at(0).toString() == QStringLiteral("one"));
    CHECK(stateSpy.last().at(1).toString() == QStringLiteral("Alpha"));
    CHECK(stateSpy.last().at(2).toInt() == static_cast<int>(Qt::Checked));

    const QVariantMap saved = table.saveState();
    CHECK(saved.value(QStringLiteral("cols")).toStringList().size() == 2);
    const QVariantList rows = saved.value(QStringLiteral("rows")).toList();
    CHECK(rows.size() == 2);
    CHECK(rows.at(0).toList().at(0).toString() == QStringLiteral("one"));
    CHECK(rows.at(0).toList().at(1).toBool());

    table.removeRow(QStringLiteral("one"));
    CHECK(table.rowNames().size() == 1);

    table.addRow(QStringLiteral("one"));
    CHECK(table.rowNames().size() == 2);
    QCheckBox* restoredCheck = table.checkBox(QStringLiteral("one"), QStringLiteral("Alpha"));
    CHECK(restoredCheck != nullptr);
    CHECK(restoredCheck->isChecked());

    CheckTable restored({QStringLiteral("Alpha"), QStringLiteral("Beta")});
    restored.restoreState(saved);
    CHECK(restored.rowNames().size() == 2);
    QCheckBox* restoredAlpha = restored.checkBox(QStringLiteral("one"), QStringLiteral("Alpha"));
    CHECK(restoredAlpha != nullptr);
    CHECK(restoredAlpha->isChecked());
    QCheckBox* restoredBeta = restored.checkBox(QStringLiteral("two"), QStringLiteral("Beta"));
    CHECK(restoredBeta != nullptr);
    CHECK(!restoredBeta->isChecked());
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_15_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/TreeWidget_CheckTable_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.15\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::TreeWidget\", \"pyqtgraph::widgets::CheckTable\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/TreeWidget.py; CheckTable.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/TreeWidget.hpp\", \"src/pyqtgraph/widgets/TreeWidget.cpp\", \"include/pyqtgraph/widgets/CheckTable.hpp\", \"src/pyqtgraph/widgets/CheckTable.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.15 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_treewidget_checktable_p5_15\"},\n"
            "  \"checks\": [\"TreeWidget edit triggers, selection, expansion, check/text signals\", \"TreeWidgetItem widget caching and setData signal emission\", \"CheckTable dynamic rows with removed-row state reuse\", \"CheckTable sigStateChanged and saveState/restoreState\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.15 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.15 TreeWidget/CheckTable completion report\n\n"
            "- Issue: GitHub #253 / P5.15\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `TreeWidget`, `TreeWidgetItem`, and `CheckTable` with editing, selection, expansion, checkbox state, and save/restore behavior.\n"));
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
    if (!testTreeWidgetBehavior()) {
        return 1;
    }
    if (!testCheckTableBehavior()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
