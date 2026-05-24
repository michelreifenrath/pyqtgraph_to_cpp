# Implementation Plan

## Goal
Add the PGGI-003 `GraphicsWidget` C++ base class, tests, hierarchy assertions, CMake wiring, and implementation report while preserving upstream PyQtGraph naming and issue-owned scope.

## Tasks
1. **Add failing GraphicsWidget construction/API tests first**: Create a focused test executable source that follows the existing manual `CHECK` style and offscreen `QApplication` guard from `test_GraphicsItem.cpp`.
   - File: `tests/graphicsItems/test_GraphicsWidget.cpp`
   - Changes: Include `<pyqtgraph/graphicsItems/GraphicsWidget.hpp>` and Qt Widgets headers; add tests for default construction/destruction, `std::is_base_of_v<QGraphicsWidget, GraphicsWidget>`, `std::is_base_of_v<QGraphicsItem, GraphicsWidget>`, `std::is_base_of_v<GraphicsItem, GraphicsWidget>`, `graphicsItem() == static_cast<QGraphicsItem*>(&widget)`, `getViewWidget() == nullptr` when not in a scene, and scene/view discovery through the inherited `GraphicsItem` helper.
   - Acceptance: Before implementation this should fail to compile because `GraphicsWidget.hpp` does not exist; after implementation the test target builds and passes.

2. **Extend hierarchy/API-shape test expectations**: Add static and runtime assertions for `GraphicsWidget` alongside the existing `GraphicsItem` assertions.
   - File: `tests/hierarchy/test_cpp_hierarchy.cpp`
   - Changes: Include `<pyqtgraph/graphicsItems/GraphicsWidget.hpp>` and `<QtWidgets/QGraphicsWidget>`; add `testGraphicsWidgetApiShape()` asserting constructibility/destructibility, base relationships to `GraphicsItem`, `QGraphicsWidget`, and `QGraphicsItem`, and host binding to `this`.
   - Acceptance: `pyqtgraph_cpp_hierarchy_cpp` fails before the class exists and passes after implementation.

3. **Declare the GraphicsWidget public API**: Add the new header with upstream attribution/source note matching `GraphicsItem.hpp`.
   - File: `include/pyqtgraph/graphicsItems/GraphicsWidget.hpp`
   - Changes: Define `pyqtgraph::graphicsItems::GraphicsWidget` as `class GraphicsWidget : public QGraphicsWidget, public GraphicsItem`; include `GraphicsItem.hpp` and `<QtWidgets/QGraphicsWidget>`; declare an explicit constructor compatible with `QGraphicsWidget` ownership, e.g. `explicit GraphicsWidget(QGraphicsItem* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});`; declare `~GraphicsWidget() override`; delete copy operations; default or delete move operations as appropriate for `QGraphicsWidget` (prefer deleting moves because Qt objects are non-movable); optionally declare upstream convenience wrappers only if they do not conflict with Qt (`setFixedHeight(qreal)`, `setFixedWidth(qreal)`, `height() const`, `width() const`).
   - Acceptance: Header compiles for clients including it directly; API remains in namespace `pyqtgraph::graphicsItems`.

4. **Implement GraphicsWidget host binding and optional upstream helpers**: Add the source file with source note and minimal native Qt behavior.
   - File: `src/pyqtgraph/graphicsItems/GraphicsWidget.cpp`
   - Changes: Constructor initializes `QGraphicsWidget(parent, flags)` and `GraphicsItem(static_cast<QGraphicsItem*>(this))`; destructor defaulted. If convenience wrappers were declared, implement `setFixedHeight` as max/min height assignment, `setFixedWidth` as max/min width assignment, and `height`/`width` using `geometry()`. Do not add `Q_OBJECT`; do not implement cached `boundingRect()`/`shape()` unless tests and issue scope explicitly require it.
   - Acceptance: `GraphicsWidget` instances bind the inherited `GraphicsItem` helper to the Qt graphics item host (`this`) and reuse `getViewWidget()` correctly.

5. **Wire GraphicsWidget into the library and tests under the existing Qt Widgets gate**: Extend the current PGGI-001 CMake block instead of adding a new option.
   - File: `CMakeLists.txt`
   - Changes: Add `src/pyqtgraph/graphicsItems/GraphicsWidget.cpp` to `target_sources(pyqtgraph_cpp ...)` inside `if(_pyqtgraph_cpp_has_graphicsitem)`. Inside `if(BUILD_TESTING)` and the same Qt Widgets gate, add executable target `pyqtgraph_cpp_graphicsitems_graphicswidget` from `tests/graphicsItems/test_GraphicsWidget.cpp`, link it like `pyqtgraph_cpp_graphicsitems_graphicsitem`, enable sanitizers, and register CTest name `pyqtgraph_cpp.graphicsItems.GraphicsWidget`.
   - Acceptance: Configuring with Qt Widgets present creates the new executable and CTest entry; configuring without Qt Widgets continues to skip graphics item targets.

