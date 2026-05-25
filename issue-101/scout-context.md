# Code Context

## Files Retrieved
1. `docs/pyqtgraph-cpp-port-workflow.md` (lines 404-493) - canonical visual validation levels, required `reports/visual-diffs/<case>/` layout, metrics schema, GPT review contract, and candidate taxonomy.
2. `docs/proposed-issues/VALIDATION-GUIDE.md` (lines 24-35, 53-65) - owned-file adjunct definitions and script/visual proof rules relevant to P0.07.
3. `oracle/scripts/compare_screenshots.py` (lines 225-388) - current visual diff CLI: reads two PNGs, writes diff PNG and metrics JSON, returns 0/1/2.
4. `tests/visual/test_compare_screenshots.py` (lines 85-270) - focused comparator tests for normal, tolerance-failure, dimension-failure, and PNG validation paths.
5. `tests/examples/test_SimplePlot_visual.py` (lines 144-206) - current fixture smoke that writes `reference.png`, `actual.png`, `diff.png`, `metrics.json` under a temp `reports/visual-diffs/SimplePlot/` layout.
6. `scripts/gate` (lines 20-92, 150-229) - current visual gate command mapping and command-runner report behavior.
7. `tests/test_gate_scripts.py` (lines 316-360) - current visual gate tests; dry-run target and argument errors only.
8. `CMakePresets.json` (lines 81-170) - visual configure/build/test presets; test preset filters CTest label `visual` and ignores no tests.
9. `oracle/scripts/render_cpp_example.py` (lines 1-132) - placeholder C++ example renderer that writes an `actual` PNG and JSON status.
10. `oracle/scripts/render_pyqtgraph_example.py` (lines 27-163) - PyQtGraph example renderer that writes a reference PNG offscreen.
11. `WORKFLOW.md` (lines 90-133) - configured repo validation is `python3 -m pytest -q`; safety gates require `git diff --check` and `scripts/gate commit`.
12. `reports/agents/PGORACLE-006.md` (lines 1-43) - prior visual-oracle report and notes on current placeholder behavior and temp artifact layout.
13. `docs/examples/SimplePlot.md` (lines 1-38) - SimplePlot visual-required status and current placeholder visual gate limitation.

## Key Code

### Canonical visual layout vs current comparator output
`docs/pyqtgraph-cpp-port-workflow.md` defines the required layout:

```text
reports/visual-diffs/<case>/
  reference.png
  actual.png
  diff.png
  metrics.json
  gpt5_vision_review.md        # when gpt_visual_review != not_applicable
```

It also says `metrics.json` should include fields like `case`, flat `dimensions`, `mean_abs_delta`, `changed_pixel_percent`, `ssim`, `tolerance`, and `deterministic_verdict` (`docs/pyqtgraph-cpp-port-workflow.md:422-452`).

Current `oracle/scripts/compare_screenshots.py` only compares caller-supplied paths and writes caller-supplied `--diff`/`--metrics` defaults of `reports/visual-diffs/diff.png` and `reports/visual-diffs/metrics.json` (`oracle/scripts/compare_screenshots.py:333-349`). Its metrics keys currently include `reference_path`, `candidate_path`, `diff_image_path`, nested `dimensions`, `mean_absolute_delta`, `changed_pixel_percentage`, `tolerances`, `passed`, and no `case`, `actual_path`, `review` path, `ssim`, or canonical per-case directory enforcement (`oracle/scripts/compare_screenshots.py:234-243, 320-329`).

### Existing focused visual tests
`tests/visual/test_compare_screenshots.py` exercises comparator behavior directly via subprocess (`tests/visual/test_compare_screenshots.py:85-101`):
- identical images pass and write metrics/diff (`lines 104-153`);
- tolerance pass/fail (`lines 156-222`);
- dimension mismatch failure (`lines 225-249`);
- oversized PNG rejection starts at `lines 252-270`.

No `P0_07` tests exist yet: `python3 -m pytest -q tests -k P0_07 --collect-only` returned exit 5 with `no tests collected (259 deselected)`.

### Existing fixture smoke
`tests/examples/test_SimplePlot_visual.py:144-206` creates a temp `reports/visual-diffs/SimplePlot/` directory with `reference.png`, `actual.png`, `diff.png`, and `metrics.json`, runs the placeholder C++ renderer, then runs the comparator. This proves layout manually in a test but not via a reusable layout/check tool and does not produce `gpt5_vision_review.md`.

### Existing gate wiring
`scripts/gate` has `VISUAL_COMMANDS = {"SimplePlot": ["python3 -m pytest tests/examples/test_SimplePlot_visual.py -q"]}` (`scripts/gate:32-34`). Visual mode validates target syntax and maps to that command (`scripts/gate:37-54, 81-92`). Non-dry-run writes command logs and a summary JSON under `--reports-dir`, propagating nonzero exit codes (`scripts/gate:174-229`).

