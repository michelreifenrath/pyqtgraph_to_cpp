# P1.08 completion evidence: Replace placeholder C++ visual renderer

Issue: #112 `[P1.08] Replace placeholder C++ visual renderer`

## Scope summary

Added a native C++/Qt SimplePlot visual renderer harness and a focused visual CTest tagged `P1.08`. The renderer captures the actual native `SimplePlot` widget with `QWidget::grab`, so the visual proof runs through `PlotWidget` → `GraphicsScene` → `PlotItem::paint` → `PlotCurveItem::paint` instead of a placeholder or manual image path.

This bounded rework addresses the latest autoreview findings only:

- `PlotItem` x-axis ticks are now generated from the rendered curve data bounds instead of the SimplePlot-specific `0..100` range.
- y-axis tick generation now uses a pixel-budget-derived, capped tick helper instead of a `0.1` data-unit loop over the whole range.
- the P1.08 visual test now requires an existing semantic review report (`PG_VISUAL_REVIEW_REPORT` or the canonical `reports/visual/P1.08/SimplePlot/gpt5_vision_review.md`) instead of generating a passing report inside the gate.
- `scripts/gate visual SimplePlot` now runs the native visual preset/CTest renderer path instead of the placeholder oracle pytest.
- the configured P1.08 CTest now writes generated reference/actual/diff/metrics artifacts to the visual build tree (`build/visual/reports/visual/P1.08`) and reads the committed GPT review report from `reports/visual/P1.08/SimplePlot/gpt5_vision_review.md`, so validation no longer rewrites tracked source artifacts.

## Changed files

Manifest-expanded target paths: none.

Shared wiring paths changed:

- `CMakeLists.txt` — registers `pyqtgraph_cpp_visual_render_example` and `P1.08.visual.SimplePlot` with labels `visual;P1.08`; CTest writes generated visual artifacts under the build tree and reads the committed GPT review report as input.
- `scripts/gate` — maps the documented `visual SimplePlot` gate to the native visual preset and P1.08 CTest.
- `tests/test_gate_scripts.py` — updates the visual gate dry-run expectation for the native renderer path.

Issue-owned/supporting paths changed:

- `tests/visual/P1_08_render_cpp_example.cpp`
- `tests/visual/test_P1_08_cpp_visual_renderer.py`
- `reports/visual/P1.08/SimplePlot/reference.png`
- `reports/visual/P1.08/SimplePlot/actual.png`
- `reports/visual/P1.08/SimplePlot/diff.png`
- `reports/visual/P1.08/SimplePlot/metrics.json`
- `reports/visual/P1.08/SimplePlot/gpt5_vision_review.md`
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

## Artifact summary

Committed canonical visual evidence:

- `reports/visual/P1.08/SimplePlot/reference.png`
- `reports/visual/P1.08/SimplePlot/actual.png`
- `reports/visual/P1.08/SimplePlot/diff.png`
- `reports/visual/P1.08/SimplePlot/metrics.json`
- `reports/visual/P1.08/SimplePlot/gpt5_vision_review.md`
- `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md`

Configured CTest/gate runs now generate fresh artifacts in `build/visual/reports/visual/P1.08/SimplePlot/` instead of rewriting the committed evidence files.

`metrics.json` records:

- dimensions: `[800, 600]`
- mean absolute delta: `3.7998421875`
- max delta: `200`
- changed pixel percent: `2.911458333333333`
- SSIM: `0.8482026901800664`
- tolerances: max mean `6.0`, max pixel `220.0`, max changed percent `5.0`, min SSIM `0.8`
- deterministic verdict: `pass`
- semantic review: `required_for_pr`, verdict `pass`, recommendation `merge_ok`, accepted `true`
- failed checks/tolerances: `[]`

Blank/placeholder guard coverage is in `tests/visual/test_P1_08_cpp_visual_renderer.py::test_P1_08_blank_and_placeholder_guards_reject_non_semantic_images` and the focused CTest path.

Manual semantic inspection is recorded in `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md` after opening/reading the regenerated reference, actual, and diff images with an image-capable tool.

## Validation results

Final local validation after bounded rework:

