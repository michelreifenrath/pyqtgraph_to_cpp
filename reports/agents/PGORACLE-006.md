# PGORACLE-006 Implementation Report

## Summary
- Added the first `SimplePlot` visual oracle fixture and focused pytest coverage.
- Added a committed 800x600 PyQtGraph reference screenshot for `SimplePlot`.
- Rework regenerated the reference from the pinned upstream `SimplePlot.py` render instead of the prior blank/window-chrome capture.
- Added a JSON/YAML-compatible no-op interaction fixture for `SimplePlot`.
- The focused test composes the existing C++ placeholder renderer and screenshot comparator, verifies deterministic placeholder mismatch metrics, verifies the reference image passes when compared to itself, and now guards the reference for visible SimplePlot content beyond dimensions.
- Rework added the narrow `scripts/gate visual SimplePlot` wiring required by the issue validation contract.

## Files Changed
- `oracle/fixtures/screenshots/SimplePlot.reference.png` - 800x600 PyQtGraph reference screenshot.
- `oracle/fixtures/interactions/SimplePlot.json` - empty `version: 1` interaction script.
- `tests/examples/test_SimplePlot_visual.py` - focused visual-oracle pytest with semantic reference-content guard.
- `scripts/gate` - narrow `visual SimplePlot` gate wiring to the focused pytest.
- `tests/test_gate_scripts.py` - focused coverage for the new visual gate mode.
- `reports/agents/PGORACLE-006.md` - this implementation report.

## Reference Fixture
- Upstream source: `pyqtgraph/examples/SimplePlot.py` from PyQtGraph 0.14.0, pinned commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- Source verification: local PyPI `pyqtgraph==0.14.0` example content matched the pinned GitHub raw file.
- Render command used a Python driver with `QT_QPA_PLATFORM=offscreen`, `numpy.random.seed(0)`, `runpy.run_path(...)` on the installed upstream `SimplePlot.py`, explicit capture of the example's `plt` widget, `800x600` resize, and a two-second Qt event drain before `widget.grab().save(...)`.
- The explicit `plt` capture avoids selecting unrelated top-level window chrome and produced a 11,203-byte PNG with 447 unique RGB colors, 296,629 dark plot-region pixels, and 10,571 bright plot-region pixels.
- Qt platform: offscreen.

## Validation
- `python3 -m pytest tests/test_gate_scripts.py::test_gate_help_lists_required_modes tests/test_gate_scripts.py::test_gate_dry_run_command_plans tests/test_gate_scripts.py::test_gate_visual_dry_run_targets_example_pytest tests/test_gate_scripts.py::test_gate_visual_requires_example_name tests/test_gate_scripts.py::test_gate_visual_requires_known_target -q` - passed, exit code 0 (`5 passed`).
- `python3 -m pytest tests/examples/test_SimplePlot_visual.py -q` - passed, exit code 0 (`4 passed`).
- `scripts/gate visual SimplePlot --dry-run` - passed, exit code 0; planned `python3 -m pytest tests/examples/test_SimplePlot_visual.py -q`.
- `scripts/gate visual SimplePlot --reports-dir /tmp/pgoracle-006-gate-reports` - passed, exit code 0.
- `python3 -m pytest -q` - passed, exit code 0 (`232 passed`).
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - passed, exit code 0 (`workflow valid: WORKFLOW.md`).
- `git diff --check` - passed, exit code 0.
- Reference fixture semantic content guard - passed; PNG dimensions are 800x600, unique RGB colors exceed 100, dark plot-region pixels exceed 100,000, and bright plot-region pixels exceed 1,000.
- Pinned-source verification probe - passed, installed PyQtGraph 0.14.0 `SimplePlot.py` matched commit `a20028b98294b9cc8770f2015a92eb342224b788`.

## Notes / Limitations
- `render_cpp_example.py` still emits a deterministic placeholder image for `SimplePlot`; the reference-vs-placeholder comparison is expected to fail with deterministic metrics until the real C++ example renderer exists.
- The focused pytest writes visual-diff artifacts under pytest `tmp_path` using the standard `reports/visual-diffs/SimplePlot/` layout to avoid leaving unowned generated report artifacts in the repository diff.
- No `port_manifest.yaml` or `CMakeLists.txt` changes were needed; `SimplePlot` is already inventoried and pytest discovers the new test directly.
- `scripts/gate`/`tests/test_gate_scripts.py` changes are limited to the directly required visual validation wiring from the rework finding.
- No PR was opened by Pi per instruction.
