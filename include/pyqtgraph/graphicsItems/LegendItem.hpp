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
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <optional>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class LegendItem : public GraphicsWidget, public GraphicsWidgetAnchor {
public:
    struct Options {
        std::optional<QSizeF> size;
        std::optional<QPointF> offset = QPointF(30.0, 30.0);
        qreal horizontalSpacing = 5.0;
        qreal verticalSpacing = 0.0;
        QPen pen = QPen(Qt::white);
        QBrush brush = QBrush(QColor(0, 0, 0, 200));
        QColor labelTextColor = Qt::white;
        bool frame = true;
        qreal labelTextPointSize = 9.0;
        int columnCount = 1;
    };

    explicit LegendItem(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    explicit LegendItem(std::optional<QPointF> offset, QGraphicsItem* parent = nullptr);
    explicit LegendItem(const Options& options, QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~LegendItem() override;

    LegendItem(const LegendItem&) = delete;
    LegendItem& operator=(const LegendItem&) = delete;
    LegendItem(LegendItem&&) = delete;
    LegendItem& operator=(LegendItem&&) = delete;

    [[nodiscard]] std::optional<QPointF> offset() const;
    void setOffset(std::optional<QPointF> offset);

    [[nodiscard]] QPen pen() const;
    void setPen(const QPen& pen);
    [[nodiscard]] QBrush brush() const;
    void setBrush(const QBrush& brush);
    [[nodiscard]] QColor labelTextColor() const;
    void setLabelTextColor(const QColor& color);
    [[nodiscard]] qreal labelTextPointSize() const noexcept;
    void setLabelTextPointSize(qreal pointSize);

    void addItem(QGraphicsItem* item, const QString& name);
    void removeItem(QGraphicsItem* item);
    void removeItem(const QString& name);
    void clear();

    void setColumnCount(int columnCount);
    [[nodiscard]] int columnCount() const noexcept;
    [[nodiscard]] int itemCount() const noexcept;
    [[nodiscard]] QString getLabel(QGraphicsItem* item) const;

    void setParentItem(QGraphicsItem* parent);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    struct Entry {
        QGraphicsItem* item = nullptr;
        QString name;
    };

    void updateSize();
    void anchorToOffsetIfReady();
    [[nodiscard]] QPen samplePen(const Entry& entry) const;

    std::vector<Entry> items_;
    std::optional<QSizeF> fixedSize_;
    std::optional<QPointF> offset_;
    qreal horizontalSpacing_ = 5.0;
    qreal verticalSpacing_ = 0.0;
    QPen pen_ = QPen(Qt::white);
    QBrush brush_ = QBrush(QColor(0, 0, 0, 200));
    QColor labelTextColor_ = Qt::white;
    bool frame_ = true;
    qreal labelTextPointSize_ = 9.0;
    int columnCount_ = 1;
};

} // namespace pyqtgraph::graphicsItems
