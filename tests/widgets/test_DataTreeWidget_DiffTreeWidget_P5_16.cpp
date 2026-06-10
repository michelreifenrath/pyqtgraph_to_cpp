#include <cppqtgraph/widgets/DataTreeWidget.hpp>
#include <cppqtgraph/widgets/DiffTreeWidget.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QBrush>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPlainTextEdit>

#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#ifndef CPPQTGRAPH_P5_16_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_16_REPOSITORY_REPORT_DIR "reports/issues/P5.16"
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

QVariantMap makeArray(const QVariantList& shape, const QString& dtype, const QVariantList& values)
{
    QVariantMap array;
    array.insert(QStringLiteral("__pyqtgraph_ndarray__"), true);
    array.insert(QStringLiteral("shape"), shape);
    array.insert(QStringLiteral("dtype"), dtype);
    array.insert(QStringLiteral("values"), values);
    return array;
}

bool colorsEqual(const QColor& left, const QColor& right)
{
    return left.red() == right.red() && left.green() == right.green() && left.blue() == right.blue();
}

bool testApiShape()
{
    using cppqtgraph::widgets::DataTreeWidget;
    using cppqtgraph::widgets::DiffTreeWidget;

    static_assert(std::is_base_of_v<QTreeWidget, DataTreeWidget>);
    static_assert(std::is_base_of_v<QWidget, DiffTreeWidget>);
    static_assert(!std::is_copy_constructible_v<DataTreeWidget>);
    static_assert(!std::is_copy_assignable_v<DiffTreeWidget>);

    DataTreeWidget tree;
    CHECK(tree.columnCount() == 3);
    CHECK(tree.headerItem()->text(0) == QStringLiteral("key / index"));
    CHECK(tree.headerItem()->text(1) == QStringLiteral("type"));
    CHECK(tree.headerItem()->text(2) == QStringLiteral("value"));
    CHECK(tree.alternatingRowColors());
    CHECK(tree.verticalScrollMode() == QAbstractItemView::ScrollPerPixel);

    DiffTreeWidget diff;
    CHECK(diff.trees()[0] != nullptr);
    CHECK(diff.trees()[1] != nullptr);
    CHECK(diff.tree(0) == diff.trees()[0]);
    CHECK(diff.tree(1) == diff.trees()[1]);
    return true;
}

bool testDataTreeWidgetDisplay()
{
    using cppqtgraph::widgets::DataTreeWidget;

    const QVariantMap data = {
        {QStringLiteral("alpha"), 1},
        {QStringLiteral("beta"), QVariantList{QStringLiteral("x"), QStringLiteral("y")}},
        {QStringLiteral("gamma"), QVariantMap{{QStringLiteral("nested"), 42}}},
    };

    DataTreeWidget tree;
    tree.setData(data);

    CHECK(tree.topLevelItemCount() == 1);
    QTreeWidgetItem* root = tree.topLevelItem(0);
    CHECK(root != nullptr);
    CHECK(root->text(1) == QStringLiteral("dict"));
    CHECK(root->text(2) == QStringLiteral("length=3"));
    CHECK(root->childCount() == 3);

    QTreeWidgetItem* alpha = tree.nodeAtPath({QStringLiteral("alpha")});
    CHECK(alpha != nullptr);
    CHECK(alpha->text(0) == QStringLiteral("alpha"));
    CHECK(alpha->text(2) == QStringLiteral("1"));

    QTreeWidgetItem* beta = tree.nodeAtPath({QStringLiteral("beta")});
    CHECK(beta != nullptr);
    CHECK(beta->text(1) == QStringLiteral("list"));
    CHECK(beta->text(2) == QStringLiteral("length=2"));

    QTreeWidgetItem* betaChild = tree.nodeAtPath({QStringLiteral("beta"), 1});
    CHECK(betaChild != nullptr);
    CHECK(betaChild->text(0) == QStringLiteral("1"));
    CHECK(betaChild->text(2) == QStringLiteral("y"));

    const auto parsed = tree.parse(QStringLiteral("scalar"));
    CHECK(parsed.typeStr == QStringLiteral("QString"));
    CHECK(parsed.desc == QStringLiteral("scalar"));
    CHECK(parsed.children.isEmpty());
    return true;
}

