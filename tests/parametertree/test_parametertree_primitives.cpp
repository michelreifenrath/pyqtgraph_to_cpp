#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>
#include <cppqtgraph/parametertree/buildParamTypes.hpp>
#include <cppqtgraph/widgets/ComboBox.hpp>
#include <cppqtgraph/widgets/SpinBox.hpp>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtCore/QTemporaryDir>
#include <QtGui/QKeySequence>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidgetItem>

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

QWidget* editorForItem(cppqtgraph::parametertree::ParameterItem* item)
{
    if (auto* widgetItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(item)) {
        return widgetItem->editorWidget();
    }
    return nullptr;
}

bool testExampleParametersGroupOrder()
{
    const auto exampleParams = cppqtgraph::parametertree::buildExampleParametersGroup();
    CHECK(exampleParams->name() == QStringLiteral("Example Parameters"));
    CHECK(exampleParams->children().size() == 6);
    CHECK(exampleParams->children()[0]->name() == QStringLiteral("Expand All"));
    CHECK(exampleParams->children()[1]->name() == QStringLiteral("Collapse All"));
    CHECK(exampleParams->children()[2]->name() == QStringLiteral("Sample List"));
    CHECK(exampleParams->children()[3]->name() == QStringLiteral("Sample Float"));
    CHECK(exampleParams->children()[4]->name() == QStringLiteral("Sample Action"));
    CHECK(exampleParams->children()[5]->name() == QStringLiteral("No Extra Options"));
    CHECK(!exampleParams->child(QStringLiteral("Sample List"))->options().value(QStringLiteral("expanded")).toBool());
    return true;
}

bool testBoolCheckboxUpdatesValue()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("flag")},
                    {QStringLiteral("type"), QStringLiteral("bool")},
                    {QStringLiteral("value"), false}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("flag"));
    CHECK(item != nullptr);
    auto* checkBox = qobject_cast<QCheckBox*>(editorForItem(item));
    CHECK(checkBox != nullptr);
    CHECK(!checkBox->isChecked());

    QSignalSpy valueSpy(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanged);
    checkBox->setChecked(true);
    QTest::qWait(0);
    CHECK(param->value().toBool());
    CHECK(valueSpy.count() >= 1);
    return true;
}

bool testStrEditorChangingSignal()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("label")},
                    {QStringLiteral("type"), QStringLiteral("str")},
                    {QStringLiteral("value"), QStringLiteral("start")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("label"));
    CHECK(item != nullptr);
    tree.setCurrentItem(item);
    QTest::qWait(0);

    auto* editor = qobject_cast<QLineEdit*>(editorForItem(item));
    CHECK(editor != nullptr);

    QSignalSpy changingSpy(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanging);
    editor->setText(QStringLiteral("typing"));
    QTest::qWait(0);
    CHECK(changingSpy.count() >= 1);
    return true;
}

bool testNumericSpinBoxSuffixAndSiPrefixLabel()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("freq")},
                    {QStringLiteral("type"), QStringLiteral("float")},
                    {QStringLiteral("value"), 1000.0},
                    {QStringLiteral("suffix"), QStringLiteral("Hz")},
                    {QStringLiteral("siPrefix"), true}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("freq")));
    CHECK(item != nullptr);
    auto* spinBox = qobject_cast<cppqtgraph::widgets::SpinBox*>(item->editorWidget());
    CHECK(spinBox != nullptr);
    CHECK(spinBox->formatText().contains(QStringLiteral("k")));
    CHECK(spinBox->formatText().contains(QStringLiteral("Hz")));

    CHECK(spinBox->isHidden());
    auto* layout = spinBox->parentWidget();
    CHECK(layout != nullptr);
    auto* displayLabel = layout->findChild<QLabel*>();
    CHECK(displayLabel != nullptr);
    CHECK(displayLabel->isVisible());
    CHECK(displayLabel->text().contains(QStringLiteral("k")));
    CHECK(displayLabel->text().contains(QStringLiteral("Hz")));
    CHECK(displayLabel->text() == spinBox->formatText());
    return true;
}

