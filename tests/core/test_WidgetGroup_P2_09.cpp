#include <pyqtgraph/WidgetGroup.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef PYQTGRAPH_CPP_P2_09_FIXTURE
#define PYQTGRAPH_CPP_P2_09_FIXTURE "oracle/fixtures/P2_09/widgetgroup_oracle.json"
#endif

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

std::string readOracleFixture()
{
    std::ifstream input(std::filesystem::path{PYQTGRAPH_CPP_P2_09_FIXTURE});
    if (!input.good()) {
        std::cerr << "missing P2.09 oracle fixture: " << PYQTGRAPH_CPP_P2_09_FIXTURE << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(std::string_view text, std::string_view needle)
{
    return text.find(needle) != std::string_view::npos;
}

bool closeTo(double actual, double expected, double tolerance = 1e-12)
{
    return std::abs(actual - expected) <= tolerance;
}

bool requireP209OracleFixture()
{
    const std::string fixture = readOracleFixture();
    CHECK(contains(fixture, "\"issue\": \"P2.09\""));
    CHECK(contains(fixture, "\"commit\": \"a20028b98294b9cc8770f2015a92eb342224b788\""));
    CHECK(contains(fixture, "\"pyqtgraph/WidgetGroup.py\""));
    CHECK(contains(fixture, "\"supported_builtin_widgets\""));
    CHECK(contains(fixture, "\"QSplitter\""));
    CHECK(contains(fixture, "\"combo_data_then_text\""));
    CHECK(contains(fixture, "\"scale_save_restore\""));
    CHECK(contains(fixture, "\"splitter_uncached_state\""));
    CHECK(contains(fixture, "floating scale values compared within 1e-12"));
    return true;
}

bool testTypeShape()
{
    using pyqtgraph::WidgetGroup;

    static_assert(std::is_base_of_v<QObject, WidgetGroup>);
    static_assert(std::is_constructible_v<WidgetGroup>);
    static_assert(std::is_constructible_v<WidgetGroup, QObject*>);
    static_assert(!std::is_copy_constructible_v<WidgetGroup>);
    static_assert(!std::is_copy_assignable_v<WidgetGroup>);

    WidgetGroup group;
    CHECK(group.state().isEmpty());

    QSpinBox spin;
    CHECK(group.acceptsType(&spin));
    QWidget plain;
    CHECK(!group.acceptsType(&plain));

    return true;
}

bool testNamedBuiltinRoundtripAndUnknownKeys()
{
    pyqtgraph::WidgetGroup group;

    QSpinBox spin;
    spin.setRange(-1000, 1000);
    spin.setValue(12);
    QDoubleSpinBox doubleSpin;
    doubleSpin.setRange(-1000.0, 1000.0);
    doubleSpin.setDecimals(6);
    doubleSpin.setValue(3.25);
    QCheckBox checkBox;
    checkBox.setChecked(true);
    QComboBox combo;
    combo.addItem("plain");
    combo.addItem("data item", 20);
    combo.setCurrentIndex(1);
    QLineEdit line;
    line.setText("original");
    QRadioButton radio;
    radio.setChecked(true);
    QSlider slider;
    slider.setRange(0, 100);
    slider.setValue(7);

    group.addWidget(&spin, "spin");
    group.addWidget(&doubleSpin, "double");
    group.addWidget(&checkBox, "check");
    group.addWidget(&combo, "combo");
    group.addWidget(&line, "line");
    group.addWidget(&radio, "radio");
    group.addWidget(&slider, "slider");

    QVariantMap saved = group.state();
    CHECK(saved.value("spin").toInt() == 12);
    CHECK(closeTo(saved.value("double").toDouble(), 3.25));
    CHECK(saved.value("check").toBool());
    CHECK(saved.value("combo").toInt() == 20);
    CHECK(saved.value("line").toString() == "original");
    CHECK(saved.value("radio").toBool());
    CHECK(saved.value("slider").toInt() == 7);

    spin.setValue(-4);
    doubleSpin.setValue(9.5);
    checkBox.setChecked(false);
    combo.setCurrentIndex(0);
    line.setText("changed without editingFinished");
    radio.setChecked(false);
    slider.setValue(99);

    QVariantMap restore = saved;
    restore.insert("unknown", 12345);
    group.setState(restore);

    CHECK(spin.value() == 12);
    CHECK(closeTo(doubleSpin.value(), 3.25));
    CHECK(checkBox.isChecked());
    CHECK(combo.currentIndex() == 1);
    CHECK(line.text() == "original");
    CHECK(radio.isChecked());
    CHECK(slider.value() == 7);
    CHECK(group.findWidget("slider") == &slider);
    CHECK(group.findWidget("unknown") == nullptr);

    combo.setCurrentIndex(0);
    CHECK(group.state().value("combo").toString() == "plain");
    group.setState(QVariantMap{{"combo", 20}});
    CHECK(combo.currentIndex() == 1);

    return true;
}

bool testScaleSaveRestore()
{
    pyqtgraph::WidgetGroup group;
    QSpinBox spin;
    spin.setRange(0, 1000);
    spin.setValue(200);

    group.addWidget(&spin, "scaled", 100.0);
    CHECK(closeTo(group.state().value("scaled").toDouble(), 2.0));

    group.setState(QVariantMap{{"scaled", 1.5}});
    CHECK(spin.value() == 150);
    CHECK(closeTo(group.state().value("scaled").toDouble(), 1.5));

    group.setScale(&spin, 50.0);
    CHECK(spin.value() == 75);
    CHECK(closeTo(group.state().value("scaled").toDouble(), 1.5));

    return true;
}

bool testSigChangedEmitsOnCacheDeltaOnly()
{
    pyqtgraph::WidgetGroup group;
    QSpinBox spin;
    spin.setRange(0, 10);
    spin.setValue(1);
    group.addWidget(&spin, "spin");

    std::vector<QString> names;
    std::vector<QVariant> values;
    QObject::connect(&group, &pyqtgraph::WidgetGroup::sigChanged,
        [&names, &values](const QString& name, const QVariant& value) {
            names.push_back(name);
            values.push_back(value);
        });

    spin.setValue(1);
    CHECK(names.empty());

    spin.setValue(2);
    CHECK(names.size() == 1);
    CHECK(names.back() == "spin");
    CHECK(values.back().toInt() == 2);
    CHECK(group.state().value("spin").toInt() == 2);

    spin.setValue(2);
    CHECK(names.size() == 1);

    return true;
}

bool testAutoAddRecursion()
{
    QWidget parent;

    auto* directSpin = new QSpinBox(&parent);
    directSpin->setObjectName("directSpin");
    directSpin->setValue(4);

    auto* groupBox = new QGroupBox(&parent);
    groupBox->setObjectName("groupBox");
    groupBox->setCheckable(true);
    groupBox->setChecked(true);
    auto* nestedSlider = new QSlider(groupBox);
    nestedSlider->setObjectName("nestedSlider");
    nestedSlider->setRange(0, 10);
    nestedSlider->setValue(6);

    auto* splitter = new QSplitter(&parent);
    splitter->setObjectName("splitter");
    auto* splitSpin = new QSpinBox(splitter);
    splitSpin->setObjectName("splitSpin");
    splitSpin->setValue(8);
    splitter->addWidget(splitSpin);
    splitter->addWidget(new QLabel("other", splitter));

    pyqtgraph::WidgetGroup group;
    group.autoAdd(&parent);
    QVariantMap state = group.state();

    CHECK(state.value("directSpin").toInt() == 4);
    CHECK(state.value("groupBox").toBool());
    CHECK(state.value("nestedSlider").toInt() == 6);
    CHECK(state.value("splitter").toString().size() > 0);
    CHECK(state.value("splitSpin").toInt() == 8);

    group.setState(QVariantMap{{"directSpin", 2}, {"nestedSlider", 3}, {"splitSpin", 5}});
    CHECK(directSpin->value() == 2);
    CHECK(nestedSlider->value() == 3);
    CHECK(splitSpin->value() == 5);

    return true;
}

bool testSplitterStateRestoreAndAllZeroFallback()
{
    QSplitter splitter;
    splitter.setObjectName("splitter");
    splitter.addWidget(new QLabel("left", &splitter));
    splitter.addWidget(new QLabel("right", &splitter));
    splitter.resize(200, 30);
    splitter.show();
    QCoreApplication::processEvents();
    splitter.setSizes({80, 120});

    pyqtgraph::WidgetGroup group;
    group.addWidget(&splitter, "splitter");

    const QVariantMap saved = group.state();
    const QString encoded = saved.value("splitter").toString();
    CHECK(!encoded.isEmpty());

    group.setState(QVariantMap{{"splitter", QVariantList{0, 0}}});
    const QList<int> fallbackSizes = splitter.sizes();
    CHECK(fallbackSizes.size() == 2);
    CHECK(fallbackSizes[0] > 0 || fallbackSizes[1] > 0);

    splitter.setSizes({10, 190});
    group.setState(QVariantMap{{"splitter", encoded}});
    const QString encodedAfterRestore = group.state().value("splitter").toString();
    CHECK(!encodedAfterRestore.isEmpty());

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ApplicationGuard app(argc, argv);

    const bool ok = requireP209OracleFixture()
        && testTypeShape()
        && testNamedBuiltinRoundtripAndUnknownKeys()
        && testScaleSaveRestore()
        && testSigChangedEmitsOnCacheDeltaOnly()
        && testAutoAddRecursion()
        && testSplitterStateRestoreAndAllZeroFallback();

    return ok ? 0 : 1;
}