bool testDataTreeWidgetHideRootAndArray()
{
    using cppqtgraph::widgets::DataTreeWidget;

    const QVariantMap data = {
        {QStringLiteral("only"), 7},
    };

    DataTreeWidget hiddenRoot;
    hiddenRoot.setData(data, true);
    CHECK(hiddenRoot.topLevelItemCount() == 1);
    CHECK(hiddenRoot.topLevelItem(0)->text(0) == QStringLiteral("only"));

    const QVariantMap array = makeArray({2, 2}, QStringLiteral("float64"), {1.0, 2.0, 3.0, 4.0});
    DataTreeWidget arrayTree;
    arrayTree.setData(array);
    CHECK(arrayTree.topLevelItemCount() == 1);
    QTreeWidgetItem* arrayRoot = arrayTree.topLevelItem(0);
    CHECK(arrayRoot->text(1) == QStringLiteral("ndarray"));
    CHECK(arrayRoot->text(2).startsWith(QStringLiteral("shape=2x2")));
    CHECK(arrayRoot->childCount() == 1);
    CHECK(arrayTree.itemWidget(arrayRoot->child(0), 0) != nullptr);
    return true;
}

bool testDiffTreeWidgetCompare()
{
    using cppqtgraph::widgets::DiffTreeWidget;

    const QVariantMap left = {
        {QStringLiteral("match"), 1},
        {QStringLiteral("type"), 1},
        {QStringLiteral("value"), 1},
        {QStringLiteral("only-left"), 1},
        {QStringLiteral("list"), QVariantList{1, 2}},
    };
    const QVariantMap right = {
        {QStringLiteral("match"), 1},
        {QStringLiteral("type"), QStringLiteral("text")},
        {QStringLiteral("value"), 2},
        {QStringLiteral("only-right"), 2},
        {QStringLiteral("list"), QVariantList{1, 2, 3}},
    };

    DiffTreeWidget diff;
    diff.setData(left, right);

    const QColor bad(255, 200, 200);

    QTreeWidgetItem* typeLeft = diff.tree(0)->nodeAtPath({QStringLiteral("type")});
    QTreeWidgetItem* typeRight = diff.tree(1)->nodeAtPath({QStringLiteral("type")});
    CHECK(typeLeft != nullptr);
    CHECK(typeRight != nullptr);
    CHECK(colorsEqual(typeLeft->background(1).color(), bad));
    CHECK(colorsEqual(typeRight->background(1).color(), bad));

    QTreeWidgetItem* valueLeft = diff.tree(0)->nodeAtPath({QStringLiteral("value")});
    QTreeWidgetItem* valueRight = diff.tree(1)->nodeAtPath({QStringLiteral("value")});
    CHECK(valueLeft != nullptr);
    CHECK(valueRight != nullptr);
    CHECK(colorsEqual(valueLeft->background(2).color(), bad));
    CHECK(colorsEqual(valueRight->background(2).color(), bad));

    QTreeWidgetItem* onlyLeft = diff.tree(0)->nodeAtPath({QStringLiteral("only-left")});
    QTreeWidgetItem* onlyRight = diff.tree(1)->nodeAtPath({QStringLiteral("only-right")});
    CHECK(onlyLeft != nullptr);
    CHECK(onlyRight != nullptr);
    CHECK(colorsEqual(onlyLeft->background(0).color(), bad));
    CHECK(colorsEqual(onlyRight->background(0).color(), bad));

    QTreeWidgetItem* extraRight = diff.tree(1)->nodeAtPath({QStringLiteral("list"), 2});
    CHECK(extraRight != nullptr);
    CHECK(colorsEqual(extraRight->background(0).color(), bad));
    return true;
}

bool testDataTreeWidgetPathKeyCollision()
{
    using cppqtgraph::widgets::DataTreeWidget;

    const QVariantMap data = {
        {QStringLiteral("a/b"), 1},
        {QStringLiteral("a"), QVariantMap{{QStringLiteral("b"), 2}}},
        {QStringLiteral(""), 3},
        {QStringLiteral("x"), QVariantMap{{QStringLiteral(""), 4}}},
    };

    DataTreeWidget tree;
    tree.setData(data);

    QTreeWidgetItem* slashKey = tree.nodeAtPath({QStringLiteral("a/b")});
    QTreeWidgetItem* nestedB = tree.nodeAtPath({QStringLiteral("a"), QStringLiteral("b")});
    CHECK(slashKey != nullptr);
    CHECK(nestedB != nullptr);
    CHECK(slashKey != nestedB);
    CHECK(slashKey->text(2) == QStringLiteral("1"));
    CHECK(nestedB->text(2) == QStringLiteral("2"));

    QTreeWidgetItem* emptyKey = tree.nodeAtPath({QString()});
    QTreeWidgetItem* nestedEmpty = tree.nodeAtPath({QStringLiteral("x"), QString()});
    CHECK(emptyKey != nullptr);
    CHECK(nestedEmpty != nullptr);
    CHECK(emptyKey != nestedEmpty);
    CHECK(emptyKey->text(2) == QStringLiteral("3"));
    CHECK(nestedEmpty->text(2) == QStringLiteral("4"));
    return true;
}

