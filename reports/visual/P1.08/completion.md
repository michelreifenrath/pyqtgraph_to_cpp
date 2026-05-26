# P1.08 completion evidence: Replace placeholder C++ visual renderer

Issue: #112 `[P1.08] Replace placeholder C++ visual renderer`

## Scope summary

Added a native C++/Qt SimplePlot visual renderer harness and a focused visual CTest tagged `P1.08`. Rework after Codex autoreview changed the harness from manually painting extracted curve data into a `QImage` to grabbing the actual native `SimplePlot` widget with `QWidget::grab`. To make that real widget/scene/item path render semantic content, the rework also added the minimal production painting path needed by this example: `PlotWidget` lays out a dark plot scene, `PlotItem` paints axes/ticks and maps child curves into plot coordinates, and `PlotCurveItem::paint` draws the stored curve data.

The proof generates canonical local visual artifacts under `reports/visual/P1.08/SimplePlot/` using the pinned PyQtGraph reference screenshot and a native C++ actual screenshot. The existing placeholder oracle script was not modified.

## Changed files

Manifest-expanded target paths: none.

Shared wiring paths changed:

- `CMakeLists.txt` — registers `pyqtgraph_cpp_visual_render_example` and `P1.08.visual.SimplePlot` with labels `visual;P1.08`.

Issue-owned/supporting paths changed:

- `tests/visual/P1_08_render_cpp_example.cpp`
- `tests/visual/test_P1_08_cpp_visual_renderer.py`
- `reports/visual/P1.08/SimplePlot/reference.png`
- `reports/visual/P1.08/SimplePlot/actual.png`
- `reports/visual/P1.08/SimplePlot/diff.png`
- `reports/visual/P1.08/SimplePlot/metrics.json`
- `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md`
- `reports/visual/P1.08/completion.md`

Autoreview-required production rendering paths changed during rework:

- `include/pyqtgraph/widgets/PlotWidget.hpp`
- `src/pyqtgraph/widgets/PlotWidget.cpp`
- `include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp`
- `src/pyqtgraph/graphicsItems/PlotItem/PlotItem.cpp`
- `src/pyqtgraph/graphicsItems/PlotCurveItem.cpp`

## TDD red result

| Command | Exit code | Result |
| --- | ---: | --- |
| `QT_QPA_PLATFORM=offscreen python3 -m pytest -q tests/visual/test_P1_08_cpp_visual_renderer.py` | 1 | Expected pre-implementation failure: `PG_CPP_VISUAL_RENDERER must point to the native renderer executable`; `1 failed, 1 passed`. |

## Rework result

The Codex autoreview finding was that `tests/visual/P1_08_render_cpp_example.cpp` could pass while bypassing the actual Qt rendering path. The reworked renderer now:

- creates `pyqtgraph::examples::createSimplePlotExample()`;
- resizes/shows/processes the actual `PlotWidget`;
- captures it with `example.widget->grab(...)`;
- reports `"render_path": "QWidget::grab"` in stdout; and
- no longer reads curve data or manually paints axes/curve inside the renderer harness.

The visual output is now produced through `PlotWidget` → `GraphicsScene` → `PlotItem::paint` → `PlotCurveItem::paint`.

## Artifact summary

Canonical visual artifacts:

- `reports/visual/P1.08/SimplePlot/reference.png`
- `reports/visual/P1.08/SimplePlot/actual.png`
- `reports/visual/P1.08/SimplePlot/diff.png`
- `reports/visual/P1.08/SimplePlot/metrics.json`
- `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md`

`metrics.json` records:

- dimensions: `[800, 600]`
- mean absolute delta: `3.8884911458333336`
- max delta: `200`
- changed pixel percent: `3.0377083333333332`
- SSIM: `0.8433645974038755`
- tolerances: max mean `6.0`, max pixel `220.0`, max changed percent `5.0`, min SSIM `0.8`
- deterministic verdict: `pass`
- failed checks/tolerances: `[]`

Blank/placeholder guard coverage is in `tests/visual/test_P1_08_cpp_visual_renderer.py::test_P1_08_blank_and_placeholder_guards_reject_non_semantic_images` and the focused CTest path.

Manual semantic inspection is recorded in `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md` after opening/reading the reference, actual, and diff images with an image-capable tool.

## Validation results

Final local validation after rework:

| Command | Exit code | Result |
| --- | ---: | --- |
| `cmake --preset visual` | 0 | Configure succeeded; build files written to `build/visual`. |
| `cmake --build --preset visual --parallel` | 0 | Build succeeded, including `pyqtgraph_cpp_visual_render_example`. |
| `QT_QPA_PLATFORM=offscreen ctest --preset visual -L P1.08 --output-on-failure` | 0 | `1/1 Test #27: P1.08.visual.SimplePlot` passed in 5.00s. |
| `QT_QPA_PLATFORM=offscreen PG_CPP_VISUAL_RENDERER="$PWD/build/visual/pyqtgraph_cpp_visual_render_example" PG_VISUAL_REPORTS_ROOT="$PWD/reports/visual/P1.08" python3 -m pytest -q tests/visual/test_P1_08_cpp_visual_renderer.py` | 0 | `2 passed in 4.03s`. |
| `QT_QPA_PLATFORM=offscreen build/visual/pyqtgraph_cpp_graphicsitems_plotcurveitem && QT_QPA_PLATFORM=offscreen build/visual/pyqtgraph_cpp_graphicsitems_plotcurveitem_setdata && QT_QPA_PLATFORM=offscreen build/visual/pyqtgraph_cpp_graphicsitems_plotitem && QT_QPA_PLATFORM=offscreen build/visual/pyqtgraph_cpp_widgets_plotwidget && QT_QPA_PLATFORM=offscreen build/visual/pyqtgraph_cpp_examples_simpleplot_test` | 0 | Related rebuilt executables passed. |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 1 | Existing proposed-issue metadata failures: blocked-by entries do not match local issues for multiple GitHub issue files, including `github-issue-112.md: ... P1.06`. |
| `git diff --check` | 0 | No whitespace errors. |
| `git diff --name-only origin/main...HEAD` | 0 | Branch diff lists the existing P1.08 wiring, visual harness/test, and report artifacts. |

LSP diagnostic attempt before the build found no diagnostics, but the C++ LSP client was not ready, so compile/build output is the authoritative C++ validation.

## Changed-file ownership check

Branch diff relative to `origin/main...HEAD`:

```text
CMakeLists.txt
reports/visual/P1.08/SimplePlot/actual.png
reports/visual/P1.08/SimplePlot/diff.png
reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md
reports/visual/P1.08/SimplePlot/metrics.json
reports/visual/P1.08/SimplePlot/reference.png
reports/visual/P1.08/completion.md
tests/visual/P1_08_render_cpp_example.cpp
tests/visual/test_P1_08_cpp_visual_renderer.py
```

Additional uncommitted rework paths in the current worktree:

```text
include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp
include/pyqtgraph/widgets/PlotWidget.hpp
src/pyqtgraph/graphicsItems/PlotCurveItem.cpp
src/pyqtgraph/graphicsItems/PlotItem/PlotItem.cpp
src/pyqtgraph/widgets/PlotWidget.cpp
```

The additional production rendering paths are outside the original issue-owned globs, but they are the smallest direct fix for the autoreview finding because the real widget grab otherwise renders only blank/placeholder output while `PlotCurveItem::paint` is empty.

## Manifest/dashboard status

Not applicable for manifest/dashboard files in this slice. The rework adds minimal production painting behavior needed to make the existing `SimplePlot` example capturable through the real native widget path; it does not add new manifest-tracked classes, examples, or assets.
