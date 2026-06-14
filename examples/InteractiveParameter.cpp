// Source note: translated/adapted from PyQtGraph examples/InteractiveParameter.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>

#include <QtWidgets/QApplication>

#include <memory>

namespace cppqtgraph::examples {

struct InteractiveParameterExample {
    std::unique_ptr<parametertree::ParameterTree> tree;
    std::shared_ptr<parametertree::Parameter> root;
    parametertree::ParameterTree* parameterTree = nullptr;
};

InteractiveParameterExample createInteractiveParameterExample()
{
    auto root = parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("Interactive Parameter Use")},
        {QStringLiteral("type"), QStringLiteral("group")},
    });

    auto tree = std::make_unique<parametertree::ParameterTree>();
    tree->setParameters(root);
    tree->setWindowTitle(QStringLiteral("pyqtgraph example: Parameter-Function Interaction"));

    parametertree::ParameterTree* treePtr = tree.get();
    return {.tree = std::move(tree), .root = std::move(root), .parameterTree = treePtr};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_INTERACTIVEPARAMETER_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    const bool smokeMode = argc > 1 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--smoke");
    auto example = cppqtgraph::examples::createInteractiveParameterExample();
    example.tree->show();
    if (smokeMode) {
        QApplication::processEvents();
        example.tree->close();
        return 0;
    }
    return QApplication::exec();
}
#endif
