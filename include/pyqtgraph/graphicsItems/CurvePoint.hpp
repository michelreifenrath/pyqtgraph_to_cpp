#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/CurvePoint.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "PlotCurveItem.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QPointer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QRectF>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <optional>

class QGraphicsPathItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class CurvePoint : public GraphicsObject {
    Q_OBJECT
    Q_PROPERTY(double position READ position WRITE setPosition)
    Q_PROPERTY(int index READ index WRITE setIndex)

public:
    explicit CurvePoint(PlotCurveItem* curve, int index = 0, bool rotate = true, QGraphicsItem* parent = nullptr);
    CurvePoint(PlotCurveItem* curve, double position, bool rotate = true, QGraphicsItem* parent = nullptr);
    ~CurvePoint() override;

    CurvePoint(const CurvePoint&) = delete;
    CurvePoint& operator=(const CurvePoint&) = delete;
    CurvePoint(CurvePoint&&) = delete;
    CurvePoint& operator=(CurvePoint&&) = delete;

    using GraphicsObject::setPos;
    void setPos(double position);

    void setPosition(double position);
    [[nodiscard]] double position() const noexcept;

    void setIndex(int index);
    [[nodiscard]] int index() const noexcept;

    [[nodiscard]] PlotCurveItem* curve() const noexcept;
    void setRotate(bool rotate);
    [[nodiscard]] bool rotate() const noexcept;

    [[nodiscard]] QPropertyAnimation* makeAnimation(
        QByteArray property = QByteArrayLiteral("position"),
        double start = 0.0,
        double end = 1.0,
        int duration = 10000,
        int loop = 1);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    enum class UpdateMode {
        Position,
        Index,
    };

    bool updateFromCurve(UpdateMode mode);

    QPointer<PlotCurveItem> curve_;
    double position_ = 0.0;
    int index_ = 0;
    bool rotate_ = true;
    UpdateMode updateMode_ = UpdateMode::Index;
};

struct CurveArrowStyle {
    bool pxMode = true;
    double angle = 0.0;
    double headLen = 20.0;
    std::optional<double> headWidth;
    double tipAngle = 25.0;
    double baseAngle = 0.0;
    std::optional<double> tailLen;
    double tailWidth = 3.0;
    QPen pen = QPen(QColor(200, 200, 200));
    QBrush brush = QBrush(QColor(50, 50, 200));
};

class CurveArrow : public CurvePoint {
public:
    explicit CurveArrow(PlotCurveItem* curve, int index = 0, QGraphicsItem* parent = nullptr);
    CurveArrow(PlotCurveItem* curve, double position, QGraphicsItem* parent = nullptr);
    ~CurveArrow() override;

    CurveArrow(const CurveArrow&) = delete;
    CurveArrow& operator=(const CurveArrow&) = delete;
    CurveArrow(CurveArrow&&) = delete;
    CurveArrow& operator=(CurveArrow&&) = delete;

    void setStyle(const CurveArrowStyle& style);
    [[nodiscard]] const CurveArrowStyle& style() const noexcept;
    [[nodiscard]] QGraphicsPathItem* arrow() noexcept;
    [[nodiscard]] const QGraphicsPathItem* arrow() const noexcept;

private:
    void createArrow();
    void applyStyle();

    CurveArrowStyle style_;
    QGraphicsPathItem* arrow_ = nullptr;
};

} // namespace pyqtgraph::graphicsItems