| Command | Exit code | Result |
| --- | ---: | --- |
| `cmake --preset visual` | 0 | Configure succeeded; build files written to `build/visual`. |
| `cmake --build --preset visual --target pyqtgraph_cpp_visual_render_example --parallel` | 0 | Build succeeded, including `pyqtgraph_cpp_visual_render_example`. |
| `scripts/gate visual SimplePlot --dry-run` | 0 | Dry-run now lists `cmake --preset visual`, native renderer target build, and `ctest --preset visual -R '^P1\\.08\\.visual\\.SimplePlot$' --output-on-failure`. |
| `python3 -m pytest -q tests/test_gate_scripts.py` | 0 | `22 passed in 10.87s`, including `test_gate_visual_dry_run_targets_native_renderer_ctest`. |
| `QT_QPA_PLATFORM=offscreen ctest --preset visual -R '^P1\\.08\\.visual\\.SimplePlot$' --output-on-failure` | 0 | `1/1 Test #27: P1.08.visual.SimplePlot` passed in `4.76 sec`; generated artifacts were written to `build/visual/reports/visual/P1.08/SimplePlot/`. |
| `QT_QPA_PLATFORM=offscreen scripts/gate visual SimplePlot --reports-dir /tmp/p1-08-visual-gate-reports` | 0 | Gate summary status `passed`; configure, native renderer build, and native P1.08 CTest each returned `0`; generated artifacts remained under the build tree. |
| `QT_QPA_PLATFORM=offscreen ctest --preset visual -L P1.08 --output-on-failure` | 0 | `1/1 Test #27: P1.08.visual.SimplePlot` passed in `3.56 sec`. |
| `QT_QPA_PLATFORM=offscreen PG_CPP_VISUAL_RENDERER="$PWD/build/visual/pyqtgraph_cpp_visual_render_example" PG_VISUAL_REPORTS_ROOT="$PWD/build/visual/reports/visual/P1.08" PG_VISUAL_REVIEW_REPORT="$PWD/reports/visual/P1.08/SimplePlot/gpt5_vision_review.md" python3 -m pytest -q tests/visual/test_P1_08_cpp_visual_renderer.py` | 0 | `2 passed in 3.37s`; the test requires the existing canonical semantic review report instead of generating one and writes configured artifacts outside tracked source files. |
| `python3 -m pytest -q tests/visual/test_P0_07_visual_artifact_layout.py` | 0 | `20 passed in 3.57s`. |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 1 | Existing proposed-issue metadata failures: blocked-by entries do not match local issues for multiple GitHub issue files, including `github-issue-112.md: ... P1.06`. |
| `git diff --check` | 0 | No whitespace errors. |
| `git diff --name-only origin/main...HEAD` | 0 | Branch diff lists the committed P1.08 wiring, visual harness/test, renderer support paths, and report artifacts. |
| `git diff --name-only` | 0 | Current bounded rework diff lists `CMakeLists.txt` and `reports/visual/P1.08/completion.md`. |

LSP diagnostic attempt before the build found no diagnostics, but the C++ LSP client was not ready, so compile/build output is the authoritative C++ validation.

## Changed-file ownership check

Branch diff relative to `origin/main...HEAD` plus the current bounded rework diff:

```text
CMakeLists.txt
include/pyqtgraph/graphicsItems/PlotItem/PlotItem.hpp
include/pyqtgraph/widgets/PlotWidget.hpp
reports/visual/P1.08/SimplePlot/actual.png
reports/visual/P1.08/SimplePlot/diff.png
reports/visual/P1.08/SimplePlot/gpt5_vision_review.md
reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md
reports/visual/P1.08/SimplePlot/metrics.json
reports/visual/P1.08/SimplePlot/reference.png
reports/visual/P1.08/completion.md
scripts/gate
src/pyqtgraph/graphicsItems/PlotCurveItem.cpp
src/pyqtgraph/graphicsItems/PlotItem/PlotItem.cpp
src/pyqtgraph/widgets/PlotWidget.cpp
tests/test_gate_scripts.py
tests/visual/P1_08_render_cpp_example.cpp
tests/visual/test_P1_08_cpp_visual_renderer.py
```

The production rendering paths are outside the original issue-owned globs, but they are the smallest direct fix for the autoreview findings because the real widget grab otherwise renders only blank/placeholder output or SimplePlot-specific axes.

## Manifest/dashboard status

Not applicable for manifest/dashboard files in this slice. The rework adds minimal production painting behavior needed to make the existing `SimplePlot` example capturable through the real native widget path; it does not add new manifest-tracked classes, examples, or assets.
