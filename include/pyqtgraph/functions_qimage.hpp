#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py and
// pyqtgraph/functions_qimage.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "core/ArrayView.hpp"
#include "functions.hpp"

#if __has_include(<QImage>)
#include <QImage>
#define PYQTGRAPH_CPP_HAS_QIMAGE_HEADER 1
#else
class QImage;
#define PYQTGRAPH_CPP_HAS_QIMAGE_HEADER 0
#endif

#include <cstdint>
#include <optional>

namespace pyqtgraph {

struct MakeQImageOptions {
    bool transpose = true;
    // When false, return a non-owning QImage only for compatible contiguous storage;
    // callers must keep the source storage alive, and unsupported layouts throw.
    bool copy = true;
    std::optional<bool> alpha = std::nullopt;
};

struct TryMakeQImageOptions {
    std::optional<ImageLevelRange> levels = std::nullopt;
    std::optional<ImageLookupTable> lut = std::nullopt;
};

[[nodiscard]] QImage makeQImage(core::ArrayView<const std::uint8_t, 2> imageData,
                                const MakeQImageOptions& options = {});
[[nodiscard]] QImage makeQImage(core::ArrayView<const std::uint8_t, 3> imageData,
                                const MakeQImageOptions& options = {});

#if PYQTGRAPH_CPP_HAS_QIMAGE_HEADER
// Upstream try_make_qimage pass-through (no levels/LUT); unsupported -> nullopt, else contiguous copy.
// Hidden when QImage is unavailable so no-Qt header consumers never instantiate std::optional<QImage>.
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 2> imageData);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 2> imageData,
                                                  const TryMakeQImageOptions& options);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 3> imageData);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint8_t, 3> imageData,
                                                  const TryMakeQImageOptions& options);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 2> imageData);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 2> imageData,
                                                  const TryMakeQImageOptions& options);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const std::uint16_t, 3> imageData);
[[nodiscard]] std::optional<QImage> tryMakeQImage(core::ArrayView<const float, 2> imageData,
                                                  const TryMakeQImageOptions& options = {});
#endif

} // namespace pyqtgraph
