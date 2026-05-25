# PGPLOT-002 PlotCurveItem Implementation Report

## Scope
- Added native C++ `pyqtgraph::graphicsItems::PlotCurveItem` skeleton derived from `GraphicsObject`.
- Added focused standalone C++ tests and hierarchy coverage.
- Wired the new source and test target in the root CMake graph.
- No Python wrapper work, no `PlotDataItem`/`PlotItem`, no rendering/data behavior, and no numeric oracle fixtures were added.

## Upstream reference
- Translated/adapted from PyQtGraph `pyqtgraph/graphicsItems/PlotCurveItem.py`.
- PyQtGraph ref: `pyqtgraph-0.14.0`.
- Pinned commit: `a20028b98294b9cc8770f2015a92eb342224b788`.

## Behavior and API choices
- `PlotCurveItem` is a concrete, non-copyable, non-movable `GraphicsObject` subclass.
- The constructor delegates to `GraphicsObject(parent)` and preserves inherited graphics-item binding/view lookup behavior.
- `boundingRect()` returns an empty `QRectF{}` for skeleton scope.
- `paint(...)` is intentionally a no-op smoke-safe skeleton and produces no pixels.

## Files changed
- `include/pyqtgraph/graphicsItems/PlotCurveItem.hpp`
- `src/pyqtgraph/graphicsItems/PlotCurveItem.cpp`
- `tests/graphicsItems/test_PlotCurveItem.cpp`
- `tests/hierarchy/test_cpp_hierarchy.cpp`
- `CMakeLists.txt`
- `reports/agents/PGPLOT-002.md`

## Validation
- `cmake --preset dev` — exit 0; configured `build/dev`.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_plotcurveitem pyqtgraph_cpp_hierarchy_cpp -j2` — exit 0.
- `cd build/dev && ctest -R 'pyqtgraph_cpp\\.(graphicsItems\\.PlotCurveItem|hierarchy\\.cpp)' --output-on-failure` — exit 0; 2/2 tests passed.
- `cd build/dev && ctest -R 'pyqtgraph_cpp\\.graphicsItems\\.(GraphicsObject|AxisItem|PlotCurveItem)' --output-on-failure` — exit 8 initially because adjacent `GraphicsObject` and `AxisItem` executables had not been built in the fresh build tree.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_graphicsobject pyqtgraph_cpp_graphicsitems_axisitem -j2` — exit 0.
- `cd build/dev && ctest -R 'pyqtgraph_cpp\\.graphicsItems\\.(GraphicsObject|AxisItem|PlotCurveItem)' --output-on-failure` — exit 0; 3/3 tests passed.
- `git diff --check` — exit 0.
- `scripts/gate focus PGPLOT-002` — exit 2; current gate CLI rejects the issue argument as unrecognized.
- `scripts/gate focus` — exit 0; compatibility focused gate passed.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0; 221 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0.

## Remaining issues / notes
- Visual validation was not run because this skeleton intentionally has a no-op `paint()` and no pixel-affecting behavior.
- No PR was opened from this Pi handoff because the repository workflow reserves commits, pushes, and PR creation for automation after validation.
