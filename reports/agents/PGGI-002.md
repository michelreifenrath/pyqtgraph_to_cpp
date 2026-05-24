# PGGI-002 GraphicsObject Implementation Report

## Summary
- Added `pyqtgraph::graphicsItems::GraphicsObject` as a Qt `QGraphicsObject` plus `GraphicsItem` bridge.
- Bound the inherited `GraphicsItem` host pointer to the concrete graphics object and enabled `ItemSendsGeometryChanges`.
- Added an `itemChange()` override that clears the inherited view-widget cache after parent or scene changes, matching the upstream cache-invalidation path.
- Wired the GraphicsObject source and focused tests into the Qt Widgets guarded build path.

## Rework notes
- Removed the out-of-scope `tester/validation.md` artifact from the branch.
- Added tests that move a cached object across scenes and reparent a cached child to a detached parent without calling `forgetViewWidget()` explicitly.

## Changed files
- `include/pyqtgraph/graphicsItems/GraphicsObject.hpp`
- `src/pyqtgraph/graphicsItems/GraphicsObject.cpp`
- `tests/graphicsItems/test_GraphicsObject.cpp`
- `tests/hierarchy/test_cpp_hierarchy.cpp`
- `CMakeLists.txt`
- `reports/agents/PGGI-002.md`

## Validation
- `cmake --build build --target pyqtgraph_cpp_graphicsitems_graphicsobject pyqtgraph_cpp_hierarchy_cpp` — exit 0.
- `ctest --test-dir build --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.GraphicsObject|hierarchy\.cpp)'` — exit 0; 2/2 tests passed.
- `scripts/gate focus PGGI-002` — exit 2; local gate CLI rejected the issue argument.
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0; 204 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0; workflow valid.

## Limitations
- Only the PGGI-002 GraphicsObject base and inherited view-widget cache coherence are implemented.
- Full PyQtGraph GraphicsObject behavior that depends on future graphics/view infrastructure remains out of scope.
