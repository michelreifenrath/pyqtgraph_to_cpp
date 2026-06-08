#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ButtonItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include <pyqtgraph/GraphicsScene/GraphicsScene.hpp>

#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtGui/QPixmap>

#include <optional>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class ButtonItem : public GraphicsObject, public pyqtgraph::GraphicsScene::GraphicsSceneEventHandler {
    Q_OBJECT

public:
    explicit ButtonItem(const QPixmap& pixmap,
        std::optional<qreal> width = std::nullopt,
        QGraphicsItem* parentItem = nullptr);
    explicit ButtonItem(const QString& imageFile,
        std::optional<qreal> width = std::nullopt,
        QGraphicsItem* parentItem = nullptr);
    ~ButtonItem() override;

    ButtonItem(const ButtonItem&) = delete;
    ButtonItem& operator=(const ButtonItem&) = delete;
    ButtonItem(ButtonItem&&) = delete;
    ButtonItem& operator=(ButtonItem&&) = delete;

    void setImageFile(const QString& imageFile);
    void setPixmap(const QPixmap& pixmap);
    [[nodiscard]] QPixmap pixmap() const;

    [[nodiscard]] bool enabled() const noexcept;
    void disable();
    void enable();

    void hoverEvent(pyqtgraph::GraphicsScene::HoverEvent* event) override;
    void mouseClickEvent(pyqtgraph::GraphicsScene::MouseClickEvent* event) override;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void clicked(pyqtgraph::graphicsItems::ButtonItem* button);

private:
    void setWidthFromPixmapIfNeeded(std::optional<qreal> width);

    QPixmap pixmap_;
    qreal width_ = 0.0;
    bool enabled_ = true;
};

} // namespace pyqtgraph::graphicsItems
