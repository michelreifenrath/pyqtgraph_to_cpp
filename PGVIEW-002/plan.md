# Implementation Plan

## Goal
Implement the PGVIEW-002 `ViewBox` numeric range model, with focused tests, CMake registration, and an implementation report, without expanding into pan/zoom interaction, rendering, signals, or auto-ranging over child items.

## Tasks
1. **Add failing range-model tests first**: Create focused tests for the new `ViewBox` range API using the existing no-framework `CHECK` style and offscreen `QApplication` guard.
   - File: `tests/graphicsItems/test_ViewBox_range.cpp`
   - Changes: Add tests for:
     - default `viewRange()` and `targetRange()` are `{{0, 1}, {0, 1}}`;
     - returned ranges are copies by mutating a local copy and re-reading the `ViewBox` state;
     - `viewRect()` and `targetRect()` initially equal `QRectF(0, 0, 1, 1)`;
     - `setXRange(2, 4, 0.0)` updates only X to `[2, 4]` and leaves Y `[0, 1]`;
     - `setYRange(-3, 7, 0.0)` updates only Y to `[-3, 7]`;
     - `setRange(QRectF(-1, 2, 4, 6), 0.0)` maps to X `[-1, 3]`, Y `[2, 8]`, and rect accessors return the same geometry;
     - reversed inputs normalize: `setXRange(5, 1, 0.0)` becomes `[1, 5]`;
     - zero span preserves the previous span around the requested center: from default state, `setXRange(5, 5, 0.0)` becomes `[4.5, 5.5]`;
     - non-finite input such as `setXRange(0, infinity, 0.0)` and `setYRange(NaN, 1, 0.0)` throws `std::invalid_argument` and leaves prior state unchanged;
     - limits clamp panning bounds: after `setLimits({.xMin = 0.0, .xMax = 10.0})`, `setXRange(-2, 3, 0.0)` becomes `[0, 5]` and `setXRange(8, 12, 0.0)` becomes `[6, 10]`;
     - limits clamp span: after `setLimits({.minXRange = 4.0})`, `setXRange(1, 2, 0.0)` becomes `[-0.5, 3.5]`; after `setLimits({.maxYRange = 4.0})`, `setYRange(-10, 10, 0.0)` becomes `[-2, 2]`.
   - Acceptance: The test compiles against the planned API and fails before production implementation because methods/types do not exist or behavior is missing.

2. **Register the new focused test target**: Add the range test executable and CTest entry beside the existing ViewBox test inside the `_pyqtgraph_cpp_has_graphicsitem` block.
   - File: `CMakeLists.txt`
   - Changes: Add executable `pyqtgraph_cpp_graphicsitems_viewbox_range` from `tests/graphicsItems/test_ViewBox_range.cpp`, link it to `pyqtgraph_cpp`, `pyqtgraph_cpp_project_options`, and `pyqtgraph_cpp_project_warnings`, call `pyqtgraph_cpp_enable_sanitizers(...)`, and add CTest name `pyqtgraph_cpp.graphicsItems.ViewBox.range`.
   - Acceptance: `cmake --preset dev` generates a build containing the new target and `ctest -N -R 'pyqtgraph_cpp\.graphicsItems\.ViewBox'` lists both ViewBox tests.

3. **Define the public range API and data model**: Extend the `ViewBox` header with small typed range/limits structures and range methods.
   - File: `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp`
   - Changes:
     - Include `QtCore/QRectF`, `<array>`, and `<optional>`.
     - Add public aliases `using AxisRange = std::array<qreal, 2>;` and `using Range2D = std::array<AxisRange, 2>;`.
     - Add public `struct Limits` with optional fields: `xMin`, `xMax`, `yMin`, `yMax`, `minXRange`, `maxXRange`, `minYRange`, `maxYRange`.
     - Add accessors: `Range2D viewRange() const;`, `Range2D targetRange() const;`, `QRectF viewRect() const;`, `QRectF targetRect() const;`, and `Limits limits() const;`.
     - Add mutators: `void setRange(const QRectF& rect, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);`, `void setRange(std::optional<AxisRange> xRange, std::optional<AxisRange> yRange, qreal padding = 0.02, bool update = true, bool disableAutoRange = true);`, `void setXRange(qreal min, qreal max, qreal padding = 0.02, bool update = true);`, `void setYRange(qreal min, qreal max, qreal padding = 0.02, bool update = true);`, and `void setLimits(const Limits& limits);`.
     - Add private members for upstream-aligned state: `Range2D targetRange_{{AxisRange{0.0, 1.0}, AxisRange{0.0, 1.0}}};`, `Range2D viewRange_` initialized the same, `Limits limits_`, and `std::array<bool, 2> autoRange_{{true, true}}`.
     - Do not add `Q_OBJECT`, signals, child groups, transform state, linked-view state, mouse state, or paint hooks in this issue.
   - Acceptance: Existing `test_ViewBox.cpp` still compiles; the new range test compiles once implementation is added.