6. **Build and run focused tests**: Validate the implementation before broader gates.
   - File: no source file changes for this task.
   - Changes: Run `cmake --preset dev`, then `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_graphicswidget pyqtgraph_cpp_hierarchy_cpp`, then `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.GraphicsWidget|hierarchy\.cpp)'`.
   - Acceptance: Configure succeeds, both targets build, and both focused CTest tests pass.

7. **Run required issue/workflow validation**: Execute the validation commands named by issue #32 where practical.
   - File: no source file changes for this task.
   - Changes: Run `scripts/gate focus PGGI-003`, `scripts/gate commit`, `python3 -m pytest -q`, and `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md`. If `scripts/gate focus PGGI-003` is rejected like PGGI-001, retry the supported local `scripts/gate focus` invocation and record both outcomes.
   - Acceptance: Required gates pass or any environment/tooling failure is captured with command, exit status, and reason.

8. **Write the required implementation report**: Document what changed and validation results after code/tests are complete.
   - File: `reports/agents/PGGI-003.md`
   - Changes: Follow the structure of `reports/agents/PGGI-001.md`: summary, design notes, changed files, validation, and limitations. Note the upstream source commit and any intentionally deferred cached `boundingRect()`/`shape()` behavior.
   - Acceptance: Report exists and accurately reflects implementation and validation outcomes.

## Files to Modify
- `CMakeLists.txt` - add `GraphicsWidget.cpp` to the Qt Widgets-gated library sources and register the new focused test executable/CTest target.
- `tests/hierarchy/test_cpp_hierarchy.cpp` - include `GraphicsWidget.hpp` and assert C++ hierarchy/API shape.

## New Files
- `include/pyqtgraph/graphicsItems/GraphicsWidget.hpp` - public C++ declaration for the upstream `GraphicsWidget` base.
- `src/pyqtgraph/graphicsItems/GraphicsWidget.cpp` - constructor/destructor and any non-conflicting upstream convenience helper implementations.
- `tests/graphicsItems/test_GraphicsWidget.cpp` - focused tests for construction, base relationships, host binding, and inherited `GraphicsItem` view lookup.
- `reports/agents/PGGI-003.md` - required issue implementation report and validation log.

## Dependencies
- Task 1 and Task 2 should be written before production code to satisfy TDD.
- Task 3 depends on the desired public API from Tasks 1-2.
- Task 4 depends on Task 3.
- Task 5 depends on Tasks 1 and 4 paths existing.
- Task 6 depends on Tasks 1-5.
- Task 7 depends on focused validation in Task 6.
- Task 8 depends on validation results from Tasks 6-7.

## Risks
- The issue requires `reports/agents/PGGI-003.md` even though reports may not appear in the owned implementation-file list; proceed because the Done definition explicitly requires it, but keep the report narrow.
- Avoid `Q_OBJECT` in `GraphicsWidget` unless a later explicit requirement justifies adding AUTOMOC/manual moc handling.
- `QGraphicsWidget` may already expose width/height-related methods; if declaring convenience wrappers causes overload/conflict warnings, omit or adjust them and keep tests focused on host binding and hierarchy.
- Upstream `boundingRect()`/`shape()` caching is not necessary for the base shell unless the issue explicitly requires pixel/geometry cache semantics; implementing it prematurely adds invalidation complexity.
- `reference/pyqtgraph` is absent in this worktree, so use the pinned snippet/commit information already captured for source notes and behavior unless the checkout is restored.
- Do not introduce or require `GraphicsObject`; issue #32 depends on PGGI-001 and upstream `GraphicsWidget` does not require `GraphicsObject`.

## Stop Rules
- Stop and ask for a decision before editing files outside the files listed above.
- Stop if Qt Widgets are unavailable and `PYQTGRAPH_CPP_REQUIRE_QT=ON` prevents configuration; record this as an environment blocker rather than weakening the issue scope.
- Stop if tests reveal that a multiple-inheritance layout other than `QGraphicsWidget, GraphicsItem` is required, because that is an API/ABI design decision.
