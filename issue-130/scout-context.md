I did **not** write `/issue-130/scout-context.md` because the task’s hard constraint says **“do not modify files”** and the target directory does not exist. Findings below.

# Code Context

## Files Retrieved

1. `include/pyqtgraph/widgets/PlotWidget.hpp` (lines 1-27) - declares current PlotWidget as `QGraphicsView`.
2. `src/pyqtgraph/widgets/PlotWidget.cpp` (lines 1-32) - constructs owned scene and raw `PlotItem`.
3. `tests/widgets/test_PlotWidget.cpp` (lines 1-126) - current focused PlotWidget test, no `P3.01` label.
4. `CMakeLists.txt` (lines 120-130, 436-444) - library source wiring and PlotWidget test registration.
5. `WORKFLOW.md` (lines 71-93, 116-138) - validation/safety policy.
6. `docs/proposed-issues/VALIDATION-GUIDE.md` (lines 8-12, 17-40, 65-66) - owned-file and interaction-ui proof rules.
7. `port_manifest.yaml` (lines 1221-1226, 5467-5477) - selector expansion for `pyqtgraph/widgets/PlotWidget.py`.
8. Installed upstream selector: `/home/michel/.hermes/hermes-agent/venv/lib/python3.11/site-packages/pyqtgraph/widgets/PlotWidget.py` (lines 12-70) - upstream `PlotWidget(GraphicsView)` behavior.

## Key Code

Current C++ architecture:

```cpp
class PlotWidget : public QGraphicsView {
private:
    std::unique_ptr<GraphicsScene::GraphicsScene> scene_;
    graphicsItems::PlotItem* plotItem_ = nullptr;
};
```

Constructor:

```cpp
PlotWidget::PlotWidget(QWidget* parent)
    : QGraphicsView(parent)
    , scene_(std::make_unique<GraphicsScene::GraphicsScene>())
    , plotItem_(new graphicsItems::PlotItem())
{
    setScene(scene_.get());
    scene_->addItem(plotItem_);
}
```

Current test asserts:
- `PlotWidget` derives from `QGraphicsView` and `QWidget`.
- `widget.scene()` is a `pyqtgraph::GraphicsScene::GraphicsScene`.
- `plotItem->scene() == widget.scene()`.
- `plotItem->getViewWidget() == &widget`.

Upstream PyQtGraph 0.14.0:
- `class PlotWidget(GraphicsView)` at line 12.
- `GraphicsView.__init__(...)`, `enableMouse(False)`.
- Creates or accepts a `PlotItem`.
- Calls `self.setCentralItem(self.plotItem)`.

## Architecture

Current port makes `PlotWidget` directly inherit `QGraphicsView` and internally own a `GraphicsScene` via `unique_ptr`; the `PlotItem` is heap-allocated raw and added to the scene, relying on Qt scene ownership. Issue #130 requires: **“PlotWidget is QWidget owning GraphicsView, scene, and PlotItem; downstream app renders.”** That conflicts with current direct `QGraphicsView` inheritance and current tests that encode that shape.

## Existing Tests / Labels

- Test target: `pyqtgraph_cpp_widgets_plotwidget`.
- CTest name: `pyqtgraph_cpp.widgets.PlotWidget`.
- No `set_tests_properties(... LABELS P3.01)` exists for PlotWidget.
- Required command `ctest --preset dev -L P3.01 --output-on-failure` likely selects no current PlotWidget test.

## Likely Failing Proof Gap

The required `interaction-ui` proof is missing:
- no scripted Qt/user-event replay,
- no pre-state/event/post-state report,
- no signal/callback assertions,
- no negative/no-op case,
- no `P3.01` test label.

## Exact Allowed Files

From issue #130:
- Manifest source selector: `pyqtgraph/widgets/PlotWidget.py`, expands to:
  - `include/pyqtgraph/widgets/PlotWidget.hpp`
  - `src/pyqtgraph/widgets/PlotWidget.cpp`
- Repository path globs: current PlotWidget header/source/tests:
  - `tests/widgets/test_PlotWidget.cpp`
- Common adjuncts: `focused-tests`.
- Shared wiring may be touched only to register/run owned tests/artifacts:
  - `CMakeLists.txt`
  - `tests/CMakeLists.txt`
  - `CMakePresets.json`
  - `cmake/**`
  - focused validation scripts

## Start Here

Open `include/pyqtgraph/widgets/PlotWidget.hpp` first: it encodes the core architecture mismatch (`PlotWidget : public QGraphicsView` instead of QWidget owning a GraphicsView).