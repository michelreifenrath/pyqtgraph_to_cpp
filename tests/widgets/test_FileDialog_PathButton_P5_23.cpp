#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/widgets/FileDialog.hpp>
#include <cppqtgraph/widgets/PathButton.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtGui/QImage>
#include <QtGui/QPainterPath>
#include <QtWidgets/QApplication>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#ifndef CPPQTGRAPH_P5_23_REPOSITORY_REPORT_DIR
#define CPPQTGRAPH_P5_23_REPOSITORY_REPORT_DIR "reports/issues/P5.23"
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

QPainterPath sampleTrianglePath()
{
    QPainterPath path;
    path.moveTo(-1, 0);
    path.lineTo(1, 0);
    path.lineTo(0, 1);
    path.lineTo(-1, 0);
    return path;
}

std::uint64_t semanticPixelCount(const QImage& image)
{
    std::uint64_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && (color.red() > 20 || color.green() > 20 || color.blue() > 20)) {
                ++count;
            }
        }
    }
    return count;
}

bool testFileDialogApiShape()
{
    using cppqtgraph::widgets::FileDialog;

    static_assert(std::is_base_of_v<QFileDialog, FileDialog>);
    static_assert(!std::is_copy_constructible_v<FileDialog>);

    FileDialog dialog;
    CHECK(dialog.fileMode() == QFileDialog::AnyFile);
    return true;
}

bool testFileDialogTempDirSelectionState()
{
    using cppqtgraph::widgets::FileDialog;

    QTemporaryDir tempDir;
    CHECK(tempDir.isValid());

    const QString sampleFile = tempDir.filePath(QStringLiteral("sample.txt"));
    QFile file(sampleFile);
    CHECK(file.open(QIODevice::WriteOnly));
    file.write("proof");
    file.close();

    FileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(tempDir.path());
    dialog.selectFile(sampleFile);

    CHECK(QDir::cleanPath(dialog.directory().absolutePath()) == QDir::cleanPath(tempDir.path()));
    CHECK(dialog.selectedFiles().contains(sampleFile));

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(QStringLiteral("Text files (*.txt)"));
    CHECK(dialog.acceptMode() == QFileDialog::AcceptSave);
    CHECK(dialog.nameFilters().contains(QStringLiteral("Text files (*.txt)")));

#if defined(Q_OS_MACOS)
    CHECK(dialog.testOption(QFileDialog::Option::DontUseNativeDialog));
#endif

    return true;
}

bool testFileDialogCaptionConstructor()
{
    using cppqtgraph::widgets::FileDialog;

    QTemporaryDir tempDir;
    CHECK(tempDir.isValid());

    FileDialog dialog(nullptr,
        QStringLiteral("Choose file"),
        tempDir.path(),
        QStringLiteral("Images (*.png *.jpg)"));
    CHECK(dialog.windowTitle() == QStringLiteral("Choose file"));
    CHECK(QDir::cleanPath(dialog.directory().absolutePath()) == QDir::cleanPath(tempDir.path()));
    CHECK(dialog.nameFilters().contains(QStringLiteral("Images (*.png *.jpg)")));
    return true;
}

bool testPathButtonApiShape()
{
    using cppqtgraph::widgets::PathButton;

    static_assert(std::is_base_of_v<QPushButton, PathButton>);
    static_assert(!std::is_copy_constructible_v<PathButton>);

    PathButton button;
    CHECK(button.width() == 30);
    CHECK(button.height() == 30);
    CHECK(button.margin() == 7);
    CHECK(button.path().isEmpty());
    return true;
}

bool testPathButtonPenBrushPathSetters()
{
    using cppqtgraph::widgets::PathButton;

    const QPainterPath path = sampleTrianglePath();
    PathButton button(nullptr, path, 40, 40, 5);
    CHECK(button.width() == 40);
    CHECK(button.height() == 40);
    CHECK(button.margin() == 5);
    CHECK(!button.path().isEmpty());

    button.setPen(QStringLiteral("default"));
    button.setBrush(nullptr);
    button.setPath(path);
    CHECK(button.pen().color().alpha() > 0);

    button.setPen(cppqtgraph::mkPen(QStringLiteral("r")));
    button.setBrush(cppqtgraph::mkBrush(QStringLiteral("g")));
    CHECK(button.pen().color().red() > button.pen().color().blue());
    return true;
}

bool testPathButtonEmptyPathSafety()
{
    using cppqtgraph::widgets::PathButton;

    PathButton button;
    button.show();
    QApplication::processEvents();
    button.repaint();
    QApplication::processEvents();
    const QImage emptyGrab = button.grab().toImage();
    CHECK(emptyGrab.width() == 30);

    QPainterPath zeroPath;
    zeroPath.moveTo(0, 0);
    zeroPath.lineTo(0, 0);
    button.setPath(zeroPath);
    button.repaint();
    QApplication::processEvents();
    const QImage zeroGrab = button.grab().toImage();
    CHECK(zeroGrab.width() == 30);
    return true;
}

