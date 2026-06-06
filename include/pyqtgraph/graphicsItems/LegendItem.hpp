#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LegendItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"
#include "GraphicsWidgetAnchor.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <cstddef>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class LegendItem : public GraphicsWidget, public GraphicsWidgetAnchor {
public:
    explicit LegendItem(QGraphicsItem* parent = nullptr,
                        QPointF offset = QPointF(30.0, 30.0),
                        QPen pen = QPen(Qt::white),
                        QBrush brush = QBrush(QColor(0, 0, 0, 180)),
                        QColor labelTextColor = Qt::white,
                        bool frame = true,
                        int columnCount = 1,
                        Qt::WindowFlags flags = Qt::WindowFlags{});
    ~LegendItem() override;

    LegendItem(const LegendItem&) = delete;
    LegendItem& operator=(const LegendItem&) = delete;
    LegendItem(LegendItem&&) = delete;
    LegendItem& operator=(LegendItem&&) = delete;

    void addItem(QGraphicsItem* item, const QString& name);
    void removeItem(QGraphicsItem* item);
    void removeItem(const QString& name);
    void clear();

    [[nodiscard]] std::size_t count() const noexcept;
    [[nodiscard]] bool contains(const QString& name) const;
    [[nodiscard]] QStringList names() const;

    void setColumnCount(int columnCount);
    [[nodiscard]] int columnCount() const noexcept;

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;
    void setBrush(const QBrush& brush);
    [[nodiscard]] QBrush brush() const;
    void setLabelTextColor(const QColor& color);
    [[nodiscard]] QColor labelTextColor() const;
    void setFrame(bool frame);
    [[nodiscard]] bool frame() const noexcept;

    void setOffset(const QPointF& offset);
    [[nodiscard]] QPointF offset() const noexcept;
    void setParentItem(QGraphicsItem* parent);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    struct Entry {
        QGraphicsItem* item = nullptr;
        QString name;
        QPen samplePen;
    };

    void updateSize();
    [[nodiscard]] QRectF entryRect(std::size_t index) const;

    std::vector<Entry> entries_;
    QPointF offset_;
    QPen pen_;
    QBrush brush_;
    QColor labelTextColor_;
    bool frame_ = true;
    int columnCount_ = 1;
    QSizeF contentSize_{80.0, 24.0};
};

} // namespace pyqtgraph::graphicsItems
