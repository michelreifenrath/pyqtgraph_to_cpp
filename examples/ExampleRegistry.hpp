// Original implementation; no PyQtGraph source translation

#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include <any>
#include <functional>
#include <memory>
#include <optional>

class QProcess;

namespace cppqtgraph::examples {

enum class ExampleStatus {
    Ported,
    Planned,
};

enum class ValidationLevel {
    Required,
    NotApplicable,
};

struct ValidationLevels {
    ValidationLevel smoke = ValidationLevel::Required;
    ValidationLevel numeric = ValidationLevel::Required;
    ValidationLevel visual = ValidationLevel::Required;
    ValidationLevel interaction = ValidationLevel::NotApplicable;
};

struct ExampleEntry {
    int order = 0;
    QString name;
    QString title;
    QString upstreamFile;
    QString cppFile;
    ExampleStatus status = ExampleStatus::Planned;
    ValidationLevels validation{};
};

enum class LaunchResult {
    Started,
    MissingExecutable,
    StartFailed,
};

struct LaunchedExample {
    std::shared_ptr<QProcess> process;
    QString executablePath;
    LaunchResult result = LaunchResult::Started;
    QString errorMessage;
};

class ExampleRegistry {
public:
    static const QVector<ExampleEntry>& entries();
    static bool canLaunch(const QString& name);
    static QString executableFileName(const QString& name);
    static QString resolveExecutablePath(const QString& name);
    static std::optional<LaunchedExample> launch(const QString& name);
    static QString formatMetadata(const ExampleEntry& entry);
    static QString statusLabel(ExampleStatus status);
    static QString validationLevelLabel(ValidationLevel level);

    using LaunchHook = std::function<std::optional<LaunchedExample>(const QString& name)>;
    static void setLaunchHookForTesting(LaunchHook hook);
    static void clearLaunchHookForTesting();
    static void setExecutableSearchDirectoryForTesting(const QString& directory);
    static void clearExecutableSearchDirectoryForTesting();
};

} // namespace cppqtgraph::examples
