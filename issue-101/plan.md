# Implementation Plan

## Goal
Add issue-scoped, test-driven visual artifact layout validation for P0.07 so focused visual runs produce the canonical screenshot/diff/metrics contract and the visual gate command runner is verified without broad rendering or workflow changes.

## Tasks
1. **Establish the focused red baseline**: Confirm the current issue selector has no passing P0.07 implementation before adding code.
   - File: none
   - Changes: Run `python3 -m pytest -q tests -k P0_07 --collect-only` and record that no P0_07 tests currently collect, matching the scout finding.
   - Acceptance: Command exits with pytest collection failure/no tests collected; this is the pre-test baseline, not the final red proof.

2. **Add the first failing artifact-contract test**: Create a P0.07-specific pytest that calls the intended visual artifact script and asserts canonical layout and metrics.
   - File: `tests/visual/test_P0_07_visual_artifact_layout.py`
   - Changes: Add helpers similar to `tests/visual/test_compare_screenshots.py` for writing tiny PNG fixtures, then add `test_P0_07_writes_canonical_visual_artifact_tree` that:
     - creates temporary `reference-source.png` and `actual-source.png` images;
     - runs `scripts/check_visual_artifacts --case P0_07_SimplePlot --reference <ref> --actual <actual> --reports-root <tmp>/reports/visual-diffs --gpt-visual-review not_applicable`;
     - asserts `<reports-root>/P0_07_SimplePlot/reference.png`, `actual.png`, `diff.png`, and `metrics.json` exist;
     - asserts `gpt5_vision_review.md` is absent for `not_applicable`;
     - asserts `metrics.json` contains at least `case`, flat `dimensions` `[width, height]`, `mean_abs_delta`, `max_delta`, `changed_pixel_percent`, `ssim`, `tolerance`, and `deterministic_verdict` as documented in `docs/pyqtgraph-cpp-port-workflow.md:422-452`.
   - Acceptance: Running `python3 -m pytest -q tests -k P0_07` fails because `scripts/check_visual_artifacts` does not exist or does not yet implement the required CLI/layout. This is the expected red proof.

3. **Add failing tests for review-file policy and failure propagation**: Cover the contract branches that should not require real GPT or broad visual rendering.
   - File: `tests/visual/test_P0_07_visual_artifact_layout.py`
   - Changes: Add tests that:
     - run with `--gpt-visual-review required_for_pr --review <existing-review.md>` and assert the script copies or writes `gpt5_vision_review.md` into the case directory;
     - run with `--gpt-visual-review required_for_pr` and no review file and assert exit code `2` plus a useful stderr message;
     - use intentionally different images and strict tolerances to assert deterministic comparison failure returns `1` while still writing `reference.png`, `actual.png`, `diff.png`, and `metrics.json`.
   - Acceptance: These tests fail for the same missing/unimplemented script before production code is added.

4. **Add failing visual gate runner tests**: Verify visual command execution behavior with a fake subprocess runner, as P0.07 requires command-runner assertions beyond dry-run.
   - File: `tests/test_gate_scripts.py`
   - Changes: Add P0_07-named tests near the existing visual gate tests:
     - `test_P0_07_gate_visual_runs_planned_command_from_repo_root_and_writes_summary`, using `monkeypatch` to replace `scripts.gate.run_gate_command` or a newly injectable runner if needed, then asserting command order, `cwd == workflow.repo_root`, timeout, summary JSON, and logs;
     - `test_P0_07_gate_visual_propagates_child_failure`, faking a nonzero `CommandResult` and asserting the gate returns that code, marks summary `failed`, and records `failed_command`.
   - File: `scripts/gate`
   - Changes: If direct monkeypatching is awkward because tests currently execute the script in a subprocess, minimally refactor `main` to accept an optional runner callable or add a small internal `run_commands(...)` function that can be imported and unit-tested. Do not change the public CLI or `VISUAL_COMMANDS` behavior unless the test proves it is necessary.
   - Acceptance: Running `python3 -m pytest -q tests -k P0_07` now includes gate failures showing the missing injectable/testable runner behavior.

5. **Implement the minimal visual artifact script**: Add an issue-scoped script rather than changing the existing comparator schema used by current tests.
   - File: `scripts/check_visual_artifacts`
   - Changes: Implement an executable Python script with CLI options:
     - required: `--case`, `--reference`, `--actual`;
     - optional: `--reports-root` defaulting to `reports/visual-diffs`, `--gpt-visual-review` choices `not_applicable`/`required_for_pr` defaulting to `not_applicable`, `--review`, and tolerance flags aligned with comparator inputs (`--max-mean-delta`, `--max-pixel-delta`, `--max-changed-percent`, plus a canonical `--min-ssim` value recorded in metrics);
     - create `<reports-root>/<case>/`;
     - copy reference and actual inputs to `reference.png` and `actual.png` in that directory;
     - call or import the existing `oracle/scripts/compare_screenshots.py` comparison logic with `diff.png` and an internal/raw metrics object;
     - write canonical `metrics.json` with the documented field names while preserving `deterministic_verdict` and enough path/tolerance detail for debugging;
     - set `ssim` to `1.0` for exact matches and a simple deterministic placeholder/derived value for non-identical images only if a real SSIM implementation is out of scope; record `min_ssim` under `tolerance` so the schema is present;
     - enforce review policy: no review file for `not_applicable`; copy the provided review file to `gpt5_vision_review.md` for `required_for_pr`; exit `2` if required and missing.
   - Acceptance: `python3 -m pytest -q tests/visual/test_P0_07_visual_artifact_layout.py` passes. Existing comparator tests still pass because `oracle/scripts/compare_screenshots.py` schema remains unchanged.

