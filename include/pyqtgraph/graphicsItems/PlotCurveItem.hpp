#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/PlotCurveItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"

#include <QtCore/QRectF>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsItem>

#include <span>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class PlotCurveItem : public GraphicsObject {
public:
    explicit PlotCurveItem(QGraphicsItem* parent = nullptr);
    ~PlotCurveItem() override;

    PlotCurveItem(const PlotCurveItem&) = delete;
    PlotCurveItem& operator=(const PlotCurveItem&) = delete;
    PlotCurveItem(PlotCurveItem&&) = delete;
    PlotCurveItem& operator=(PlotCurveItem&&) = delete;

    enum class ConnectMode { All, Pairs, Finite };
    enum class StepMode { None, Left, Right, Center };

    void setData(std::span<const double> y);
    void setData(std::span<const double> x, std::span<const double> y);

    void setPen(const QPen& pen);
    [[nodiscard]] QPen pen() const;

    void setConnectMode(ConnectMode mode);
    [[nodiscard]] ConnectMode connectMode() const noexcept;

    void setStepMode(StepMode mode);
    [[nodiscard]] StepMode stepMode() const noexcept;

    void setSkipFiniteCheck(bool skipFiniteCheck);
    [[nodiscard]] bool skipFiniteCheck() const noexcept;

    [[nodiscard]] std::span<const double> xData() const noexcept;
    [[nodiscard]] std::span<const double> yData() const noexcept;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void refreshDataBoundsCache();

    std::vector<double> xData_;
    std::vector<double> yData_;
    std::vector<double> dataBoundsYData_;
    QRectF bounds_;
    QPen pen_;
    ConnectMode connectMode_ = ConnectMode::All;
    StepMode stepMode_ = StepMode::None;
    bool skipFiniteCheck_ = false;
};

} // namespace pyqtgraph::graphicsItems
