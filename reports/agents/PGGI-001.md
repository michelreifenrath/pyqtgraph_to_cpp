# PGGI-001 GraphicsItem Implementation Report

## Summary
- Added `pyqtgraph::graphicsItems::GraphicsItem` as a narrow C++ helper/base for Qt `QGraphicsItem` hosts.
- Implemented upstream-aligned `getViewWidget()` / `forgetViewWidget()` behavior using a `QPointer<QGraphicsView>` cache in the implementation so destroyed views do not leave dangling pointers.
- Wired the source and focused tests behind Qt 6 Widgets availability in `CMakeLists.txt`.

## Design notes
- `GraphicsItem` is intentionally not a `QObject` or `QGraphicsItem` subclass; it is bound to an actual Qt graphics item host supplied at construction or via `setGraphicsItem()`.
- The public header keeps the Qt Widgets cache implementation private through a small pimpl, while public API still uses Qt graphics item/view pointer types.
- Deferred upstream methods that depend on future ported classes, including ViewBox, pixel-vector, transform, export, SVG, and context-menu helpers.

## Changed files
- `include/pyqtgraph/graphicsItems/GraphicsItem.hpp`
- `src/pyqtgraph/graphicsItems/GraphicsItem.cpp`
- `tests/graphicsItems/test_GraphicsItem.cpp`
- `tests/hierarchy/test_cpp_hierarchy.cpp`
- `CMakeLists.txt`
- `reports/agents/PGGI-001.md`

## Validation
- `cmake -S . -B build -DPYQTGRAPH_CPP_BUILD_TESTS=ON` — exit 0; CMake warned that `PYQTGRAPH_CPP_BUILD_TESTS` is unused by this project.
- `cmake --build build --target pyqtgraph_cpp_hierarchy_cpp pyqtgraph_cpp_graphicsitems_graphicsitem` — exit 0.
- `ctest --test-dir build --output-on-failure -R 'pyqtgraph_cpp\.(hierarchy\.cpp|graphicsItems\.GraphicsItem)'` — exit 0; 2/2 focused tests passed.
- `python3 -m pytest -q` — first run exit 1 because new source/header attribution notes were missing; fixed attribution notes.
- `python3 -m pytest -q` — exit 0; 204 passed.
- `scripts/gate focus PGGI-001` — exit 2; repository gate CLI rejected the issue argument as unrecognized.
- `scripts/gate focus` — exit 0; focused gate passed with the supported local invocation.
- `scripts/gate commit` — exit 0.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0; workflow valid.

## Limitations
- Only the PGGI-001 view-widget cache surface is implemented.
- Full PyQtGraph `GraphicsItem` behavior that requires additional graphics/viewbox infrastructure remains intentionally out of scope.
