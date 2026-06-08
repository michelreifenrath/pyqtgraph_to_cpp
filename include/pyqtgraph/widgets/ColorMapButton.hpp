#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <pyqtgraph/colormap.hpp>

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

class QMouseEvent;
class QPaintEvent;

namespace pyqtgraph::widgets {

class ColorMapMenu;

class ColorMapButton : public QWidget {
    Q_OBJECT

public:
    explicit ColorMapButton(QWidget* parent = nullptr);
    ~ColorMapButton() override;

    ColorMapButton(const ColorMapButton&) = delete;
    ColorMapButton& operator=(const ColorMapButton&) = delete;
    ColorMapButton(ColorMapButton&&) = delete;
    ColorMapButton& operator=(ColorMapButton&&) = delete;

    void setColorMap(const pyqtgraph::ColorMap& colorMap);
    void setColorMap(const QString& name);
    [[nodiscard]] pyqtgraph::ColorMap colorMap() const;
    [[nodiscard]] ColorMapMenu* getMenu();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    void sigColorMapChanged(const pyqtgraph::ColorMap& colorMap);

private:
    void setColorMapInternal(const pyqtgraph::ColorMap& colorMap, bool emitSignal);
    void colorMapChanged();
    [[nodiscard]] QImage colorMapImage() const;
    void paintColorMap(QPainter& painter, const QRect& rect) const;

    pyqtgraph::ColorMap colorMap_;
    mutable QImage cachedImage_;
    ColorMapMenu* menu_{nullptr};
};

} // namespace pyqtgraph::widgets