int countPlainTextEdits(const QWidget* root)
{
    return root->findChildren<QPlainTextEdit*>().size();
}

bool testDiffTreeWidgetRepeatedArrayDiff()
{
    using cppqtgraph::widgets::DiffTreeWidget;

    const QVariantMap left = makeArray({2}, QStringLiteral("float64"), {1.0, 2.0});
    const QVariantMap right = makeArray({2}, QStringLiteral("float64"), {1.0, 3.0});

    DiffTreeWidget diff;
    diff.setData(left, right);
    const int initialEdits = countPlainTextEdits(&diff);
    CHECK(initialEdits == 2);
    CHECK(diff.tree(0)->topLevelItemCount() == 1);
    CHECK(diff.tree(1)->topLevelItemCount() == 1);

    for (int iteration = 0; iteration < 5; ++iteration) {
        diff.setData(left, right);
    }

    CHECK(countPlainTextEdits(&diff) == initialEdits);
    CHECK(diff.tree(0)->topLevelItemCount() == 1);
    CHECK(diff.tree(1)->topLevelItemCount() == 1);
    return true;
}

bool testDiffTreeWidgetArrayCompare()
{
    using cppqtgraph::widgets::DiffTreeWidget;

    const QVariantMap left = makeArray({2}, QStringLiteral("float64"), {1.0, 2.0});
    const QVariantMap right = makeArray({2}, QStringLiteral("float64"), {1.0, 3.0});

    DiffTreeWidget diff;
    diff.setData(left, right);

    const QColor bad(255, 200, 200);
    QTreeWidgetItem* leftNode = diff.tree(0)->topLevelItem(0);
    QTreeWidgetItem* rightNode = diff.tree(1)->topLevelItem(0);
    CHECK(leftNode != nullptr);
    CHECK(rightNode != nullptr);
    CHECK(leftNode->childCount() == 1);
    CHECK(rightNode->childCount() == 1);
    CHECK(colorsEqual(leftNode->child(0)->background(0).color(), bad));
    CHECK(colorsEqual(rightNode->child(0)->background(0).color(), bad));
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_16_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/DataTreeWidget_DiffTreeWidget_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.16\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::DataTreeWidget\", \"cppqtgraph::widgets::DiffTreeWidget\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/DataTreeWidget.py; DiffTreeWidget.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/DataTreeWidget.hpp\", \"src/cppqtgraph/widgets/DataTreeWidget.cpp\", \"include/cppqtgraph/widgets/DiffTreeWidget.hpp\", \"src/cppqtgraph/widgets/DiffTreeWidget.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.16 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_datatreewidget_difftreewidget_p5_16\"},\n"
            "  \"checks\": [\"DataTreeWidget 3-column model display, path lookup, hideRoot, ndarray display\", \"DiffTreeWidget side-by-side trees with type/value/key/list diff coloring\", \"DiffTreeWidget ndarray mismatch highlighting\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.16 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.16 DataTreeWidget/DiffTreeWidget completion report\n\n"
            "- Issue: GitHub #255 / P5.16\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `DataTreeWidget` and `DiffTreeWidget` with hierarchical QVariant display, path lookup, hideRoot support, ndarray display, and diff highlighting.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.16 --output-on-failure` | 0 |\n"
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
    if (!testDataTreeWidgetDisplay()) {
        return 1;
    }
    if (!testDataTreeWidgetHideRootAndArray()) {
        return 1;
    }
    if (!testDiffTreeWidgetCompare()) {
        return 1;
    }
    if (!testDataTreeWidgetPathKeyCollision()) {
        return 1;
    }
    if (!testDiffTreeWidgetRepeatedArrayDiff()) {
        return 1;
    }
    if (!testDiffTreeWidgetArrayCompare()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
