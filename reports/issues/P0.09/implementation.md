# P0.09 final acceptance checklist script

## Changed files

- `scripts/summarize_status` - read-only Python CLI for deterministic manifest/dashboard acceptance summary and filesystem-aware `--require-complete` gate, including final-acceptance evidence checks.
- `tests/test_summarize_status_P0_09.py` - focused pytest fixtures covering normal output, complete gate pass, stale complete metadata with missing target files, missing/invalid validation metadata, incomplete gate, and missing/failing final-acceptance evidence paths.
- `reports/issues/P0.09/implementation.md` - this completion artifact.

## TDD red result

Command: `python3 -m pytest -q tests -k P0_09`

Exit code: `1`

Concise failure: the initial focused tests failed before implementation because `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-103/scripts/summarize_status` did not exist (`[Errno 2] No such file or directory`).

## Final validation commands

- Command: `python3 -m pytest -q tests -k P0_09`
  - Exit code: `0`
  - Evidence: `14 passed, 295 deselected`
- Command: `python3 -m py_compile scripts/summarize_status`
  - Exit code: `0`
- Command: `python3 scripts/summarize_status`
  - Exit code: `0`
  - Evidence: printed consistent real-manifest counts, including `source_files: total=213 ported=16 complete=16 incomplete=197`, `examples: total=129 ported=1 complete=1 incomplete=128`, `example_validation_levels: total=129 numeric_required=4 visual_required=111 interaction_required=46 gpt_visual_required=111`, and `final_acceptance_evidence: criteria=8 passed=0 example_proofs=0/1`.
- Command: `python3 scripts/summarize_status --require-complete`
  - Exit code: `1` (expected for the current incomplete real manifest)
  - Evidence: printed `require_complete: failed` with incomplete buckets for `source_files`, `examples`, `example_assets`, and `classes`, plus missing final-acceptance evidence at `reports/issues/P0.09/final_acceptance_evidence.yaml`.
- Command: `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp`
  - Exit code: `1`
  - Evidence: existing proposed-issue linter failures such as `github-issue-103.md: blocked-by entry does not match a local issue: P0.02`; not caused by this change and outside the owned files for P0.09.
- Command: `git diff --check`
  - Exit code: `0`
- Command: `git diff --name-only origin/main...HEAD`
  - Exit code: `0`
  - Evidence: branch diff lists only issue-owned paths: `reports/issues/P0.09/implementation.md`, `scripts/summarize_status`, and `tests/test_summarize_status_P0_09.py`.
- Command: `git diff --name-only`
  - Exit code: `0`
  - Evidence: current rework diff lists only issue-owned paths: `reports/issues/P0.09/implementation.md`, `scripts/summarize_status`, and `tests/test_summarize_status_P0_09.py`.
- Command: `git status --short`
  - Exit code: `0`
  - Evidence: current worktree has only issue-owned modified paths: `reports/issues/P0.09/implementation.md`, `scripts/summarize_status`, and `tests/test_summarize_status_P0_09.py`.

## Artifact paths

- `reports/issues/P0.09/implementation.md`

## Required evidence

- Passing check: `test_P0_09_require_complete_passes_on_all_complete_fixture` creates every declared target file, writes complete final-acceptance evidence, and passes under `python3 -m pytest -q tests -k P0_09`.
- Final-evidence failure cases: `test_P0_09_require_complete_fails_without_final_evidence_proof`, `test_P0_09_require_complete_fails_without_final_criterion_proof`, `test_P0_09_require_complete_fails_on_blocking_autoreview_evidence`, and `test_P0_09_require_complete_fails_without_human_approval` prove `--require-complete` cannot pass without recorded proof for final acceptance criteria.
- Stale metadata failure case: `test_P0_09_require_complete_fails_on_stale_complete_metadata` uses valid-looking `ported`/`complete` metadata without target files, asserts `--require-complete` returns nonzero, records effective `source_files` incompleteness, and reports a missing target path.
- Incomplete metadata failure case: `test_P0_09_require_complete_fails_with_clear_bucket` asserts `--require-complete` returns nonzero and reports `source_files: 1 incomplete`.
- Metadata validation failure cases: `test_P0_09_invalid_status_metadata_fails` asserts invalid status fields fail clearly, and `test_P0_09_validation_level_mismatch_fails` asserts missing/unknown example validation rows fail clearly.
- Path-safety failure cases: `test_P0_09_require_complete_rejects_absolute_target_path` and `test_P0_09_require_complete_rejects_parent_traversal_target_path` assert existing files outside the repository are rejected instead of counted complete.

## Rework note

The autoreview finding is addressed by making `--require-complete` require `reports/issues/P0.09/final_acceptance_evidence.yaml` proof for all documented final criteria: example validation runs, core hierarchy checks, required-platform tests, performance benchmarks, autoreview status, package install, downstream `find_package(pyqtgraph-cpp)`, and human approval. The real repository intentionally has no passing final-acceptance evidence yet, so `scripts/summarize_status --require-complete` fails until those proofs are recorded.

## Manifest/dashboard update applicability

Manifest/dashboard updates are not applicable for this issue because no tracked source files, classes, examples, or example assets were changed.
