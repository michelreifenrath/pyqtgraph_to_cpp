# Code Context

## Files Retrieved
1. `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp` (lines 1-28) - current public ViewBox skeleton; only constants, constructor/destructor, deleted copy/move.
2. `src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp` (lines 1-16) - constructor delegates to `GraphicsWidget`; no state/range behavior yet.
3. `tests/graphicsItems/test_ViewBox.cpp` (lines 1-112) - existing PGVIEW-001 smoke tests and minimal no-framework test harness.
4. `CMakeLists.txt` (lines 111-128, 286-313) - library source registration and existing ViewBox test target pattern.
5. `include/pyqtgraph/graphicsItems/GraphicsWidget.hpp` (lines 1-34) and `src/pyqtgraph/graphicsItems/GraphicsWidget.cpp` (lines 1-52) - dependency from #35/#PGVIEW-001: ViewBox base APIs for geometry, width/height, itemChange, and inherited view discovery.
6. `include/pyqtgraph/graphicsItems/GraphicsItem.hpp` (lines 1-33) - inherited `graphicsItem()`, `getViewWidget()`, `forgetViewWidget()` API.
7. `reports/agents/PGVIEW-001.md` (lines 1-38) - prior implementation assumptions and explicit deferred scope.
8. `oracle/scripts/generate_numeric_oracles.py` (lines 1-120 searched; no ViewBox/range fixtures currently) - only relevant if issue requires adding numeric oracle fixtures.
9. `WORKFLOW.md` (lines 84-90, 124-128) and `CMakePresets.json` (lines 1-118) - configured validation and dev preset.
10. Upstream pinned PyQtGraph source, fetched from `https://raw.githubusercontent.com/pyqtgraph/pyqtgraph/a20028b98294b9cc8770f2015a92eb342224b788/pyqtgraph/graphicsItems/ViewBox/ViewBox.py` (local checkout path from `reference/PYQTGRAPH_REF`, but checkout is absent) - relevant upstream lines: class constants/state 74-179, accessors 483-498, range-setting 540-704, limits 741-787, updateViewRange 1575-1704.

## Key Code

Current C++ public API is only a skeleton:

```cpp
// include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp:12-25
class ViewBox : public GraphicsWidget {
public:
    static constexpr int PanMode = 3;
    static constexpr int RectMode = 1;
    static constexpr int XAxis = 0;
    static constexpr int YAxis = 1;
    static constexpr int XYAxes = 2;

    explicit ViewBox(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~ViewBox() override;
    ...
};
```

Implementation has no members/state:

```cpp
// src/pyqtgraph/graphicsItems/ViewBox/ViewBox.cpp:9-14
ViewBox::ViewBox(QGraphicsItem* parent, Qt::WindowFlags flags)
    : GraphicsWidget(parent, flags)
{
}

ViewBox::~ViewBox() = default;
```

Existing tests use a local `CHECK` helper and offscreen `QApplication`; no Catch/GTest. Preserve this style for `tests/graphicsItems/test_ViewBox_range.cpp` unless the issue says otherwise.

Likely range-model API to expose from upstream behavior:
- `viewRange()` returns a copy of actual visible range `[[xmin,xmax],[ymin,ymax]]`.
- `targetRange()` returns a copy of requested range.
- `viewRect()` / `targetRect()` return `QRectF(xMin, yMin, xWidth, yHeight)`.
- `setRange(rect/xRange/yRange, padding, update, disableAutoRange)` validates finite ranges, handles zero span by preserving prior scale, applies padding, clamps to limits, updates target range, then updates view range.
- `setXRange(min,max,padding,update)` and `setYRange(...)` delegate to `setRange`.
- `setLimits(xMin,xMax,yMin,yMax,minXRange,maxXRange,minYRange,maxYRange)` constrains panning/span; upstream rejects invalid keyword names, but C++ should likely use typed parameters/options rather than kwargs.

Upstream initial state (ViewBox.py:143-177):
- `targetRange` and `viewRange` start as `[[0,1],[0,1]]`.
- `autoRange` starts `[True, True]`; `autoPan` and `autoVisibleOnly` false.
- `mouseEnabled` mirrors constructor `enableMouse`.
- `limits.xLimits/yLimits` default `[-1E307, +1E307]`; `xRange/yRange` default `[None, None]`.
- `logMode` default `[False, False]`; `_effectiveLimits()` clamps log axes to about `[-307.6,+308.2]`.

Upstream setRange details to test numerically:
- Throws if none of rect/xRange/yRange are supplied.
- Reorders reversed ranges with `min(range)`/`max(range)`.
- Throws on `NaN`/`inf`.
- Default padding uses `suggestPadding`; for deterministic unit tests prefer `padding=0.0`.
- Zero span expands around the point by half the previous span (initial `[0,1]` means `setXRange(5,5,padding=0)` should become `[4.5,5.5]`).
- Limits can impose max/min range and shift ranges inside bounds.

