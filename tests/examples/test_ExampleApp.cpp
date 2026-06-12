#include "../../examples/ExampleRegistry.cpp"

#define CPPQTGRAPH_EXAMPLEAPP_NO_MAIN
#include "../../examples/ExampleApp.cpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>

#include <iostream>
#include <memory>
#include <string_view>

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

int findListRowByName(QListWidget* list, const QString& name)
{
    for (int row = 0; row < list->count(); ++row) {
        if (list->item(row)->data(Qt::UserRole).toString() == name) {
            return row;
        }
    }
    return -1;
}

std::optional<cppqtgraph::examples::LaunchedExample> makeFalseBinaryLaunch(const QString& /*name*/)
{
    auto process = std::make_shared<QProcess>();
    process->setProgram(QStringLiteral("/bin/false"));
    process->start();
    if (!process->waitForStarted(3000)) {
        return std::nullopt;
    }

    return cppqtgraph::examples::LaunchedExample{
        .process = process,
        .executablePath = QStringLiteral("/bin/false"),
        .result = cppqtgraph::examples::LaunchResult::Started,
        .errorMessage = {},
    };
}

std::optional<cppqtgraph::examples::LaunchedExample> makeAsyncStartFailureLaunch(const QString& name)
{
    auto process = std::make_shared<QProcess>();
    const QString missingPath = QStringLiteral("/nonexistent/cppqtgraph_examples_%1").arg(name);
    process->setProgram(missingPath);
    process->start();

    return cppqtgraph::examples::LaunchedExample{
        .process = process,
        .executablePath = missingPath,
        .result = cppqtgraph::examples::LaunchResult::Started,
        .errorMessage = {},
    };
}

bool testRegistryOrderAndStatus()
{
    const QVector<cppqtgraph::examples::ExampleEntry>& entries = cppqtgraph::examples::ExampleRegistry::entries();
    CHECK(entries.size() == 11);

    CHECK(entries[0].name == QStringLiteral("ImageItem"));
    CHECK(entries[0].status == cppqtgraph::examples::ExampleStatus::Ported);
    CHECK(entries[1].name == QStringLiteral("CLIexample"));
    CHECK(entries[1].status == cppqtgraph::examples::ExampleStatus::Ported);
    CHECK(entries[2].name == QStringLiteral("Plotting"));
    CHECK(entries[2].status == cppqtgraph::examples::ExampleStatus::Ported);
    CHECK(entries[3].name == QStringLiteral("ImageView"));
    CHECK(entries[3].status == cppqtgraph::examples::ExampleStatus::Planned);
    CHECK(entries[10].name == QStringLiteral("DateAxisItem"));
    CHECK(entries[10].status == cppqtgraph::examples::ExampleStatus::Planned);

    // SimplePlot is intentionally unlisted (upstream browser marks it trivial).
    CHECK(!cppqtgraph::examples::ExampleRegistry::canLaunch(QStringLiteral("SimplePlot")));
    CHECK(cppqtgraph::examples::ExampleRegistry::canLaunch(QStringLiteral("ImageItem")));
    CHECK(cppqtgraph::examples::ExampleRegistry::canLaunch(QStringLiteral("Plotting")));
    CHECK(!cppqtgraph::examples::ExampleRegistry::canLaunch(QStringLiteral("ImageView")));
    CHECK(!cppqtgraph::examples::ExampleRegistry::canLaunch(QStringLiteral("unknown")));

    return true;
}

bool testRegistryLaunchPendingFailsClosed()
{
    CHECK(!cppqtgraph::examples::ExampleRegistry::launch(QStringLiteral("ImageView")).has_value());
    CHECK(!cppqtgraph::examples::ExampleRegistry::launch(QStringLiteral("parametertree")).has_value());
    return true;
}

bool testRegistryResolvesBuiltExecutables()
{
    for (const cppqtgraph::examples::ExampleEntry& entry : cppqtgraph::examples::ExampleRegistry::entries()) {
        if (entry.status != cppqtgraph::examples::ExampleStatus::Ported) {
            continue;
        }

        const QString path = cppqtgraph::examples::ExampleRegistry::resolveExecutablePath(entry.name);
        CHECK(!path.isEmpty());
        CHECK(QFileInfo::exists(path));
        CHECK(path.endsWith(cppqtgraph::examples::ExampleRegistry::executableFileName(entry.name)));
    }

    return true;
}