bool testListComboUpdatesValue()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("choice")},
                    {QStringLiteral("type"), QStringLiteral("list")},
                    {QStringLiteral("limits"), QVariantList{QStringLiteral("a"), QStringLiteral("b")}},
                    {QStringLiteral("value"), QStringLiteral("a")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("choice"));
    CHECK(item != nullptr);
    auto* combo = qobject_cast<cppqtgraph::widgets::ComboBox*>(editorForItem(item));
    CHECK(combo != nullptr);
    combo->setCurrentIndex(1);
    QTest::qWait(0);
    CHECK(param->value().toString() == QStringLiteral("b"));
    return true;
}

bool testSimpleParameterCtorInterpretsValueAndDefault()
{
    auto boolParam = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("flag")},
                    {QStringLiteral("type"), QStringLiteral("bool")},
                    {QStringLiteral("value"), QStringLiteral("true")}});
    CHECK(boolParam->value().metaType().id() == QMetaType::Bool);
    CHECK(boolParam->value().toBool());

    auto intParam = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("count")},
                    {QStringLiteral("type"), QStringLiteral("int")},
                    {QStringLiteral("default"), QStringLiteral("7")}});
    CHECK(intParam->value().metaType().id() == QMetaType::Int);
    CHECK(intParam->value().toInt() == 7);
    return true;
}

bool testValueLessListShowsNoSelectionUntilChosen()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("choice")},
                    {QStringLiteral("type"), QStringLiteral("list")},
                    {QStringLiteral("limits"), QVariantList{QStringLiteral("a"), QStringLiteral("b")}}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    CHECK(!param->options().contains(QStringLiteral("value")));

    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("choice"));
    CHECK(item != nullptr);
    auto* combo = qobject_cast<cppqtgraph::widgets::ComboBox*>(editorForItem(item));
    CHECK(combo != nullptr);
    CHECK(combo->count() == 2);
    CHECK(combo->currentIndex() == -1);
    CHECK(item->text(1).isEmpty());

    QSignalSpy valueSpy(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanged);
    combo->setCurrentIndex(0);
    QTest::qWait(0);
    CHECK(param->options().contains(QStringLiteral("value")));
    CHECK(param->value().toString() == QStringLiteral("a"));
    CHECK(valueSpy.count() >= 1);
    CHECK(combo->currentIndex() == 0);
    return true;
}

bool testValueLessListDoesNotMutateSampleFloatWidget()
{
    const auto exampleParams = cppqtgraph::parametertree::buildExampleParametersGroup();
    auto* floatGroup = exampleParams->child(QStringLiteral("Sample Float"));
    CHECK(floatGroup != nullptr);

    auto* limitsParam = floatGroup->child(QStringLiteral("limits"));
    auto* widgetParam = floatGroup->child(QStringLiteral("widget"));
    CHECK(limitsParam != nullptr);
    CHECK(widgetParam != nullptr);
    CHECK(!limitsParam->options().contains(QStringLiteral("value")));
    CHECK(widgetParam->value().toDouble() == 0.0);

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(exampleParams, false);
    tree.show();
    QTest::qWait(0);

    CHECK(!limitsParam->options().contains(QStringLiteral("value")));
    CHECK(widgetParam->value().toDouble() == 0.0);
    return true;
}

bool testActionEnabledAndIconOptions()
{
    QTemporaryDir tempDir;
    CHECK(tempDir.isValid());
    QPixmap pixmap(8, 8);
    pixmap.fill(Qt::blue);
    const QString iconPath = tempDir.path() + QStringLiteral("/icon.png");
    CHECK(pixmap.save(iconPath));

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("Run")},
                    {QStringLiteral("type"), QStringLiteral("action")},
                    {QStringLiteral("enabled"), false},
                    {QStringLiteral("icon"), iconPath}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("Run")));
    CHECK(item != nullptr);
    CHECK(!item->actionButton()->isEnabled());
    CHECK(!item->actionButton()->icon().isNull());
    return true;
}

