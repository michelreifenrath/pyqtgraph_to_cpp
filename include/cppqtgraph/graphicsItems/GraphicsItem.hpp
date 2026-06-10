#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/GraphicsItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <memory>

class QGraphicsItem;
class QGraphicsView;

namespace cppqtgraph::graphicsItems {

class GraphicsItem {
public:
    explicit GraphicsItem(QGraphicsItem* host = nullptr);
    virtual ~GraphicsItem();

    GraphicsItem(const GraphicsItem&) = delete;
    GraphicsItem& operator=(const GraphicsItem&) = delete;
    GraphicsItem(GraphicsItem&&) noexcept;
    GraphicsItem& operator=(GraphicsItem&&) noexcept;

    void setGraphicsItem(QGraphicsItem* host) noexcept;
    [[nodiscard]] QGraphicsItem* graphicsItem() const noexcept;

    [[nodiscard]] QGraphicsView* getViewWidget() const;
    void forgetViewWidget() const noexcept;

private:
    struct Private;
    std::unique_ptr<Private> d_;
};

} // namespace cppqtgraph::graphicsItems
