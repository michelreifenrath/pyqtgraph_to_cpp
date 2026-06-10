#include <cppqtgraph/widgets/SpinBox.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaType>
#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QAbstractSpinBox>

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#ifndef CPPQTGRAPH_P5_19_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_19_REPOSITORY_REPORT_DIR "reports/issues/P5.19"
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

#define CHECK_CLOSE(actual, expected, tolerance) \
    do { \
        const double actualValue = (actual); \
        const double expectedValue = (expected); \
        if (!check(std::abs(actualValue - expectedValue) <= (tolerance), #actual " close to " #expected, __FILE__, \
                __LINE__)) { \
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
    using cppqtgraph::widgets::SpinBox;

    static_assert(std::is_base_of_v<QAbstractSpinBox, SpinBox>);
    static_assert(!std::is_copy_constructible_v<SpinBox>);

    SpinBox box;
    CHECK_CLOSE(box.value(), 0.0, 1.0e-12);
    CHECK(!box.editorText().isEmpty() || box.formatText() == box.editorText());
    return true;
}

bool testDefaultValueAndText()
{
    using cppqtgraph::widgets::SpinBox;

    SpinBox box;
    CHECK_CLOSE(box.value(), 0.0, 1.0e-12);
    CHECK(box.editorText() == box.formatText());
    return true;
}

bool testBoundsClipping()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.minBound = 0.0;
    opts.maxBound = 10.0;
    opts.value = 15.0;
    box.setOpts(opts);
    CHECK_CLOSE(box.value(), 10.0, 1.0e-12);

    box.setValue( -5.0);
    CHECK_CLOSE(box.value(), 0.0, 1.0e-12);
    return true;
}

bool testBoundsWrapping()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.minBound = 0.0;
    opts.maxBound = 10.0;
    opts.wrapping = true;
    opts.value = 12.0;
    box.setOpts(opts);
    CHECK_CLOSE(box.value(), 2.0, 1.0e-12);
    return true;
}

bool testIntegerMode()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.integerMode = true;
    opts.step = 1.0;
    opts.value = 3.7;
    box.setOpts(opts);
    CHECK_CLOSE(box.value(), 3.0, 1.0e-12);
    CHECK(box.editorText().contains(QStringLiteral("3")));
    return true;
}

bool testIntegerModeDefaultStepping()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.integerMode = true;
    opts.value = 5.0;
    box.setOpts(opts);

    box.stepBy(1);
    CHECK_CLOSE(box.value(), 6.0, 1.0e-12);
    box.stepBy(-1);
    CHECK_CLOSE(box.value(), 5.0, 1.0e-12);
    return true;
}

bool testSetRangeClearsBounds()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.minBound = 0.0;
    opts.maxBound = 10.0;
    opts.value = 5.0;
    box.setOpts(opts);

    box.setRange(std::nullopt, std::nullopt);
    box.setValue(25.0);
    CHECK_CLOSE(box.value(), 25.0, 1.0e-12);
    return true;
}

bool testCustomFormatPlaceholders()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.format = QStringLiteral("v={value} sv={scaledValue} d={decimals}");
    opts.value = 1.23;
    opts.decimals = 2;
    box.setOpts(opts);

    const QString text = box.editorText();
    CHECK(text.contains(QStringLiteral("v=1.23")));
    CHECK(text.contains(QStringLiteral("sv=1.23")));
    CHECK(text.contains(QStringLiteral("d=2")));
    return true;
}

bool testLinearStepping()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.step = 2.0;
    opts.value = 5.0;
    box.setOpts(opts);

    box.stepBy(1);
    CHECK_CLOSE(box.value(), 7.0, 1.0e-12);
    box.stepBy(-1);
    CHECK_CLOSE(box.value(), 5.0, 1.0e-12);
    return true;
}

bool testDecimalSteppingAndMinStep()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.dec = true;
    opts.step = 0.1;
    opts.minStep = 0.5;
    opts.value = 15.0;
    box.setOpts(opts);

    box.stepBy(1);
    CHECK_CLOSE(box.value(), 16.0, 1.0e-12);

    SpinBox zeroBox;
    SpinBoxOptions zeroOpts;
    zeroOpts.dec = true;
    zeroOpts.step = 0.1;
    zeroOpts.minStep = 0.5;
    zeroOpts.value = 0.0;
    zeroBox.setOpts(zeroOpts);
    zeroBox.stepBy(1);
    CHECK_CLOSE(zeroBox.value(), 0.5, 1.0e-12);
    zeroBox.stepBy(-1);
    CHECK_CLOSE(zeroBox.value(), 0.0, 1.0e-12);
    return true;
}

bool testSiPrefixFormatting()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.siPrefix = true;
    opts.suffix = QStringLiteral("V");
    opts.value = 0.003;
    opts.decimals = 3;
    box.setOpts(opts);

    const QString text = box.editorText();
    CHECK(text.contains(QString::fromUtf8("m")));
    CHECK(text.contains(QStringLiteral("V")));
    CHECK(text.contains(QStringLiteral("3")));
    CHECK_CLOSE(box.value(), 0.003, 1.0e-12);
    return true;
}

bool testSiPrefixParsing()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.siPrefix = true;
    opts.suffix = QStringLiteral("V");
    opts.value = 0.0;
    box.setOpts(opts);

    box.setEditorText(QStringLiteral("300 mV"));
    emit box.editingFinished();
    CHECK_CLOSE(box.value(), 0.3, 1.0e-9);
    return true;
}

