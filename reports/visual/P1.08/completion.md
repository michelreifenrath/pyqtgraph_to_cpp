# P1.08 completion evidence: Replace placeholder C++ visual renderer

Issue: #112 `[P1.08] Replace placeholder C++ visual renderer`

## Scope summary

Added a native C++/Qt SimplePlot visual renderer harness and a focused visual CTest tagged `P1.08`. The proof generates canonical local visual artifacts under `reports/visual/P1.08/SimplePlot/` using the pinned PyQtGraph reference screenshot and a native C++ actual screenshot. The existing placeholder oracle script was not modified.

## Changed files

Manifest-expanded target paths: none; this issue adds a focused visual-render proof/harness and does not update manifest-tracked source, class, example, or asset records.

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

## TDD red result

| Command | Exit code | Result |
| --- | ---: | --- |
| `QT_QPA_PLATFORM=offscreen python3 -m pytest -q tests/visual/test_P1_08_cpp_visual_renderer.py` | 1 | Expected pre-implementation failure: `PG_CPP_VISUAL_RENDERER must point to the native renderer executable`; `1 failed, 1 passed`. |

## Artifact summary

Canonical visual artifacts:

- `reports/visual/P1.08/SimplePlot/reference.png`
- `reports/visual/P1.08/SimplePlot/actual.png`
- `reports/visual/P1.08/SimplePlot/diff.png`
- `reports/visual/P1.08/SimplePlot/metrics.json`
- `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md`

`metrics.json` records:

- dimensions: `[800, 600]`
- mean absolute delta: `3.8896223958333334`
- max delta: `200`
- changed pixel percent: `2.986041666666667`
- SSIM: `0.8432871186697161`
- tolerances: max mean `6.0`, max pixel `220.0`, max changed percent `5.0`, min SSIM `0.8`
- deterministic verdict: `pass`
- failed checks/tolerances: `[]`

Blank/placeholder guard coverage is in `tests/visual/test_P1_08_cpp_visual_renderer.py::test_P1_08_blank_and_placeholder_guards_reject_non_semantic_images` and the focused CTest path.

Manual semantic inspection is recorded in `reports/visual/P1.08/SimplePlot/manual_semantic_inspection.md` after opening/reading the reference, actual, and diff images with an image-capable tool.

## Validation results

Final local validation:

| Command | Exit code | Result |
| --- | ---: | --- |
| `cmake --preset visual` | 0 | Configure succeeded; build files written to `build/visual`. |
| `cmake --build --preset visual --parallel` | 0 | Build succeeded, including `pyqtgraph_cpp_visual_render_example`. |
| `QT_QPA_PLATFORM=offscreen ctest --preset visual -L P1.08 --output-on-failure` | 0 | `1/1 Test #27: P1.08.visual.SimplePlot` passed in 4.28s. |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 1 | Existing proposed-issue metadata failures: blocked-by entries do not match local issues for multiple GitHub issue files, including `github-issue-112.md: ... P1.06`. |
| `git diff --check` | 0 | No whitespace errors. |
| `git diff --name-only origin/main...HEAD` | 0 | No output because this handoff leaves an uncommitted worktree diff as requested. |

Rework validation after the pytest gate finding:

| Command | Exit code | Result |
| --- | ---: | --- |
| `python3 -m pytest -q tests/visual/test_P1_08_cpp_visual_renderer.py` | 0 | `1 passed, 1 skipped`; direct pytest now skips only the native-renderer integration when `PG_CPP_VISUAL_RENDERER` is absent. |
| `python3 -m pytest -q` | 0 | `302 passed, 1 skipped`; fixes the reported validation failure. |
| `cmake --preset visual` | 0 | Configure succeeded; build files written to `build/visual`. |
| `cmake --build --preset visual --parallel` | 0 | Build succeeded, including `pyqtgraph_cpp_visual_render_example`. |
| `QT_QPA_PLATFORM=offscreen ctest --preset visual -L P1.08 --output-on-failure` | 0 | `1/1 Test #27: P1.08.visual.SimplePlot` passed in 4.61s. |
| `git diff --check` | 0 | No whitespace errors. |

Additional uncommitted changed-file ownership check:

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

All modified paths match issue-owned `tests/visual/**`, `reports/visual/P1.08/**`, or the allowed shared wiring path `CMakeLists.txt`.

## Manifest/dashboard status

Not applicable. This issue does not change manifest-tracked source classes, examples, or assets; it adds focused visual proof infrastructure and generated local evidence.
