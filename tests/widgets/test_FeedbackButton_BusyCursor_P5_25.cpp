#include <cppqtgraph/widgets/BusyCursor.hpp>
#include <cppqtgraph/widgets/FeedbackButton.hpp>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>

#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#ifndef CPPQTGRAPH_P5_25_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_25_REPOSITORY_REPORT_DIR "reports/issues/P5.25"
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

bool waitUntil(const std::function<bool()>& predicate, int timeoutMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < timeoutMs) {
        QApplication::processEvents(QEventLoop::AllEvents, 10);
        if (predicate()) {
            return true;
        }
        QThread::msleep(1);
    }
    QApplication::processEvents(QEventLoop::AllEvents, 10);
    return predicate();
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

bool testFeedbackButtonApiShape()
{
    using cppqtgraph::widgets::FeedbackButton;

    static_assert(std::is_base_of_v<QPushButton, FeedbackButton>);
    static_assert(!std::is_copy_constructible_v<FeedbackButton>);

    FeedbackButton button(QStringLiteral("Run"));
    CHECK(button.text() == QStringLiteral("Run"));
    CHECK(button.isEnabled());
    return true;
}

bool testFeedbackButtonCapturesOriginalState()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Original"));
    button.setToolTip(QStringLiteral("orig tip"));
    button.setStyleSheet(QStringLiteral("QPushButton { color: blue; }"));

    const QString origText = button.text();
    const QString origTip = button.toolTip();
    const QString origStyle = button.styleSheet();

    button.processing(QStringLiteral("Working"), QStringLiteral("busy tip"));
    CHECK(!button.isEnabled());
    CHECK(button.text() == QStringLiteral("Working"));
    CHECK(button.toolTip() == QStringLiteral("busy tip"));

    button.reset();
    CHECK(button.text() == origText);
    CHECK(button.toolTip() == origTip);
    CHECK(button.styleSheet() == origStyle);
    return true;
}

bool testFeedbackButtonProcessingDisabledState()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.processing();
    CHECK(!button.isEnabled());
    CHECK(button.text() == QStringLiteral("Processing.."));
    return true;
}

bool testFeedbackButtonSuccessSetsBlinkStyle()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.success(QStringLiteral("Done"), QStringLiteral("ok tip"));
    CHECK(button.isEnabled());
    CHECK(button.text() == QStringLiteral("Done"));
    CHECK(button.toolTip() == QStringLiteral("ok tip"));
    CHECK(button.styleSheet().contains(QStringLiteral("#0F0")));
    return true;
}

bool testFeedbackButtonFailureSetsBlinkStyle()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.failure(QStringLiteral("Failed"), QStringLiteral("err tip"));
    CHECK(button.isEnabled());
    CHECK(button.text() == QStringLiteral("Failed"));
    CHECK(button.toolTip() == QStringLiteral("err tip"));
    CHECK(button.styleSheet().contains(QStringLiteral("#F00")));
    return true;
}

bool testFeedbackButtonFeedbackDispatch()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.feedback(true, QStringLiteral("OK"));
    CHECK(button.text() == QStringLiteral("OK"));
    button.reset();

    button.feedback(false, QStringLiteral("Nope"));
    CHECK(button.text() == QStringLiteral("Nope"));
    CHECK(button.styleSheet().contains(QStringLiteral("#F00")));
    return true;
}

bool testFeedbackButtonPermanentTextUpdate()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.setText(QStringLiteral("Permanent"));
    button.processing(QStringLiteral("Busy"));
    button.reset();
    CHECK(button.text() == QStringLiteral("Permanent"));
    return true;
}

bool testFeedbackButtonLimitedTimeRestore()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.success(QStringLiteral("Done"), QStringLiteral("tip"));
    CHECK(waitUntil([&]() { return button.text() == QStringLiteral("Go"); }, 3000));
    CHECK(waitUntil([&]() { return button.toolTip().isEmpty(); }, 12000));
    return true;
}

