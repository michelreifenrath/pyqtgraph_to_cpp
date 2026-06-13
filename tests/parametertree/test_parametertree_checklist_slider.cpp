#include <cppqtgraph/parametertree/Parameter.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/parametertree/ParameterTree.hpp>
#include <cppqtgraph/parametertree/buildParamTypes.hpp>
#include <cppqtgraph/parametertree/parameterTypes/ChecklistParameter.hpp>
#include <cppqtgraph/parametertree/parameterTypes/SliderParameter.hpp>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

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

QCheckBox* findCheckBox(cppqtgraph::parametertree::ParameterTree& tree, const QString& name)
{
    auto* item = findItemByName(tree.invisibleRootItem(), name);
    if (auto* widgetItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(item)) {
        return qobject_cast<QCheckBox*>(widgetItem->editorWidget());
    }
    return nullptr;
}

QRadioButton* findRadio(cppqtgraph::parametertree::ParameterTree& tree, const QString& name)
{
    auto* item = findItemByName(tree.invisibleRootItem(), name);
    if (auto* widgetItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(item)) {
        return qobject_cast<QRadioButton*>(widgetItem->editorWidget());
    }
    return nullptr;
}

QPushButton* findMetaButton(cppqtgraph::parametertree::ParameterTree& tree, const QString& text)
{
    auto* item = findItemByName(tree.invisibleRootItem(), QStringLiteral("widget"));
    if (item == nullptr) {
        return nullptr;
    }
    const auto buttons = item->treeWidget()->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text() == text) {
            return button;
        }
    }
    return nullptr;
}

bool variantListEqual(const QVariant& left, const QVariant& right)
{
    if (left.metaType().id() != QMetaType::QVariantList || right.metaType().id() != QMetaType::QVariantList) {
        return left == right;
    }
    return left.toList() == right.toList();
}

bool testChecklistNonExclusiveInitialState()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("checklist")},
                    {QStringLiteral("limits"),
                     QVariantList{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}},
                    {QStringLiteral("value"),
                     QVariantList{QStringLiteral("one"), QStringLiteral("three")}}});

    CHECK(param->value().toList().size() == 2);
    CHECK(param->child(QStringLiteral("one"))->value().toBool());
    CHECK(!param->child(QStringLiteral("two"))->value().toBool());
    CHECK(param->child(QStringLiteral("three"))->value().toBool());
    return true;
}

bool testChecklistExclusiveCoercesSingleValue()
{
    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("checklist")},
                    {QStringLiteral("exclusive"), true},
                    {QStringLiteral("limits"),
                     QVariantList{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}},
                    {QStringLiteral("value"), QVariantList{}}});

    CHECK(param->value().toString() == QStringLiteral("one"));
    CHECK(param->child(QStringLiteral("one"))->value().toBool());
    CHECK(!param->child(QStringLiteral("two"))->value().toBool());
    return true;
}

bool testChecklistMetaButtonsAndExclusiveDisable()
{
    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("checklist")},
                    {QStringLiteral("limits"),
                     QVariantList{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}},
                    {QStringLiteral("expanded"), true}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* clearBtn = findMetaButton(tree, QStringLiteral("Clear All"));
    auto* selectBtn = findMetaButton(tree, QStringLiteral("Select All"));
    CHECK(clearBtn != nullptr);
    CHECK(selectBtn != nullptr);
    CHECK(clearBtn->isEnabled());
    CHECK(selectBtn->isEnabled());

    QTest::mouseClick(selectBtn, Qt::LeftButton);
    QTest::qWait(0);
    CHECK(param->value().toList().size() == 3);

    QTest::mouseClick(clearBtn, Qt::LeftButton);
    QTest::qWait(0);
    CHECK(param->value().toList().isEmpty());

    param->setOpts({{QStringLiteral("exclusive"), true}});
    QTest::qWait(0);
    CHECK(!clearBtn->isEnabled());
    CHECK(!selectBtn->isEnabled());
    return true;
}

