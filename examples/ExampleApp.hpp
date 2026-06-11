// Original implementation; no PyQtGraph source translation

#pragma once

#include <QtCore/QString>
#include <QtWidgets/QMainWindow>

#include <functional>
#include <memory>
#include <vector>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace cppqtgraph::examples {

struct LaunchedExample;

class ExampleAppWindow : public QMainWindow {
public:
    explicit ExampleAppWindow(QWidget* parent = nullptr);

    QListWidget* exampleListWidget() const;
    QLabel* metadataPreviewLabel() const;
    QPushButton* runButton() const;
    QLineEdit* filterLineEdit() const;

    QString selectedExampleName() const;
    bool isRunEnabled() const;
    bool runSelectedExample();
    bool activateSelectedExampleForTesting();

    using LaunchHook = std::function<bool(const QString& name)>;
    void setLaunchHookForTesting(LaunchHook hook);
    void clearLaunchHookForTesting();

    const std::vector<std::shared_ptr<LaunchedExample>>& activeLaunchesForTesting() const;

private:
    bool activateSelectedExample();
    void rebuildExampleList();
    void updateMetadataPreview();
    void updateRunButtonState();

    QListWidget* exampleList_ = nullptr;
    QLabel* metadataPreview_ = nullptr;
    QPushButton* runButton_ = nullptr;
    QLineEdit* filterLineEdit_ = nullptr;
    LaunchHook launchHook_;
    std::vector<std::shared_ptr<LaunchedExample>> activeLaunches_;
};

} // namespace cppqtgraph::examples
