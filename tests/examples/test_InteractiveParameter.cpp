#define CPPQTGRAPH_INTERACTIVEPARAMETER_NO_MAIN
#include "../../examples/InteractiveParameter.cpp"

#include <QtCore/QProcess>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

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

#ifndef CPPQTGRAPH_INTERACTIVEPARAMETER_EXECUTABLE
#define CPPQTGRAPH_INTERACTIVEPARAMETER_EXECUTABLE ""
#endif

QString executablePath()
{
    return QString::fromUtf8(CPPQTGRAPH_INTERACTIVEPARAMETER_EXECUTABLE);
}

bool testInteractiveParameterFactory()
{
    auto example = cppqtgraph::examples::createInteractiveParameterExample();

    CHECK(example.tree != nullptr);
    CHECK(example.root != nullptr);
    CHECK(example.parameterTree != nullptr);
    CHECK(example.parameterTree == example.tree.get());
    CHECK(example.tree->windowTitle()
          == QStringLiteral("pyqtgraph example: Parameter-Function Interaction"));
    CHECK(example.parameterTree->parameters() == example.root.get());
    CHECK(example.root->name() == QStringLiteral("Interactive Parameter Use"));
    CHECK(example.root->title() == QStringLiteral("Interactive Parameter Use"));
    CHECK(example.root->type() == QStringLiteral("group"));

    CHECK(example.parameterTree->topLevelItemCount() == 1);
    CHECK(example.parameterTree->topLevelItem(0)->text(0)
          == QStringLiteral("Interactive Parameter Use"));

    example.tree->show();
    QApplication::processEvents();
    const QPixmap pixmap = example.tree->grab();
    CHECK(!pixmap.isNull());

    return true;
}

bool testDirectExecutableSmoke()
{
    const QString path = executablePath();
    if (path.isEmpty()) {
        return true;
    }

    QProcess process;
    process.setProgram(path);
    process.setArguments({QStringLiteral("--smoke")});
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    process.setProcessEnvironment(environment);
    process.start();
    CHECK(process.waitForStarted(5000));
    CHECK(process.waitForFinished(10000));
    CHECK(process.exitStatus() == QProcess::NormalExit);
    CHECK(process.exitCode() == 0);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testInteractiveParameterFactory()) {
        return 1;
    }
    if (!testDirectExecutableSmoke()) {
        return 1;
    }

    return 0;
}
