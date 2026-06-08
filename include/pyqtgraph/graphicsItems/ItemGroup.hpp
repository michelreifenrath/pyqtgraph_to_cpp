#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ItemGroup.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class ItemGroup : public GraphicsObject {
public:
    explicit ItemGroup(QGraphicsItem* parent = nullptr);
    ~ItemGroup() override;

    ItemGroup(const ItemGroup&) = delete;
    ItemGroup& operator=(const ItemGroup&) = delete;
    ItemGroup(ItemGroup&&) = delete;
    ItemGroup& operator=(ItemGroup&&) = delete;

    void addItem(QGraphicsItem* item);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

} // namespace pyqtgraph::graphicsItems
