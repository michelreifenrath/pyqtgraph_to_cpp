# Issue #97 P0.03 completion/rework

## Rework finding

Gate finding: Pi completed but left no git changes. This bounded rework adds the issue-owned dashboard command, focused tests, generated dashboard artifact, and completion report. No commits, pushes, merges, workflow-policy edits, or scope expansion were performed.

## Implemented behavior

- `scripts/summarize_status` reads `port_manifest.yaml` and validates manifest summary counts plus row `status`/`completion` metadata.
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
- inconsistent manifest summary metadata.

Required evidence detail:

- Passing check: `scripts/summarize_status --check` verified `reports/dashboard/status.md`.
- Failing-fixture case: `test_P0_03_check_rejects_stale_dashboard_metadata_without_writes` mutates fixture manifest metadata after generating a dashboard and verifies `--check` fails as stale while leaving the dashboard file unchanged.

## TDD / red-green evidence

Red phase after adding the initial focused tests and before adding `scripts/summarize_status`:

- `python3 -m pytest -q tests -k P0_03` exited 1 with six failing tests because `scripts/summarize_status` did not exist.

Green phase after implementation:

- `python3 -m pytest -q tests -k P0_03` exited 0: `7 passed, 295 deselected in 0.48s`.
- `python3 -m pytest -q tests/oracle/test_P0_03_status_dashboard.py::test_P0_03_check_rejects_stale_dashboard_metadata_without_writes` exited 0: `1 passed in 0.09s`.
- `scripts/summarize_status --check` exited 0: `dashboard verified: reports/dashboard/status.md`.

## Required validation commands

- `python3 -m pytest -q tests -k P0_03` exited 0: `7 passed, 295 deselected in 0.48s`.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` exited 1 with pre-existing GitHub issue metadata failures: multiple `github-issue-*.md` entries report `blocked-by entry does not match a local issue` (including P0.02, P0.08, P1.06, P1.04, P1.01, P0.06, P1.03, and P0.01 blockers). This issue does not own those GitHub issue metadata files.
- `git diff --check` exited 0 with no output.
- `git diff --name-only origin/main...HEAD` exited 0 with no output because this handoff intentionally leaves an uncommitted worktree diff for review.

Additional local checks:

- LSP diagnostics for `scripts/summarize_status` and `tests/oracle/test_P0_03_status_dashboard.py`: no diagnostics found; no LSP server was configured for the extensionless script.
- `python3 scripts/generate_manifest --check` exited 0: `port manifest verified (213 source files, 129 examples, 355 classes)`.

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

Active uncommitted diff paths are issue-owned:

- `scripts/summarize_status` — explicitly named in repository path globs.
- `reports/dashboard/status.md` — matches `reports/dashboard/**`.
- `tests/oracle/test_P0_03_status_dashboard.py` — focused test named for P0.03.
- `reports/issues/P0.03/completion.md` — focused-doc-report completion artifact.

`reports/status.md`, `WORKFLOW.md`, automation policy files, production source, examples, CMake, `port_manifest.yaml`, and GitHub Actions were not modified.

## Visual validation

Not applicable. This issue changes manifest/dashboard reporting infrastructure only; no rendered output or pixel-affecting code changed.
