# P1.09 changed examples runner

## Summary
- Added `scripts/run_changed_examples` for changed local example selection and CTest dispatch.
- Current manifest-expanded local target mapping: `examples/SimplePlot.cpp` -> `pyqtgraph_cpp.examples.SimplePlot`.
- Changed manifest-expanded target paths beyond the existing SimplePlot mapping: none.
- Changed shared wiring paths: none.
- Manifest/dashboard update status: not applicable; no tracked source/example files were changed.

## Validation
- Initial TDD check before implementation: `python3 -m pytest -q tests -k P1_09` exited 1 with 4 expected failures because `scripts/run_changed_examples` did not exist.
- Final focused tests: `python3 -m pytest -q tests -k P1_09` exited 0 (`4 passed, 303 deselected`).
- CLI smoke: `python3 scripts/run_changed_examples --help` exited 0.
- Dry-run smoke: `python3 scripts/run_changed_examples --dry-run SimplePlot` exited 0 and printed `ctest --preset dev -R '^pyqtgraph_cpp.examples.SimplePlot$' --output-on-failure`.
- Proposed-issues check: `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` exited 1 with blocked-by consistency errors in generated `github-issue-*.md` issue snapshots (`P1.08`, `P5.09`, `P7.01`, `P8.05`, and `P8.07` references); no P1.09 implementation files were implicated.
- Whitespace check: `git diff --check` exited 0.
- Branch changed-file command: `git diff --name-only origin/main...HEAD` exited 0 with no output because this handoff leaves uncommitted worktree changes rather than commits.
- Worktree changed-file check: `git status --short --untracked-files=all` listed only owned paths: `reports/examples/P1.09/README.md`, `scripts/run_changed_examples`, and `tests/test_P1_09_changed_examples.py`.

## Artifacts
- Script: `scripts/run_changed_examples`
- Tests: `tests/test_P1_09_changed_examples.py`
- Report: `reports/examples/P1.09/README.md`
