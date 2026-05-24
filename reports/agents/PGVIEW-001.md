# PGVIEW-001 Implementation Report

## Summary
- Added the minimal `pyqtgraph::graphicsItems::ViewBox` skeleton derived from `GraphicsWidget`.
- Exposed upstream mode/axis constants: `PanMode`, `RectMode`, `XAxis`, `YAxis`, and `XYAxes`.
- Added focused ViewBox construction, hierarchy, constants, parent, and inherited view-discovery coverage.
- Extended the central C++ hierarchy smoke test to include ViewBox API shape checks.
- Wired the ViewBox source and focused test target into CMake.

## Design notes
- `ViewBox` construction delegates directly to `GraphicsWidget(parent, flags)` so inherited `GraphicsItem` host binding remains `static_cast<QGraphicsItem*>(this)`.
- Copy and move operations are deleted, matching the existing Qt graphics-item shells.
- This is intentionally a skeleton only: no range, mouse, menu, child group, auto-range, paint, transform, or rendering behavior was implemented.
- `Q_OBJECT` and upstream signals are intentionally deferred until C++ range/state signal payload types are defined.
- No visual artifacts are applicable because this is a non-pixel API/hierarchy skeleton.
- No PR was opened by Pi.

## Changed files
- `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp`
- `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp`
- `tests/graphicsItems/test_ViewBox.cpp`
- `tests/hierarchy/test_cpp_hierarchy.cpp`
- `CMakeLists.txt`
- `reports/agents/PGVIEW-001.md`

## Validation
- `cmake --preset dev` — passed.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_viewbox pyqtgraph_cpp_hierarchy_cpp` — passed.
- `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.ViewBox|hierarchy\.cpp)'` — passed: 2/2 tests passed.
- `scripts/gate focus PGVIEW-001` — failed as unsupported CLI usage: `unrecognized arguments: PGVIEW-001`.
- `scripts/gate focus` — passed.
- `scripts/gate commit` — passed.
- `python3 -m pytest -q` — passed: 221 tests passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — passed: workflow valid.

## Open risks
- Full upstream ViewBox behavior remains out of scope for this skeleton and will need later scoped issues.