4. **Implement range accessors and mutators**: Add numeric range behavior matching the scoped upstream-derived semantics.
   - File: `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp`
   - Changes:
     - Include `<algorithm>`, `<array>`, `<cmath>`, `<limits>`, `<optional>`, and `<stdexcept>` as needed.
     - Implement range accessors as by-value copies and rect accessors as `QRectF(xMin, yMin, xMax - xMin, yMax - yMin)`.
     - Implement `setXRange`/`setYRange` by delegating to optional-axis `setRange`.
     - Implement `setRange(QRectF, ...)` by converting rect left/right and top/bottom to axis ranges and delegating.
     - For each supplied axis range: reject non-finite endpoints or padding with `std::invalid_argument`; normalize reversed endpoints; if span is zero, expand around the requested center using the previous `targetRange_` span for that axis; apply padding by expanding both sides by `span * padding`; then clamp to the effective limits.
     - Implement limit clamping per axis: apply `max*Range` by reducing span around center, apply `min*Range` by expanding span around center, then shift the range inside `xMin/xMax` or `yMin/yMax` while preserving span where possible; if span is wider than bounded limits, center/fill the bounded interval deterministically rather than producing inverted ranges.
     - On validation failure, throw before modifying any member state so previous state remains unchanged.
     - When `disableAutoRange` is true, set `autoRange_[axis] = false` for each supplied axis; keep this internal until a later issue owns public auto-range API.
     - If `update` is true, copy `targetRange_` to `viewRange_`; if `update` is false, update only `targetRange_` and leave `viewRange_` unchanged.
   - Acceptance: `pyqtgraph_cpp_graphicsitems_viewbox_range` passes and existing ViewBox smoke tests still pass.

5. **Validate `setLimits` inputs and storage**: Implement typed limits with deterministic validation.
   - File: `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp`
   - Changes: In `setLimits`, reject non-finite optional values, reject `xMin > xMax` / `yMin > yMax`, reject negative min/max range spans, and reject `minXRange > maxXRange` / `minYRange > maxYRange` when both are set. Store `limits_`, then re-clamp current `targetRange_` and `viewRange_` through the same limit helper so existing ranges obey new limits.
   - Acceptance: Add or include test assertions that invalid limits throw and valid limits affect later range changes; all state remains valid after applying limits.

6. **Do not add numeric oracle fixtures unless explicitly required**: Keep upstream-derived expected values in the focused C++ test because PGVIEW-002 can be validated deterministically without generated fixtures.
   - File: `oracle/scripts/generate_numeric_oracles.py`
   - Changes: None planned. Only edit this file if the issue owner explicitly requires generated ViewBox oracle fixtures; otherwise editing it would expand scope.
   - Acceptance: No oracle script changes are needed for the focused tests and validation commands below.

7. **Write the implementation report**: Document what was implemented, what stayed out of scope, files changed, and validation results.
   - File: `reports/agents/PGVIEW-002.md`
   - Changes: Add sections for Summary, API/data model, Design notes, Out-of-scope items, Changed files, Validation, and Risks/Open questions. Explicitly mention no visual artifacts are applicable because this is non-pixel numeric state.
   - Acceptance: Report accurately lists the new API, CMake test target, all validation commands run, and any failures/limitations.

8. **Run focused and configured validation**: Verify both new and existing ViewBox coverage plus repository gates.
   - File: N/A
   - Changes: Run these commands after implementation:
     - `cmake --preset dev`
     - `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_viewbox pyqtgraph_cpp_graphicsitems_viewbox_range`
     - `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.graphicsItems\.ViewBox'`
     - `scripts/gate focus`
     - `scripts/gate commit`
     - `python3 -m pytest -q`
   - Acceptance: All commands pass, or any failures are documented in `reports/agents/PGVIEW-002.md` with the exact failing command and reason.

## Files to Modify
- `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp` - add typed range/limits API and private numeric state.
- `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp` - implement range accessors, setRange/setXRange/setYRange, limit validation, and clamping helpers.
- `CMakeLists.txt` - register the new ViewBox range test executable and CTest entry inside the graphics-item test block.
- `reports/agents/PGVIEW-002.md` - implementation report with design notes and validation results.

## New Files
- `tests/graphicsItems/test_ViewBox_range.cpp` - focused TDD coverage for default range state, range setters, rect accessors, reversed/zero/non-finite inputs, and limits.

## Dependencies
- Task 1 should happen before production edits to preserve the TDD workflow.
- Task 2 is required before building/running the new test through CTest.
- Task 3 must precede Tasks 4 and 5 because implementation and tests depend on the declared API.
- Task 4 depends on Task 3 and should be completed before full validation.
- Task 5 depends on Task 4's shared clamping helpers.
- Task 7 should be completed after implementation and validation so it can record actual results.
- Task 8 depends on Tasks 1-5 and should be run before final handoff.

## Risks
- The C++ API for Python-style `ViewBox.setRange` keyword arguments is not predetermined; the plan uses typed overloads and `std::optional` instead of emulating Python kwargs. Changing this API later would require updating tests.
- Exact upstream padding heuristics (`suggestPadding`) are broader than the issue needs; deterministic tests should pass `padding = 0.0` except for simple validation of finite padding.
- Limit-clamping behavior has edge cases when requested spans exceed hard bounds; keep behavior deterministic and documented rather than expanding into full upstream transform/link logic.
- `update = false` semantics should remain numeric only: update `targetRange_` but not `viewRange_`; do not introduce rendering/transform updates in PGVIEW-002.
- Auto-range is represented internally only to support `disableAutoRange`; public auto-range/item-based behavior, linked views, mouse pan/zoom, aspect locking, transforms, signals, and visual validation are out of scope for this issue.
