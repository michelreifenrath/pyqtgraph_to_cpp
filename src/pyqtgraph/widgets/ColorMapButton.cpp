// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../../../include/pyqtgraph/widgets/ColorMapButton.hpp"

#include "../../../include/pyqtgraph/widgets/ColorMapMenu.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

namespace pyqtgraph::widgets {
namespace {

pyqtgraph::ColorMap defaultColorMap()
{
    return pyqtgraph::ColorMap({0.0, 1.0}, {QColor(0, 0, 0), QColor(255, 255, 255)});
}

QImage imageFromLookupBytes(const std::vector<std::uint8_t>& bytes, std::size_t rows, std::size_t channels, bool horizontal)
{
    if (bytes.empty() || rows == 0 || channels < 3) {
        return {};
    }

    QImage image(horizontal ? static_cast<int>(rows) : 1,
        horizontal ? 1 : static_cast<int>(rows),
        QImage::Format_RGBA8888);
    for (std::size_t row = 0; row < rows; ++row) {
        const std::size_t offset = row * channels;
        const QColor color(bytes[offset],
            bytes[offset + 1],
            bytes[offset + 2],
            channels >= 4 ? bytes[offset + 3] : 255);
        if (horizontal) {
            image.setPixelColor(static_cast<int>(row), 0, color);
        } else {
            image.setPixelColor(0, static_cast<int>(rows - row - 1), color);
        }
    }
    return image;
}

} // namespace

ColorMapButton::ColorMapButton(QWidget* parent)
    : QWidget(parent)
    , colorMap_(defaultColorMap())
{
    setMinimumHeight(15);
    setMinimumWidth(30);
}

ColorMapButton::~ColorMapButton() = default;

void ColorMapButton::setColorMap(const pyqtgraph::ColorMap& colorMap)
{
    setColorMapInternal(colorMap, true);
}

void ColorMapButton::setColorMap(const QString& name)
{
    if (const auto resolved = pyqtgraph::get(name)) {
        setColorMapInternal(*resolved, true);
        return;
    }
    setColorMapInternal(defaultColorMap(), true);
}

pyqtgraph::ColorMap ColorMapButton::colorMap() const
{
    return colorMap_;
}

ColorMapMenu* ColorMapButton::getMenu()
{
    if (menu_ == nullptr) {
        menu_ = new ColorMapMenu(this, {}, false, true);
        connect(menu_, &ColorMapMenu::sigColorMapTriggered, this, [this](const pyqtgraph::ColorMap& colorMap) {
            setColorMap(colorMap);
        });
    }
    return menu_;
}

void ColorMapButton::setColorMapInternal(const pyqtgraph::ColorMap& colorMap, bool emitSignal)
{
    colorMap_ = colorMap;
    cachedImage_ = QImage();
    if (emitSignal) {
        colorMapChanged();
    } else {
        update();
    }
}

void ColorMapButton::colorMapChanged()
{
    emit sigColorMapChanged(colorMap_);
    update();
}

QImage ColorMapButton::colorMapImage() const
{
    if (!cachedImage_.isNull()) {
        return cachedImage_;
    }

    const auto lut = colorMap_.getLookupTable(0.0, 1.0, 256, true, pyqtgraph::ColorMap::OutputMode::Byte);
    cachedImage_ = imageFromLookupBytes(lut.bytes, lut.rows(), lut.channels, true);
    return cachedImage_;
}

void ColorMapButton::paintColorMap(QPainter& painter, const QRect& rect) const
{
    painter.save();
    const QImage image = colorMapImage();
    painter.drawImage(rect, image);

    const QString text = colorMap_.name();
    const QColor centerColor = image.isNull() ? QColor(128, 128, 128) : image.pixelColor(image.rect().center());
    const QPen pen = centerColor.lightnessF() >= 0.55 ? QPen(Qt::black) : QPen(Qt::white);
    const QRect textRect = painter.boundingRect(rect, Qt::AlignCenter, text);
    painter.setPen(pen);
    painter.drawText(textRect, text);
    painter.restore();
}

void ColorMapButton::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    paintColorMap(painter, contentsRect());
}

void ColorMapButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event == nullptr || event->button() != Qt::LeftButton) {
        return;
    }

    QPoint globalPos = mapToGlobal(pos());
    globalPos.setY(globalPos.y() + height());
    getMenu()->popup(globalPos);
}

} // namespace pyqtgraph::widgets