bool testRegistryLaunchesPortedExamplesViaHook()
{
    int hookCalls = 0;
    cppqtgraph::examples::ExampleRegistry::setLaunchHookForTesting(
        [&hookCalls](const QString& name) -> std::optional<cppqtgraph::examples::LaunchedExample> {
            ++hookCalls;
            return cppqtgraph::examples::LaunchedExample{
                .process = std::make_shared<QProcess>(),
                .executablePath = QStringLiteral("mock://%1").arg(name),
                .result = cppqtgraph::examples::LaunchResult::Started,
                .errorMessage = {},
            };
        });

    int portedCount = 0;
    for (const cppqtgraph::examples::ExampleEntry& entry : cppqtgraph::examples::ExampleRegistry::entries()) {
        if (entry.status != cppqtgraph::examples::ExampleStatus::Ported) {
            continue;
        }

        ++portedCount;
        std::optional<cppqtgraph::examples::LaunchedExample> launched =
            cppqtgraph::examples::ExampleRegistry::launch(entry.name);
        CHECK(launched.has_value());
        CHECK(launched->result == cppqtgraph::examples::LaunchResult::Started);
        CHECK(launched->executablePath.contains(entry.name));
    }

    CHECK(portedCount == 3);
    CHECK(hookCalls == 3);

    cppqtgraph::examples::ExampleRegistry::clearLaunchHookForTesting();
    return true;
}

bool testRegistryLaunchMissingExecutable()
{
    cppqtgraph::examples::ExampleRegistry::setExecutableSearchDirectoryForTesting(
        QStringLiteral("/nonexistent/example/search/path"));

    std::optional<cppqtgraph::examples::LaunchedExample> launched =
        cppqtgraph::examples::ExampleRegistry::launch(QStringLiteral("Plotting"));
    CHECK(launched.has_value());
    CHECK(launched->result == cppqtgraph::examples::LaunchResult::MissingExecutable);
    CHECK(launched->errorMessage.contains(QStringLiteral("not found")));

    cppqtgraph::examples::ExampleRegistry::clearExecutableSearchDirectoryForTesting();
    return true;
}

bool testExampleAppListAndMetadata()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();
    CHECK(list != nullptr);
    CHECK(list->count() == 11);
    CHECK(findListRowByName(list, QStringLiteral("SimplePlot")) < 0);

    const int imageItemRow = findListRowByName(list, QStringLiteral("ImageItem"));
    CHECK(imageItemRow >= 0);
    CHECK(list->item(imageItemRow)->text().contains(QStringLiteral("runnable")));

    const int imageViewRow = findListRowByName(list, QStringLiteral("ImageView"));
    CHECK(imageViewRow >= 0);
    CHECK(list->item(imageViewRow)->text().contains(QStringLiteral("pending")));
    CHECK((list->item(imageViewRow)->flags() & Qt::ItemIsEnabled) == 0);

    list->setCurrentRow(imageItemRow);
    CHECK(window.selectedExampleName() == QStringLiteral("ImageItem"));

    const QString metadata = window.metadataPreviewLabel()->text();
    CHECK(metadata.contains(QStringLiteral("ImageItem smoke slice")));
    CHECK(metadata.contains(QStringLiteral("pyqtgraph/examples/ImageItem.py")));
    CHECK(metadata.contains(QStringLiteral("examples/ImageItem.cpp")));
    CHECK(metadata.contains(QStringLiteral("Status: ported")));
    CHECK(metadata.contains(QStringLiteral("smoke=required")));
    CHECK(metadata.contains(QStringLiteral("interaction=not_applicable")));

    return true;
}

bool testRunButtonState()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();

    list->setCurrentRow(findListRowByName(list, QStringLiteral("ImageItem")));
    CHECK(window.isRunEnabled());

    list->setCurrentRow(findListRowByName(list, QStringLiteral("ImageView")));
    CHECK(!window.isRunEnabled());
    CHECK(!window.runSelectedExample());

    return true;
}

