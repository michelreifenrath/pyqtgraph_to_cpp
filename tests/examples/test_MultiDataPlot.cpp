#define CPPQTGRAPH_MULTIDATAPLOT_NO_MAIN
#include "../../examples/MultiDataPlot.cpp"

#include <cppqtgraph/functions.hpp>
#include <cppqtgraph/parametertree/ParameterItem.hpp>
#include <cppqtgraph/widgets/ColorButton.hpp>
#include <cppqtgraph/widgets/ComboBox.hpp>

#include <QtGui/QColor>
#include <QtCore/QProcess>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>

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

#ifndef CPPQTGRAPH_P458_FIXTURE
#define CPPQTGRAPH_P458_FIXTURE ""
#endif

#ifndef CPPQTGRAPH_MULTIDATAPLOT_EXECUTABLE
#define CPPQTGRAPH_MULTIDATAPLOT_EXECUTABLE ""
#endif

QString fixturePath()
{
    return QString::fromUtf8(CPPQTGRAPH_P458_FIXTURE);
}

QString executablePath()
{
    return QString::fromUtf8(CPPQTGRAPH_MULTIDATAPLOT_EXECUTABLE);
}

cppqtgraph::parametertree::ParameterItem* findParameterItem(QTreeWidgetItem* root, const QString& name)
{
    if (root == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < root->childCount(); ++i) {
        if (auto* item = dynamic_cast<cppqtgraph::parametertree::ParameterItem*>(root->child(i))) {
            if (item->parameter() != nullptr && item->parameter()->name() == name) {
                return item;
            }
            if (auto* nested = findParameterItem(item, name)) {
                return nested;
            }
        }
    }
    return nullptr;
}

QWidget* editorWidgetFor(cppqtgraph::parametertree::ParameterItem* item)
{
    if (auto* widgetItem = dynamic_cast<cppqtgraph::parametertree::WidgetParameterItem*>(item)) {
        return widgetItem->editorWidget();
    }
    return nullptr;
}

bool testFixtureLoad()
{
    cppqtgraph::examples::MultiDataPlotFixture fixture;
    CHECK(cppqtgraph::examples::loadMultiDataPlotFixture(fixturePath(), fixture));
    CHECK(fixture.valueKeys.size() == 4);
    CHECK(fixture.valueKeys[0]
          == QStringLiteral("None (replaced by integer indices)"));
    CHECK(fixture.valueKeys[1] == QStringLiteral("Single curve values"));
    CHECK(fixture.singleCurveValues.size() == 15);
    CHECK(fixture.singleCurveValues.front() == 2.0);
    CHECK(fixture.singleCurveValues.back() == 19.0);
    CHECK(fixture.containerValues.size() == 5);
    CHECK(fixture.containerValues.front().size() == 15);
    CHECK(fixture.matrix2d.size() == 6);
    CHECK(fixture.matrix2d.front().size() == 15);
    CHECK(fixture.matrix2d.front()[0] == 22.0);
    CHECK(fixture.matrix2d.front()[1] == 24.0);
    CHECK(fixture.randomSelections.size() == 4);
    CHECK(fixture.randomSelections[0].xtype
          == QStringLiteral("None (replaced by integer indices)"));
    CHECK(fixture.randomSelections[0].ytype == QStringLiteral("2D matrix"));
    CHECK(fixture.randomSelections[1].xtype == QStringLiteral("2D matrix"));
    CHECK(fixture.randomSelections[1].ytype
          == QStringLiteral("None (replaced by integer indices)"));
    CHECK(fixture.randomSelections[2].xtype == QStringLiteral("Single curve values"));
    CHECK(fixture.randomSelections[2].ytype == QStringLiteral("2D matrix"));
    CHECK(fixture.randomSelections[3].xtype == QStringLiteral("2D matrix"));
    CHECK(fixture.randomSelections[3].ytype
          == QStringLiteral("container of (optionally) mixed-size curve values"));
    return true;
}

