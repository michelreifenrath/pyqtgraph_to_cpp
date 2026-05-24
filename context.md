# Code Context

## Files Retrieved
1. `AGENTS.md` (lines 1-43) - repository rules: issue scope first, owned-file-only edits, TDD, pinned upstream source, source notes.
2. `WORKFLOW.md` (lines 1-43, 50-64) - validation config and safety gates; default validation includes `python3 -m pytest -q`.
3. `docs/pyqtgraph-cpp-port-workflow.md` (lines 84-98, 143-168, 643-650) - non-negotiable port rules, attribution/source-note format, Phase E order (`PGGI-001`, `PGGI-002`, `PGGI-003`).
4. `reference/PYQTGRAPH_REF` (lines 1-5) and `reference/source.lock` (lines 1-5) - pinned upstream ref is `pyqtgraph-0.14.0`, commit `a20028b98294b9cc8770f2015a92eb342224b788`; checkout path says `reference/pyqtgraph`, but that directory is absent in this worktree.
5. `port_manifest.yaml` (lines 1694-1727) - manifest entries for `GraphicsItem`, `GraphicsObject`, `GraphicsWidget`; `GraphicsWidget` target paths and bases are `GraphicsItem`, `QtWidgets.QGraphicsWidget`.
6. `oracle/fixtures/hierarchy_pyqtgraph.json` (lines 2099-2108, 2173-2184) - upstream hierarchy fixture: `GraphicsItem` children include `GraphicsObject` and `GraphicsWidget`; `GraphicsWidget` bases are `GraphicsItem`, `QtWidgets.QGraphicsWidget`.
7. `include/pyqtgraph/graphicsItems/GraphicsItem.hpp` (lines 1-35) - existing dependency from PGGI-001; helper/base API with host `QGraphicsItem*` and view-cache methods.
8. `src/pyqtgraph/graphicsItems/GraphicsItem.cpp` (lines 1-81) - implementation pattern: pimpl, `QPointer<QGraphicsView>` cache, host set/forget behavior.
9. `tests/graphicsItems/test_GraphicsItem.cpp` (lines 1-136) - graphics item C++ test style, offscreen `QApplication` guard, manual `CHECK` helper.
10. `tests/hierarchy/test_cpp_hierarchy.cpp` (lines 1-72) - hierarchy/API-shape static assertions currently covering `GraphicsItem`.
11. `CMakeLists.txt` (lines 18-38, 66-73, 133-152) - Qt feature gates and test-target wiring for graphics item targets.
12. `cmake/PyQtGraphCppOptions.cmake` (lines 1-38) and `CMakePresets.json` (lines 1-92) - Qt/OpenCV requirement options; use standard `BUILD_TESTING`, not `PYQTGRAPH_CPP_BUILD_TESTS`.
13. `reports/agents/PGGI-001.md` (lines 1-30) - prior implementation report: design decisions and validation command history for `GraphicsItem`.
14. GitHub issue #32 via `gh issue view 32` - owned files, validation commands, done definition, required report `reports/agents/PGGI-003.md`.
15. Pinned upstream `pyqtgraph/graphicsItems/GraphicsWidget.py` fetched from commit `a20028b...` (lines 1-48) - source behavior for this port.

## Key Code

Existing dependency (`include/pyqtgraph/graphicsItems/GraphicsItem.hpp:15-29`):
```cpp
class GraphicsItem {
public:
    explicit GraphicsItem(QGraphicsItem* host = nullptr);
    virtual ~GraphicsItem();
    void setGraphicsItem(QGraphicsItem* host) noexcept;
    [[nodiscard]] QGraphicsItem* graphicsItem() const noexcept;
    [[nodiscard]] QGraphicsView* getViewWidget() const;
    void forgetViewWidget() const noexcept;
};
```

Existing `GraphicsItem` cache behavior (`src/pyqtgraph/graphicsItems/GraphicsItem.cpp:18-26`, `55-78`): stores a `QGraphicsItem* host`, a mutable `QPointer<QGraphicsView> viewWidget`, and a `viewWidgetCachePopulated` flag. `getViewWidget()` returns the cached view if populated, otherwise first scene view; `forgetViewWidget()` clears cache.

Upstream `GraphicsWidget.py` at pinned commit (important lines):
```python
class GraphicsWidget(GraphicsItem, QtWidgets.QGraphicsWidget):
    def __init__(self, *args, **kwargs):
        QtWidgets.QGraphicsWidget.__init__(self, *args, **kwargs)
        GraphicsItem.__init__(self)
        self._boundingRectCache = self._previousGeometry = None
        self._painterPathCache = None
        self.geometryChanged.connect(self._resetCachedProperties)

    @QtCore.Slot()
    def _resetCachedProperties(self):
        self._boundingRectCache = self._previousGeometry = None
        self._painterPathCache = None

    def setFixedHeight(self, h):
        self.setMaximumHeight(h)
        self.setMinimumHeight(h)

    def setFixedWidth(self, h):
        self.setMaximumWidth(h)
        self.setMinimumWidth(h)

    def height(self): return self.geometry().height()
    def width(self): return self.geometry().width()

    def boundingRect(self):
        geometry = self.geometry()
        if geometry != self._previousGeometry:
            self._painterPathCache = None
            br = self.mapRectFromParent(geometry).normalized()
            self._boundingRectCache = br
            self._previousGeometry = geometry
        else:
            br = self._boundingRectCache
        return QtCore.QRectF(br)

    def shape(self):
        p = self._painterPathCache
        if p is None:
            self._painterPathCache = p = QtGui.QPainterPath()
            p.addRect(self.boundingRect())
        return p
```

