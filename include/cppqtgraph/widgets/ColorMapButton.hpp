#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/ColorMapButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <cppqtgraph/colormap.hpp>

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

class QMouseEvent;
class QPaintEvent;

namespace cppqtgraph::widgets {

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

    void setColorMap(const cppqtgraph::ColorMap& colorMap);
    void setColorMap(const QString& name);
    [[nodiscard]] cppqtgraph::ColorMap colorMap() const;
    [[nodiscard]] ColorMapMenu* getMenu();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    void sigColorMapChanged(const cppqtgraph::ColorMap& colorMap);

private:
    void setColorMapInternal(const cppqtgraph::ColorMap& colorMap, bool emitSignal);
    void colorMapChanged();
    [[nodiscard]] QImage colorMapImage() const;
    void paintColorMap(QPainter& painter, const QRect& rect) const;

    cppqtgraph::ColorMap colorMap_;
    mutable QImage cachedImage_;
    ColorMapMenu* menu_{nullptr};
};

} // namespace cppqtgraph::widgets
