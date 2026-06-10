#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/widgets/RawImageWidget.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "cppqtgraph/core/ArrayView.hpp"
#include "cppqtgraph/functions.hpp"

#include <QtGui/QImage>
#include <QtWidgets/QWidget>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class QPaintEvent;

namespace cppqtgraph::widgets {

class RawImageWidget : public QWidget {
    Q_OBJECT

public:
    enum class AxisOrder {
        ColMajor,
        RowMajor,
    };

    explicit RawImageWidget(QWidget* parent = nullptr);
    ~RawImageWidget() override;

    RawImageWidget(const RawImageWidget&) = delete;
    RawImageWidget& operator=(const RawImageWidget&) = delete;
    RawImageWidget(RawImageWidget&&) = delete;
    RawImageWidget& operator=(RawImageWidget&&) = delete;

    void setImage(core::ArrayView<const std::uint8_t, 2> image);
    void setImage(core::ArrayView<const std::uint8_t, 3> image);
    void setImage(core::ArrayView<const std::uint16_t, 2> image);
    void setImage(core::ArrayView<const std::uint16_t, 3> image);
    void setImage(core::ArrayView<const float, 2> image);
    void clearImage();

    void setAxisOrder(AxisOrder axisOrder);
    [[nodiscard]] AxisOrder axisOrder() const noexcept;

    void setLevels(std::optional<ImageLevelRange> levels);
    [[nodiscard]] std::optional<ImageLevelRange> levels() const noexcept;
    void setLookupTable(ImageLookupTable lut);
    void clearLookupTable();
    [[nodiscard]] std::optional<ImageLookupTable> lookupTable() const noexcept;

    void setScaled(bool scaled);
    [[nodiscard]] bool scaled() const noexcept;

    [[nodiscard]] bool hasImage() const noexcept;
    [[nodiscard]] std::size_t width() const noexcept;
    [[nodiscard]] std::size_t height() const noexcept;
    [[nodiscard]] const QImage& cachedImage() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    enum class DataKind {
        None,
        UInt8Rank2,
        UInt8Rank3,
        UInt16Rank2,
        UInt16Rank3,
        FloatRank2,
    };

    template <typename T, std::size_t Rank>
    void setImageImpl(core::ArrayView<const T, Rank> image, DataKind kind, std::vector<T>& destination);

    void invalidateCache();
    void ensureCachedImage() const;

    [[nodiscard]] std::optional<ImageLookupTable> lookupTableView() const noexcept;

    DataKind dataKind_ = DataKind::None;
    AxisOrder axisOrder_ = AxisOrder::ColMajor;
    std::array<std::size_t, 3> shape_{};
    std::vector<std::uint8_t> uint8Data_;
    std::vector<std::uint16_t> uint16Data_;
    std::vector<float> floatData_;
    std::optional<ImageLevelRange> levels_;
    std::vector<std::uint8_t> lookupTableData_;
    std::size_t lookupTableRows_ = 0;
    std::size_t lookupTableChannels_ = 0;
    bool scaled_ = false;
    mutable QImage cachedImage_;
    mutable bool cacheValid_ = false;
};

} // namespace cppqtgraph::widgets
