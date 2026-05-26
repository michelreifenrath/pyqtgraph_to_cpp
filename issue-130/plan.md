Read-only constraint noted: I did not modify or write `issue-130/plan.md`. Plan content:

# Implementation Plan

## Goal
Make `PlotWidget` a `QWidget` wrapper that owns an internal `QGraphicsView`, `GraphicsScene`, and `PlotItem`, with a focused P3.01 interaction proof and report.

## Tasks

1. **Add one focused failing P3.01 proof**
   - File: `tests/widgets/test_PlotWidget.cpp`
   - Changes:
     - Update API-shape assertions:
       - `PlotWidget` is based on `QWidget`.
       - `PlotWidget` is **not** based on `QGraphicsView`.
     - Add/runtime update checks that:
       - exactly one child `QGraphicsView` exists via `findChildren<QGraphicsView*>()`;
       - child view has a non-null scene;
       - scene is `pyqtgraph::GraphicsScene::GraphicsScene`;
       - `getPlotItem()` is non-null;
       - `plotItem->scene() == childView->scene()`;
       - `childView->scene()->items().contains(plotItem)`;
       - `plotItem->getViewWidget() == childView`.
     - Add a scripted interaction/no-op proof:
       - pre-state: child view count, scene pointer, plot item pointer, scene item membership, viewport size/geometry;
       - event sequence: resize/show/process events or a deterministic no-op Qt event sent to the widget/view;
       - post-state: same owned view/scene/plot item pointers remain valid, plot item remains in scene, no extra child views created;
       - signals/callbacks: record “none expected/observed” unless a meaningful signal is attached;
       - negative/no-op case: sending the no-op event does not replace the owned view/scene/plot item.
   - Acceptance:
     - Test fails before production change because `PlotWidget` currently inherits `QGraphicsView` directly and has no child view.

2. **Label the focused test as P3.01**
   - File: `CMakeLists.txt`
   - Changes:
     - After `add_test(NAME pyqtgraph_cpp.widgets.PlotWidget ...)`, add:
       - `set_tests_properties(pyqtgraph_cpp.widgets.PlotWidget PROPERTIES LABELS "P3.01")`
     - If the test writes the interaction report, add a compile definition for the artifact path, e.g.:
       - `${CMAKE_CURRENT_BINARY_DIR}/reports/issues/P3.01/plotwidget-interaction.md`
   - Acceptance:
     - `ctest --preset dev -L P3.01 --show-only=json-v1` includes `pyqtgraph_cpp.widgets.PlotWidget`.

3. **Make the smallest production architecture change**
   - File: `include/pyqtgraph/widgets/PlotWidget.hpp`
   - Changes:
     - Change inheritance from `QGraphicsView` to `QWidget`.
     - Include/forward-declare `QGraphicsView`.
     - Add private member:
       - `std::unique_ptr<QGraphicsView> view_;`
     - Keep:
       - `std::unique_ptr<GraphicsScene::GraphicsScene> scene_;`
       - `graphicsItems::PlotItem* plotItem_ = nullptr;`
       - existing `getPlotItem()` API.
   - Acceptance:
     - Header compiles with `PlotWidget` as a `QWidget`.

4. **Wire internal view, scene, and plot item**
   - File: `src/pyqtgraph/widgets/PlotWidget.cpp`
   - Changes:
     - Construct `PlotWidget` as `QWidget(parent)`.
     - Create `view_ = std::make_unique<QGraphicsView>(this)`.
     - Create `scene_`.
     - Create `plotItem_ = new graphicsItems::PlotItem()`.
     - Set `view_->setScene(scene_.get())`.
     - Add `plotItem_` to the scene.
     - Put `view_` in a zero-margin layout so downstream apps render the internal graphics view.
     - Destructor should detach the scene from `view_`, not from `this`.
   - Acceptance:
     - P3.01 test passes and downstream use of `PlotWidget` as a normal widget still shows the internal view.

5. **Write required interaction report**
   - File: `reports/issues/P3.01/plotwidget-interaction.md`
   - Changes:
     - Record:
       - command run and exit code;
       - artifact path;
       - pre-state: child view count, scene type, plot item pointer presence, item membership, view/viewport geometry;
       - event sequence;
       - post-state;
       - signal/callback observation or explicit “none expected” note;
       - negative/no-op case result;
       - note that no screenshot/visual artifact is required because this proof validates ownership/interaction state, not pixel rendering.
   - Acceptance:
     - Report exists and matches the state asserted by the focused test.

6. **Run focused validation**
   - Files: none
   - Commands:
     - `cmake --preset dev`
     - `cmake --build --preset dev --target pyqtgraph_cpp_widgets_plotwidget`
     - `ctest --preset dev -L P3.01 --output-on-failure`
     - `git diff --check`
   - Acceptance:
     - All commands exit 0.

## Files to Modify
- `tests/widgets/test_PlotWidget.cpp` - P3.01 failing proof and interaction/no-op assertions.
- `CMakeLists.txt` - P3.01 label and optional report-path compile definition.
- `include/pyqtgraph/widgets/PlotWidget.hpp` - change `PlotWidget` from `QGraphicsView` subclass to `QWidget` owner.
- `src/pyqtgraph/widgets/PlotWidget.cpp` - construct/layout owned internal `QGraphicsView`, scene, and plot item.
- `reports/issues/P3.01/plotwidget-interaction.md` - required interaction proof report.

## New Files
- `reports/issues/P3.01/plotwidget-interaction.md` - local interaction-ui validation report.

## Dependencies
- Task 1 should be done before Tasks 3-4 to prove the current failure.
- Task 2 is required before focused `-L P3.01` validation is meaningful.
- Task 5 depends on the focused proof details from Task 1.
- Task 6 runs after all implementation/report changes.

## Risks
- `tests/hierarchy/test_cpp_hierarchy.cpp` currently encodes `PlotWidget` as a `QGraphicsView`; scout findings do not list it as owned. Full C++ test builds may fail unless ownership is expanded or that broader hierarchy proof is deferred.
- Existing downstream code may call inherited `QGraphicsView` APIs on `PlotWidget`; the issue explicitly asks for `QWidget` owning a view, so do not add compatibility shims unless directed.
- `reports/issues/P3.01/**` appears allowed through `focused-tests`, but confirm issue ownership if automation rejects report paths.
- Avoid broad cleanup or introducing a custom `GraphicsView` class; no such ported class currently exists.