# PGPLOT-007 PlotWidget skeleton

## Summary
- Added `pyqtgraph::widgets::PlotWidget` as a `QGraphicsView` skeleton.
- The widget owns a `QGraphicsScene`, creates a scene-owned `graphicsItems::PlotItem`, and exposes stable mutable/const `getPlotItem()` accessors.
- Scope is intentionally limited to the skeleton API; plotting convenience forwarding is not implemented.

## Modified files
- `include/pyqtgraph/widgets/PlotWidget.hpp`
- `src/pyqtgraph/widgets/PlotWidget.cpp`
- `tests/widgets/test_PlotWidget.cpp`
- `tests/hierarchy/test_cpp_hierarchy.cpp`
- `CMakeLists.txt`
- `reports/agents/PGPLOT-007.md`

## Tests and validation
- `cmake --preset dev` — passed.
- `cmake --build --preset dev --target pyqtgraph_cpp_widgets_plotwidget pyqtgraph_cpp_hierarchy_cpp` — passed.
- `ctest --preset dev -R 'pyqtgraph_cpp.widgets.PlotWidget|pyqtgraph_cpp.hierarchy.cpp' --output-on-failure` — passed.
- `scripts/gate focus PGPLOT-007` — failed: this gate CLI does not accept an issue-id argument.
- `scripts/gate focus` — passed.
- `python3 -m pytest -q` — passed, 225 tests.
- `scripts/gate commit` — passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — passed.

## Notes
- No numeric oracle fixtures were needed.
- No visual validation artifacts were needed because this skeleton does not add pixel-affecting rendering behavior beyond Qt scene/widget ownership.
