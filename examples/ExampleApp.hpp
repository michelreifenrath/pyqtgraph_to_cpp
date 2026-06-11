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
    QLabel* statusNoticeLabel() const;
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
    void handleLaunchedExample(const QString& name, std::shared_ptr<LaunchedExample> launched);
    bool activateSelectedExample();
    void rebuildExampleList();
    void updateMetadataPreview();
    void updateRunButtonState();
    void showStatusNotice(const QString& message);

    QListWidget* exampleList_ = nullptr;
    QLabel* metadataPreview_ = nullptr;
    QLabel* statusNotice_ = nullptr;
    QPushButton* runButton_ = nullptr;
    QLineEdit* filterLineEdit_ = nullptr;
    LaunchHook launchHook_;
    std::vector<std::shared_ptr<LaunchedExample>> activeLaunches_;
};

} // namespace cppqtgraph::examples
