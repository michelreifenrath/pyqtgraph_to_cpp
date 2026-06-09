# P0.05 changed-file ownership proof

## Summary

Added a local changed-file ownership gate. `scripts/check_changed_file_ownership` verifies that branch edits stay inside the active `ownership.yaml` claim for the current branch, including common adjuncts and workflow shared-integration allowlists. `scripts/gate commit` and `scripts/gate merge` now reject out-of-scope edits before configured validation runs.

## TDD red run

Command run before implementation:

```bash
python3 -m pytest -q tests -k P0_05
```

Exit code: `1`

Initial failure: missing `scripts/check_changed_file_ownership` (`FileNotFoundError` from the focused pytest harness).

## Final validation

| Command | Exit code | Result |
| --- | ---: | --- |
| `python3 -m pytest -q tests -k P0_05` | 0 | `7 passed, 279 deselected` |
| `scripts/check_manifest_ownership --manifest port_manifest.yaml --ownership ownership.yaml` | 0 | `manifest ownership check passed` |
| `python3 -m pytest -q` | 0 | `275 passed, 11 skipped` |
| `git diff --check` | 0 | no whitespace errors |
| `git diff --name-only origin/main...HEAD` | 0 | lists only owned paths below |

## Failure fixtures

- `test_P0_05_changed_file_ownership_fails_for_out_of_scope_path` edits `src/pyqtgraph/Foo.cpp` outside the claim and asserts `out-of-scope changed file: src/pyqtgraph/Foo.cpp`.
- `test_P0_05_changed_file_ownership_rejects_missing_active_claim` uses an empty `claims` list and asserts `missing active ownership claim for branch ai/issue-99`.
- `test_P0_05_changed_file_ownership_rejects_stale_inactive_claim` uses `status: done` and asserts the same missing-active-claim failure.
- `test_P0_05_changed_file_ownership_rejects_inconsistent_duplicate_claims` asserts duplicate active claims fail with `multiple active claims`.
- `test_P0_05_gate_commit_rejects_out_of_scope_branch_changes` proves `scripts/gate commit` exits non-zero before later validation when ownership fails.

## Scope and ownership

Changed files match the issue-owned selectors:

- `ownership.yaml`
- `scripts/check_changed_file_ownership`
- `scripts/claim_ticket`
- `scripts/gate` (narrow shared wiring for commit/merge enforcement)
- `tests/test_board_policy.py`
- `reports/issues/P0.05/completion.md`

No manifest rows, production C++ source, examples, CMake wiring, or dashboard inventory status changed.

## Manifest/dashboard applicability

`port_manifest.yaml` was not changed. This issue adds manifest-infra ownership validation only; no tracked source, class, example, or asset implementation status changed.
