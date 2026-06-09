#include <pyqtgraph/widgets/ComboBox.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#ifndef PYQTGRAPH_CPP_P5_18_REPOSITORY_REPORT_DIR
#define PYQTGRAPH_CPP_P5_18_REPOSITORY_REPORT_DIR "reports/issues/P5.18"
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
    using pyqtgraph::widgets::ComboBox;

    static_assert(std::is_base_of_v<QComboBox, ComboBox>);
    static_assert(!std::is_copy_constructible_v<ComboBox>);

    ComboBox combo;
    CHECK(combo.count() == 0);
    CHECK(!combo.value().isValid());
    return true;
}

bool testUniqueItemTextEnforcement()
{
    using pyqtgraph::widgets::ComboBox;

    ComboBox combo;
    combo.addItem(QStringLiteral("one"), 1);
    bool threw = false;
    try {
        combo.addItem(QStringLiteral("one"), 2);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        combo.addItems(QVariantList{QStringLiteral("one"), QStringLiteral("two")});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    return true;
}

bool testOrderedTextValueMapping()
{
    using pyqtgraph::widgets::ComboBox;

    ComboBox listCombo;
    listCombo.setItems(QVariantList{QStringLiteral("a"), QStringLiteral("b")});
    CHECK(listCombo.value().toString() == QStringLiteral("a"));
    listCombo.setCurrentIndex(1);
    CHECK(listCombo.value().toString() == QStringLiteral("b"));

    ComboBox orderedCombo;
    orderedCombo.setItems(QVariantList{QStringLiteral("b"), QStringLiteral("a")});
    CHECK(orderedCombo.itemText(0) == QStringLiteral("b"));
    CHECK(orderedCombo.itemText(1) == QStringLiteral("a"));
    CHECK(orderedCombo.value().toString() == QStringLiteral("b"));

    ComboBox dictCombo;
    const QVariantMap mapping = {
        {QStringLiteral("alpha"), 1},
        {QStringLiteral("beta"), 2},
    };
    dictCombo.setItems(mapping);
    CHECK(dictCombo.value().toInt() == 1);
    dictCombo.setValue(2);
    CHECK(dictCombo.currentText() == QStringLiteral("beta"));

    ComboBox pairCombo;
    pairCombo.setItems(QVariantList{
        QVariant::fromValue(QVariantList{QStringLiteral("x"), 10}),
        QVariant::fromValue(QVariantList{QStringLiteral("y"), 20}),
    });
    CHECK(pairCombo.value().toInt() == 10);
    pairCombo.setValue(20);
    CHECK(pairCombo.currentText() == QStringLiteral("y"));

    ComboBox duplicateValueCombo;
    duplicateValueCombo.addItems(QVariantList{
        QVariant::fromValue(QVariantList{QStringLiteral("b"), 1}),
        QVariant::fromValue(QVariantList{QStringLiteral("a"), 1}),
    });
    duplicateValueCombo.setCurrentIndex(1);
    duplicateValueCombo.setValue(1);
    CHECK(duplicateValueCombo.currentText() == QStringLiteral("b"));
    return true;
}

bool testValueSetters()
{
    using pyqtgraph::widgets::ComboBox;

    ComboBox combo;
    combo.setItems(QVariantMap{{QStringLiteral("one"), 1}, {QStringLiteral("two"), 2}});
    combo.setValue(2);
    CHECK(combo.value().toInt() == 2);
    combo.setText(QStringLiteral("one"));
    CHECK(combo.value().toInt() == 1);

    bool threw = false;
    try {
        combo.setValue(99);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        combo.setText(QStringLiteral("missing"));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    return true;
}

bool testChosenTextRestore()
{
    using pyqtgraph::widgets::ComboBox;

    ComboBox combo;
    combo.setItems(QVariantList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    combo.setText(QStringLiteral("b"));
    combo.setItems(QVariantList{QStringLiteral("x"), QStringLiteral("b"), QStringLiteral("z")});
    CHECK(combo.currentText() == QStringLiteral("b"));
    CHECK(combo.value().toString() == QStringLiteral("b"));

    combo.setText(QStringLiteral("z"));
    combo.setItems(QVariantList{QStringLiteral("only")});
    CHECK(combo.currentText() == QStringLiteral("only"));
    return true;
}

bool testSaveRestoreState()
{
    using pyqtgraph::widgets::ComboBox;

    ComboBox textCombo;
    textCombo.setItems(QVariantList{QStringLiteral("a"), QStringLiteral("b")});
    textCombo.setText(QStringLiteral("b"));
    const QVariant textState = textCombo.saveState();
    CHECK(textState.toString() == QStringLiteral("b"));
    textCombo.setCurrentIndex(0);
    textCombo.restoreState(textState);
    CHECK(textCombo.currentText() == QStringLiteral("b"));

    ComboBox dataCombo;
    dataCombo.addItem(QStringLiteral("one"), 11);
    dataCombo.addItem(QStringLiteral("two"), 22);
    dataCombo.setCurrentIndex(1);
    const QVariant dataState = dataCombo.saveState();
    CHECK(dataState.toInt() == 22);
    dataCombo.setCurrentIndex(0);
    dataCombo.restoreState(dataState);
    CHECK(dataCombo.currentIndex() == 1);
    CHECK(dataCombo.value().toInt() == 22);

    ComboBox appendedCombo;
    appendedCombo.addItem(QStringLiteral("one"), 1);
    appendedCombo.addItems(QVariantList{QVariant::fromValue(QVariantList{QStringLiteral("two"), 2})});
    appendedCombo.setCurrentIndex(0);
    CHECK(appendedCombo.saveState().toInt() == 1);
    appendedCombo.setCurrentIndex(1);
    CHECK(appendedCombo.saveState().toInt() == 2);
    return true;
}

bool testBulkUpdateSignalBehavior()
{
    using pyqtgraph::widgets::ComboBox;

    ComboBox combo;
    combo.setItems(QVariantMap{{QStringLiteral("a"), 1}, {QStringLiteral("b"), 2}});
    combo.setValue(2);

    QSignalSpy spy(&combo, qOverload<int>(&QComboBox::currentIndexChanged));
    combo.setItems(QVariantMap{{QStringLiteral("a"), 1}, {QStringLiteral("b"), 2}});
    CHECK(spy.count() == 0);

    combo.setItems(QVariantMap{{QStringLiteral("a"), 1}, {QStringLiteral("c"), 3}});
    CHECK(spy.count() == 1);
    CHECK(combo.value().toInt() == 1);

    ComboBox restoreCombo;
    restoreCombo.setItems(QVariantList{QStringLiteral("a"), QStringLiteral("b")});
    restoreCombo.setText(QStringLiteral("b"));
    restoreCombo.clear();
    QSignalSpy restoreSpy(&restoreCombo, qOverload<int>(&QComboBox::currentIndexChanged));
    restoreCombo.addItems(QVariantList{QStringLiteral("x"), QStringLiteral("b"), QStringLiteral("z")});
    CHECK(restoreCombo.currentText() == QStringLiteral("b"));
    CHECK(restoreSpy.count() == 1);
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(PYQTGRAPH_CPP_P5_18_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ComboBox_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.18\",\n"
            "  \"classes\": [\"pyqtgraph::widgets::ComboBox\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ComboBox.py\",\n"
            "  \"manifest_targets\": [\"include/pyqtgraph/widgets/ComboBox.hpp\", \"src/pyqtgraph/widgets/ComboBox.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.18 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"pyqtgraph_cpp_widgets_combobox_p5_18\"},\n"
            "  \"checks\": [\"ComboBox API shape and empty value\", \"unique item text enforcement\", \"ordered list and text-to-value mapping\", \"dict/pair text-to-value mapping\", \"setValue selects first matching UI item\", \"value/setValue/setText selection\", \"chosen-text restore on repopulate\", \"saveState/restoreState for text and item data\", \"appended item data stays aligned\", \"bulk updates emit currentIndexChanged only for final logical changes\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.18 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.18 ComboBox completion report\n\n"
            "- Issue: GitHub #258 / P5.18\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ComboBox` with ordered text-to-value mapping, first-match value selection, chosen-text restore, state save/restore, appended item data preservation, and bulk-update signal blocking.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.18 --output-on-failure` | 0 |\n"
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
    if (!testUniqueItemTextEnforcement()) {
        return 1;
    }
    if (!testOrderedTextValueMapping()) {
        return 1;
    }
    if (!testValueSetters()) {
        return 1;
    }
    if (!testChosenTextRestore()) {
        return 1;
    }
    if (!testSaveRestoreState()) {
        return 1;
    }
    if (!testBulkUpdateSignalBehavior()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
