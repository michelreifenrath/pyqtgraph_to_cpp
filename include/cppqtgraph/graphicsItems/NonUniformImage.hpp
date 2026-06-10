#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/graphicsItems/NonUniformImage.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "GraphicsObject.hpp"
#include "cppqtgraph/core/ArrayView.hpp"
#include "cppqtgraph/functions.hpp"

#include <QtCore/QRectF>
#include <QtGui/QPen>
#include <QtGui/QPicture>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace cppqtgraph::graphicsItems {

class NonUniformImage : public GraphicsObject {
public:
    NonUniformImage(core::ArrayView<const double, 1> x,
                    core::ArrayView<const double, 1> y,
                    core::ArrayView<const double, 2> z,
                    QGraphicsItem* parent = nullptr);
    ~NonUniformImage() override;

    NonUniformImage(const NonUniformImage&) = delete;
    NonUniformImage& operator=(const NonUniformImage&) = delete;
    NonUniformImage(NonUniformImage&&) = delete;
    NonUniformImage& operator=(NonUniformImage&&) = delete;

    void setLookupTable(ImageLookupTable lut);
    void clearLookupTable();
    [[nodiscard]] std::optional<ImageLookupTable> lookupTable() const noexcept;

    void setLevels(ImageLevelRange levels);
    void clearLevels();
    [[nodiscard]] ImageLevelRange getLevels() const;

    void setBorder(std::optional<QPen> border);
    [[nodiscard]] const std::optional<QPen>& border() const noexcept;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void validateInput(core::ArrayView<const double, 1> x,
                       core::ArrayView<const double, 1> y,
                       core::ArrayView<const double, 2> z) const;
    void copyData(core::ArrayView<const double, 1> x,
                  core::ArrayView<const double, 1> y,
                  core::ArrayView<const double, 2> z);
    void resetDefaultLookupTable();
    void copyLookupTable(ImageLookupTable lut);
    [[nodiscard]] std::optional<ImageLookupTable> lookupTableView() const noexcept;
    [[nodiscard]] std::vector<double> cellEdges(const std::vector<double>& centers) const;
    void invalidatePicture();
    void generatePicture() const;

    std::vector<double> x_;
    std::vector<double> y_;
    std::vector<double> z_;
    mutable std::optional<ImageLevelRange> levels_;
    std::vector<std::uint8_t> lookupTableData_;
    std::size_t lookupTableRows_ = 0;
    std::size_t lookupTableChannels_ = 0;
    std::optional<QPen> border_;
    mutable QPicture picture_;
    mutable bool pictureValid_ = false;
};

} // namespace cppqtgraph::graphicsItems
