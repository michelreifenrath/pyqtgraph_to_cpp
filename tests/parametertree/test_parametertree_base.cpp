#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>

#define CPPQTGRAPH_PARAMETERTREE_NO_MAIN
#include "../../examples/parametertree.cpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>

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

bool testParameterFactoryAndChildOrder()
{
    const std::shared_ptr<cppqtgraph::parametertree::Parameter> root =
        cppqtgraph::examples::buildParametertreeRoot();
    CHECK(root != nullptr);
    CHECK(root->name() == QStringLiteral("params"));
    CHECK(root->type() == QStringLiteral("group"));
    CHECK(root->children().size() == 5);
    CHECK(root->children()[0]->name() == QStringLiteral("Example Parameters"));
    CHECK(root->children()[1]->name() == QStringLiteral("Save/Restore functionality"));
    CHECK(root->children()[2]->name() == QStringLiteral("Custom context menu"));
    CHECK(root->children()[3]->name() == QStringLiteral("Custom parameter group (reciprocal values)"));
    CHECK(root->children()[4]->name() == QStringLiteral("Expandable Parameter Group"));

    const std::shared_ptr<cppqtgraph::parametertree::Parameter> boolParam =
        cppqtgraph::parametertree::Parameter::create(
            QVariantMap{{QStringLiteral("name"), QStringLiteral("flag")},
                        {QStringLiteral("type"), QStringLiteral("bool")},
                        {QStringLiteral("value"), true}});
    CHECK(boolParam->type() == QStringLiteral("bool"));
    CHECK(boolParam->value().toBool());

    const std::shared_ptr<cppqtgraph::parametertree::Parameter> actionParam =
        cppqtgraph::parametertree::Parameter::create(
            QVariantMap{{QStringLiteral("name"), QStringLiteral("Run")},
                        {QStringLiteral("type"), QStringLiteral("action")}});
    CHECK(actionParam->type() == QStringLiteral("action"));

    return true;
}

bool testParameterTreeShell()
{
    const std::shared_ptr<cppqtgraph::parametertree::Parameter> root =
        cppqtgraph::examples::buildParametertreeRoot();

    cppqtgraph::parametertree::ParameterTree tree;
    CHECK(tree.columnCount() == 2);
    CHECK(tree.headerItem()->text(0) == QStringLiteral("Parameter"));
    CHECK(tree.headerItem()->text(1) == QStringLiteral("Value"));

    tree.setParameters(root, false);
    CHECK(tree.parameters() == root.get());
    CHECK(tree.topLevelItemCount() == 1);

    const QTreeWidgetItem* hiddenRoot = tree.topLevelItem(0);
    CHECK(hiddenRoot->text(0).isEmpty());
    CHECK(hiddenRoot->childCount() == 5);
    CHECK(hiddenRoot->child(0)->text(0) == QStringLiteral("Example Parameters"));
    CHECK(hiddenRoot->child(1)->text(0) == QStringLiteral("Save/Restore functionality"));
    CHECK(hiddenRoot->child(4)->text(0) == QStringLiteral("Expandable Parameter Group"));

    return true;
}

bool testExampleLayoutAndSharedRoot()
{
    auto example = cppqtgraph::examples::createParametertreeExample();
    CHECK(example.window != nullptr);
    CHECK(example.root != nullptr);
    CHECK(example.tree1 != nullptr);
    CHECK(example.tree2 != nullptr);
    CHECK(example.tree1->parameters() == example.root.get());
    CHECK(example.tree2->parameters() == example.root.get());
    CHECK(example.window->windowTitle() == QStringLiteral("pyqtgraph example: Parameter Tree"));

    const auto labels = example.window->findChildren<QLabel*>();
    CHECK(!labels.isEmpty());
    CHECK(labels.front()->text().contains(QStringLiteral("two views of the same data")));

    example.window->show();
    const QPixmap pixmap = example.window->grab();
    CHECK(!pixmap.isNull());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testParameterFactoryAndChildOrder()) {
        return 1;
    }
    if (!testParameterTreeShell()) {
        return 1;
    }
    if (!testExampleLayoutAndSharedRoot()) {
        return 1;
    }

    return 0;
}
