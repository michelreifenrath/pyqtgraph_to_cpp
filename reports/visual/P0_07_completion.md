# P0.07 completion evidence: Standardize visual artifact layout

Issue: #101 `[P0.07] Standardize visual artifact layout`

## Scope summary

Implemented and exercised the canonical visual artifact layout helper for local visual proof runs. The helper writes/validates these per-case artifact paths:

- `reports/visual-diffs/<case>/reference.png`
- `reports/visual-diffs/<case>/actual.png`
- `reports/visual-diffs/<case>/diff.png`
- `reports/visual-diffs/<case>/metrics.json`
- `reports/visual-diffs/<case>/gpt5_vision_review.md` when semantic review evidence is required/provided

## Changed files

Manifest-expanded target paths: none; this script-infra issue does not change tracked source, class, example, or asset manifest targets.

Shared wiring paths changed: none.

Issue-owned/supporting paths changed:

- `scripts/check_visual_artifacts`
- `tests/visual/test_P0_07_visual_artifact_layout.py`
- `tests/examples/test_SimplePlot_visual.py`
- `reports/visual/P0_07_completion.md`

## Focused proof coverage

- Normal fixture path: `test_P0_07_writes_canonical_visual_artifact_tree`
- Review-report path: `test_P0_07_required_gpt_review_is_copied_to_canonical_name`
- Failure path: `test_P0_07_required_gpt_review_without_report_exits_2`
- Case path escape failure path: `test_P0_07_rejects_case_paths_outside_reports_root`
- Visual mismatch path: `test_P0_07_mismatch_returns_1_and_preserves_artifacts`
- Command-mode child stdout isolation: `test_P0_07_command_runner_keeps_child_stdout_out_of_metrics_json`
- Fake-runner subprocess plan/cwd/env/order/stdout-stderr capture: `test_P0_07_command_runner_sets_env_and_runs_reference_before_actual`
- Fake-runner nonzero child exit propagation: `test_P0_07_command_runner_propagates_nonzero_child_exit`
- Command-mode custom-cwd absolute artifact paths: `test_P0_07_command_runner_exports_absolute_artifacts_for_custom_cwd`
- Real SSIM gate, not mean-delta surrogate: `test_P0_07_min_ssim_gate_uses_structural_similarity_not_mean_delta`
- Minimal fixture smoke through the canonical checker: `test_simpleplot_visual_oracle_generates_placeholder_diff_artifacts`

## Artifact paths verified by focused tests

The focused tests validate these canonical paths for fixture and command-mode cases:

- `<reports-root>/<case>/reference.png`
- `<reports-root>/<case>/actual.png`
- `<reports-root>/<case>/diff.png`
- `<reports-root>/<case>/metrics.json`
- `<reports-root>/<case>/gpt5_vision_review.md` when `--review-report` is supplied

The SimplePlot smoke test validates the same layout under a temporary `reports/visual-diffs/SimplePlot/` fixture root.

## Validation results

Final local validation after autoreview rework:

| Command | Exit code | Result |
| --- | ---: | --- |
| `python3 -m pytest -q tests/visual/test_P0_07_visual_artifact_layout.py -k command_runner` | 0 | `4 passed, 6 deselected in 0.08s` |
| `python3 -m pytest -q tests -k P0_07` | 0 | `10 passed, 259 deselected in 0.36s` |
| `python3 -m pytest -q tests/examples/test_SimplePlot_visual.py` | 0 | `4 passed in 3.81s` |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 1 | Fails on existing proposed-issue metadata: `github-issue-{96,100,101,102,105,108,129,166,195,197,207}.md: blocked-by entry does not match a local issue: P0.01` |
| `git diff --check` | 0 | No whitespace errors |
| `git diff --name-only origin/main...HEAD` | 0 | `reports/visual/P0_07_completion.md`; `scripts/check_visual_artifacts`; `tests/examples/test_SimplePlot_visual.py`; `tests/visual/test_P0_07_visual_artifact_layout.py` |

### `scripts/check_proposed_issues` failure output

```text
github-issue-96.md: blocked-by entry does not match a local issue: P0.01
github-issue-100.md: blocked-by entry does not match a local issue: P0.01
github-issue-101.md: blocked-by entry does not match a local issue: P0.01
github-issue-102.md: blocked-by entry does not match a local issue: P0.01
github-issue-105.md: blocked-by entry does not match a local issue: P0.01
github-issue-108.md: blocked-by entry does not match a local issue: P0.01
github-issue-129.md: blocked-by entry does not match a local issue: P0.01
github-issue-166.md: blocked-by entry does not match a local issue: P0.01
github-issue-195.md: blocked-by entry does not match a local issue: P0.01
github-issue-197.md: blocked-by entry does not match a local issue: P0.01
github-issue-207.md: blocked-by entry does not match a local issue: P0.01
```

## Manifest/dashboard status

Not applicable. This issue adds script/test infrastructure for visual artifact layout and does not change manifest-tracked C++ sources, classes, examples, or assets.
