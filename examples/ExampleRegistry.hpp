// Original implementation; no PyQtGraph source translation

#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include <any>
#include <optional>
#include <vector>

class QWidget;

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

struct LaunchedExample {
    std::vector<QWidget*> windows;
    std::any holder;

    void showAll() const;
};

class ExampleRegistry {
public:
    static const QVector<ExampleEntry>& entries();
    static bool canLaunch(const QString& name);
    static std::optional<LaunchedExample> launch(const QString& name);
    static QString formatMetadata(const ExampleEntry& entry);
    static QString statusLabel(ExampleStatus status);
    static QString validationLevelLabel(ValidationLevel level);
};

} // namespace cppqtgraph::examples
