#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>

#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

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

cppqtgraph::parametertree::WidgetParameterItem* findWidgetItem(cppqtgraph::parametertree::ParameterTree& tree,
                                                               const QString& name)
{
    return dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), name));
}

std::shared_ptr<cppqtgraph::parametertree::Parameter> makeSyncTestRoot()
{
    const QVariantMap alpha = {{QStringLiteral("name"), QStringLiteral("alpha")},
                                 {QStringLiteral("type"), QStringLiteral("str")},
                                 {QStringLiteral("value"), QStringLiteral("initial")},
                                 {QStringLiteral("default"), QStringLiteral("initial")}};
    const QVariantMap beta = {{QStringLiteral("name"), QStringLiteral("beta")},
                                {QStringLiteral("type"), QStringLiteral("str")},
                                {QStringLiteral("value"), QStringLiteral("two")},
                                {QStringLiteral("default"), QStringLiteral("two")}};
    const QVariantMap group1 = {{QStringLiteral("name"), QStringLiteral("group1")},
                                {QStringLiteral("type"), QStringLiteral("group")},
                                {QStringLiteral("children"),
                                 QVariantList{QVariant::fromValue(alpha), QVariant::fromValue(beta)}}};
    const QVariantMap root = {{QStringLiteral("name"), QStringLiteral("params")},
                              {QStringLiteral("type"), QStringLiteral("group")},
                              {QStringLiteral("children"), QVariantList{QVariant::fromValue(group1)}}};
    return cppqtgraph::parametertree::Parameter::create(root);
}

struct DualTreeFixture {
    std::shared_ptr<cppqtgraph::parametertree::Parameter> root;
    std::unique_ptr<cppqtgraph::parametertree::ParameterTree> tree1;
    std::unique_ptr<cppqtgraph::parametertree::ParameterTree> tree2;

    explicit DualTreeFixture()
    {
        root = makeSyncTestRoot();
        tree1 = std::make_unique<cppqtgraph::parametertree::ParameterTree>();
        tree2 = std::make_unique<cppqtgraph::parametertree::ParameterTree>();
        tree1->setParameters(root, false);
        tree2->setParameters(root, false);
        tree1->show();
        tree2->show();
        QTest::qWait(0);
    }
};

bool testSharedValueSyncBetweenTwoViews()
{
    DualTreeFixture fixture;
    auto* item1 = findWidgetItem(*fixture.tree1, QStringLiteral("alpha"));
    auto* item2 = findWidgetItem(*fixture.tree2, QStringLiteral("alpha"));
    CHECK(item1 != nullptr);
    CHECK(item2 != nullptr);

    fixture.root->child(QStringLiteral("group1"))->child(QStringLiteral("alpha"))->setValue(QStringLiteral("synced"));
    QTest::qWait(0);

    CHECK(item1->text(1) == QStringLiteral("synced"));
    CHECK(item2->text(1) == QStringLiteral("synced"));
    return true;
}

bool testChildAddRemovePropagatesToBothViews()
{
    DualTreeFixture fixture;
    auto* group = fixture.root->child(QStringLiteral("group1"));
    CHECK(group != nullptr);

    const int before = findItemByName(fixture.tree1->invisibleRootItem(), QStringLiteral("group1"))->childCount();
    group->addChild(cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("gamma")},
                    {QStringLiteral("type"), QStringLiteral("str")},
                    {QStringLiteral("value"), QStringLiteral("new")}}));
    QTest::qWait(0);

    const int afterAdd1 =
        findItemByName(fixture.tree1->invisibleRootItem(), QStringLiteral("group1"))->childCount();
    const int afterAdd2 =
        findItemByName(fixture.tree2->invisibleRootItem(), QStringLiteral("group1"))->childCount();
    CHECK(afterAdd1 == before + 1);
    CHECK(afterAdd2 == before + 1);
    CHECK(findWidgetItem(*fixture.tree1, QStringLiteral("gamma")) != nullptr);
    CHECK(findWidgetItem(*fixture.tree2, QStringLiteral("gamma")) != nullptr);

    group->removeChild(group->child(QStringLiteral("gamma")));
    QTest::qWait(0);

    CHECK(findItemByName(fixture.tree1->invisibleRootItem(), QStringLiteral("group1"))->childCount() == before);
    CHECK(findItemByName(fixture.tree2->invisibleRootItem(), QStringLiteral("group1"))->childCount() == before);
    CHECK(findWidgetItem(*fixture.tree1, QStringLiteral("gamma")) == nullptr);
    return true;
}

