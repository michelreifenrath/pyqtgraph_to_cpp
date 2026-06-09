#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/SpinBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <optional>

#include <QtCore/QString>
#include <QtWidgets/QAbstractSpinBox>
#include <QtWidgets/QWidget>

namespace pyqtgraph {
class SignalProxy;
} // namespace pyqtgraph

namespace pyqtgraph::widgets {

class ErrorBox;

struct SpinBoxOptions {
    std::optional<double> minBound;
    std::optional<double> maxBound;
    std::optional<bool> wrapping;
    std::optional<double> step;
    std::optional<double> minStep;
    std::optional<bool> dec;
    std::optional<bool> integerMode;
    std::optional<bool> finite;
    std::optional<QString> prefix;
    std::optional<QString> suffix;
    std::optional<double> suffixPower;
    std::optional<bool> siPrefix;
    std::optional<double> scaleAtZero;
    std::optional<double> delay;
    std::optional<bool> delayUntilEditFinished;
    std::optional<int> decimals;
    std::optional<QString> format;
    std::optional<bool> compactHeight;
    std::optional<double> value;
};

class SpinBox : public QAbstractSpinBox {
    Q_OBJECT

public:
    explicit SpinBox(QWidget* parent = nullptr, double value = 0.0);

    SpinBox(const SpinBox&) = delete;
    SpinBox& operator=(const SpinBox&) = delete;
    SpinBox(SpinBox&&) = delete;
    SpinBox& operator=(SpinBox&&) = delete;

    [[nodiscard]] double value() const;
    double setValue(std::optional<double> value = std::nullopt, bool update = true, bool delaySignal = false);

    void setOpts(const SpinBoxOptions& opts);

    void setMinimum(std::optional<double> minimum, bool update = true);
    void setMaximum(std::optional<double> maximum, bool update = true);
    void setRange(std::optional<double> minimum, std::optional<double> maximum);

    [[nodiscard]] bool wrapping() const;
    void setWrapping(bool enable);

    void setPrefix(const QString& prefix);
    void setSuffix(const QString& suffix);
    void setSingleStep(double step);
    void setDecimals(int decimals);

    [[nodiscard]] QString formatText() const;
    [[nodiscard]] std::optional<double> interpret() const;
    [[nodiscard]] QString editorText() const;
    void setEditorText(const QString& text);

    QAbstractSpinBox::StepEnabled stepEnabled() const override;
    void stepBy(int steps) override;

    QValidator::State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;

signals:
    void valueChanged(double value);
    void sigValueChanged(SpinBox* self);
    void sigValueChanging(SpinBox* self, double value);

private slots:
    void editingFinishedEvent();
    void delayedChange();

private:
    struct Options {
        std::optional<double> minBound;
        std::optional<double> maxBound;
        bool wrapping = false;
        double step = 0.01;
        double minStep = 0.01;
        bool dec = false;
        bool integerMode = false;
        bool finite = true;
        QString prefix;
        QString suffix;
        double suffixPower = 1.0;
        bool siPrefix = false;
        std::optional<double> scaleAtZero;
        double delay = 0.3;
        bool delayUntilEditFinished = true;
        int decimals = 6;
        QString format = QStringLiteral("{prefix}{prefixGap}{scaledValueString}{suffixGap}{siPrefix}{suffix}");
        bool compactHeight = true;
    };

    void applyOpts(const SpinBoxOptions& opts);
    void updateText();
    void emitChanged();
    [[nodiscard]] double stepByValue(int steps) const;
    [[nodiscard]] bool valueInRange(double value) const;
    void updateHeight();

    Options opts_;
    double val_ = 0.0;
    double lastValEmitted_ = 0.0;
    bool hasLastValEmitted_ = false;
    QString lastText_;
    bool textValid_ = true;
    bool skipValidate_ = false;
    int lastFontHeight_ = -1;
    ErrorBox* errorBox_ = nullptr;
    pyqtgraph::SignalProxy* changeProxy_ = nullptr;
};

class ErrorBox : public QWidget {
    Q_OBJECT

public:
    explicit ErrorBox(QWidget* parent);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void resizeToParent();
};

} // namespace pyqtgraph::widgets
