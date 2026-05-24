# PGSCENE-002 Implementation Report

## Summary
- Added native C++/Qt `MouseDragEvent`, `MouseClickEvent`, and `HoverEvent` skeletons under `pyqtgraph::GraphicsScene`.
- Implemented copied event state, item-relative coordinate mapping, acceptance/current-item tracking, hover enter/exit detection, and click/drag claim maps.
- Added focused unit coverage for API shape, stored accessors, acceptance behavior, item mapping, and hover claim behavior.
- Wired `mouseEvents.hpp/.cpp` into the Qt Widgets library build and added the focused CTest executable `pyqtgraph_cpp_graphicsscene_mouseevents`.

## Files changed
- `include/pyqtgraph/GraphicsScene/mouseEvents.hpp`
- `src/pyqtgraph/GraphicsScene/mouseEvents.cpp`
- `tests/graphicsItems/test_mouseEvents.cpp`
- `CMakeLists.txt`
- `reports/agents/PGSCENE-002.md`

## Notes
- No Python wrappers or numeric oracle fixtures were added.
- The approved plan requested `QPointer<QGraphicsItem>` claim maps, but `QPointer` only supports QObject-derived types and plain `QGraphicsItem` is not QObject-derived. I attempted to escalate this decision, but supervisor routing was ambiguous/unavailable. To keep the generic `QGraphicsItem` API compiling, hover claim maps use raw `QGraphicsItem*`, consistent with `currentItem()` and `acceptedItem()`.
- `MouseClickEvent::lastPos()` intentionally mirrors current `pos()` because upstream exposes `lastPos()` without a separately initialized last scene position in the constructor.

## Validation
- `cmake --preset dev` — exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsscene_mouseevents` — exit 0.
- `ctest --test-dir build/dev --output-on-failure -R '^pyqtgraph_cpp\.GraphicsScene\.mouseEvents$'` — exit 0, 1/1 test passed.
- `git diff --check` — exit 0.
- `scripts/gate focus PGSCENE-002` — exit 2; gate does not accept an issue argument.
- `scripts/gate focus` — exit 0 as the available equivalent focused gate.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0, 221 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0.

## Handoff risks
- Hover enter semantics are intentionally minimal because full scene dispatch is out of scope.
- Claim maps hold raw item pointers; future full dispatch may need a lifetime-aware owner strategy for `QGraphicsObject` versus plain `QGraphicsItem`.
- No PR was opened from this Pi handoff because the workflow/user instructions forbid committing, pushing, or merging; release automation can commit/push/open the PR after review.
