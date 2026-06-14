#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>

#include <cmath>
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

cppqtgraph::parametertree::ParameterItem* findItemByName(QTreeWidgetItem* root, const QString& name)
{
    if (root == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < root->childCount(); ++i) {
        if (auto* item = dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(root->child(i))) {
            if (item->parameter() != nullptr && item->parameter()->name() == name) {
                return item;
            }
            if (auto* nested = findItemByName(item, name)) {
                return nested;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<cppqtgraph::parametertree::Parameter> makeReciprocalGroup()
{
    auto group = cppqtgraph::parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("reciprocal")},
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
    });

    auto* a = group->child(QStringLiteral("A = 1/B"));
    auto* b = group->child(QStringLiteral("B = 1/A"));
    QObject::connect(a,
                     &cppqtgraph::parametertree::Parameter::sigValueChanged,
                     group.get(),
                     [b](cppqtgraph::parametertree::Parameter*, const QVariant& value) {
                         const double numeric = value.toDouble();
                         if (numeric != 0.0) {
                             b->setValue(1.0 / numeric, true);
                         }
                     });
    QObject::connect(b,
                     &cppqtgraph::parametertree::Parameter::sigValueChanged,
                     group.get(),
                     [a](cppqtgraph::parametertree::Parameter*, const QVariant& value) {
                         const double numeric = value.toDouble();
                         if (numeric != 0.0) {
                             a->setValue(1.0 / numeric, true);
                         }
                     });
    return group;
}

std::shared_ptr<cppqtgraph::parametertree::Parameter> makeScalableGroup()
{
    auto group = cppqtgraph::parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("scalable")},
        {QStringLiteral("type"), QStringLiteral("group")},
        {QStringLiteral("addText"), QStringLiteral("Add")},
        {QStringLiteral("addList"),
         QVariantList{QStringLiteral("str"), QStringLiteral("float"), QStringLiteral("int")}},
        {QStringLiteral("children"),
         QVariantList{
             QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("ScalableParam 1")},
                                             {QStringLiteral("type"), QStringLiteral("str")},
                                             {QStringLiteral("value"), QStringLiteral("seed")}}),
         }},
    });

    auto* scalable = dynamic_cast<cppqtgraph::parametertree::GroupParameter*>(group.get());
    if (scalable != nullptr) {
        QObject::connect(scalable,
                         &cppqtgraph::parametertree::GroupParameter::sigAddNew,
                         scalable,
                         [](cppqtgraph::parametertree::GroupParameter* self, const QString& typ) {
                             QVariant defaultValue;
                             if (typ == QStringLiteral("str")) {
                                 defaultValue = QString();
                             } else if (typ == QStringLiteral("float")) {
                                 defaultValue = 0.0;
                             } else if (typ == QStringLiteral("int")) {
                                 defaultValue = 0;
                             } else {
                                 return;
                             }
                             const int nextIndex = static_cast<int>(self->children().size()) + 1;
                             self->addChild(cppqtgraph::parametertree::Parameter::create(QVariantMap{
                                 {QStringLiteral("name"),
                                  QStringLiteral("ScalableParam %1").arg(nextIndex)},
                                 {QStringLiteral("type"), typ},
                                 {QStringLiteral("value"), defaultValue},
                                 {QStringLiteral("removable"), true},
                                 {QStringLiteral("renamable"), true},
                             }));
                         });
    }
    return group;
}

bool testReciprocalEditsUpdateOtherChild()
{
    auto group = makeReciprocalGroup();
    auto* a = group->child(QStringLiteral("A = 1/B"));
    auto* b = group->child(QStringLiteral("B = 1/A"));
    CHECK(a != nullptr);
    CHECK(b != nullptr);

    a->setValue(2.0);
    CHECK(std::abs(b->value().toDouble() - 0.5) < 1e-9);

    b->setValue(4.0);
    CHECK(std::abs(a->value().toDouble() - 0.25) < 1e-9);
    return true;
}

