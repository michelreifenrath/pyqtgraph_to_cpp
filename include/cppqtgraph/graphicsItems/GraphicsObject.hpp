#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsObject.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsItem.hpp"

#include <QtWidgets/QGraphicsObject>

class QGraphicsItem;

namespace cppqtgraph::graphicsItems {

class GraphicsObject : public QGraphicsObject, public GraphicsItem {
public:
    explicit GraphicsObject(QGraphicsItem* parent = nullptr);
    ~GraphicsObject() override;

    GraphicsObject(const GraphicsObject&) = delete;
    GraphicsObject& operator=(const GraphicsObject&) = delete;
    GraphicsObject(GraphicsObject&&) = delete;
    GraphicsObject& operator=(GraphicsObject&&) = delete;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
};

} // namespace cppqtgraph::graphicsItems