bool testActionActivationAndStateChanged()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("Run")},
                    {QStringLiteral("type"), QStringLiteral("action")},
                    {QStringLiteral("shortcut"), QStringLiteral("Ctrl+Shift+P")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("Run")));
    CHECK(item != nullptr);

    auto* action = dynamic_cast<cppqtgraph::parametertree::ActionParameter*>(param.get());
    CHECK(action != nullptr);

    int activationCount = 0;
    int stateChangedCount = 0;
    QObject::connect(action, &cppqtgraph::parametertree::ActionParameter::sigActivated, &tree,
                     [&](cppqtgraph::parametertree::Parameter*) { ++activationCount; });
    QObject::connect(param.get(), &cppqtgraph::parametertree::Parameter::sigStateChanged, &tree,
                     [&](cppqtgraph::parametertree::Parameter*, const QString& changeDesc, const QVariant&) {
                         if (changeDesc == QStringLiteral("activated")) {
                             ++stateChangedCount;
                         }
                     });

    QTest::mouseClick(item->actionButton(), Qt::LeftButton);
    QTest::qWait(0);
    CHECK(activationCount == 1);
    CHECK(stateChangedCount == 1);

    tree.setFocus();
    QTest::keySequence(&tree, QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    QTest::qWait(0);
    CHECK(activationCount == 2);
    CHECK(stateChangedCount == 2);
    return true;
}

bool testExpandCollapseAllActions()
{
    const auto exampleParams = cppqtgraph::parametertree::buildExampleParametersGroup();

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(exampleParams, false);
    tree.show();
    QTest::qWait(0);

    auto* expandAll = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("Expand All")));
    auto* collapseAll = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("Collapse All")));
    CHECK(expandAll != nullptr);
    CHECK(collapseAll != nullptr);

    auto* sampleListItem = findItemByName(tree.invisibleRootItem(), QStringLiteral("Sample List"));
    CHECK(sampleListItem != nullptr);
    CHECK(!sampleListItem->isExpanded());

    QTest::mouseClick(expandAll->actionButton(), Qt::LeftButton);
    QTest::qWait(0);
    CHECK(sampleListItem->isExpanded());

    QTest::mouseClick(collapseAll->actionButton(), Qt::LeftButton);
    QTest::qWait(0);
    CHECK(!sampleListItem->isExpanded());
    return true;
}

bool testNumericSuffixChangePreservesLimits()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("val")},
                    {QStringLiteral("type"), QStringLiteral("float")},
                    {QStringLiteral("value"), 3.0}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("val")));
    CHECK(item != nullptr);
    auto* spinBox = qobject_cast<cppqtgraph::widgets::SpinBox*>(item->editorWidget());
    CHECK(spinBox != nullptr);

    param->setOpts({{QStringLiteral("limits"), QVariantList{1.0, 5.0}}});
    QTest::qWait(0);
    param->setValue(10.0);
    QTest::qWait(0);
    CHECK(spinBox->value() == 5.0);

    param->setOpts({{QStringLiteral("suffix"), QStringLiteral("Hz")}});
    QTest::qWait(0);
    param->setValue(10.0);
    QTest::qWait(0);
    CHECK(spinBox->value() == 5.0);
    return true;
}

bool testNumericLimitsClearMaxBound()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("val")},
                    {QStringLiteral("type"), QStringLiteral("float")},
                    {QStringLiteral("value"), 3.0},
                    {QStringLiteral("limits"), QVariantList{1.0, 5.0}}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("val")));
    CHECK(item != nullptr);
    auto* spinBox = qobject_cast<cppqtgraph::widgets::SpinBox*>(item->editorWidget());
    CHECK(spinBox != nullptr);

    param->setValue(10.0);
    QTest::qWait(0);
    CHECK(spinBox->value() == 5.0);

    param->setOpts({{QStringLiteral("limits"), QVariantList{0.0, QVariant()}}});
    QTest::qWait(0);

    param->setValue(3.0);
    QTest::qWait(0);
    param->setValue(10.0);
    QTest::qWait(0);
    CHECK(spinBox->value() == 10.0);
    return true;
}

