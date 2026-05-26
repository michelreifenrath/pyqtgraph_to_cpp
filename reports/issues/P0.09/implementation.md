# P0.09 final acceptance checklist script

## Changed files

- `scripts/summarize_status` - read-only Python CLI for deterministic manifest/dashboard acceptance summary and `--require-complete` gate.
- `tests/test_summarize_status_P0_09.py` - focused pytest fixtures covering normal output, complete gate pass, stale/missing/invalid validation metadata failures, and incomplete gate failure.
- `reports/issues/P0.09/implementation.md` - this completion artifact.

## TDD red result

Command: `python3 -m pytest -q tests -k P0_09`

Exit code: `1`

Concise failure: the initial focused tests failed before implementation because `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-103/scripts/summarize_status` did not exist (`[Errno 2] No such file or directory`).

## Final validation commands

- Command: `python3 -m pytest -q tests -k P0_09`
  - Exit code: `0`
  - Evidence: `7 passed, 295 deselected in 0.38s`
- Command: `python3 scripts/summarize_status`
  - Exit code: `0`
  - Evidence: printed consistent real-manifest counts, including `source_files: total=213 ported=16 complete=16 incomplete=197` and `example_validation_levels: total=129 numeric_required=4 visual_required=111 interaction_required=46 gpt_visual_required=111`.
- Command: `python3 scripts/summarize_status --require-complete`
  - Exit code: `1` (expected for the current incomplete real manifest)
  - Evidence: printed `require_complete: failed` with incomplete buckets for `source_files`, `examples`, `example_assets`, and `classes`.
- Command: `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp`
  - Exit code: `1`
  - Evidence: existing proposed-issue linter failures such as `github-issue-103.md: blocked-by entry does not match a local issue: P0.02`; not caused by this change and outside the owned files for P0.09.
- Command: `git diff --check`
  - Exit code: `0`
- Command: `git diff --name-only origin/main...HEAD`
  - Exit code: `0`
  - Evidence: no committed branch diff against `origin/main...HEAD`; this worktree intentionally leaves an uncommitted review diff.
- Command: `git diff --name-only`
  - Exit code: `0`
  - Evidence: only issue-owned paths are present: `reports/issues/P0.09/implementation.md`, `scripts/summarize_status`, and `tests/test_summarize_status_P0_09.py`.
- Command: `git status --short`
  - Exit code: `0`
  - Evidence: the same three issue-owned paths are visible as intent-to-add worktree files.
- Command: `python3 -m py_compile scripts/summarize_status`
  - Exit code: `0`
- Scratch artifact check: no `subagent-artifacts/` or `scripts/__pycache__/` directories remain in the worktree.

## Artifact paths

- `reports/issues/P0.09/implementation.md`

## Required evidence

- Passing check: `test_P0_09_require_complete_passes_on_all_complete_fixture` passes under `python3 -m pytest -q tests -k P0_09`.
- Failing-fixture case: `test_P0_09_require_complete_fails_with_clear_bucket` asserts `--require-complete` returns nonzero and reports `source_files: 1 incomplete`.
- Metadata failure cases: `test_P0_09_invalid_status_metadata_fails` asserts invalid status fields fail clearly, and `test_P0_09_validation_level_mismatch_fails` asserts missing/unknown example validation rows fail clearly.

## Manifest/dashboard update applicability

Manifest/dashboard updates are not applicable for this issue because no tracked source files, classes, examples, or example assets were changed.
