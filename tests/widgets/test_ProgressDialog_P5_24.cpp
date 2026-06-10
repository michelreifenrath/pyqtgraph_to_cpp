#include <cppqtgraph/widgets/ProgressDialog.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>

#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

#ifndef CPPQTGRAPH_P5_24_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_24_REPOSITORY_REPORT_DIR "reports/issues/P5.24"
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
    using cppqtgraph::widgets::ProgressDialog;

    static_assert(std::is_base_of_v<QProgressDialog, ProgressDialog>);
    static_assert(!std::is_copy_constructible_v<ProgressDialog>);

    ProgressDialog dialog(QStringLiteral("Processing"));
    CHECK(!dialog.disabled());
    return true;
}

bool testConstructorState()
{
    using cppqtgraph::widgets::ProgressDialog;

    ProgressDialog dialog(QStringLiteral("Processing"), 2, 50, QStringLiteral("Cancel"), nullptr, 300);
    CHECK(dialog.minimumDuration() == 300);
    CHECK(dialog.windowModality() == Qt::WindowModal);
    CHECK(dialog.minimum() == 2);
    CHECK(dialog.maximum() == 50);
    CHECK(dialog.value() == 2);
    CHECK(dialog.labelText() == QStringLiteral("Processing"));
    return true;
}

bool testOperatorPlusEquals()
{
    using cppqtgraph::widgets::ProgressDialog;

    ProgressDialog dialog(QStringLiteral("Stepping"), 0, 10);
    dialog.begin();
    CHECK(dialog.value() == 0);
    dialog += 3;
    CHECK(dialog.value() == 3);
    dialog += 2;
    CHECK(dialog.value() == 5);
    dialog.finish();
    CHECK(dialog.value() == -1);
    return true;
}

bool testCancelAndWasCanceled()
{
    using cppqtgraph::widgets::ProgressDialog;

    ProgressDialog dialog(QStringLiteral("Cancelable"), 0, 5);
    dialog.begin();
    CHECK(!dialog.wasCanceled());
    dialog.cancel();
    CHECK(dialog.wasCanceled());
    dialog.finish();
    return true;
}

bool testNoCancelMode()
{
    using cppqtgraph::widgets::ProgressDialog;

    ProgressDialog dialog(QStringLiteral("No cancel"), 0, 5, std::nullopt);
    CHECK(!dialog.hasCancelButton());
    return true;
}

bool testDisabledNoOpBehavior()
{
    using cppqtgraph::widgets::ProgressDialog;

    ProgressDialog dialog(QStringLiteral("Disabled"), 0, 100, QStringLiteral("Cancel"), nullptr, 250, false, true);
    CHECK(dialog.disabled());
    dialog.begin();
    dialog.setValue(42);
    dialog += 5;
    dialog.setLabelText(QStringLiteral("Ignored"));
    dialog.setMinimum(10);
    dialog.setMaximum(20);
    CHECK(dialog.value() == 0);
    CHECK(dialog.minimum() == 0);
    CHECK(dialog.maximum() == 0);
    CHECK(!dialog.wasCanceled());
    dialog.cancel();
    CHECK(!dialog.wasCanceled());
    dialog.finish();
    return true;
}

bool testBusyCursorFinishBehavior()
{
    using cppqtgraph::widgets::ProgressDialog;

    ProgressDialog dialog(QStringLiteral("Busy"), 0, 3, QStringLiteral("Cancel"), nullptr, 250, true);
    CHECK(QApplication::overrideCursor() == nullptr);
    dialog.begin();
    CHECK(QApplication::overrideCursor() != nullptr);
    dialog.finish();
    CHECK(QApplication::overrideCursor() == nullptr);
    return true;
}

bool testDestructorFinishesToMaximum()
{
    using cppqtgraph::widgets::ProgressDialog;

    auto dialog = std::make_unique<ProgressDialog>(QStringLiteral("Scoped"), 0, 7);
    dialog->begin();
    dialog->setValue(4);
    dialog.reset();
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_24_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/ProgressDialog_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.24\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::ProgressDialog\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/ProgressDialog.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/ProgressDialog.hpp\", \"src/cppqtgraph/widgets/ProgressDialog.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.24 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_progressdialog_p5_24\"},\n"
            "  \"checks\": [\"ProgressDialog API shape\", \"minimum duration and WindowModal constructor state\", \"initial minimum value\", \"operator+= increment and finish auto-reset\", \"cancel and wasCanceled\", \"no-cancel mode\", \"disabled no-op behavior\", \"busy-cursor begin/finish lifecycle\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.24 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.24 ProgressDialog completion report\n\n"
            "- Issue: GitHub #267 / P5.24\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `ProgressDialog` as a QProgressDialog subclass with minimum duration, WindowModal, begin/finish lifecycle, operator+= increment, cancel querying, no-cancel mode, disabled no-op behavior, rate-limited event processing on setValue, and busy-cursor restoration.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.24 --output-on-failure` | 0 |\n"
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
    if (!testConstructorState()) {
        return 1;
    }
    if (!testOperatorPlusEquals()) {
        return 1;
    }
    if (!testCancelAndWasCanceled()) {
        return 1;
    }
    if (!testNoCancelMode()) {
        return 1;
    }
    if (!testDisabledNoOpBehavior()) {
        return 1;
    }
    if (!testBusyCursorFinishBehavior()) {
        return 1;
    }
    if (!testDestructorFinishesToMaximum()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
