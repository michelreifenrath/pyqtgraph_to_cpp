// Source note: translated/adapted from PyQtGraph examples/parametertree.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include <memory>

namespace cppqtgraph::examples {

std::shared_ptr<parametertree::Parameter> buildParametertreeRoot()
{
    return parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("params")},
        {QStringLiteral("type"), QStringLiteral("group")},
        {QStringLiteral("children"),
         QVariantList{
             QVariant::fromValue(QVariantMap{
                 {QStringLiteral("name"), QStringLiteral("Example Parameters")},
                 {QStringLiteral("type"), QStringLiteral("group")},
                 {QStringLiteral("children"),
                  QVariantList{
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("Collapse All")},
                                                      {QStringLiteral("type"), QStringLiteral("action")}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("Expand All")},
                                                      {QStringLiteral("type"), QStringLiteral("action")}}),
                  }},
             }),
             QVariant::fromValue(QVariantMap{
                 {QStringLiteral("name"), QStringLiteral("Save/Restore functionality")},
                 {QStringLiteral("type"), QStringLiteral("group")},
                 {QStringLiteral("children"),
                  QVariantList{
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("Save State")},
                                                      {QStringLiteral("type"), QStringLiteral("action")}}),
                      QVariant::fromValue(QVariantMap{
                          {QStringLiteral("name"), QStringLiteral("Restore State")},
                          {QStringLiteral("type"), QStringLiteral("action")},
                          {QStringLiteral("children"),
                           QVariantList{
                               QVariant::fromValue(QVariantMap{{QStringLiteral("name"),
                                                                QStringLiteral("Add missing items")},
                                                               {QStringLiteral("type"), QStringLiteral("bool")},
                                                               {QStringLiteral("value"), true}}),
                               QVariant::fromValue(QVariantMap{{QStringLiteral("name"),
                                                                QStringLiteral("Remove extra items")},
                                                               {QStringLiteral("type"), QStringLiteral("bool")},
                                                               {QStringLiteral("value"), true}}),
                           }},
                      }),
                  }},
             }),
             QVariant::fromValue(QVariantMap{
                 {QStringLiteral("name"), QStringLiteral("Custom context menu")},
                 {QStringLiteral("type"), QStringLiteral("group")},
                 {QStringLiteral("children"),
                  QVariantList{
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("List contextMenu")},
                                                      {QStringLiteral("type"), QStringLiteral("float")},
                                                      {QStringLiteral("value"), 0}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("Dict contextMenu")},
                                                      {QStringLiteral("type"), QStringLiteral("float")},
                                                      {QStringLiteral("value"), 0}}),
                  }},
             }),
             QVariant::fromValue(QVariantMap{
                 {QStringLiteral("name"), QStringLiteral("Custom parameter group (reciprocal values)")},
                 {QStringLiteral("type"), QStringLiteral("group")},
                 {QStringLiteral("children"),
                  QVariantList{
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("A = 1/B")},
                                                      {QStringLiteral("type"), QStringLiteral("float")},
                                                      {QStringLiteral("value"), 7.0}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("B = 1/A")},
                                                      {QStringLiteral("type"), QStringLiteral("float")},
                                                      {QStringLiteral("value"), 1.0 / 7.0}}),
                  }},
             }),
             QVariant::fromValue(QVariantMap{
                 {QStringLiteral("name"), QStringLiteral("Expandable Parameter Group")},
                 {QStringLiteral("type"), QStringLiteral("group")},
                 {QStringLiteral("tip"), QStringLiteral("Click to add children")},
                 {QStringLiteral("children"),
                  QVariantList{
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("ScalableParam 1")},
                                                      {QStringLiteral("type"), QStringLiteral("str")},
                                                      {QStringLiteral("value"), QStringLiteral("default param 1")}}),
                      QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("ScalableParam 2")},
                                                      {QStringLiteral("type"), QStringLiteral("str")},
                                                      {QStringLiteral("value"), QStringLiteral("default param 2")}}),
                  }},
             }),
         }},
    });
}

struct ParametertreeExample {
    std::unique_ptr<QWidget> window;
    std::shared_ptr<parametertree::Parameter> root;
    parametertree::ParameterTree* tree1 = nullptr;
    parametertree::ParameterTree* tree2 = nullptr;
};

ParametertreeExample createParametertreeExample()
{
    auto root = buildParametertreeRoot();

    auto window = std::make_unique<QWidget>();
    window->setWindowTitle(QStringLiteral("pyqtgraph example: Parameter Tree"));

    auto* layout = new QGridLayout(window.get());
    auto* label = new QLabel(QStringLiteral(
        "These are two views of the same data. They should always display the same values."));
    layout->addWidget(label, 0, 0, 1, 2);

    auto* tree1 = new parametertree::ParameterTree(window.get());
    tree1->setParameters(root, false);
    layout->addWidget(tree1, 1, 0, 1, 1);

    auto* tree2 = new parametertree::ParameterTree(window.get());
    tree2->setParameters(root, false);
    layout->addWidget(tree2, 1, 1, 1, 1);

    return {.window = std::move(window), .root = std::move(root), .tree1 = tree1, .tree2 = tree2};
}

} // namespace cppqtgraph::examples

#ifndef CPPQTGRAPH_PARAMETERTREE_NO_MAIN
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    auto example = cppqtgraph::examples::createParametertreeExample();
    example.window->show();
    return QApplication::exec();
}
#endif