bool testScalableAddComboCreatesTypedChildren()
{
    auto group = makeScalableGroup();
    CHECK(dynamic_cast<cppqtgraph::parametertree::GroupParameter*>(group.get()) != nullptr);
    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(group, false);
    tree.show();
    QTest::qWait(0);

    CHECK(tree.topLevelItemCount() == 1);
    auto* groupItem = dynamic_cast<cppqtgraph::parametertree::GroupParameterItem*>(tree.topLevelItem(0));
    CHECK(groupItem != nullptr);
    auto* combo = groupItem->addComboWidget();
    CHECK(combo != nullptr);
    CHECK(combo->count() == 4);

    combo->setCurrentIndex(combo->findText(QStringLiteral("str")));
    QTest::qWait(0);
    auto* strChild = group->child(QStringLiteral("ScalableParam 2"));
    CHECK(strChild != nullptr);
    CHECK(strChild->type() == QStringLiteral("str"));
    CHECK(strChild->value().toString().isEmpty());
    CHECK(strChild->options().value(QStringLiteral("removable")).toBool());
    CHECK(strChild->options().value(QStringLiteral("renamable")).toBool());

    combo->setCurrentIndex(combo->findText(QStringLiteral("float")));
    QTest::qWait(0);
    auto* floatChild = group->child(QStringLiteral("ScalableParam 3"));
    CHECK(floatChild != nullptr);
    CHECK(floatChild->type() == QStringLiteral("float"));
    CHECK(std::abs(floatChild->value().toDouble()) < 1e-9);

    combo->setCurrentIndex(combo->findText(QStringLiteral("int")));
    QTest::qWait(0);
    auto* intChild = group->child(QStringLiteral("ScalableParam 4"));
    CHECK(intChild != nullptr);
    CHECK(intChild->type() == QStringLiteral("int"));
    CHECK(intChild->value().toInt() == 0);
    CHECK(combo->currentIndex() == 0);
    return true;
}

bool testContextMenuEmitsInternalNames()
{
    auto listParam = cppqtgraph::parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("listCtx")},
        {QStringLiteral("type"), QStringLiteral("float")},
        {QStringLiteral("value"), 0},
        {QStringLiteral("context"),
         QVariantList{QStringLiteral("menu1"), QStringLiteral("menu2")}},
    });
    auto dictParam = cppqtgraph::parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("dictCtx")},
        {QStringLiteral("type"), QStringLiteral("float")},
        {QStringLiteral("value"), 0},
        {QStringLiteral("context"),
         QVariantMap{{QStringLiteral("changeName"), QStringLiteral("Title")},
                     {QStringLiteral("internal"), QStringLiteral("What the user sees")}}},
    });

    QSignalSpy listSpy(listParam.get(), &cppqtgraph::parametertree::Parameter::sigContextMenu);
    listParam->contextMenu(QStringLiteral("menu1"));
    listParam->contextMenu(QStringLiteral("menu2"));
    CHECK(listSpy.size() == 2);
    CHECK(listSpy.at(0).at(1).toString() == QStringLiteral("menu1"));
    CHECK(listSpy.at(1).at(1).toString() == QStringLiteral("menu2"));

    QSignalSpy dictSpy(dictParam.get(), &cppqtgraph::parametertree::Parameter::sigContextMenu);
    dictParam->contextMenu(QStringLiteral("changeName"));
    dictParam->contextMenu(QStringLiteral("internal"));
    CHECK(dictSpy.size() == 2);
    CHECK(dictSpy.at(0).at(1).toString() == QStringLiteral("changeName"));
    CHECK(dictSpy.at(1).at(1).toString() == QStringLiteral("internal"));
    return true;
}

bool testRemoveSignalKeepsParameterAlive()
{
    auto group = cppqtgraph::parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("group")},
        {QStringLiteral("type"), QStringLiteral("group")},
        {QStringLiteral("children"),
         QVariantList{QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("child")},
                                                       {QStringLiteral("type"), QStringLiteral("str")},
                                                       {QStringLiteral("value"), QStringLiteral("x")},
                                                       {QStringLiteral("removable"), true}})}},
    });

    auto* child = group->child(QStringLiteral("child"));
    CHECK(child != nullptr);

    QString nameAtSignal;
    QObject::connect(child,
                     &cppqtgraph::parametertree::Parameter::sigRemoved,
                     child,
                     [&nameAtSignal](cppqtgraph::parametertree::Parameter* removed) {
                         nameAtSignal = removed->name();
                     });

    child->remove();
    CHECK(nameAtSignal == QStringLiteral("child"));
    CHECK(group->child(QStringLiteral("child")) == nullptr);
    return true;
}

