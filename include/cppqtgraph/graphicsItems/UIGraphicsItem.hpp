#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/UIGraphicsItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class UIGraphicsItem : public GraphicsObject {
public:
    explicit UIGraphicsItem(const QRectF& bounds = QRectF(0.0, 0.0, 1.0, 1.0),
                            QGraphicsItem* parent = nullptr);
    ~UIGraphicsItem() override;

    UIGraphicsItem(const UIGraphicsItem&) = delete;
    UIGraphicsItem& operator=(const UIGraphicsItem&) = delete;
    UIGraphicsItem(UIGraphicsItem&&) = delete;
    UIGraphicsItem& operator=(UIGraphicsItem&&) = delete;

    [[nodiscard]] QRectF viewBounds() const noexcept;
    void setViewBounds(const QRectF& bounds);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void updateView();

    QRectF viewBounds_;
    mutable QRectF cachedBoundingRect_;
};

} // namespace cppqtgraph::graphicsItems
