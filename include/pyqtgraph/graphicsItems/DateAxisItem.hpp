#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/DateAxisItem.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "AxisItem.hpp"

#include <QtCore/QString>
#include <QtWidgets/QGraphicsItem>

#include <memory>
#include <optional>
#include <vector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace pyqtgraph::graphicsItems {

class DateAxisItem : public AxisItem {
public:
    DateAxisItem();
    explicit DateAxisItem(
        QGraphicsItem* parent,
        Qt::WindowFlags flags = Qt::WindowFlags{});
    explicit DateAxisItem(
        Orientation orientation,
        std::optional<int> utcOffset = std::nullopt,
        QGraphicsItem* parent = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags{});
    explicit DateAxisItem(
        const QString& orientation,
        std::optional<int> utcOffset = std::nullopt,
        QGraphicsItem* parent = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags{});
    ~DateAxisItem() override;

    DateAxisItem(const DateAxisItem&) = delete;
    DateAxisItem& operator=(const DateAxisItem&) = delete;
    DateAxisItem(DateAxisItem&&) = delete;
    DateAxisItem& operator=(DateAxisItem&&) = delete;

    void setUtcOffset(std::optional<int> utcOffset);
    [[nodiscard]] std::optional<int> utcOffset() const;

    void setZoomLevelForDensity(double density) const;
    [[nodiscard]] QString zoomLevelName() const;
    [[nodiscard]] double minSpacing() const noexcept;

    [[nodiscard]] std::vector<TickLevel> tickValues(double minimum, double maximum, double size) const;
    [[nodiscard]] std::vector<QString> tickStrings(
        const std::vector<double>& values,
        double scale,
        double spacing) const;

    // DateAxisItem supplies its own draw specs/paint path because the current
    // AxisItem extension points are non-virtual. Unsupported inherited styling
    // knobs fall back to AxisItem defaults; utcOffset, orientation, range,
    // pens, and tick font are honored for the P3.08 public surface.
    [[nodiscard]] std::optional<DrawSpecs> generateDrawSpecs(QPainter& painter) const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    struct Private;
    std::unique_ptr<Private> dDate_;
};

} // namespace pyqtgraph::graphicsItems
