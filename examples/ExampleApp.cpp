// Original implementation; no PyQtGraph source translation

#include "ExampleApp.hpp"
#include "ExampleRegistry.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace cppqtgraph::examples {

namespace {

QString listItemLabel(const ExampleEntry& entry)
{
    const QString status = entry.status == ExampleStatus::Ported ? QStringLiteral("runnable")
                                                                 : QStringLiteral("pending");
    return QStringLiteral("%1 (%2)").arg(entry.title, status);
}

} // namespace

ExampleAppWindow::ExampleAppWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("cppqtgraph_example_app"));
    setWindowTitle(QStringLiteral("CppQtGraph Examples"));

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);

    filterLineEdit_ = new QLineEdit(central);
    filterLineEdit_->setObjectName(QStringLiteral("example_filter"));
    filterLineEdit_->setPlaceholderText(QStringLiteral("Filter examples"));
    rootLayout->addWidget(filterLineEdit_);

    auto* splitter = new QSplitter(Qt::Horizontal, central);

    exampleList_ = new QListWidget(splitter);
    exampleList_->setObjectName(QStringLiteral("example_list"));
    splitter->addWidget(exampleList_);

    metadataPreview_ = new QLabel(splitter);
    metadataPreview_->setObjectName(QStringLiteral("example_metadata"));
    metadataPreview_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    metadataPreview_->setWordWrap(true);
    metadataPreview_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    splitter->addWidget(metadataPreview_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    rootLayout->addWidget(splitter, 1);

    runButton_ = new QPushButton(QStringLiteral("Run"), central);
    runButton_->setObjectName(QStringLiteral("example_run_button"));
    runButton_->setEnabled(false);
    rootLayout->addWidget(runButton_);

    setCentralWidget(central);
    resize(960, 640);

    rebuildExampleList();

    connect(filterLineEdit_, &QLineEdit::textChanged, this, [this](const QString&) { rebuildExampleList(); });
    connect(exampleList_, &QListWidget::currentRowChanged, this, [this](int) {
        updateMetadataPreview();
        updateRunButtonState();
    });
    connect(exampleList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        activateSelectedExample();
    });
    connect(runButton_, &QPushButton::clicked, this, [this]() { activateSelectedExample(); });

    if (exampleList_->count() > 0) {
        exampleList_->setCurrentRow(0);
        updateMetadataPreview();
        updateRunButtonState();
    }
}

QListWidget* ExampleAppWindow::exampleListWidget() const
{
    return exampleList_;
}

QLabel* ExampleAppWindow::metadataPreviewLabel() const
{
    return metadataPreview_;
}

QPushButton* ExampleAppWindow::runButton() const
{
    return runButton_;
}

QLineEdit* ExampleAppWindow::filterLineEdit() const
{
    return filterLineEdit_;
}

QString ExampleAppWindow::selectedExampleName() const
{
    const QListWidgetItem* item = exampleList_->currentItem();
    if (item == nullptr) {
        return {};
    }
    return item->data(Qt::UserRole).toString();
}

bool ExampleAppWindow::isRunEnabled() const
{
    return runButton_->isEnabled();
}

bool ExampleAppWindow::runSelectedExample()
{
    return activateSelectedExample();
}

bool ExampleAppWindow::activateSelectedExampleForTesting()
{
    return activateSelectedExample();
}

bool ExampleAppWindow::activateSelectedExample()
{
    const QString name = selectedExampleName();
    if (name.isEmpty() || !ExampleRegistry::canLaunch(name)) {
        return false;
    }

    if (launchHook_) {
        return launchHook_(name);
    }

    std::optional<LaunchedExample> launched = ExampleRegistry::launch(name);
    if (!launched.has_value()) {
        return false;
    }

    auto holder = std::make_shared<LaunchedExample>(std::move(*launched));
    holder->showAll();
    activeLaunches_.push_back(holder);
    return true;
}

void ExampleAppWindow::setLaunchHookForTesting(LaunchHook hook)
{
    launchHook_ = std::move(hook);
}

void ExampleAppWindow::clearLaunchHookForTesting()
{
    launchHook_ = {};
}

const std::vector<std::shared_ptr<LaunchedExample>>& ExampleAppWindow::activeLaunchesForTesting() const
{
    return activeLaunches_;
}

void ExampleAppWindow::rebuildExampleList()
{
    const QString filter = filterLineEdit_->text().trimmed();
    const QString previousSelection = selectedExampleName();

    exampleList_->clear();
    for (const ExampleEntry& entry : ExampleRegistry::entries()) {
        if (!filter.isEmpty()) {
            const QString haystack = entry.name + QLatin1Char(' ') + entry.title;
            if (!haystack.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }
        }

        auto* item = new QListWidgetItem(listItemLabel(entry), exampleList_);
        item->setData(Qt::UserRole, entry.name);
        if (entry.status == ExampleStatus::Planned) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
        exampleList_->addItem(item);
    }

    int rowToSelect = 0;
    if (!previousSelection.isEmpty()) {
        for (int row = 0; row < exampleList_->count(); ++row) {
            if (exampleList_->item(row)->data(Qt::UserRole).toString() == previousSelection) {
                rowToSelect = row;
                break;
            }
        }
    }

    if (exampleList_->count() > 0) {
        exampleList_->setCurrentRow(rowToSelect);
        updateMetadataPreview();
        updateRunButtonState();
    } else {
        metadataPreview_->clear();
        runButton_->setEnabled(false);
    }
}

void ExampleAppWindow::updateMetadataPreview()
{
    const QString name = selectedExampleName();
    if (name.isEmpty()) {
        metadataPreview_->clear();
        return;
    }

    for (const ExampleEntry& entry : ExampleRegistry::entries()) {
        if (entry.name == name) {
            metadataPreview_->setText(ExampleRegistry::formatMetadata(entry));
            return;
        }
    }

    metadataPreview_->clear();
}

void ExampleAppWindow::updateRunButtonState()
{
    const QString name = selectedExampleName();
    runButton_->setEnabled(!name.isEmpty() && ExampleRegistry::canLaunch(name));
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_EXAMPLEAPP_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    cppqtgraph::examples::ExampleAppWindow window;
    window.show();
    return QApplication::exec();
}
#endif
