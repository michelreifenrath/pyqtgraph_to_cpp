#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LegendItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"
#include "GraphicsWidgetAnchor.hpp"

#include <QtCore/QPointF>
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
    explicit LegendItem(
        const QSizeF& size = QSizeF{},
        std::optional<QPointF> offset = std::nullopt,
        QGraphicsItem* parent = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags{});
    ~LegendItem() override;

    LegendItem(const LegendItem&) = delete;
    LegendItem& operator=(const LegendItem&) = delete;
    LegendItem(LegendItem&&) = delete;
    LegendItem& operator=(LegendItem&&) = delete;

    void setOffset(const QPointF& offset);
    [[nodiscard]] std::optional<QPointF> offset() const noexcept;

    void setPen(const QPen& pen = QPen(Qt::NoPen));
    [[nodiscard]] QPen pen() const;
    void setBrush(const QBrush& brush = QBrush(Qt::NoBrush));
    [[nodiscard]] QBrush brush() const;
    void setLabelTextColor(const QColor& color);
    [[nodiscard]] std::optional<QColor> labelTextColor() const noexcept;
    void setLabelTextSize(const QString& size);
    [[nodiscard]] QString labelTextSize() const;

    void addItem(QGraphicsItem* item, const QString& name);
    void removeItem(QGraphicsItem* item);
    void removeItem(const QString& name);
    void clear();

    void setColumnCount(int columnCount);
    [[nodiscard]] int columnCount() const noexcept;
    [[nodiscard]] int rowCount() const noexcept;
    [[nodiscard]] int itemCount() const noexcept;
    [[nodiscard]] QString labelForItem(QGraphicsItem* item) const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    struct Entry {
        QGraphicsItem* item = nullptr;
        QString name;
        QPen samplePen;
    };

    void updateSizeToContents();
    [[nodiscard]] QPen samplePenForItem(QGraphicsItem* item) const;
    [[nodiscard]] QFont labelFont() const;

    std::vector<Entry> items_;
    QSizeF fixedSize_;
    std::optional<QPointF> offset_;
    QPen pen_ = QPen(Qt::NoPen);
    QBrush brush_ = QBrush(Qt::NoBrush);
    std::optional<QColor> labelTextColor_;
    QString labelTextSize_ = QStringLiteral("9pt");
    int columnCount_ = 1;
    int rowCount_ = 1;
    qreal horizontalSpacing_ = 5.0;
    qreal verticalSpacing_ = 0.0;
    qreal sampleWidth_ = 34.0;
    qreal horizontalPadding_ = 8.0;
    qreal verticalPadding_ = 5.0;
};

} // namespace pyqtgraph::graphicsItems
