// Original implementation; no PyQtGraph source translation

#include "ExampleRegistry.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

namespace cppqtgraph::examples {

namespace {

QVector<ExampleEntry> makeEntries()
{
    return {
        {.order = 0,
         .name = QStringLiteral("SimplePlot"),
         .title = QStringLiteral("Simple Plot smoke slice"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/SimplePlot.py"),
         .cppFile = QStringLiteral("examples/SimplePlot.cpp"),
         .status = ExampleStatus::Ported,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::NotApplicable}},
        {.order = 0,
         .name = QStringLiteral("ImageItem"),
         .title = QStringLiteral("ImageItem smoke slice"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/ImageItem.py"),
         .cppFile = QStringLiteral("examples/ImageItem.cpp"),
         .status = ExampleStatus::Ported,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::NotApplicable}},
        {.order = 1,
         .name = QStringLiteral("CLIexample"),
         .title = QStringLiteral("Command-line usage"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/CLIexample.py"),
         .cppFile = QStringLiteral("examples/CLIexample.cpp"),
         .status = ExampleStatus::Ported,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::NotApplicable}},
        {.order = 2,
         .name = QStringLiteral("Plotting"),
         .title = QStringLiteral("Basic Plotting"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/Plotting.py"),
         .cppFile = QStringLiteral("examples/Plotting.cpp"),
         .status = ExampleStatus::Ported,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 3,
         .name = QStringLiteral("ImageView"),
         .title = QStringLiteral("ImageView"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/ImageView.py"),
         .cppFile = QStringLiteral("examples/ImageView.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 4,
         .name = QStringLiteral("parametertree"),
         .title = QStringLiteral("ParameterTree"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/parametertree.py"),
         .cppFile = QStringLiteral("examples/parametertree.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 5,
         .name = QStringLiteral("MultiDataPlot"),
         .title = QStringLiteral("Plotting Datasets"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/MultiDataPlot.py"),
         .cppFile = QStringLiteral("examples/MultiDataPlot.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 6,
         .name = QStringLiteral("InteractiveParameter"),
         .title = QStringLiteral("Parameter-Function Interaction"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/InteractiveParameter.py"),
         .cppFile = QStringLiteral("examples/InteractiveParameter.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 7,
         .name = QStringLiteral("crosshair"),
         .title = QStringLiteral("Crosshair / Mouse Interaction"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/crosshair.py"),
         .cppFile = QStringLiteral("examples/crosshair.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 8,
         .name = QStringLiteral("DataSlicing"),
         .title = QStringLiteral("Data Slicing"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/DataSlicing.py"),
         .cppFile = QStringLiteral("examples/DataSlicing.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 9,
         .name = QStringLiteral("customPlot"),
         .title = QStringLiteral("Plot Customization"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/customPlot.py"),
         .cppFile = QStringLiteral("examples/customPlot.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::Required}},
        {.order = 10,
         .name = QStringLiteral("DateAxisItem"),
         .title = QStringLiteral("Timestamps on x axis"),
         .upstreamFile = QStringLiteral("pyqtgraph/examples/DateAxisItem.py"),
         .cppFile = QStringLiteral("examples/DateAxisItem.cpp"),
         .status = ExampleStatus::Planned,
         .validation = {.smoke = ValidationLevel::Required,
                        .numeric = ValidationLevel::Required,
                        .visual = ValidationLevel::Required,
                        .interaction = ValidationLevel::NotApplicable}},
    };
}

ExampleRegistry::LaunchHook gLaunchHook;
QString gExecutableSearchDirectory;

QString searchDirectory()
{
    if (!gExecutableSearchDirectory.isEmpty()) {
        return gExecutableSearchDirectory;
    }
    if (QCoreApplication::instance() != nullptr) {
        return QCoreApplication::applicationDirPath();
    }
    return {};
}

QString resolveExistingExecutablePath(const QString& fileName, const QString& directory)
{
    if (directory.isEmpty()) {
        return {};
    }

    const QString directPath = QDir(directory).filePath(fileName);
    if (QFileInfo::exists(directPath)) {
        return directPath;
    }

#if defined(Q_OS_WIN)
    const QString windowsPath = QDir(directory).filePath(fileName + QStringLiteral(".exe"));
    if (QFileInfo::exists(windowsPath)) {
        return windowsPath;
    }
#endif

    return {};
}

} // namespace

const QVector<ExampleEntry>& ExampleRegistry::entries()
{
    static const QVector<ExampleEntry> kEntries = makeEntries();
    return kEntries;
}

QString ExampleRegistry::statusLabel(ExampleStatus status)
{
    switch (status) {
    case ExampleStatus::Ported:
        return QStringLiteral("ported");
    case ExampleStatus::Planned:
        return QStringLiteral("planned");
    }
    return QStringLiteral("unknown");
}

QString ExampleRegistry::validationLevelLabel(ValidationLevel level)
{
    switch (level) {
    case ValidationLevel::Required:
        return QStringLiteral("required");
    case ValidationLevel::NotApplicable:
        return QStringLiteral("not_applicable");
    }
    return QStringLiteral("unknown");
}

QString ExampleRegistry::formatMetadata(const ExampleEntry& entry)
{
    const auto& validation = entry.validation;
    return QStringLiteral("Title: %1\n"
                          "Name: %2\n"
                          "Upstream: %3\n"
                          "C++ file: %4\n"
                          "Status: %5\n"
                          "Validation: smoke=%6, numeric=%7, visual=%8, interaction=%9")
        .arg(entry.title,
             entry.name,
             entry.upstreamFile,
             entry.cppFile,
             statusLabel(entry.status),
             validationLevelLabel(validation.smoke),
             validationLevelLabel(validation.numeric),
             validationLevelLabel(validation.visual),
             validationLevelLabel(validation.interaction));
}

bool ExampleRegistry::canLaunch(const QString& name)
{
    for (const ExampleEntry& entry : entries()) {
        if (entry.name == name) {
            return entry.status == ExampleStatus::Ported;
        }
    }
    return false;
}

QString ExampleRegistry::executableFileName(const QString& name)
{
    return QStringLiteral("cppqtgraph_examples_%1").arg(name.toLower());
}

QString ExampleRegistry::resolveExecutablePath(const QString& name)
{
    return resolveExistingExecutablePath(executableFileName(name), searchDirectory());
}

std::optional<LaunchedExample> ExampleRegistry::launch(const QString& name)
{
    if (!canLaunch(name)) {
        return std::nullopt;
    }

    if (gLaunchHook) {
        return gLaunchHook(name);
    }

    const QString fileName = executableFileName(name);
    const QString executablePath = resolveExistingExecutablePath(fileName, searchDirectory());
    if (executablePath.isEmpty()) {
        return LaunchedExample{
            .process = nullptr,
            .executablePath = fileName,
            .result = LaunchResult::MissingExecutable,
            .errorMessage = QStringLiteral("Example executable not found: %1").arg(fileName),
        };
    }

    auto process = std::make_shared<QProcess>();
    process->setProgram(executablePath);
    process->start();
    if (process->state() == QProcess::NotRunning && process->error() != QProcess::UnknownError) {
        return LaunchedExample{
            .process = process,
            .executablePath = executablePath,
            .result = LaunchResult::StartFailed,
            .errorMessage = QStringLiteral("Failed to start example: %1").arg(process->errorString()),
        };
    }

    return LaunchedExample{
        .process = process,
        .executablePath = executablePath,
        .result = LaunchResult::Started,
        .errorMessage = {},
    };
}

void ExampleRegistry::setLaunchHookForTesting(LaunchHook hook)
{
    gLaunchHook = std::move(hook);
}

void ExampleRegistry::clearLaunchHookForTesting()
{
    gLaunchHook = {};
}

void ExampleRegistry::setExecutableSearchDirectoryForTesting(const QString& directory)
{
    gExecutableSearchDirectory = directory;
}

void ExampleRegistry::clearExecutableSearchDirectoryForTesting()
{
    gExecutableSearchDirectory.clear();
}

} // namespace cppqtgraph::examples
