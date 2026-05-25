# Code Context

## Files Retrieved
1. `include/pyqtgraph/graphicsItems/PlotCurveItem.hpp` (lines 1-34) - current PlotCurveItem public API is only the PGPLOT-002 skeleton.
2. `src/pyqtgraph/graphicsItems/PlotCurveItem.cpp` (lines 1-35) - current implementation has empty bounds and no-op paint; no data storage.
3. `tests/graphicsItems/test_PlotCurveItem.cpp` (lines 1-157) - existing standalone graphics item tests; no setData tests yet.
4. `CMakeLists.txt` (lines 22-54, 112-130, 243-323) - Qt Widgets feature gate, library source registration, and current PlotCurveItem test target.
5. `include/pyqtgraph/PlotData.hpp` (lines 49-76) - reusable native `PlotData` one-dimensional double field container.
6. `src/pyqtgraph/PlotData.cpp` (lines 53-138) - `PlotData::set` and extrema cache behavior.
7. `tests/core/test_PlotData.cpp` (lines 40-129) - examples of repository test style and PlotData expectations.
8. `reports/agents/PGPLOT-002.md` (lines 1-34) - dependency handoff: skeleton intentionally did not implement data behavior.
9. `docs/pyqtgraph-cpp-port-workflow.md` (lines 660-663) - roadmap names PGPLOT-003 as `PlotCurveItem::setData` tests.
10. `port_manifest.yaml` (lines 2719-2741) - upstream source/class mapping: PlotCurveItem.py contains OpenGLState, PlotCurveItem, ROIPlotItem.
11. `oracle/scripts/generate_numeric_oracles.py` (lines 1-220) - numeric-oracle generator exists but currently has no PlotCurveItem fixture path.
12. `CMakePresets.json` (lines 8-24, 25-42, 43-58) - dev/release/CI presets require Qt and build tests.

## Key Code

Current `PlotCurveItem` API is a pure skeleton:

```cpp
// include/pyqtgraph/graphicsItems/PlotCurveItem.hpp:19-33
class PlotCurveItem : public GraphicsObject {
public:
    explicit PlotCurveItem(QGraphicsItem* parent = nullptr);
    ~PlotCurveItem() override;
    ...
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};
```

Current implementation has no x/y data, options, bounds cache, invalidation, or signal-specific behavior:

```cpp
// src/pyqtgraph/graphicsItems/PlotCurveItem.cpp:23-34
QRectF PlotCurveItem::boundingRect() const
{
    return QRectF{};
}

void PlotCurveItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}
```

Existing PlotCurveItem tests only cover construction, inheritance, parent/view discovery, null bounding rect, and no-op paint smoke (`tests/graphicsItems/test_PlotCurveItem.cpp:40-157`). They use a local `CHECK` macro and offscreen `QApplication` guard; new focused tests can follow this style.

Useful native numeric container already exists:

```cpp
// include/pyqtgraph/PlotData.hpp:49-76
class PlotData {
public:
    using Values = std::vector<double>;
    void addFields(std::initializer_list<std::string> fields);
    bool hasField(std::string_view field) const;
    const Values& operator[](std::string_view field) const;
    Values& operator[](std::string_view field);
    void set(std::string field, std::span<const double> values);
    ...
};
```

`PlotData::set` copies a span/vector/initializer-list into an owning `std::vector<double>` (`src/pyqtgraph/PlotData.cpp:75-90`). It does not invalidate cached extrema by design; do not use `PlotData` extrema cache blindly for PlotCurveItem bounds unless that stale-cache behavior is acceptable.

## Architecture