bool testDisabledActionShortcutDoesNotActivate()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("Run")},
                    {QStringLiteral("type"), QStringLiteral("action")},
                    {QStringLiteral("shortcut"), QStringLiteral("Ctrl+Shift+P")},
                    {QStringLiteral("enabled"), false}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("Run")));
    CHECK(item != nullptr);

    auto* action = dynamic_cast<cppqtgraph::parametertree::ActionParameter*>(param.get());
    CHECK(action != nullptr);

    int activationCount = 0;
    QObject::connect(action, &cppqtgraph::parametertree::ActionParameter::sigActivated, &tree,
                     [&](cppqtgraph::parametertree::Parameter*) { ++activationCount; });

    tree.setFocus();
    QTest::keySequence(&tree, QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    QTest::qWait(0);
    CHECK(activationCount == 0);

    param->setOpts({{QStringLiteral("enabled"), true}});
    QTest::qWait(0);
    QTest::keySequence(&tree, QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    QTest::qWait(0);
    CHECK(activationCount == 1);

    param->setOpts({{QStringLiteral("enabled"), false}});
    QTest::qWait(0);
    QTest::keySequence(&tree, QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    QTest::qWait(0);
    CHECK(activationCount == 1);
    return true;
}

bool testBuildExampleParametersGroupReleases()
{
    std::weak_ptr<cppqtgraph::parametertree::Parameter> weak;
    {
        auto exampleParams = cppqtgraph::parametertree::buildExampleParametersGroup();
        weak = exampleParams;
        CHECK(!weak.expired());

        cppqtgraph::parametertree::ParameterTree tree;
        tree.setParameters(exampleParams, false);
        tree.show();
        QTest::qWait(0);

        auto* expandAll = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
            findItemByName(tree.invisibleRootItem(), QStringLiteral("Expand All")));
        CHECK(expandAll != nullptr);
        QTest::mouseClick(expandAll->actionButton(), Qt::LeftButton);
        QTest::qWait(0);

        exampleParams.reset();
    }
    CHECK(weak.expired());
    return true;
}

bool testActionRenameUpdatesButtonText()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("Run")},
                    {QStringLiteral("type"), QStringLiteral("action")}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::ActionParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("Run")));
    CHECK(item != nullptr);
    CHECK(item->actionButton()->text() == QStringLiteral("Run"));

    param->setName(QStringLiteral("Execute"));
    QTest::qWait(0);
    CHECK(item->actionButton()->text() == QStringLiteral("Execute"));
    return true;
}

bool testTwoTreePrimitiveSync()
{
    const auto exampleParams = cppqtgraph::parametertree::buildExampleParametersGroup();
    auto tree1 = std::make_unique<cppqtgraph::parametertree::ParameterTree>();
    auto tree2 = std::make_unique<cppqtgraph::parametertree::ParameterTree>();
    tree1->setParameters(exampleParams, false);
    tree2->setParameters(exampleParams, false);
    tree1->show();
    tree2->show();
    QTest::qWait(0);

    auto* noExtra = exampleParams->child(QStringLiteral("No Extra Options"));
    CHECK(noExtra != nullptr);
    auto* strParam = noExtra->child(QStringLiteral("str"));
    CHECK(strParam != nullptr);

    strParam->setValue(QStringLiteral("synced"));
    QTest::qWait(0);

    auto* item1 = findItemByName(tree1->invisibleRootItem(), QStringLiteral("str"));
    auto* item2 = findItemByName(tree2->invisibleRootItem(), QStringLiteral("str"));
    CHECK(item1 != nullptr);
    CHECK(item2 != nullptr);
    CHECK(item1->text(1) == QStringLiteral("synced"));
    CHECK(item2->text(1) == QStringLiteral("synced"));
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard application(argc, argv);

    if (!testExampleParametersGroupOrder()) {
        return 1;
    }
    if (!testSimpleParameterCtorInterpretsValueAndDefault()) {
        return 1;
    }
    if (!testValueLessListShowsNoSelectionUntilChosen()) {
        return 1;
    }
    if (!testValueLessListDoesNotMutateSampleFloatWidget()) {
        return 1;
    }
    if (!testActionEnabledAndIconOptions()) {
        return 1;
    }
    if (!testBoolCheckboxUpdatesValue()) {
        return 1;
    }
    if (!testStrEditorChangingSignal()) {
        return 1;
    }
    if (!testNumericSpinBoxSuffixAndSiPrefixLabel()) {
        return 1;
    }
    if (!testNumericSuffixChangePreservesLimits()) {
        return 1;
    }
    if (!testNumericLimitsClearMaxBound()) {
        return 1;
    }
    if (!testListComboUpdatesValue()) {
        return 1;
    }
    if (!testActionActivationAndStateChanged()) {
        return 1;
    }
    if (!testDisabledActionShortcutDoesNotActivate()) {
        return 1;
    }
    if (!testExpandCollapseAllActions()) {
        return 1;
    }
    if (!testBuildExampleParametersGroupReleases()) {
        return 1;
    }
    if (!testActionRenameUpdatesButtonText()) {
        return 1;
    }
    if (!testTwoTreePrimitiveSync()) {
        return 1;
    }

    return 0;
}