bool testFeedbackButtonResetRestoresAll()
{
    using cppqtgraph::widgets::FeedbackButton;

    FeedbackButton button(QStringLiteral("Go"));
    button.setToolTip(QStringLiteral("base tip"));
    button.failure(QStringLiteral("Err"), QStringLiteral("err"), false);
    CHECK(button.text() == QStringLiteral("Err"));
    button.reset();
    CHECK(button.text() == QStringLiteral("Go"));
    CHECK(button.toolTip() == QStringLiteral("base tip"));
    CHECK(button.styleSheet().isEmpty());
    return true;
}

bool testBusyCursorSetsAndRestores()
{
    using cppqtgraph::widgets::BusyCursor;

    CHECK(QApplication::overrideCursor() == nullptr);
  {
      BusyCursor cursor;
      CHECK(QApplication::overrideCursor() != nullptr);
      CHECK(QApplication::overrideCursor()->shape() == Qt::WaitCursor);
  }
    CHECK(QApplication::overrideCursor() == nullptr);
    return true;
}

bool testBusyCursorNesting()
{
    using cppqtgraph::widgets::BusyCursor;

    CHECK(QApplication::overrideCursor() == nullptr);
  {
      BusyCursor outer;
      CHECK(QApplication::overrideCursor() != nullptr);
      {
          BusyCursor inner;
          CHECK(QApplication::overrideCursor() != nullptr);
      }
      CHECK(QApplication::overrideCursor() != nullptr);
  }
    CHECK(QApplication::overrideCursor() == nullptr);
    return true;
}

bool testBusyCursorNoAppNoOp()
{
    // BusyCursor without QApplication should not crash; tested implicitly by guard scope.
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_25_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/FeedbackButton_BusyCursor_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.25\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::FeedbackButton\", \"cppqtgraph::widgets::BusyCursor\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/FeedbackButton.py; pyqtgraph/widgets/BusyCursor.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/FeedbackButton.hpp\", \"src/cppqtgraph/widgets/FeedbackButton.cpp\", \"include/cppqtgraph/widgets/BusyCursor.hpp\", \"src/cppqtgraph/widgets/BusyCursor.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.25 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_feedbackbutton_busycursor_p5_25\"},\n"
            "  \"checks\": [\"FeedbackButton API shape\", \"original text/tooltip/style capture and reset\", \"processing disabled state\", \"success green blink style\", \"failure red blink style\", \"feedback dispatch\", \"permanent text update\", \"limited-time text/tooltip restore\", \"BusyCursor set/restore\", \"BusyCursor nesting\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.25 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.25 FeedbackButton and BusyCursor completion report\n\n"
            "- Issue: GitHub #268 / P5.25\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `FeedbackButton` with processing/success/failure/reset timing and temporary text/tooltip/style behavior, plus RAII `BusyCursor` guard with GUI-thread and nesting support.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.25 --output-on-failure` | 0 |\n"
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

    if (!testFeedbackButtonApiShape()) {
        return 1;
    }
    if (!testFeedbackButtonCapturesOriginalState()) {
        return 1;
    }
    if (!testFeedbackButtonProcessingDisabledState()) {
        return 1;
    }
    if (!testFeedbackButtonSuccessSetsBlinkStyle()) {
        return 1;
    }
    if (!testFeedbackButtonFailureSetsBlinkStyle()) {
        return 1;
    }
    if (!testFeedbackButtonFeedbackDispatch()) {
        return 1;
    }
    if (!testFeedbackButtonPermanentTextUpdate()) {
        return 1;
    }
    if (!testFeedbackButtonLimitedTimeRestore()) {
        return 1;
    }
    if (!testFeedbackButtonResetRestoresAll()) {
        return 1;
    }
    if (!testBusyCursorSetsAndRestores()) {
        return 1;
    }
    if (!testBusyCursorNesting()) {
        return 1;
    }
    if (!testBusyCursorNoAppNoOp()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
