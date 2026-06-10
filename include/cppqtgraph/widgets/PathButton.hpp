#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/PathButton.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QPushButton>

class QPaintEvent;

namespace cppqtgraph::widgets {

class PathButton : public QPushButton {
    Q_OBJECT

public:
    explicit PathButton(QWidget* parent = nullptr);
    PathButton(QWidget* parent, const QPainterPath& path, int width = 30, int height = 30, int margin = 7);

    PathButton(const PathButton&) = delete;
    PathButton& operator=(const PathButton&) = delete;
    PathButton(PathButton&&) = delete;
    PathButton& operator=(PathButton&&) = delete;

    [[nodiscard]] int margin() const { return margin_; }
    void setMargin(int margin);

    [[nodiscard]] QPainterPath path() const { return path_; }
    void setPath(const QPainterPath& path);

    [[nodiscard]] QPen pen() const { return pen_; }
    void setPen(const QPen& pen);
    void setPen(const QString& color);

    [[nodiscard]] QBrush brush() const { return brush_; }
    void setBrush(const QBrush& brush);
    void setBrush(std::nullptr_t);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int margin_ = 7;
    QPainterPath path_;
    QPen pen_;
    QBrush brush_ = Qt::NoBrush;
};

} // namespace cppqtgraph::widgets
