# Issue #97 P0.03 completion/rework

## Rework finding

Original gate finding: Pi completed but left no git changes. That bounded rework added the issue-owned dashboard command, focused tests, generated dashboard artifact, and completion report. Current autoreview rework fixes stale manifest trust in `scripts/summarize_status --require-complete` by validating complete rows against their manifest target paths before dashboard counts are accepted. No commits, pushes, merges, workflow-policy edits, or scope expansion were performed.

## Implemented behavior

- `scripts/summarize_status` reads `port_manifest.yaml` and validates manifest summary counts plus row `status`/`completion` metadata.
- Rows marked `completion: complete` must have their required `target_header_path`, `target_source_path`, or `target_path` entries present on disk before dashboard counts or completion gates can pass.
- Markdown output and `--format json` are deterministic and manifest-driven.
- `--update-dashboard` writes `reports/dashboard/status.md`.
- `--check` is read-only and fails clearly when the dashboard artifact is missing, dashboard metadata is missing/stale, or rendered content is stale.
- `--require-complete` exits nonzero when tracked manifest rows are incomplete.

## Current dashboard counts

Generated artifact: `reports/dashboard/status.md`.

| Area | Total | Complete | Partial | Missing |
| --- | ---: | ---: | ---: | ---: |
| Source files | 213 | 16 | 0 | 197 |
| Classes | 355 | 24 | 0 | 331 |
| Examples | 129 | 1 | 0 | 128 |
| Example assets | 16 | 0 | 0 | 16 |

## Focused proof and failure fixtures

Focused tests are in `tests/oracle/test_P0_03_status_dashboard.py` and cover:

- dashboard CLI modes;
- manifest-driven markdown and JSON counts;
- `--update-dashboard` plus read-only `--check` for a current dashboard;
- missing dashboard artifact without writes;
- stale dashboard metadata without writes;
- missing dashboard metadata without writes;
- inconsistent manifest summary metadata;
- all-complete stale manifest metadata with a missing target file.

Required evidence detail:

- Passing check: `scripts/summarize_status --check` verified `reports/dashboard/status.md`.
- Failing-fixture case: `test_P0_03_require_complete_rejects_complete_rows_with_missing_targets` marks every fixture row complete, deletes `src/pyqtgraph/PlotData.cpp`, and verifies `--require-complete` fails on stale complete target metadata.

## TDD / red-green evidence

Red phase after adding the initial focused tests and before adding `scripts/summarize_status`:

- `python3 -m pytest -q tests -k P0_03` exited 1 with six failing tests because `scripts/summarize_status` did not exist.

Green phase after implementation:

- `python3 -m pytest -q tests -k P0_03` exited 0: `8 passed, 295 deselected in 1.03s`.
- `python3 -m pytest -q tests/oracle/test_P0_03_status_dashboard.py` exited 0: `8 passed in 0.57s`.
- `python3 -m pytest -q tests/oracle/test_P0_03_status_dashboard.py::test_P0_03_require_complete_rejects_complete_rows_with_missing_targets` exited 0: `1 passed in 0.07s`.
- `scripts/summarize_status --check` exited 0: `dashboard verified: reports/dashboard/status.md`.

## Required validation commands

- `python3 -m pytest -q tests -k P0_03` exited 0: `8 passed, 295 deselected in 1.03s`.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` exited 1 with pre-existing GitHub issue metadata failures: multiple `github-issue-*.md` entries report `blocked-by entry does not match a local issue` (including P0.02, P0.08, P1.06, P1.04, P1.01, P0.06, P1.03, and P0.01 blockers). This issue does not own those GitHub issue metadata files.
- `git diff --check` exited 0 with no output.
- `git diff --name-only origin/main...HEAD` exited 0 with output:

  ```text
  reports/dashboard/status.md
  reports/issues/P0.03/completion.md
  scripts/summarize_status
  tests/oracle/test_P0_03_status_dashboard.py
  ```

Additional local checks:

- LSP diagnostics for `scripts/summarize_status` and `tests/oracle/test_P0_03_status_dashboard.py`: no diagnostics found; no LSP server was configured for the extensionless script.
- `scripts/generate_manifest --check` exited 0: `port manifest verified (213 source files, 129 examples, 355 classes)`.
- `scripts/summarize_status --require-complete` exited 1 as expected for the current incomplete manifest: `Source files: 197 incomplete; Classes: 331 incomplete; Examples: 128 incomplete; Example assets: 16 incomplete`.

## Artifact paths

- Dashboard CLI/check: `scripts/summarize_status`
- Deterministic dashboard artifact: `reports/dashboard/status.md`
- Focused tests and failure fixtures: `tests/oracle/test_P0_03_status_dashboard.py`
- Completion report: `reports/issues/P0.03/completion.md`

## Manifest-expanded targets and shared wiring

Manifest source selectors: none. Manifest example selectors: none. Therefore no manifest-expanded source, class, example, or asset target paths are owned or changed by this issue.

Shared wiring paths changed: none.

`port_manifest.yaml` was read and checked but not modified; no tracked source, class, example, or asset status changed.

## Changed-file ownership check

Branch diff paths from `git diff --name-only origin/main...HEAD` are issue-owned or adjunct-owned:

- `reports/dashboard/status.md` — matches `reports/dashboard/**`.
- `reports/issues/P0.03/completion.md` — focused-doc-report completion artifact.
- `scripts/summarize_status` — explicitly named in repository path globs.
- `tests/oracle/test_P0_03_status_dashboard.py` — focused test named for P0.03.

Ownership result: passed; no branch-diff path requires issue scope expansion. Current uncommitted rework paths are `scripts/summarize_status`, `tests/oracle/test_P0_03_status_dashboard.py`, and `reports/issues/P0.03/completion.md`, all within the issue-owned paths or focused adjuncts.

`reports/status.md`, `WORKFLOW.md`, automation policy files, production source, examples, CMake, `port_manifest.yaml`, and GitHub Actions were not modified.

## Visual validation

Not applicable. This issue changes manifest/dashboard reporting infrastructure only; no rendered output or pixel-affecting code changed.