bool testChecklistDelayedValueSignals()
{
    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("checklist")},
                    {QStringLiteral("delay"), 0.05},
                    {QStringLiteral("limits"),
                     QVariantList{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}},
                    {QStringLiteral("value"), QVariantList{QStringLiteral("two")}}});

    QSignalSpy changing(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanging);
    QSignalSpy changed(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanged);
    CHECK(changing.isValid());
    CHECK(changed.isValid());

    param->child(QStringLiteral("one"))->setValue(true);
    CHECK(changing.count() >= 1);
    CHECK(changed.isEmpty());
    QTest::qWait(80);
    CHECK(changed.count() >= 1);
    return true;
}

bool testSliderLimitsStepLabelsAndSignals()
{
    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("slider")},
                    {QStringLiteral("limits"), QVariantList{0, 10}},
                    {QStringLiteral("step"), 2.0},
                    {QStringLiteral("precision"), 0},
                    {QStringLiteral("format"), QStringLiteral("{0:>3}")},
                    {QStringLiteral("value"), 4.0}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("widget")));
    CHECK(item != nullptr);
    auto* editor = item->editorWidget();
    CHECK(editor != nullptr);
    auto* slider = editor->findChild<QSlider*>();
    CHECK(slider != nullptr);
    CHECK(slider->maximum() == 5);
    CHECK(slider->value() == 2);

    const QList<QLabel*> labels = editor->findChildren<QLabel*>();
    CHECK(!labels.isEmpty());
    CHECK(labels.first()->text() == QStringLiteral("  4"));

    QSignalSpy changing(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanging);
    QSignalSpy changed(param.get(), &cppqtgraph::parametertree::Parameter::sigValueChanged);
    slider->resize(300, 30);
    const int y = slider->height() / 2;
    const auto xFor = [&](int value) {
        const int max = std::max(1, slider->maximum());
        return (slider->width() * value) / max;
    };
    QTest::mousePress(slider, Qt::LeftButton, Qt::NoModifier, QPoint(xFor(2), y));
    QTest::mouseMove(slider, QPoint(xFor(3), y), 10);
    CHECK(changing.count() >= 1);
    QTest::mouseRelease(slider, Qt::LeftButton, Qt::NoModifier, QPoint(xFor(3), y));
    slider->setValue(3);
    QTest::qWait(0);
    CHECK(changed.count() >= 1);
    CHECK(param->value().toDouble() == 6.0);
    CHECK(labels.first()->text() == QStringLiteral("  6"));
    return true;
}

bool testSliderCustomFormatOmitsSuffixOnInlineLabel()
{
    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("slider")},
                    {QStringLiteral("limits"), QVariantList{0, 10}},
                    {QStringLiteral("step"), 2.0},
                    {QStringLiteral("precision"), 0},
                    {QStringLiteral("format"), QStringLiteral("{0:>3}")},
                    {QStringLiteral("suffix"), QStringLiteral("ms")},
                    {QStringLiteral("value"), 4.0}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("widget")));
    CHECK(item != nullptr);
    auto* editor = item->editorWidget();
    CHECK(editor != nullptr);
    const QList<QLabel*> labels = editor->findChildren<QLabel*>();
    CHECK(!labels.isEmpty());
    CHECK(labels.first()->text() == QStringLiteral("  4"));

    auto fallbackParam = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("slider")},
                    {QStringLiteral("limits"), QVariantList{0, 10}},
                    {QStringLiteral("step"), 2.0},
                    {QStringLiteral("precision"), 0},
                    {QStringLiteral("suffix"), QStringLiteral("ms")},
                    {QStringLiteral("value"), 4.0}});

    cppqtgraph::parametertree::ParameterTree fallbackTree;
    fallbackTree.setParameters(fallbackParam, false);
    fallbackTree.show();
    QTest::qWait(0);

    auto* fallbackItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(fallbackTree.invisibleRootItem(), QStringLiteral("widget")));
    CHECK(fallbackItem != nullptr);
    const QList<QLabel*> fallbackLabels = fallbackItem->editorWidget()->findChildren<QLabel*>();
    CHECK(!fallbackLabels.isEmpty());
    CHECK(fallbackLabels.first()->text() == QStringLiteral("4 ms"));
    return true;
}

