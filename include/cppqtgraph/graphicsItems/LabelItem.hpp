#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/LabelItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsWidget.hpp"
#include "GraphicsWidgetAnchor.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QColor>

#include <map>
#include <optional>
#include <string>

class QGraphicsTextItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class LabelItem : public GraphicsWidget, public GraphicsWidgetAnchor {
public:
    struct TextStyleOptions {
        std::optional<QColor> color;
        QString justify = QStringLiteral("center");
        std::optional<QString> family;
        std::optional<QString> size;
        std::optional<bool> bold;
        std::optional<bool> italic;
    };

    explicit LabelItem(const QString& text = QStringLiteral(" "),
                       QGraphicsItem* parent = nullptr,
                       qreal angle = 0.0);
    explicit LabelItem(const QString& text, const TextStyleOptions& options, QGraphicsItem* parent = nullptr, qreal angle = 0.0);
    ~LabelItem() override;

    LabelItem(const LabelItem&) = delete;
    LabelItem& operator=(const LabelItem&) = delete;
    LabelItem(LabelItem&&) = delete;
    LabelItem& operator=(LabelItem&&) = delete;

    void setAttr(const QString& attr, const QVariant& value);
    void setText(const QString& text);
    void setText(const QString& text, const TextStyleOptions& options);
    void setAngle(qreal angle);
    [[nodiscard]] qreal angle() const noexcept;
    [[nodiscard]] QString justify() const;
    [[nodiscard]] QRectF itemRect() const;
    [[nodiscard]] QSizeF sizeHint(Qt::SizeHint hint, const QSizeF& constraint = QSizeF()) const override;

protected:
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;

private:
    void applyStyledHtml(const QString& text);
    void updateMin();
    void layoutText();

    QGraphicsTextItem* textItem_ = nullptr;
    QString text_ = QStringLiteral(" ");
    TextStyleOptions style_;
    qreal angle_ = 0.0;
    std::map<Qt::SizeHint, QSizeF> sizeHints_;
};

} // namespace cppqtgraph::graphicsItems