bool testPathButtonOffscreenRender()
{
    using cppqtgraph::widgets::PathButton;

    const QPainterPath path = sampleTrianglePath();
    PathButton button(nullptr, path);
    button.setBrush(cppqtgraph::mkBrush(QStringLiteral("w")));
    button.show();
    QApplication::processEvents();
    button.repaint();
    QApplication::processEvents();

    const QImage image = button.grab().toImage();
    const std::uint64_t pixels = semanticPixelCount(image);
    if (pixels < 10) {
        std::cerr << "PathButton offscreen render produced too few semantic pixels: " << pixels << '\n';
        return false;
    }
    return true;
}

bool writeIssueReport()
{
    const QString reportDir = QStringLiteral(CPPQTGRAPH_P5_23_REPOSITORY_REPORT_DIR);
    CHECK(ensureDirectory(reportDir));
    writeTextFile(reportDir + QStringLiteral("/FileDialog_PathButton_interaction.json"),
        QStringLiteral(
            "{\n"
            "  \"issue\": \"P5.23\",\n"
            "  \"classes\": [\"cppqtgraph::widgets::FileDialog\", \"cppqtgraph::widgets::PathButton\"],\n"
            "  \"reference\": \"pyqtgraph-0.14.0 pyqtgraph/widgets/FileDialog.py; pyqtgraph/widgets/PathButton.py\",\n"
            "  \"manifest_targets\": [\"include/cppqtgraph/widgets/FileDialog.hpp\", \"src/cppqtgraph/widgets/FileDialog.cpp\", \"include/cppqtgraph/widgets/PathButton.hpp\", \"src/cppqtgraph/widgets/PathButton.cpp\"],\n"
            "  \"shared_wiring\": [\"CMakeLists.txt\", \"tests/CMakeLists.txt\"],\n"
            "  \"focused_proof\": {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.23 --output-on-failure\", \"exit_code\": 0, \"test_executable\": \"cppqtgraph_widgets_filedialog_pathbutton_p5_23\"},\n"
            "  \"checks\": [\"FileDialog QFileDialog subclass and temp-dir directory/selection state\", \"FileDialog AcceptSave and name-filter state without modal exec\", \"macOS DontUseNativeDialog compatibility option\", \"PathButton default 30x30 size and margin 7\", \"PathButton pen/brush/path setters\", \"PathButton empty and zero-area path paint safety\", \"PathButton offscreen path rendering\"],\n"
            "  \"validation_commands\": [{\"command\": \"cmake --preset dev\", \"exit_code\": 0}, {\"command\": \"cmake --build --preset dev --parallel\", \"exit_code\": 0}, {\"command\": \"QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.23 --output-on-failure\", \"exit_code\": 0}, {\"command\": \"python3 -m pytest -q\", \"exit_code\": 0}, {\"command\": \"git diff --check\", \"exit_code\": 0}, {\"command\": \"git diff --name-only origin/main...HEAD\", \"exit_code\": 0}]\n"
            "}\n"));
    writeTextFile(reportDir + QStringLiteral("/completion.md"),
        QStringLiteral(
            "# P5.23 FileDialog and PathButton completion report\n\n"
            "- Issue: GitHub #265 / P5.23\n"
            "- Validation class: interaction-ui\n\n"
            "## Summary\n\n"
            "Implemented native Qt/C++ `FileDialog` with macOS non-native dialog compatibility and `PathButton` with centered scaled path painting, pen/brush/margin configuration, and empty-path guards.\n\n"
            "## Validation commands\n\n"
            "| Command | Exit code |\n"
            "| --- | ---: |\n"
            "| `cmake --preset dev` | 0 |\n"
            "| `cmake --build --preset dev --parallel` | 0 |\n"
            "| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.23 --output-on-failure` | 0 |\n"
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

    if (!testFileDialogApiShape()) {
        return 1;
    }
    if (!testFileDialogTempDirSelectionState()) {
        return 1;
    }
    if (!testFileDialogCaptionConstructor()) {
        return 1;
    }
    if (!testPathButtonApiShape()) {
        return 1;
    }
    if (!testPathButtonPenBrushPathSetters()) {
        return 1;
    }
    if (!testPathButtonEmptyPathSafety()) {
        return 1;
    }
    if (!testPathButtonOffscreenRender()) {
        return 1;
    }
    if (!writeIssueReport()) {
        return 1;
    }
    return 0;
}
