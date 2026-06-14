#define CPPQTGRAPH_INTERACTIVEPARAMETER_NO_MAIN
#include "../../examples/InteractiveParameter.cpp"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
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

#ifndef CPPQTGRAPH_P479_FIXTURE
#define CPPQTGRAPH_P479_FIXTURE ""
#endif

QString executablePath()
{
    return QString::fromUtf8(CPPQTGRAPH_INTERACTIVEPARAMETER_EXECUTABLE);
}

QString fixturePath()
{
    return QString::fromUtf8(CPPQTGRAPH_P479_FIXTURE);
}

bool jsonValueMatchesVariant(const QJsonValue& expected, const QVariant& actual)
{
    if (expected.isBool()) {
        return actual.typeId() == QMetaType::Bool && actual.toBool() == expected.toBool();
    }
    if (expected.isDouble()) {
        if (actual.typeId() == QMetaType::Int || actual.typeId() == QMetaType::LongLong) {
            return actual.toInt() == expected.toInt();
        }
        return qFuzzyCompare(actual.toDouble(), expected.toDouble());
    }
    if (expected.isString()) {
        return actual.typeId() == QMetaType::QString && actual.toString() == expected.toString();
    }
    if (expected.isObject()) {
        if (actual.typeId() != QMetaType::QVariantMap) {
            return false;
        }
        const QVariantMap actualMap = actual.toMap();
        const QJsonObject expectedObject = expected.toObject();
        for (auto it = expectedObject.begin(); it != expectedObject.end(); ++it) {
            if (!actualMap.contains(it.key())) {
                return false;
            }
            if (!jsonValueMatchesVariant(it.value(), actualMap.value(it.key()))) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool compareParameterToOracle(const cppqtgraph::parametertree::Parameter& param,
                              const QJsonObject& expected,
                              std::string_view path)
{
    CHECK(param.name() == expected.value(QStringLiteral("name")).toString());
    CHECK(param.title() == expected.value(QStringLiteral("title")).toString());
    CHECK(param.type() == expected.value(QStringLiteral("type")).toString());

    if (expected.contains(QStringLiteral("value"))) {
        CHECK(jsonValueMatchesVariant(expected.value(QStringLiteral("value")), param.value()));
    }
    if (expected.contains(QStringLiteral("default"))) {
        CHECK(param.hasDefault());
        CHECK(jsonValueMatchesVariant(expected.value(QStringLiteral("default")), param.defaultValue()));
    }
    if (expected.contains(QStringLiteral("button"))) {
        const QVariant button = param.options().value(QStringLiteral("button"));
        CHECK(jsonValueMatchesVariant(expected.value(QStringLiteral("button")), button));
    }

    if (!expected.contains(QStringLiteral("children"))) {
        return true;
    }

    const QJsonArray expectedChildren = expected.value(QStringLiteral("children")).toArray();
    CHECK(static_cast<int>(param.children().size()) == expectedChildren.size());

    for (int index = 0; index < expectedChildren.size(); ++index) {
        const QJsonObject childExpected = expectedChildren.at(index).toObject();
        const std::string childPath =
            std::string(path) + '/' + childExpected.value(QStringLiteral("name")).toString().toStdString();
        CHECK(compareParameterToOracle(*param.children().at(static_cast<std::size_t>(index)),
                                       childExpected,
                                       childPath));
    }

    return true;
}

bool loadOracleTree(QJsonObject& out)
{
    QFile file(fixturePath());
    CHECK(file.open(QIODevice::ReadOnly));
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    CHECK(document.isObject());
    const QJsonObject root = document.object();
    CHECK(root.contains(QStringLiteral("tree")));
    out = root.value(QStringLiteral("tree")).toObject();
    return true;
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

    CHECK(example.root->children().size() == 2);
    CHECK(example.root->children()[0]->name() == QStringLiteral("easySample"));
    CHECK(example.root->children()[0]->type() == QStringLiteral("_actiongroup"));
    CHECK(example.root->children()[1]->name() == QStringLiteral("stringParams"));
    CHECK(example.root->children()[1]->type() == QStringLiteral("_actiongroup"));

    QJsonObject oracleTree;
    CHECK(loadOracleTree(oracleTree));
    CHECK(compareParameterToOracle(*example.root, oracleTree, "root"));

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
