#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/GraphicsScene/mouseEvents.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include <pyqtgraph/Point.hpp>

#include <QtCore/QHash>
#include <QtCore/Qt>
#include <QtCore/qglobal.h>

#include <memory>

class QGraphicsItem;
class QGraphicsSceneMouseEvent;

namespace pyqtgraph::GraphicsScene {

class MouseClickEvent;

class MouseDragEvent {
public:
    MouseDragEvent(QGraphicsSceneMouseEvent* moveEvent, QGraphicsSceneMouseEvent* pressEvent,
        QGraphicsSceneMouseEvent* lastEvent, bool start = false, bool finish = false);
    MouseDragEvent(QGraphicsSceneMouseEvent* moveEvent, const MouseClickEvent& pressEvent,
        const MouseDragEvent* lastEvent, bool start = false, bool finish = false);

    void accept(QGraphicsItem* item = nullptr) noexcept;
    void ignore() noexcept;
    [[nodiscard]] bool isAccepted() const noexcept;

    void setCurrentItem(QGraphicsItem* item) noexcept;
    [[nodiscard]] QGraphicsItem* currentItem() const noexcept;
    [[nodiscard]] QGraphicsItem* acceptedItem() const noexcept;

    [[nodiscard]] pyqtgraph::Point scenePos() const;
    [[nodiscard]] pyqtgraph::Point screenPos() const;
    [[nodiscard]] pyqtgraph::Point buttonDownScenePos() const;
    [[nodiscard]] pyqtgraph::Point buttonDownScenePos(Qt::MouseButton button) const;
    [[nodiscard]] pyqtgraph::Point buttonDownScreenPos() const;
    [[nodiscard]] pyqtgraph::Point buttonDownScreenPos(Qt::MouseButton button) const;
    [[nodiscard]] pyqtgraph::Point lastScenePos() const;
    [[nodiscard]] pyqtgraph::Point lastScreenPos() const;
    [[nodiscard]] pyqtgraph::Point pos() const;
    [[nodiscard]] pyqtgraph::Point lastPos() const;
    [[nodiscard]] pyqtgraph::Point buttonDownPos() const;
    [[nodiscard]] pyqtgraph::Point buttonDownPos(Qt::MouseButton button) const;

    [[nodiscard]] Qt::MouseButtons buttons() const noexcept;
    [[nodiscard]] Qt::MouseButton button() const noexcept;
    [[nodiscard]] bool isStart() const noexcept;
    [[nodiscard]] bool isFinish() const noexcept;
    [[nodiscard]] Qt::KeyboardModifiers modifiers() const noexcept;

private:
    pyqtgraph::Point scenePos_;
    pyqtgraph::Point screenPos_;
    QHash<Qt::MouseButton, pyqtgraph::Point> buttonDownScenePos_;
    QHash<Qt::MouseButton, pyqtgraph::Point> buttonDownScreenPos_;
    pyqtgraph::Point lastScenePos_;
    pyqtgraph::Point lastScreenPos_;
    Qt::MouseButtons buttons_ = Qt::NoButton;
    Qt::MouseButton button_ = Qt::NoButton;
    Qt::KeyboardModifiers modifiers_ = Qt::NoModifier;
    bool start_ = false;
    bool finish_ = false;
    bool accepted_ = false;
    QGraphicsItem* currentItem_ = nullptr;
    QGraphicsItem* acceptedItem_ = nullptr;
};

class MouseClickEvent {
public:
    explicit MouseClickEvent(QGraphicsSceneMouseEvent* pressEvent, bool doubleClick = false);

    void accept(QGraphicsItem* item = nullptr) noexcept;
    void ignore() noexcept;
    [[nodiscard]] bool isAccepted() const noexcept;

    void setCurrentItem(QGraphicsItem* item) noexcept;
    [[nodiscard]] QGraphicsItem* currentItem() const noexcept;
    [[nodiscard]] QGraphicsItem* acceptedItem() const noexcept;