- `PlotCurveItem` lives under `pyqtgraph::graphicsItems` and derives from `GraphicsObject`, which already provides GraphicsItem binding and view discovery tested by PGPLOT-002.
- CMake only builds graphics-item sources/tests when Qt Core+Gui+Widgets are found (`CMakeLists.txt:50-54`). With the `dev` preset, Qt is required (`CMakePresets.json:8-24`), so focused PlotCurveItem tests should be available in normal validation.
- Library source registration already includes `src/pyqtgraph/graphicsItems/PlotCurveItem.cpp` (`CMakeLists.txt:112-124`).
- Existing registered test target is `pyqtgraph_cpp_graphicsitems_plotcurveitem` from `tests/graphicsItems/test_PlotCurveItem.cpp`, with CTest name `pyqtgraph_cpp.graphicsItems.PlotCurveItem` (`CMakeLists.txt:315-323`).
- The issue-owned file list names a new `tests/graphicsItems/test_PlotCurveItem_setData.cpp`; it does not exist yet. Planner should decide whether to add a separate executable/CTest registration in `CMakeLists.txt` (owned for registration) rather than expanding the skeleton test file.
- The pinned upstream checkout is not present in this worktree (`reference/PYQTGRAPH_REF` says checkout path should be `reference/pyqtgraph`, but that directory is absent). Upstream behavior should be checked by fetching/restoring the pinned source or adding an oracle probe before implementing edge cases.

Likely setData surface to test/implement (confirm against pinned PyQtGraph before coding):
- Accept x/y vectors and y-only vectors (auto x as 0..N-1 in upstream-like behavior).
- Copy data, not retain caller storage; expose const accessors for x/y or equivalent testable API.
- Reject mismatched x/y lengths; define behavior for empty data.
- Update `boundingRect()` from finite x/y extents; include null/empty behavior.
- Clear/replace previous data on repeated `setData` calls; ensure bounds update.
- Handle NaN/Inf consistently with PyQtGraph/numeric-oracle policy.
- Trigger Qt item geometry invalidation (`prepareGeometryChange()` before bounds changes) and repaint scheduling (`update()`) if bounds/pixels will change.
- Option-related upstream kwargs (`connect`, `pen`, `brush`, `fillLevel`, `stepMode`, etc.) may be out of PGPLOT-003 scope unless issue body explicitly requires them; avoid broad API expansion without confirmation.

## Start Here

Open `include/pyqtgraph/graphicsItems/PlotCurveItem.hpp` first. It has no setData/data access API yet, so tests and implementation will need to define the minimal native C++ API before adding behavior in `src/pyqtgraph/graphicsItems/PlotCurveItem.cpp`.

## Test/build registration status

- `tests/graphicsItems/test_PlotCurveItem_setData.cpp` is absent.
- `CMakeLists.txt` currently registers only `tests/graphicsItems/test_PlotCurveItem.cpp` as `pyqtgraph_cpp.graphicsItems.PlotCurveItem`.
- If adding the owned setData test file, add an executable and `add_test`, e.g. target name consistent with existing style such as `pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata` and CTest name `pyqtgraph_cpp.graphicsItems.PlotCurveItem.setData`.

## Validation commands

Recommended focused sequence after implementation:

```sh
cmake --preset dev
cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_plotcurveitem pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata -j2
cd build/dev && ctest -R 'pyqtgraph_cpp\.graphicsItems\.PlotCurveItem' --output-on-failure
```

Then run repository-required checks when practical:

```sh
git diff --check
scripts/gate commit
python3 -m pytest -q
```

PGPLOT-002 notes `scripts/gate focus PGPLOT-002` failed because the gate CLI rejected the issue argument; `scripts/gate focus` passed there.

## Risks and open questions

- Upstream `PlotCurveItem.py` is not locally available despite the manifest/pinned ref. Behavior details should be verified before edge-case semantics are locked in.
- The issue title says “setData tests”, but acceptance says implement behavior too. Use TDD: add failing `test_PlotCurveItem_setData.cpp`, then implement minimal passing API.
- Pixel drawing is deferred to PGPLOT-004, so PGPLOT-003 should avoid paint implementation unless needed for bounds/data invalidation.
- `PlotData` can store arrays but its cached extrema behavior is intentionally stale after replacement; PlotCurveItem probably needs fresh bounds after each `setData`.
- Owned files do not include hierarchy tests, so avoid changing `tests/hierarchy/test_cpp_hierarchy.cpp` unless the issue scope is expanded.
