# PGPLOT-003 Implementation Report

## Scope
- Implemented minimal native C++ `PlotCurveItem::setData` behavior for data storage, y-only auto-x generation, data replacement, mismatch validation, and cached bounding rect updates.
- No painting/path/options/visual behavior was added; `paint()` remains a no-op.
- Non-finite NaN/Inf semantics were not asserted or implemented specially because they were outside verified issue scope.

## API and behavior
- Added `setData(std::span<const double> y)`; generates x values `0.0..n-1` and stores copies.
- Added `setData(std::span<const double> x, std::span<const double> y)`; validates equal lengths before mutation and throws `std::invalid_argument` on mismatch.
- Added const `xData()` / `yData()` accessors returning spans over internally owned vectors.
- `boundingRect()` now returns cached bounds from finite tested data; empty data returns a null `QRectF{}`.
- Successful mutations call `prepareGeometryChange()` when bounds change and `update()` after replacement.

## Tests added
- `tests/graphicsItems/test_PlotCurveItem_setData.cpp` covers:
  - y-only auto-x generation and bounds,
  - x/y data copying independent of caller mutation,
  - repeated replacement and bounds update,
  - empty data clearing data and bounds,
  - mismatched lengths throwing while preserving previous data/bounds.
- Registered as CTest `pyqtgraph_cpp.graphicsItems.PlotCurveItem.setData`.

## Validation
- `cmake --preset dev` — exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_plotcurveitem pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata -j2` — exit 0.
- `cd build/dev && ctest -R 'pyqtgraph_cpp\.graphicsItems\.PlotCurveItem' --output-on-failure` — exit 0.
- `scripts/gate focus PGPLOT-003` — exit 2; gate CLI does not accept an issue argument (`unrecognized arguments: PGPLOT-003`).
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0.

## Rework cleanup
- Removed the unowned `pgplot-003/scout.md` artifact from the submitted diff.
- Removed local scratch artifacts from `pgplot-003/` and kept the remaining diff limited to issue-owned files.

## Risks / notes
- Requested root `context.md` and `plan.md` were absent; implementation followed available scout/planning context and direct source inspection.