    [[nodiscard]] pyqtgraph::Point scenePos() const;
    [[nodiscard]] pyqtgraph::Point screenPos() const;
    [[nodiscard]] pyqtgraph::Point pos() const;
    // Upstream MouseClickEvent exposes lastPos(), but its constructor does not initialise
    // a distinct last scene position; this skeleton safely mirrors the current position.
    [[nodiscard]] pyqtgraph::Point lastPos() const;

    [[nodiscard]] Qt::MouseButtons buttons() const noexcept;
    [[nodiscard]] Qt::MouseButton button() const noexcept;
    [[nodiscard]] bool doubleClick() const noexcept;
    [[nodiscard]] Qt::KeyboardModifiers modifiers() const noexcept;
    [[nodiscard]] qint64 time() const noexcept;

private:
    pyqtgraph::Point scenePos_;
    pyqtgraph::Point screenPos_;
    Qt::MouseButtons buttons_ = Qt::NoButton;
    Qt::MouseButton button_ = Qt::NoButton;
    Qt::KeyboardModifiers modifiers_ = Qt::NoModifier;
    qint64 time_ = 0;
    bool doubleClick_ = false;
    bool accepted_ = false;
    QGraphicsItem* currentItem_ = nullptr;
    QGraphicsItem* acceptedItem_ = nullptr;
};

class HoverEvent {
public:
    explicit HoverEvent(QGraphicsSceneMouseEvent* moveEvent, bool acceptable);

    void setCurrentItem(QGraphicsItem* item) noexcept;
    [[nodiscard]] QGraphicsItem* currentItem() const noexcept;

    void setEnter(bool enter) noexcept;
    void setExit(bool exit) noexcept;
    [[nodiscard]] bool isEnter() const noexcept;
    [[nodiscard]] bool isExit() const noexcept;

    bool acceptClicks(Qt::MouseButton button, QGraphicsItem* item = nullptr);
    bool acceptDrags(Qt::MouseButton button, QGraphicsItem* item = nullptr);

    [[nodiscard]] pyqtgraph::Point scenePos() const;
    [[nodiscard]] pyqtgraph::Point screenPos() const;
    [[nodiscard]] pyqtgraph::Point lastScenePos() const;
    [[nodiscard]] pyqtgraph::Point lastScreenPos() const;
    [[nodiscard]] pyqtgraph::Point pos() const;
    [[nodiscard]] pyqtgraph::Point lastPos() const;

    [[nodiscard]] Qt::MouseButtons buttons() const noexcept;
    [[nodiscard]] Qt::KeyboardModifiers modifiers() const noexcept;
    [[nodiscard]] QHash<Qt::MouseButton, QGraphicsItem*> clickItems() const;
    [[nodiscard]] QHash<Qt::MouseButton, QGraphicsItem*> dragItems() const;

private:
    struct ItemClaim {
        QGraphicsItem* item = nullptr;
        std::weak_ptr<void> lifetimeToken;

        [[nodiscard]] bool isLive() const noexcept { return item != nullptr && !lifetimeToken.expired(); }
    };

    static ItemClaim makeItemClaim(QGraphicsItem* item);
    static QHash<Qt::MouseButton, QGraphicsItem*> exposedClaims(const QHash<Qt::MouseButton, ItemClaim>& claims);

    pyqtgraph::Point scenePos_;
    pyqtgraph::Point screenPos_;
    pyqtgraph::Point lastScenePos_;
    pyqtgraph::Point lastScreenPos_;
    Qt::MouseButtons buttons_ = Qt::NoButton;
    Qt::KeyboardModifiers modifiers_ = Qt::NoModifier;
    bool acceptable_ = false;
    bool enter_ = false;
    bool exit_ = false;
    QGraphicsItem* currentItem_ = nullptr;
    QHash<Qt::MouseButton, ItemClaim> clickItems_;
    QHash<Qt::MouseButton, ItemClaim> dragItems_;
};

} // namespace pyqtgraph::GraphicsScene