bool testVisibleExpandedTitleOptionsSync()
{
    DualTreeFixture fixture;
    auto* alpha = fixture.root->child(QStringLiteral("group1"))->child(QStringLiteral("alpha"));
    alpha->setOpts({{QStringLiteral("visible"), false},
                    {QStringLiteral("title"), QStringLiteral("Alpha Title")},
                    {QStringLiteral("expanded"), false}});
    QTest::qWait(0);

    auto* item1 = findItemByName(fixture.tree1->invisibleRootItem(), QStringLiteral("alpha"));
    auto* item2 = findItemByName(fixture.tree2->invisibleRootItem(), QStringLiteral("alpha"));
    CHECK(item1 != nullptr);
    CHECK(item2 != nullptr);
    CHECK(item1->isHidden());
    CHECK(item2->isHidden());
    CHECK(item1->text(0) == QStringLiteral("Alpha Title"));
    CHECK(item2->text(0) == QStringLiteral("Alpha Title"));
    return true;
}

bool testSelectionShowsEditor()
{
    DualTreeFixture fixture;
    auto* item = findWidgetItem(*fixture.tree1, QStringLiteral("alpha"));
    CHECK(item != nullptr);

    fixture.tree1->setCurrentItem(item);
    item->selected(true);
    QTest::qWait(0);

    auto* editor = qobject_cast<QLineEdit*>(item->editorWidget());
    CHECK(editor != nullptr);
    CHECK(!editor->isHidden());
    return true;
}

bool testTabFocusTraversal()
{
    DualTreeFixture fixture;
    auto* alpha = findWidgetItem(*fixture.tree1, QStringLiteral("alpha"));
    auto* beta = findWidgetItem(*fixture.tree1, QStringLiteral("beta"));
    CHECK(alpha != nullptr);
    CHECK(beta != nullptr);

    fixture.tree1->setCurrentItem(alpha);
    alpha->selected(true);
    QTest::qWait(0);

    QTest::keyClick(alpha->editorWidget(), Qt::Key_Tab);
    QTest::qWait(0);
    CHECK(fixture.tree1->currentItem() == beta);
    CHECK(!qobject_cast<QLineEdit*>(beta->editorWidget())->isHidden());
    return true;
}

bool testDefaultReset()
{
    DualTreeFixture fixture;
    auto* alphaParam = fixture.root->child(QStringLiteral("group1"))->child(QStringLiteral("alpha"));
    auto* item1 = findWidgetItem(*fixture.tree1, QStringLiteral("alpha"));
    auto* item2 = findWidgetItem(*fixture.tree2, QStringLiteral("alpha"));
    CHECK(item1 != nullptr);
    CHECK(item2 != nullptr);

    alphaParam->setValue(QStringLiteral("changed"));
    QTest::qWait(0);
    CHECK(item1->defaultButton()->isEnabled());

    QTest::mouseClick(item1->defaultButton(), Qt::LeftButton);
    QTest::qWait(0);

    CHECK(alphaParam->value().toString() == QStringLiteral("initial"));
    CHECK(item1->text(1) == QStringLiteral("initial"));
    CHECK(item2->text(1) == QStringLiteral("initial"));
    CHECK(!item1->defaultButton()->isEnabled());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testSharedValueSyncBetweenTwoViews()) {
        return 1;
    }
    if (!testChildAddRemovePropagatesToBothViews()) {
        return 1;
    }
    if (!testVisibleExpandedTitleOptionsSync()) {
        return 1;
    }
    if (!testSelectionShowsEditor()) {
        return 1;
    }
    if (!testTabFocusTraversal()) {
        return 1;
    }
    if (!testDefaultReset()) {
        return 1;
    }

    return 0;
}
