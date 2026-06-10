#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsWidgetAnchor.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <QtCore/QMetaObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>

class QGraphicsWidget;

namespace cppqtgraph::graphicsItems {

class GraphicsWidgetAnchor {
public:
    explicit GraphicsWidgetAnchor(QGraphicsWidget* item = nullptr);
    virtual ~GraphicsWidgetAnchor();

    GraphicsWidgetAnchor(const GraphicsWidgetAnchor&) = delete;
    GraphicsWidgetAnchor& operator=(const GraphicsWidgetAnchor&) = delete;
    GraphicsWidgetAnchor(GraphicsWidgetAnchor&&) = delete;
    GraphicsWidgetAnchor& operator=(GraphicsWidgetAnchor&&) = delete;

    void setAnchorItem(QGraphicsWidget* item);
    [[nodiscard]] QGraphicsWidget* anchorItem() const noexcept;

    void anchor(const QPointF& itemPos, const QPointF& parentPos, const QPointF& offset = QPointF{});
    void autoAnchor(const QPointF& pos, bool relative = true);

protected:
    void updateAnchorPosition();

private:
    void connectItemGeometryChanged();
    void disconnectItemGeometryChanged() noexcept;
    void connectParentGeometryChanged(QGraphicsWidget* parent);
    void disconnectParentGeometryChanged() noexcept;

    QPointer<QGraphicsWidget> item_;
    QPointer<QGraphicsWidget> parent_;
    QPointF parentAnchor_;
    QPointF itemAnchor_;
    QPointF offset_;
    bool hasAnchor_ = false;
    QMetaObject::Connection itemGeometryChangedConnection_;
    QMetaObject::Connection parentGeometryChangedConnection_;
};

} // namespace cppqtgraph::graphicsItems