bool testLaunchDispatchViaHook()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();

    QString hookedName;
    window.setLaunchHookForTesting([&hookedName](const QString& name) {
        hookedName = name;
        return true;
    });

    list->setCurrentRow(findListRowByName(list, QStringLiteral("Plotting")));
    CHECK(window.runSelectedExample());
    CHECK(hookedName == QStringLiteral("Plotting"));

    hookedName.clear();
    CHECK(window.activateSelectedExampleForTesting());
    CHECK(hookedName == QStringLiteral("Plotting"));

    hookedName.clear();
    Q_EMIT list->itemDoubleClicked(list->currentItem());
    CHECK(hookedName == QStringLiteral("Plotting"));

    window.clearLaunchHookForTesting();
    list->setCurrentRow(findListRowByName(list, QStringLiteral("ImageView")));
    CHECK(!window.runSelectedExample());

    return true;
}

bool testMissingBinaryShowsNotice()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();

    cppqtgraph::examples::ExampleRegistry::setExecutableSearchDirectoryForTesting(
        QStringLiteral("/nonexistent/example/search/path"));

    list->setCurrentRow(findListRowByName(list, QStringLiteral("ImageItem")));
    CHECK(window.runSelectedExample());
    CHECK(window.statusNoticeLabel()->text().contains(QStringLiteral("not found")));
    CHECK(window.isRunEnabled());

    cppqtgraph::examples::ExampleRegistry::clearExecutableSearchDirectoryForTesting();
    return true;
}

bool testAsyncStartFailureShowsNotice()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();

    cppqtgraph::examples::ExampleRegistry::setLaunchHookForTesting(makeAsyncStartFailureLaunch);

    list->setCurrentRow(findListRowByName(list, QStringLiteral("ImageItem")));
    CHECK(window.runSelectedExample());

    for (int attempt = 0; attempt < 100; ++attempt) {
        QCoreApplication::processEvents();
        if (window.statusNoticeLabel()->isVisible()
            && window.statusNoticeLabel()->text().contains(QStringLiteral("Failed to start"))) {
            break;
        }
        QTest::qWait(10);
    }

    CHECK(window.statusNoticeLabel()->text().contains(QStringLiteral("Failed to start")));
    CHECK(window.isRunEnabled());

    cppqtgraph::examples::ExampleRegistry::clearLaunchHookForTesting();
    return true;
}

bool testFailingChildKeepsLauncherAlive()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();

    cppqtgraph::examples::ExampleRegistry::setLaunchHookForTesting(makeFalseBinaryLaunch);

    list->setCurrentRow(findListRowByName(list, QStringLiteral("ImageItem")));
    CHECK(window.runSelectedExample());

    const std::vector<std::shared_ptr<cppqtgraph::examples::LaunchedExample>>& launches =
        window.activeLaunchesForTesting();
    CHECK(!launches.empty());
    CHECK(launches.back()->process != nullptr);
    CHECK(launches.back()->process->waitForFinished(5000));

    QCoreApplication::processEvents();

    CHECK(window.statusNoticeLabel()->text().contains(QStringLiteral("exited with code")));
    CHECK(window.isRunEnabled());
    CHECK(window.runSelectedExample());

    cppqtgraph::examples::ExampleRegistry::clearLaunchHookForTesting();
    return true;
}

bool testFilterNarrowsList()
{
    cppqtgraph::examples::ExampleAppWindow window;
    QListWidget* list = window.exampleListWidget();
    QLineEdit* filter = window.filterLineEdit();

    CHECK(list->count() == 11);
    filter->setText(QStringLiteral("plot"));
    CHECK(list->count() < 11);
    CHECK(findListRowByName(list, QStringLiteral("Plotting")) >= 0);
    CHECK(findListRowByName(list, QStringLiteral("ImageItem")) < 0);

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ApplicationGuard guard(argc, argv);

    if (!testRegistryOrderAndStatus()) {
        return 1;
    }
    if (!testRegistryLaunchPendingFailsClosed()) {
        return 1;
    }
    if (!testRegistryResolvesBuiltExecutables()) {
        return 1;
    }
    if (!testRegistryLaunchesPortedExamplesViaHook()) {
        return 1;
    }
    if (!testRegistryLaunchMissingExecutable()) {
        return 1;
    }
    if (!testExampleAppListAndMetadata()) {
        return 1;
    }
    if (!testRunButtonState()) {
        return 1;
    }
    if (!testLaunchDispatchViaHook()) {
        return 1;
    }
    if (!testMissingBinaryShowsNotice()) {
        return 1;
    }
    if (!testAsyncStartFailureShowsNotice()) {
        return 1;
    }
    if (!testFailingChildKeepsLauncherAlive()) {
        return 1;
    }
    if (!testFilterNarrowsList()) {
        return 1;
    }

    return 0;
}