6. **Keep existing SimplePlot smoke aligned only if needed**: Reuse the new script in the existing SimplePlot visual smoke if the P0.07 tests or acceptance require the real fixture path to use the contract.
   - File: `tests/examples/test_SimplePlot_visual.py`
   - Changes: Prefer no change. If necessary, replace the manual comparator subprocess call with `scripts/check_visual_artifacts --case SimplePlot --reference <reference> --actual <actual> --reports-root <tmp>/reports/visual-diffs --gpt-visual-review not_applicable`, then keep the same assertions for `reference.png`, `actual.png`, `diff.png`, and `metrics.json`.
   - Acceptance: `python3 -m pytest -q tests/examples/test_SimplePlot_visual.py` passes and still uses temporary artifacts, not repository reports.

7. **Complete the visual gate runner implementation/refactor**: Make the tests from task 4 pass without altering the visual command target.
   - File: `scripts/gate`
   - Changes: Extract the non-dry-run command loop into a testable function or allow `main(argv, runner=run_gate_command)` while preserving subprocess behavior for normal CLI execution. Ensure logs and summary JSON remain byte-compatible with existing tests where possible.
   - Acceptance: P0.07 gate tests pass and existing gate tests remain green: `python3 -m pytest -q tests/test_gate_scripts.py -k 'gate_visual or P0_07'`.

8. **Run focused and safety validation**: Verify the implementation within the issue-owned scope and check for accidental scope expansion.
   - File: none
   - Changes: Run focused commands:
     - `python3 -m pytest -q tests -k P0_07`
     - `python3 -m pytest -q tests/visual/test_compare_screenshots.py`
     - `python3 -m pytest -q tests/examples/test_SimplePlot_visual.py`
     - `python3 -m pytest -q tests/test_gate_scripts.py -k 'gate_visual or P0_07'`
     - `scripts/gate visual SimplePlot --dry-run`
     - `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp`
     - `git diff --check`
     - `git diff --name-only origin/main...HEAD`
   - Acceptance: Focused tests pass, dry-run still prints `python3 -m pytest tests/examples/test_SimplePlot_visual.py -q`, diff check is clean, and changed files are limited to the issue-owned test/script/report paths.

## Files to Modify
- `tests/visual/test_P0_07_visual_artifact_layout.py` - new focused red/green tests for canonical visual artifact layout, metrics schema, review-file policy, and comparator failure artifact persistence.
- `scripts/check_visual_artifacts` - new executable visual artifact contract/check script that writes `reports/visual-diffs/<case>/`-style output and canonical metrics.
- `tests/test_gate_scripts.py` - add P0.07-specific tests for visual gate command runner cwd/order/summary and nonzero propagation.
- `scripts/gate` - only if needed, minimal testability refactor for the command runner; preserve public CLI and existing visual command mapping.
- `tests/examples/test_SimplePlot_visual.py` - optional; update only if acceptance requires the existing SimplePlot smoke to use the new contract script.

## New Files
- `tests/visual/test_P0_07_visual_artifact_layout.py` - focused TDD tests selected by `python3 -m pytest -q tests -k P0_07`.
- `scripts/check_visual_artifacts` - reusable local visual artifact layout/check script under `scripts/**visual*` ownership.

## Dependencies
- Task 2 must be done before task 5 to preserve TDD red/green proof.
- Task 3 depends on the same new test file from task 2.
- Task 4 must precede task 7 to prove the gate behavior gap before refactoring.
- Task 5 depends on existing comparator behavior in `oracle/scripts/compare_screenshots.py`; avoid changing comparator schema unless the script cannot safely wrap it.
- Task 6 is optional and should be attempted only after task 5 is stable.
- Task 8 depends on all implementation tasks and should be the final validation pass.

## Risks
- The provided `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-101/context.md` path was absent during planning; this plan relies on `issue-101/scout-context.md` and direct reads of the relevant files.
- There is a path-scope ambiguity: canonical docs require `reports/visual-diffs/<case>/`, while issue adjunct guidance mentions `reports/visual/<issue-id>/**`. For implementation, use `reports/visual-diffs/<case>/` for generated visual diff artifacts because it is the canonical workflow contract; keep any issue completion notes, if required, under `reports/visual/P0.07/**` only if the issue explicitly owns/requests them.
- Do not change `oracle/scripts/compare_screenshots.py` metrics names unless unavoidable; existing tests assert the current schema exactly. A wrapper script is safer and narrower.
- Real GPT-5.5 semantic visual review generation is out of scope for a local deterministic script. The script should validate/copy an externally provided review when `required_for_pr`, not invent semantic review content.
- Adding CMake/CTest visual labels is likely out of scope for P0.07 unless explicitly required by the issue; do not touch `CMakeLists.txt`, `tests/CMakeLists.txt`, or presets based only on the scout note.
- Avoid writing persistent visual artifacts into repository `reports/visual-diffs/` during tests; use `tmp_path` reports roots. Only completion/report artifacts should be persisted if explicitly requested by the issue owner.
- No commits, pushes, PRs, workflow edits, broad rendering changes, or source/example behavior changes should be made by the writer unless a focused failing P0.07 test requires them.