## Architecture

`ViewBox` is a `GraphicsWidget`, which is both `QGraphicsWidget` and `GraphicsItem`. The #35/#PGVIEW-001 base guarantees `GraphicsItem(static_cast<QGraphicsItem*>(this))`, `graphicsItem()` identity, cached view-widget discovery, and `height()/width()` wrappers over `geometry()`.

The range model can be implemented without painting or child-group support by adding private state to `ViewBox` (likely via an internal `struct Private` or direct members) and pure numeric methods. Keep Qt dependencies limited to `QRectF`/`qreal` and existing `QGraphicsItem`/`QGraphicsWidget`. Signals, child group, matrix transforms, auto-ranging over items, mouse interaction, menu, and painting were explicitly deferred in PGVIEW-001 and likely belong to later issues (PGVIEW-003+).

CMake currently registers only `tests/graphicsItems/test_ViewBox.cpp` as target `pyqtgraph_cpp_graphicsitems_viewbox` and ctest name `pyqtgraph_cpp.graphicsItems.ViewBox`. For this issue-owned `tests/graphicsItems/test_ViewBox_range.cpp`, add a sibling executable/test target in the `_pyqtgraph_cpp_has_graphicsitem` block, e.g. `pyqtgraph_cpp_graphicsitems_viewbox_range` with ctest name `pyqtgraph_cpp.graphicsItems.ViewBox.range`.

`oracle/scripts/generate_numeric_oracles.py` has no ViewBox/range fixture. Since upstream checkout is absent despite `reference/PYQTGRAPH_REF` naming `reference/pyqtgraph`, only add oracle generation if the issue explicitly requires deterministic numeric fixtures; otherwise local focused C++ tests with upstream-derived expected values are simpler.

## Start Here

Open `include/pyqtgraph/graphicsItems/ViewBox/ViewBox.hpp` first to design the public range-model API and state representation, then `tests/graphicsItems/test_ViewBox.cpp` for the local test harness pattern before creating `tests/graphicsItems/test_ViewBox_range.cpp`.

## Likely Tests

Suggested deterministic cases for `test_ViewBox_range.cpp`:
1. Default construction: `viewRange()` and `targetRange()` both `{{0,1},{0,1}}`; returned values are copies.
2. `setXRange(2, 4, padding=0)` updates only X target/view, leaves Y `[0,1]`; `setYRange(-3, 7, padding=0)` likewise.
3. `setRange(QRectF(...), padding=0)` maps rect left/right/top/bottom to ranges and `viewRect()/targetRect()` return matching geometry.
4. Reversed inputs normalize min/max.
5. Zero span preserves prior span around center.
6. Non-finite input is rejected and previous state remains unchanged.
7. `setLimits` clamps panning bounds and min/max range span.
8. Default padding behavior only if API exposes `suggestPadding`; otherwise avoid default-padding assertions until implemented.

## Validation Commands

Focused commands likely useful after implementation:

```sh
cmake --preset dev
cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_viewbox pyqtgraph_cpp_graphicsitems_viewbox_range
ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.graphicsItems\.ViewBox'
scripts/gate focus
scripts/gate commit
python3 -m pytest -q
```

`WORKFLOW.md` configured validation includes `python3 -m pytest -q`; PGVIEW-001 also successfully used `scripts/gate focus`, `scripts/gate commit`, and focused CMake/ctest commands.

## Constraints, Risks, Open Questions

- Owned files are strict: only `ViewBox.hpp`, `ViewBox.cpp`, `tests/graphicsItems/test_ViewBox_range.cpp`, `CMakeLists.txt` for registration, `oracle/scripts/generate_numeric_oracles.py` only for numeric fixtures, and `reports/agents/PGVIEW-002.md`.
- Do not modify existing `tests/graphicsItems/test_ViewBox.cpp` unless issue ownership is clarified; create the new range test file instead.
- Upstream `ViewBox.py` local checkout is missing; behavior above was fetched from the pinned GitHub commit. If network is unavailable later, use the line references captured here.
- C++ API design for Python kwargs/optional limits is not predetermined. Prefer a small typed API that tests can call clearly (for example `setLimits(std::optional<qreal> xMin = ..., ...)` or a `ViewBoxLimits` struct) rather than emulating Python kwargs.
- Avoid introducing `Q_OBJECT`/signals unless required; PGVIEW-001 intentionally deferred signal payload design and AUTOMOC complexity.
- Aspect locking, linked views, auto range over child items, transforms/matrix updates, mouse pan/zoom, menu, and rendering are later-scope risks; keep PGVIEW-002 to the range model unless issue text says otherwise.
