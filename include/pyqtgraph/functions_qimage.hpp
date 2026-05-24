#pragma once

// Source note: translated/adapted from PyQtGraph pyqtgraph/functions.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#include "core/ArrayView.hpp"

#if __has_include(<QImage>)
#include <QImage>
#else
class QImage;
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

[[nodiscard]] QImage makeQImage(core::ArrayView<const std::uint8_t, 2> imageData,
                                const MakeQImageOptions& options = {});
[[nodiscard]] QImage makeQImage(core::ArrayView<const std::uint8_t, 3> imageData,
                                const MakeQImageOptions& options = {});

} // namespace pyqtgraph