bool testFactoryLayoutAndGrab()
{
    cppqtgraph::examples::MultiDataPlotOptions options{
        .dataFixturePath = fixturePath(),
        .plotFirstSelection = true,
    };
    auto example = cppqtgraph::examples::createMultiDataPlotExample(options);

    CHECK(example.window != nullptr);
    CHECK(example.plotWidget != nullptr);
    CHECK(example.parameterTree != nullptr);
    CHECK(example.root != nullptr);
    CHECK(example.fixture != nullptr);
    CHECK(example.window->windowTitle() == QStringLiteral("pyqtgraph example: Plotting Datasets"));
    CHECK(example.parameterTree->minimumWidth() == 150);

    auto* layout = qobject_cast<QHBoxLayout*>(example.window->layout());
    CHECK(layout != nullptr);
    CHECK(layout->count() == 2);
    CHECK(layout->itemAt(0)->widget() == example.plotWidget);
    CHECK(layout->itemAt(1)->widget() == example.parameterTree);

    CHECK(example.root->child(QStringLiteral("next_plot")) != nullptr);
    CHECK(example.root->child(QStringLiteral("text")) != nullptr);
    CHECK(example.root->child(QStringLiteral("text"))->readonly());

    example.window->show();
    QApplication::processEvents();
    const QPixmap pixmap = example.window->grab();
    CHECK(!pixmap.isNull());
    CHECK(example.plotWidget->getPlotItem() != nullptr);

    return true;
}

bool testParameterEditorsUseSpecializedWidgets()
{
    cppqtgraph::examples::MultiDataPlotOptions options{
        .dataFixturePath = fixturePath(),
        .plotFirstSelection = false,
    };
    auto example = cppqtgraph::examples::createMultiDataPlotExample(options);
    example.window->show();
    QApplication::processEvents();

    auto* xtypeItem = findParameterItem(example.parameterTree->invisibleRootItem(), QStringLiteral("xtype"));
    auto* ytypeItem = findParameterItem(example.parameterTree->invisibleRootItem(), QStringLiteral("ytype"));
    auto* symbolItem = findParameterItem(example.parameterTree->invisibleRootItem(), QStringLiteral("symbol"));
    auto* symbolBrushItem =
        findParameterItem(example.parameterTree->invisibleRootItem(), QStringLiteral("symbolBrush"));
    auto* textItem = findParameterItem(example.parameterTree->invisibleRootItem(), QStringLiteral("text"));
    CHECK(xtypeItem != nullptr);
    CHECK(ytypeItem != nullptr);
    CHECK(symbolItem != nullptr);
    CHECK(symbolBrushItem != nullptr);
    CHECK(textItem != nullptr);

    CHECK(qobject_cast<cppqtgraph::widgets::ComboBox*>(editorWidgetFor(xtypeItem)) != nullptr);
    CHECK(qobject_cast<cppqtgraph::widgets::ComboBox*>(editorWidgetFor(ytypeItem)) != nullptr);
    CHECK(qobject_cast<QLineEdit*>(editorWidgetFor(xtypeItem)) == nullptr);
    CHECK(qobject_cast<QLineEdit*>(editorWidgetFor(ytypeItem)) == nullptr);

    auto* symbolCombo = qobject_cast<cppqtgraph::widgets::ComboBox*>(editorWidgetFor(symbolItem));
    CHECK(symbolCombo != nullptr);
    QStringList expectedSymbols;
    for (const auto& entry : cppqtgraph::symbolPaths()) {
        expectedSymbols.append(entry.first);
    }
    CHECK(symbolCombo->count() == expectedSymbols.size());
    for (int index = 0; index < expectedSymbols.size(); ++index) {
        CHECK(symbolCombo->itemText(index) == expectedSymbols.at(index));
    }

    auto* colorButton = qobject_cast<cppqtgraph::widgets::ColorButton*>(editorWidgetFor(symbolBrushItem));
    CHECK(colorButton != nullptr);
    CHECK(example.root->child(QStringLiteral("next_plot"))->child(QStringLiteral("symbolBrush"))->value().value<QColor>()
        == QColor(Qt::red));

    auto* textEdit = qobject_cast<QTextEdit*>(editorWidgetFor(textItem));
    CHECK(textEdit != nullptr);
    CHECK(textEdit->isReadOnly());
    CHECK(!example.fixture->randomSelections.empty());
    const QString expectedText = QStringLiteral("x=%1\ny=%2")
                                     .arg(example.fixture->randomSelections.front().xtype,
                                          example.fixture->randomSelections.front().ytype);
    CHECK(textEdit->toPlainText() == expectedText);

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

    if (!testFixtureLoad()) {
        return 1;
    }
    if (!testFactoryLayoutAndGrab()) {
        return 1;
    }
    if (!testParameterEditorsUseSpecializedWidgets()) {
        return 1;
    }
    if (!testDirectExecutableSmoke()) {
        return 1;
    }

    return 0;
}
