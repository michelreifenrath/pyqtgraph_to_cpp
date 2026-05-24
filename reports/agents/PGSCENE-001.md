# PGSCENE-001 GraphicsScene Shell Implementation

## Summary
- Added a native Qt/C++ `pyqtgraph::GraphicsScene::GraphicsScene` shell derived from `QGraphicsScene`.
- Exposed click radius and move distance state, `getViewWidget()`, `prepareForPaint()`, a render wrapper that prepares before delegating to Qt, and add/remove wrappers that emit item notifications.
- Wired the new source and focused test target through CMake with AUTOMOC enabled for the library target.
- Visual validation is not applicable for this hierarchy/API shell issue.

## Design notes
- Source notes follow the existing translated/adapted PyQtGraph attribution pattern and the pinned PyQtGraph 0.14.0 commit used by dependency work.
- `addItem(QGraphicsItem*)`, `removeItem(QGraphicsItem*)`, and `render(QPainter*, ...)` intentionally hide the same-signature Qt methods rather than override them; Qt's methods are not virtual, so notifications/preparation are emitted when callers use the concrete `GraphicsScene` API.
- Mouse, drag, hover, and full event-behavior machinery are deferred to PGSCENE-002.
- `tests/hierarchy/test_cpp_hierarchy.cpp` was not edited because it was not listed in the hard-constraint owned files for this implementation task.

## Files changed
- `include/pyqtgraph/GraphicsScene/GraphicsScene.hpp`
- `src/pyqtgraph/GraphicsScene/GraphicsScene.cpp`
- `tests/graphicsItems/test_GraphicsScene.cpp`
- `CMakeLists.txt`
- `reports/agents/PGSCENE-001.md`

## Validation
- `cmake --preset dev` — exit 0; configured and generated `build/dev`.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsscene_graphicsscene pyqtgraph_cpp_hierarchy_cpp` — exit 0; built library, new GraphicsScene test, and hierarchy smoke target.
- `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.(GraphicsScene\.GraphicsScene|hierarchy\.cpp)'` — exit 0; 2/2 tests passed.
- Rework red check: `ctest --test-dir build/dev --output-on-failure -R '^pyqtgraph_cpp\.GraphicsScene\.GraphicsScene$'` failed before the render wrapper with `prepareCount == 1` not met.
- `scripts/gate focus PGSCENE-001` — exit 2; local gate CLI rejected the issue argument as an unrecognized argument.
- `scripts/gate focus` — exit 0; gate started and completed its configured focus checks (`python3 -m pytest -q`).
- Rework focused check: `cmake --build build/dev --target pyqtgraph_cpp_graphicsscene_graphicsscene` — exit 0.
- Rework focused check: `ctest --test-dir build/dev --output-on-failure -R '^pyqtgraph_cpp\.GraphicsScene\.GraphicsScene$'` — exit 0.
- `scripts/gate commit` — exit 0; gate completed configured commit checks including diff checks and pytest.
- `python3 -m pytest -q` — exit 0; 208 passed in 32.76s.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0; workflow valid.

## PR / branch note
- No commit, push, merge, branch change, or PR creation was performed.