CMake graphics gate (`CMakeLists.txt:32-38`, `66-73`, `133-152`): `_pyqtgraph_cpp_has_graphicsitem` requires Qt6 Core/Gui/Widgets. Add `GraphicsWidget.cpp` to the existing `if(_pyqtgraph_cpp_has_graphicsitem)` library sources and add a sibling test executable/test inside the same block.

Likely C++ API shape for PGGI-003: a concrete `pyqtgraph::graphicsItems::GraphicsWidget` that derives from both `QtWidgets::QGraphicsWidget` and the existing `GraphicsItem` helper. Constructor should initialize `QGraphicsWidget` then bind the `GraphicsItem` host to `this` (as `QGraphicsItem*`). Tests should assert it is a `QGraphicsWidget`/`QGraphicsItem` and a `GraphicsItem`, is constructible/destructible, and `graphicsItem() == static_cast<QGraphicsItem*>(&widget)`.

## Architecture

- This repo is building a native Qt/C++ port, not Python bindings.
- `GraphicsItem` is not itself a Qt item; it wraps/binds a host `QGraphicsItem*` and implements upstream helper behavior such as `getViewWidget()` caching.
- Upstream `GraphicsWidget` uses multiple inheritance: PyQtGraph `GraphicsItem` + Qt `QGraphicsWidget`. The C++ port can mirror that with multiple inheritance: `class GraphicsWidget : public QGraphicsWidget, public GraphicsItem` (or reversed only if intentionally chosen; Qt type first is conventional for Qt-derived class layout and use).
- `QGraphicsWidget` already provides Qt geometry, fixed size, `geometryChanged`, `boundingRect()`, and `shape()` virtuals. Upstream overrides add caching/workarounds. For a base issue, decide whether to implement only the class shell/host binding plus convenience `height()`, `width()`, `setFixedHeight()`, `setFixedWidth()`, or also cache `boundingRect()`/`shape()`. If implementing cache, connect `geometryChanged` to a reset function without requiring `Q_OBJECT` if possible (lambda or regular member callable); adding `Q_OBJECT` would require AUTOMOC/CMake changes.
- `GraphicsObject` is not implemented in this worktree even though Phase E lists it as PGGI-002 before PGGI-003. Issue #32 only declares dependency #30/PGGI-001 and upstream `GraphicsWidget` does not depend on `GraphicsObject`, so do not add/require `GraphicsObject` unless scope changes.
- Tests are standalone executables using `iostream`/manual `CHECK`, not a testing framework. Qt Widgets tests set `QT_QPA_PLATFORM=offscreen` and create a `QApplication` guard.
- CMake owns all target wiring at the root. `tests/CMakeLists.txt` only has the smoke test; new graphics tests should be added in root `CMakeLists.txt` alongside existing `GraphicsItem` targets.

## Start Here

Open `include/pyqtgraph/graphicsItems/GraphicsItem.hpp` first to match the existing dependency API and source-note style, then `CMakeLists.txt` lines 66-73 and 133-152 to add `GraphicsWidget.cpp` and `test_GraphicsWidget.cpp` under the existing Qt Widgets gate.

## Likely Changes for Implementer

Owned implementation files only:
- Create `include/pyqtgraph/graphicsItems/GraphicsWidget.hpp` with required upstream source note. Include `GraphicsItem.hpp` and forward/include `QGraphicsWidget` as needed.
- Create `src/pyqtgraph/graphicsItems/GraphicsWidget.cpp` with source note. Initialize both bases and bind `GraphicsItem` to `this`.
- Create `tests/graphicsItems/test_GraphicsWidget.cpp`, following `test_GraphicsItem.cpp` style and offscreen `QApplication` guard.
- Update `tests/hierarchy/test_cpp_hierarchy.cpp` to include/assert `GraphicsWidget` API shape in addition to `GraphicsItem`.
- Update `CMakeLists.txt` to compile source and add test target/name, likely `pyqtgraph_cpp_graphicsitems_graphicswidget` and CTest name `pyqtgraph_cpp.graphicsItems.GraphicsWidget`.
- Required implementation report path is `reports/agents/PGGI-003.md`, but it is not in the issue-owned files list provided to this scout; implementer should confirm whether report writing is permitted by the issue Done definition (it explicitly requires it).

## Validation Commands

From issue #32:
```sh
scripts/gate focus PGGI-003
scripts/gate commit
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```

Focused C++ commands likely useful before full gate:
```sh
cmake --preset dev
cmake --build build/dev --target pyqtgraph_cpp_graphicsitems_graphicswidget pyqtgraph_cpp_hierarchy_cpp
ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.GraphicsWidget|hierarchy\.cpp)'
```
If not using presets, ensure `BUILD_TESTING=ON`; `PYQTGRAPH_CPP_BUILD_TESTS` is not a recognized project option per the prior PGGI-001 report.

## Risks / Open Questions

- `reference/pyqtgraph` is absent despite `source.lock` naming it; upstream source had to be fetched from GitHub by pinned commit. If network is unavailable later, rely on the snippet above or restore the pinned checkout.
- Adding `Q_OBJECT` to `GraphicsWidget` would require CMake AUTOMOC or manual moc handling; avoid unless necessary.
- `QGraphicsWidget::height()`/`width()` may already exist via `QGraphicsWidget`/`QGraphicsLayoutItem`; adding same-name methods could be redundant or conflict-prone. Test desired API before adding convenience wrappers.
- Upstream caching of `boundingRect()`/`shape()` may be unnecessary initially because Qt already implements these, but downstream PyQtGraph classes may expect the override semantics. If implemented, test invalidation on geometry changes.
- The Done definition requires `reports/agents/PGGI-003.md`, but the user’s owned implementation file list excludes reports. The task context says required report, so implementation likely needs to write it despite owned-list tension.
