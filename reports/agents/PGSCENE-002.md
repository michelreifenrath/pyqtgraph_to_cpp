# PGSCENE-002 Implementation Report

## Summary
- Added native C++/Qt `MouseDragEvent`, `MouseClickEvent`, and `HoverEvent` skeletons under `pyqtgraph::GraphicsScene`.
- Implemented copied event state, item-relative coordinate mapping, acceptance/current-item tracking, dispatcher-settable hover enter/exit state, and lifetime-guarded click/drag claim maps.
- Reworked drag skeletons to support durable construction from `MouseClickEvent` plus prior `MouseDragEvent` state, and to track button-down positions per left/middle/right mouse button.
- Store `MouseClickEvent::time()` as monotonic `std::chrono::steady_clock` milliseconds captured at wrapper construction, not the raw Qt window-system event timestamp.
- Added focused unit coverage for API shape, stored accessors, monotonic click construction time, per-button drag start positions, durable drag continuation state, acceptance behavior, item mapping, and hover claim behavior.
- Wired `mouseEvents.hpp/.cpp` into the Qt Widgets library build and added the focused CTest executable `pyqtgraph_cpp_graphicsscene_mouseevents`.

## Files changed
- `include/pyqtgraph/GraphicsScene/mouseEvents.hpp`
- `src/pyqtgraph/GraphicsScene/mouseEvents.cpp`
- `tests/graphicsItems/test_mouseEvents.cpp`
- `CMakeLists.txt`
- `reports/agents/PGSCENE-002.md`

## Notes
- No Python wrappers or numeric oracle fixtures were added.
- Hover claim maps keep the public `QGraphicsItem*` API but store a per-item lifetime token internally, so claims for deleted items are filtered out before `clickItems()`/`dragItems()` expose them.
- `HoverEvent::setExit()` mirrors `setEnter()` so future dispatch can mark exit state while preserving move positions/buttons/modifiers.
- `MouseDragEvent` keeps the original Qt-pointer constructor as a snapshot convenience, but future dispatch can use the durable wrapper-state constructor to avoid stale press/last Qt event pointers.
- `MouseClickEvent::time()` intentionally uses the same monotonic clock family future scene drag-gate code should subtract from; the arbitrary epoch must not be compared to wall time or Qt event timestamps.
- `MouseClickEvent::lastPos()` intentionally mirrors current `pos()` because upstream exposes `lastPos()` without a separately initialized last scene position in the constructor.

## Validation
- `git diff --check` — exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsscene_mouseevents` — exit 0.
- `ctest --test-dir build/dev --output-on-failure -R '^pyqtgraph_cpp\.GraphicsScene\.mouseEvents$'` — exit 0, 1/1 test passed.
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0.
- `python3 -m pytest -q` — exit 0, 221 passed in 36.37s.
- Rework subagent reviewer — scoped advisory pass, confirmed the monotonic steady-clock fix/test shape for the prior autoreview finding.

## Handoff risks
- Hover enter/exit default state remains constructor-compatible and is dispatcher-settable so future per-item scene dispatch can mark both states without position loss.
- Hover claim maps now drop deleted-item claims before exposing them; future full dispatch can consume `clickItems()`/`dragItems()` without retaining stale hover claims.
- No PR was opened from this Pi handoff because the workflow/user instructions forbid committing, pushing, or merging; release automation can commit/push/open the PR after review.