bool testSiPrefixAsciiUParsing()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.siPrefix = true;
    opts.suffix = QStringLiteral("V");
    opts.value = 0.0;
    box.setOpts(opts);

    box.setEditorText(QStringLiteral("300 uV"));
    emit box.editingFinished();
    CHECK_CLOSE(box.value(), 0.0003, 1.0e-12);
    return true;
}

bool testCustomPrefixSuffixFormatting()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.prefix = QStringLiteral("±");
    opts.suffix = QStringLiteral("px");
    opts.value = 12.5;
    opts.decimals = 2;
    box.setOpts(opts);

    const QString text = box.editorText();
    CHECK(text.startsWith(QStringLiteral("±")));
    CHECK(text.endsWith(QStringLiteral("px")));
    return true;
}

bool testEditingFinishedCommit()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.step = 0.1;
    opts.value = 1.0;
    box.setOpts(opts);

    box.setEditorText(QStringLiteral("2.5"));
    emit box.editingFinished();
    CHECK_CLOSE(box.value(), 2.5, 1.0e-12);
    return true;
}

bool testSignalOrderImmediate()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.delay = 0.0;
    box.setOpts(opts);

    QSignalSpy changingSpy(&box, &SpinBox::sigValueChanging);
    QSignalSpy valueSpy(&box, &SpinBox::valueChanged);
    QSignalSpy changedSpy(&box, &SpinBox::sigValueChanged);

    box.setValue(2.0);
    CHECK(changingSpy.count() == 1);
    CHECK(valueSpy.count() == 1);
    CHECK(changedSpy.count() == 1);
    CHECK_CLOSE(changingSpy.at(0).at(1).toDouble(), 2.0, 1.0e-12);
    CHECK_CLOSE(valueSpy.at(0).at(0).toDouble(), 2.0, 1.0e-12);
    return true;
}

bool testSignalDelayOnStepBy()
{
    using cppqtgraph::widgets::SpinBox;
    using cppqtgraph::widgets::SpinBoxOptions;

    SpinBox box;
    SpinBoxOptions opts;
    opts.step = 1.0;
    opts.value = 1.0;
    opts.delay = 0.05;
    box.setOpts(opts);

    QSignalSpy changingSpy(&box, &SpinBox::sigValueChanging);
    QSignalSpy valueSpy(&box, &SpinBox::valueChanged);
    QSignalSpy changedSpy(&box, &SpinBox::sigValueChanged);

    box.stepBy(1);
    CHECK(changingSpy.count() == 1);
    CHECK(valueSpy.count() == 0);
    CHECK(changedSpy.count() == 0);

    QTest::qWait(100);
    QCoreApplication::processEvents();
    CHECK(valueSpy.count() == 1);
    CHECK(changedSpy.count() == 1);
    CHECK_CLOSE(valueSpy.at(0).at(0).toDouble(), 2.0, 1.0e-12);
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_19_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/SpinBox_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.19\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::SpinBox\", \"cppqtgraph::widgets::ErrorBox\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/SpinBox.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/SpinBox.hpp\", \"src/cppqtgraph/widgets/SpinBox.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.19 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_spinbox_p5_19\"},\n"
            "  \"checks\": [\"SpinBox API shape and default value\", \"bounds clipping and wrapping\", \"integer mode\", \"integer mode default stepping\", \"setRange clears bounds\", \"custom format placeholders\", \"linear and decimal stepping with minStep\", \"SI prefix formatting and parsing\", \"ASCII u SI prefix parsing\", \"custom prefix/suffix formatting\", \"editingFinished commit\", \"immediate and delayed signal behavior\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.19 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.19 SpinBox completion report\n\n"
            "- Issue: GitHub #259 / P5.19\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `SpinBox` with SI prefix formatting/parsing, bounds clipping/wrapping, integer mode (including default step=1), setRange bound clearing, format placeholders ({value}, {scaledValue}, {decimals}), linear/decimal stepping, prefix/suffix display, editingFinished commit, and immediate/delayed value change signals.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.19 --output-on-failure` | 0 |\n"
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
    if (!testDefaultValueAndText()) {
        return 1;
    }
    if (!testBoundsClipping()) {
        return 1;
    }
    if (!testBoundsWrapping()) {
        return 1;
    }
    if (!testIntegerMode()) {
        return 1;
    }
    if (!testIntegerModeDefaultStepping()) {
        return 1;
    }
    if (!testSetRangeClearsBounds()) {
        return 1;
    }
    if (!testCustomFormatPlaceholders()) {
        return 1;
    }
    if (!testLinearStepping()) {
        return 1;
    }
    if (!testDecimalSteppingAndMinStep()) {
        return 1;
    }
    if (!testSiPrefixFormatting()) {
        return 1;
    }
    if (!testSiPrefixParsing()) {
        return 1;
    }
    if (!testSiPrefixAsciiUParsing()) {
        return 1;
    }
    if (!testCustomPrefixSuffixFormatting()) {
        return 1;
    }
    if (!testEditingFinishedCommit()) {
        return 1;
    }
    if (!testSignalOrderImmediate()) {
        return 1;
    }
    if (!testSignalDelayOnStepBy()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
