# PGVIEW-002 Implementation Report

## Summary
Implemented the scoped `ViewBox` numeric range model for default ranges, range/rect accessors, range setters, finite-input validation, padding, update=false target/view separation, and typed range limits.

Rework hardened finite large-value range requests so zero-span/collapsed ranges cannot store infinities or unusable zero-width target/view ranges, and optional-axis `setRange` now rejects calls with no supplied axes before mutating state.

No visual artifacts are applicable: this issue changes non-pixel numeric state only and does not add rendering, transforms, or interaction behavior.

## API/data model
- Added `ViewBox::AxisRange` and `ViewBox::Range2D` typed range aliases.
- Added `ViewBox::Limits` with optional x/y hard bounds and min/max span constraints.
- Added accessors: `viewRange()`, `targetRange()`, `viewRect()`, `targetRect()`, and `limits()`.
- Added mutators: `setRange(QRectF, ...)`, optional-axis `setRange(...)`, `setXRange(...)`, `setYRange(...)`, and `setLimits(...)`.
- Added internal `targetRange_`, `viewRange_`, `limits_`, and `autoRange_` state.

## Design notes
- Range accessors return by-value copies.
- Rect accessors map `[xMin, xMax]`/`[yMin, yMax]` to `QRectF(xMin, yMin, width, height)`.
- Reversed endpoints are normalized.
- Zero-span requests expand around a stable center using the previous target span when representable, then a quantization-sized fallback for large offsets where the previous span collapses.
- Derived non-finite endpoints/padding expansions and no-axis optional `setRange` calls throw `std::invalid_argument` before mutating state.
- Non-finite range endpoints, non-finite padding, non-finite limits, inverted hard bounds, negative span limits, and min-span greater than max-span throw `std::invalid_argument` before mutating state.
- Limit clamping applies max span, min span, then hard-bound shifting while preserving span when possible. If a span is wider than both hard bounds, the bounded interval is used deterministically.
- `update=false` updates `targetRange_` only; `viewRange_` is left unchanged.
- `disableAutoRange=true` updates only internal axis flags for supplied axes; no public auto-range API was added.

## Out-of-scope items
- No pan/zoom/mouse interaction.
- No linked views, aspect locking, transforms, signals, child-item auto-ranging, or paint hooks.
- No numeric oracle fixture generation was needed.

## Changed files
- `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp`
- `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp`
- `tests/graphicsItems/test_ViewBox_range.cpp`
- `CMakeLists.txt`
- `reports/agents/PGVIEW-002.md`

## Validation
- `cmake --preset dev` — exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_viewbox pyqtgraph_cpp_graphicsitems_viewbox_range` — exit 0.
- `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.graphicsItems\.ViewBox'` — exit 0; 2/2 tests passed.
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0; 225 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0.
- `git diff --check` — exit 0.

## Risks/Open questions
- The C++ optional-axis overload is a scoped substitute for Python keyword-style `setRange`; future API expansion may adjust call ergonomics.
- Padding is deterministic numeric expansion and does not attempt upstream `suggestPadding` viewport heuristics.
