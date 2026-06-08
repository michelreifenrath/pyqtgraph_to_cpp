// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/ColorButton.hpp"

#include <pyqtgraph/functions.hpp>

#include <QtGui/QBrush>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QColorDialog>

namespace pyqtgraph::widgets {

namespace {

QColor normalizeColor(const QColor& color)
{
    return pyqtgraph::mkColor(color);
}

QColor normalizeColor(std::initializer_list<int> color)
{
    if (color.size() == 3) {
        const auto it = color.begin();
        return pyqtgraph::mkColor(static_cast<double>(it[0]), static_cast<double>(it[1]), static_cast<double>(it[2]));
    }
    if (color.size() == 4) {
        const auto it = color.begin();
        return pyqtgraph::mkColor(static_cast<double>(it[0]),
            static_cast<double>(it[1]),
            static_cast<double>(it[2]),
            static_cast<double>(it[3]));
    }
    throw std::invalid_argument("ColorButton color initializer list must contain 3 or 4 values");
}

} // namespace

ColorButton::ColorButton(QWidget* parent, const QColor& color, int padding)
    : QPushButton(parent)
    , paddingLeft_(padding)
    , paddingTop_(padding)
    , paddingRight_(-padding)
    , paddingBottom_(-padding)
    , colorDialog_(std::make_unique<QColorDialog>(this))
{
    colorDialog_->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel, true);
    colorDialog_->setOption(QColorDialog::ColorDialogOption::DontUseNativeDialog, true);
    connect(colorDialog_.get(), &QColorDialog::currentColorChanged, this, &ColorButton::dialogColorChanged);
    connect(colorDialog_.get(), &QColorDialog::rejected, this, &ColorButton::colorRejected);
    connect(colorDialog_.get(), &QColorDialog::colorSelected, this, &ColorButton::colorSelected);
    connect(this, &QPushButton::clicked, this, &ColorButton::selectColor);

    setMinimumHeight(15);
    setMinimumWidth(15);
    setColor(color, true);
}

ColorButton::ColorButton(QWidget* parent, std::initializer_list<int> color, int padding)
    : ColorButton(parent, normalizeColor(color), padding)
{
}

ColorButton::~ColorButton() = default;

void ColorButton::setColor(const QColor& color, bool finished)
{
    color_ = normalizeColor(color);
    update();
    if (finished) {
        emit sigColorChanged(this);
    } else {
        emit sigColorChanging(this);
    }
}

void ColorButton::setColor(std::initializer_list<int> color, bool finished)
{
    setColor(normalizeColor(color), finished);
}

QColor ColorButton::color(const QString& mode) const
{
    const QColor normalized = normalizeColor(color_);
    if (mode == QStringLiteral("qcolor")) {
        return normalized;
    }
    if (mode == QStringLiteral("byte")) {
        return QColor::fromRgba(normalized.rgba());
    }
    if (mode == QStringLiteral("float")) {
        return QColor::fromRgbF(normalized.redF(), normalized.greenF(), normalized.blueF(), normalized.alphaF());
    }
    throw std::invalid_argument("ColorButton::color mode must be qcolor, byte, or float");
}

std::array<int, 4> ColorButton::saveState() const
{
    const QRgb rgba = color_.rgba();
    return {qRed(rgba), qGreen(rgba), qBlue(rgba), qAlpha(rgba)};
}

void ColorButton::restoreState(const std::array<int, 4>& state)
{
    setColor(QColor(state[0], state[1], state[2], state[3]), true);
}

void ColorButton::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event);

    QPainter painter(this);
    const QRect rect = this->rect().adjusted(paddingLeft_, paddingTop_, paddingRight_, paddingBottom_);
    painter.setBrush(pyqtgraph::mkBrush(QStringLiteral("w")));
    painter.drawRect(rect);
    painter.setBrush(QBrush(Qt::BrushStyle::DiagCrossPattern));
    painter.drawRect(rect);
    painter.setBrush(pyqtgraph::mkBrush(color_));
    painter.drawRect(rect);
}

void ColorButton::selectColor()
{
    origColor_ = color();
    colorDialog_->setCurrentColor(color());
    colorDialog_->open();
}

void ColorButton::dialogColorChanged(const QColor& color)
{
    if (color.isValid()) {
        setColor(color, false);
    }
}

void ColorButton::colorRejected()
{
    setColor(origColor_, false);
}

void ColorButton::colorSelected(const QColor& /*color*/)
{
    setColor(color_, true);
}

} // namespace pyqtgraph::widgets