Current gate tests cover dry-run and invalid visual arguments only (`tests/test_gate_scripts.py:316-360`). They do **not** fake-run subprocess execution for visual mode, assert cwd/env/order, or assert nonzero propagation for a visual child command, which P0.07 acceptance explicitly requires for command runners.

## Architecture

Visual validation currently has three loose layers:

1. **Renderers**: `oracle/scripts/render_pyqtgraph_example.py` can render a PyQtGraph reference screenshot offscreen; `oracle/scripts/render_cpp_example.py` currently writes a deterministic placeholder C++ screenshot and JSON status.
2. **Comparator**: `oracle/scripts/compare_screenshots.py` compares two PNGs and writes a diff PNG plus metrics JSON. It is path-driven, not case/layout-driven.
3. **Tests/gate**: `tests/examples/test_SimplePlot_visual.py` manually assembles a standard-looking temp artifact tree for SimplePlot. `scripts/gate visual SimplePlot` runs that pytest. CMake has a visual preset filtering label `visual`, but no CTest-labeled visual test is evident in `CMakeLists.txt`/`tests/CMakeLists.txt` grep results; pytest visual tests are not registered with CTest.

Report/doc paths are inconsistent with P0.07 ownership:
- canonical docs and existing tests use `reports/visual-diffs/<case>/`;
- issue #101 owned globs mention `reports/visual/**` and `focused-visual` mentions `reports/visual/<issue-id>/**`;
- repository currently has `reports/visual-diffs/.gitkeep` and no `reports/visual/` directory.

## Current Behavior Gaps

- No focused `P0_07` test exists; the issue validation selector currently collects nothing.
- No dedicated visual artifact layout/check script was found under `scripts/**visual*`; existing tooling is in `oracle/scripts/*` and `scripts/gate`.
- Comparator defaults are not per-case (`reports/visual-diffs/diff.png`, `metrics.json`) and do not automatically create/validate the full required per-case tree.
- Current metrics schema does not match the canonical documented example names (`mean_absolute_delta` vs `mean_abs_delta`, `changed_pixel_percentage` vs `changed_pixel_percent`, `tolerances` vs `tolerance`) and lacks `case`/`ssim`.
- No tooling writes or validates `gpt5_vision_review.md`; only docs specify it when GPT visual review is applicable.
- Visual gate tests are dry-run/argument focused and miss the P0.07 required fake-runner assertions for subprocess plan/order/cwd/env and nonzero exit propagation.
- CMake visual preset exists but likely no `visual`-labeled CTest tests are registered yet; grep found many `add_test` calls but no `LABELS visual`.

## Likely Focused Test Locations

- Best primary location: `tests/visual/test_P0_07_visual_artifact_layout.py` or `tests/visual/test_visual_artifact_layout.py` with test names containing `P0_07` so `python3 -m pytest -q tests -k P0_07` selects only this issue.
- If changing `scripts/gate` visual command-runner behavior, add P0.07-specific tests in `tests/test_gate_scripts.py` only for the shared wiring/fake-runner requirement.
- If adding a script under owned `scripts/**visual*`, likely names: `scripts/check_visual_artifacts`, `scripts/visual_artifacts`, or similar; keep tests in `tests/visual/**`.
- Completion artifacts, if any, should likely be under `reports/visual/P0.07/**` per issue-owned globs, but this conflicts with existing canonical `reports/visual-diffs/<case>/` docs/tests and should be resolved by implementation scope or issue update.

## Commands to Run for P0_07

Issue #101 validation commands:

```sh
python3 -m pytest -q tests -k P0_07
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
git diff --check
git diff --name-only origin/main...HEAD
```

Useful focused baseline/current checks:

```sh
python3 -m pytest -q tests/visual/test_compare_screenshots.py
python3 -m pytest -q tests/examples/test_SimplePlot_visual.py
python3 -m pytest -q tests/test_gate_scripts.py::test_gate_visual_dry_run_targets_example_pytest tests/test_gate_scripts.py::test_gate_visual_requires_example_name tests/test_gate_scripts.py::test_gate_visual_requires_known_target
scripts/gate visual SimplePlot --dry-run
```

If CMake wiring is added for visual tests, also consider:

```sh
cmake --preset visual
cmake --build --preset visual --parallel
QT_QPA_PLATFORM=offscreen ctest --preset visual --output-on-failure
```

## Start Here
Open `tests/visual/test_compare_screenshots.py` first. It is already under P0.07-owned `tests/visual/**`, contains the existing comparator test helpers and failure-path patterns, and can host the first failing `P0_07` proof without touching production source.
