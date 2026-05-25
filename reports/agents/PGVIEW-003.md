# PGVIEW-003 Implementation Report

## Summary
Implemented scoped deterministic `ViewBox` pan/zoom range math through public numeric `scaleBy` and `translateBy` APIs.

The implementation uses `targetRect()` as the source of truth, validates supplied scale factors, centers, and translations for finite values before mutating, and delegates all range updates to the existing `setRange`/axis setters with `padding=0.0`. This preserves existing target/view updates, normalization, and limit clamping behavior.

No visual artifacts are applicable: this issue changes non-pixel numeric state only and does not add rendering, mouse, wheel, or graphics-transform behavior.

## API/data model
- Added `ViewBox::scaleBy(std::optional<qreal> x, std::optional<qreal> y, std::optional<QPointF> center = std::nullopt)`.
- Added `ViewBox::scaleBy(const QPointF& scale, std::optional<QPointF> center = std::nullopt)`.
- Added `ViewBox::translateBy(std::optional<qreal> x, std::optional<qreal> y)`.
- Added `ViewBox::translateBy(const QPointF& offset)`.
- No new persistent state was added; pan/zoom methods mutate the existing `targetRange_`/`viewRange_` through existing range APIs.

## Design notes
- `scaleBy` defaults omitted axes to scale factor `1.0` for math, but only supplied axes are applied to the range update.
- A missing scale center defaults to `targetRect().center()`.
- Scale math follows upstream shape: `center + (edge - center) * scale` for top-left and bottom-right corners.
- `translateBy(QPointF)` shifts both axes via `targetRect().translated(offset)`.
- Optional-axis `translateBy` shifts only supplied axes.
- Optional-axis calls with no supplied axes are no-ops, matching upstream `scaleBy`/`translateBy` behavior for empty axis requests.
- Non-finite scale factors, centers, and translations throw `std::invalid_argument` before mutation.
- Limit clamping is inherited by delegating to `setRange`/`setXRange`/`setYRange`.

## Test/fixture details
- Added `oracle/fixtures/interactions/ViewBox_basic_pan_zoom.json` as a deterministic C++ numeric fixture with `version: 1`, compatibility `steps: []`, cases, operations, and expected final ranges.
- Added `tests/graphicsItems/test_ViewBox_pan_zoom.cpp` covering fixture loading, default-center scaling, explicit-center optional-axis scaling, point and optional-axis translation, target/view synchronization, limit clamping, invalid-input state preservation, and no-axis no-op behavior.
- Registered the new test target and CTest name in `CMakeLists.txt` with `PYQTGRAPH_CPP_INTERACTION_FIXTURE_DIR` pointing at `oracle/fixtures/interactions`.

## Out-of-scope items
- No mouse drag handling.
- No wheel event handling.
- No aspect locking.
- No linked views.
- No graphics transforms or rendering changes.
- No interaction-runner support for numeric range assertions.
- No numeric-oracle generator changes; the new interaction fixture is static and consumed by the C++ test.

## Changed files
- `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp`
- `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp`
- `tests/graphicsItems/test_ViewBox_pan_zoom.cpp`
- `oracle/fixtures/interactions/ViewBox_basic_pan_zoom.json`
- `CMakeLists.txt`
- `reports/agents/PGVIEW-003.md`

## Validation
- `cmake --preset dev` — exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_viewbox pyqtgraph_cpp_graphicsitems_viewbox_range pyqtgraph_cpp_graphicsitems_viewbox_pan_zoom` — exit 0.
- `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.graphicsItems\.ViewBox'` — exit 0; 3/3 tests passed.
- `scripts/gate focus PGVIEW-003` — exit 2; current gate rejects target arguments for focus mode (`focus mode does not accept an example name`).
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0; 234 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0; workflow valid.
- `git diff --check` — exit 0.

## Risks/Open questions
- The issue validation target names `scripts/gate focus PGVIEW-003`, but the current `scripts/gate` focus mode rejects target arguments. Validation used `scripts/gate focus` per the planner direction.
- Pinned upstream source checkout is absent in this worktree; upstream math was aligned to available PyQtGraph 0.14.0 behavior documented in the task context.

## Handoff
- No PR was opened from this Pi handoff because the workflow/user instructions forbid committing, pushing, or merging; release automation can commit, push, and open/update the PR after review.