bool testSliderSpanModeUsesPinnedReferenceValues()
{
    const QVariantList span = [](const std::vector<double>& values) {
        QVariantList list;
        for (double value : values) {
            list.append(value);
        }
        return list;
    }(cppqtgraph::parametertree::arangeSquared(10));

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("slider")},
                    {QStringLiteral("span"), span},
                    {QStringLiteral("precision"), 0},
                    {QStringLiteral("value"), 16.0}});

    CHECK(param->value().toDouble() == 16.0);

    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);
    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("widget")));
    auto* slider = item->editorWidget()->findChild<QSlider*>();
    CHECK(slider != nullptr);
    CHECK(slider->maximum() == 9);
    CHECK(slider->value() == 4);
    return true;
}

bool testSliderNonDividingStepSpan()
{
    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);

    auto param = cppqtgraph::parametertree::Parameter::create(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("widget")},
                    {QStringLiteral("type"), QStringLiteral("slider")},
                    {QStringLiteral("limits"), QVariantList{0, 10}},
                    {QStringLiteral("step"), 3.0},
                    {QStringLiteral("precision"), 0},
                    {QStringLiteral("value"), 9.0}});

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(param, false);
    tree.show();
    QTest::qWait(0);

    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(tree.invisibleRootItem(), QStringLiteral("widget")));
    CHECK(item != nullptr);
    auto* slider = item->editorWidget()->findChild<QSlider*>();
    CHECK(slider != nullptr);
    CHECK(slider->maximum() == 4);
    CHECK(slider->value() == 3);
    CHECK(param->value().toDouble() == 9.0);
    return true;
}

bool testSliderHowToSetSwapsSpanAndLimits()
{
    int argc = 0;
    char** argv = nullptr;
    ApplicationGuard guard(argc, argv);

    auto exampleParams = cppqtgraph::parametertree::buildExampleParametersGroup();
    auto* sliderGroup = exampleParams->child(QStringLiteral("Sample Slider"));
    CHECK(sliderGroup != nullptr);
    auto* widgetParam = sliderGroup->child(QStringLiteral("widget"));
    auto* howToSet = sliderGroup->child(QStringLiteral("How to Set"));
    auto* stepOption = sliderGroup->child(QStringLiteral("step"));
    CHECK(widgetParam != nullptr);
    CHECK(howToSet != nullptr);
    CHECK(stepOption != nullptr);

    cppqtgraph::parametertree::ParameterTree tree;
    tree.setParameters(exampleParams, false);
    tree.show();
    QTest::qWait(0);

    auto* sliderGroupItem = findItemByName(tree.invisibleRootItem(), QStringLiteral("Sample Slider"));
    CHECK(sliderGroupItem != nullptr);
    auto* item = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(
        findItemByName(sliderGroupItem, QStringLiteral("widget")));
    CHECK(item != nullptr);
    auto* slider = item->editorWidget()->findChild<QSlider*>();
    CHECK(slider != nullptr);
    CHECK(slider->maximum() == 100);

    howToSet->setValue(QStringLiteral("Use span"));
    QTest::qWait(0);
    CHECK(slider->maximum() == 49);

    howToSet->setValue(QStringLiteral("Use step + limits"));
    QTest::qWait(0);
    CHECK(slider->maximum() == 100);
    CHECK(widgetParam->value().toDouble() == 0.0);

    stepOption->setValue(2.0);
    QTest::qWait(0);
    CHECK(slider->maximum() == 50);

    howToSet->setValue(QStringLiteral("Use span"));
    QTest::qWait(0);
    CHECK(slider->maximum() == 49);
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    const bool ok = testChecklistNonExclusiveInitialState()
        && testChecklistExclusiveCoercesSingleValue()
        && testChecklistMetaButtonsAndExclusiveDisable()
        && testChecklistDelayedValueSignals()
        && testSliderLimitsStepLabelsAndSignals()
        && testSliderCustomFormatOmitsSuffixOnInlineLabel()
        && testSliderNonDividingStepSpan()
        && testSliderSpanModeUsesPinnedReferenceValues()
        && testSliderHowToSetSwapsSpanAndLimits();

    if (!ok) {
        return 1;
    }

    std::cout << "All parametertree checklist/slider tests passed\n";
    return 0;
}
