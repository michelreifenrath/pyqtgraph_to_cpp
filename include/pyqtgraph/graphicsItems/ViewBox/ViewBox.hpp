#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/ViewBox/ViewBox.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "../GraphicsWidget.hpp"

#include <QtCore/QRectF>
#include <QtCore/Qt>
#include <QtWidgets/QGraphicsItem>

#include <array>
#include <optional>

namespace pyqtgraph::graphicsItems {

class ViewBox : public GraphicsWidget {
public:
    static constexpr int PanMode = 3;
    static constexpr int RectMode = 1;
    static constexpr int XAxis = 0;
    static constexpr int YAxis = 1;
    static constexpr int XYAxes = 2;

    using AxisRange = std::array<qreal, 2>;
    using Range2D = std::array<AxisRange, 2>;

    struct Limits {
        std::optional<qreal> xMin;
        std::optional<qreal> xMax;
        std::optional<qreal> yMin;
        std::optional<qreal> yMax;
        std::optional<qreal> minXRange;
        std::optional<qreal> maxXRange;
        std::optional<qreal> minYRange;
        std::optional<qreal> maxYRange;
    };

    explicit ViewBox(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~ViewBox() override;

    ViewBox(const ViewBox&) = delete;
    ViewBox& operator=(const ViewBox&) = delete;
    ViewBox(ViewBox&&) = delete;
    ViewBox& operator=(ViewBox&&) = delete;

    [[nodiscard]] Range2D viewRange() const;
    [[nodiscard]] Range2D targetRange() const;
    [[nodiscard]] QRectF viewRect() const;
    [[nodiscard]] QRectF targetRect() const;
    [[nodiscard]] Limits limits() const;

    void setRange(const QRectF& rect, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);
    void setRange(std::optional<AxisRange> xRange,
                  std::optional<AxisRange> yRange,
                  qreal padding = 0.02,
                  bool update = true,
                  bool disableAutoRange = true);
    void setXRange(qreal min, qreal max, qreal padding = 0.02, bool update = true);
    void setYRange(qreal min, qreal max, qreal padding = 0.02, bool update = true);
    void setLimits(const Limits& limits);

private:
    Range2D targetRange_{{AxisRange{0.0, 1.0}, AxisRange{0.0, 1.0}}};
    Range2D viewRange_{{AxisRange{0.0, 1.0}, AxisRange{0.0, 1.0}}};
    Limits limits_;
    std::array<bool, 2> autoRange_{{true, true}};
};

} // namespace pyqtgraph::graphicsItems