bool testScalableGroupKeepsAddRowLast()
{
    auto group = makeScalableGroup();
    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(group, false);
    tree.show();
    QTest::qWait(0);

    auto* groupItem = dynamic_cast<cppqtgraph::parametertree::GroupParameterItem*>(tree.topLevelItem(0));
    CHECK(groupItem != nullptr);
    CHECK(groupItem->childCount() >= 2);

    auto* firstChild = dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(groupItem->child(0));
    CHECK(firstChild != nullptr);
    CHECK(firstChild->parameter()->name() == QStringLiteral("ScalableParam 1"));

    auto* lastChild = groupItem->child(groupItem->childCount() - 1);
    CHECK(dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(lastChild) == nullptr);
    CHECK(groupItem->addComboWidget() != nullptr);

    auto* combo = groupItem->addComboWidget();
    combo->setCurrentIndex(combo->findText(QStringLiteral("str")));
    QTest::qWait(0);

    CHECK(groupItem->childCount() >= 3);
    auto* addedChild =
        dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(groupItem->child(groupItem->childCount() - 2));
    CHECK(addedChild != nullptr);
    CHECK(addedChild->parameter()->name() == QStringLiteral("ScalableParam 2"));

    lastChild = groupItem->child(groupItem->childCount() - 1);
    CHECK(dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(lastChild) == nullptr);
    return true;
}

bool testRenameAndRemoveUpdateModelAndViews()
{
    auto group = cppqtgraph::parametertree::Parameter::create(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("group")},
        {QStringLiteral("type"), QStringLiteral("group")},
        {QStringLiteral("children"),
         QVariantList{QVariant::fromValue(QVariantMap{{QStringLiteral("name"), QStringLiteral("child")},
                                                       {QStringLiteral("type"), QStringLiteral("str")},
                                                       {QStringLiteral("value"), QStringLiteral("x")},
                                                       {QStringLiteral("removable"), true},
                                                       {QStringLiteral("renamable"), true}})}},
    });

    std::unique_ptr<cppqtgraph::parametertree::ParameterTree> tree1 =
        std::make_unique<cppqtgraph::parametertree::ParameterTree>();
    std::unique_ptr<cppqtgraph::parametertree::ParameterTree> tree2 =
        std::make_unique<cppqtgraph::parametertree::ParameterTree>();
    tree1->setParameters(group, false);
    tree2->setParameters(group, false);
    tree1->show();
    tree2->show();
    QTest::qWait(0);

    auto* child = group->child(QStringLiteral("child"));
    CHECK(child != nullptr);
    CHECK(child->setName(QStringLiteral("renamed")) == QStringLiteral("renamed"));
    QTest::qWait(0);
    CHECK(group->child(QStringLiteral("renamed")) == child);
    CHECK(findItemByName(tree1->invisibleRootItem(), QStringLiteral("renamed")) != nullptr);
    CHECK(findItemByName(tree2->invisibleRootItem(), QStringLiteral("renamed")) != nullptr);
    CHECK(findItemByName(tree1->invisibleRootItem(), QStringLiteral("child")) == nullptr);

    child->remove();
    QTest::qWait(0);
    CHECK(group->child(QStringLiteral("renamed")) == nullptr);
    CHECK(findItemByName(tree1->invisibleRootItem(), QStringLiteral("renamed")) == nullptr);
    CHECK(findItemByName(tree2->invisibleRootItem(), QStringLiteral("renamed")) == nullptr);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ApplicationGuard guard(argc, argv);

    struct TestCase {
        const char* name;
        bool (*fn)();
    };

    const TestCase tests[] = {
        {"reciprocal_edits_update_other_child", testReciprocalEditsUpdateOtherChild},
        {"scalable_add_combo_creates_typed_children", testScalableAddComboCreatesTypedChildren},
        {"context_menu_emits_internal_names", testContextMenuEmitsInternalNames},
        {"remove_signal_keeps_parameter_alive", testRemoveSignalKeepsParameterAlive},
        {"scalable_group_keeps_add_row_last", testScalableGroupKeepsAddRowLast},
        {"rename_and_remove_update_model_and_views", testRenameAndRemoveUpdateModelAndViews},
    };

    int failed = 0;
    for (const TestCase& test : tests) {
        if (!test.fn()) {
            std::cerr << "FAILED: " << test.name << '\n';
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}
