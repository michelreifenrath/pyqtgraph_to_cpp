# PGGI-003 GraphicsWidget Implementation Report

## Summary
- Added `pyqtgraph::graphicsItems::GraphicsWidget` as a native C++ `QGraphicsWidget` subclass with the existing `GraphicsItem` helper bound to the widget host.
- Implemented upstream-aligned convenience methods for fixed width/height and geometry width/height access.
- Added focused construction, hierarchy, host-binding, view-discovery, and geometry-helper tests.
- Wired the new source and test target behind the existing Qt 6 Widgets graphics item gate in `CMakeLists.txt`.

## Design notes
- `GraphicsWidget` uses multiple inheritance matching upstream shape: `QGraphicsWidget` plus `GraphicsItem`.
- The constructor initializes `QGraphicsWidget` first and binds `GraphicsItem` to `static_cast<QGraphicsItem*>(this)` so inherited helpers operate on the Qt graphics item host.
- `GraphicsWidget::graphicsItem()` is declared on the concrete class to expose the `GraphicsItem` host binding without ambiguity from Qt's `QGraphicsLayoutItem::graphicsItem()` member.
- Copy and move operations are deleted because Qt graphics objects are QObject/QGraphicsItem-like non-copyable, non-movable runtime objects.
- `Q_OBJECT` was intentionally not added; no new moc/AUTOMOC wiring is required.
- Upstream cached `boundingRect()` and `shape()` behavior is deferred because the issue scope only requires the base class shell/API shape and host binding, and Qt already provides default `QGraphicsWidget` geometry behavior.

## Changed files
- `include/pyqtgraph/graphicsItems/GraphicsWidget.hpp`
- `src/pyqtgraph/graphicsItems/GraphicsWidget.cpp`
- `tests/graphicsItems/test_GraphicsWidget.cpp`
- `tests/hierarchy/test_cpp_hierarchy.cpp`
- `CMakeLists.txt`
- `reports/agents/PGGI-003.md`

## Validation
- `cmake --preset dev` — exit 0; configured with Qt 6 Widgets available.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_graphicswidget pyqtgraph_cpp_hierarchy_cpp` — first run exit 2 due to ambiguous `graphicsItem()` between `GraphicsItem` and Qt `QGraphicsLayoutItem`; fixed by adding concrete `GraphicsWidget::graphicsItem()` forwarding to `GraphicsItem`.
- `cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_graphicswidget pyqtgraph_cpp_hierarchy_cpp` — exit 0.
- `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.GraphicsWidget|hierarchy\.cpp)'` — exit 0; 2/2 focused tests passed.
- `scripts/gate focus PGGI-003` — exit 2; local gate CLI rejected the issue argument as unrecognized.
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 -m pytest -q` — exit 0; 204 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0; workflow valid.

## Limitations
- Cached upstream `boundingRect()` / `shape()` invalidation was not implemented in this base issue.
- No visual artifacts were generated because this change adds a base widget/helper class and non-pixel-focused API tests.
