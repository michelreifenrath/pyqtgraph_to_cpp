#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtGui/QColor>
#include <QtWidgets/QPushButton>

#include <array>
#include <memory>

class QColorDialog;
class QPaintEvent;

namespace pyqtgraph::widgets {

class ColorButton : public QPushButton {
    Q_OBJECT

public:
    explicit ColorButton(QWidget* parent = nullptr, const QColor& color = QColor(128, 128, 128), int padding = 6);
    explicit ColorButton(QWidget* parent, std::initializer_list<int> color, int padding = 6);
    ~ColorButton() override;

    ColorButton(const ColorButton&) = delete;
    ColorButton& operator=(const ColorButton&) = delete;
    ColorButton(ColorButton&&) = delete;
    ColorButton& operator=(ColorButton&&) = delete;

    void setColor(const QColor& color, bool finished = true);
    void setColor(std::initializer_list<int> color, bool finished = true);

    [[nodiscard]] QColor color(const QString& mode = QStringLiteral("qcolor")) const;
    [[nodiscard]] std::array<int, 4> saveState() const;
    void restoreState(const std::array<int, 4>& state);

protected:
    void paintEvent(QPaintEvent* event) override;

public slots:
    void selectColor();

private slots:
    void dialogColorChanged(const QColor& color);
    void colorRejected();
    void colorSelected(const QColor& color);

signals:
    void sigColorChanging(ColorButton* button);
    void sigColorChanged(ColorButton* button);

private:
    QColor color_ = QColor(128, 128, 128);
    int paddingLeft_ = 6;
    int paddingTop_ = 6;
    int paddingRight_ = -6;
    int paddingBottom_ = -6;
    QColor origColor_;
    std::unique_ptr<QColorDialog> colorDialog_;
};

} // namespace pyqtgraph::widgets
