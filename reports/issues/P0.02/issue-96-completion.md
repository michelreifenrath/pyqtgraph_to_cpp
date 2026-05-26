# Issue #96 P0.02 completion/rework

## Rework decision

Attempt 6 rejected the generated `port_manifest.yaml` refresh because it made the review surface too large. This rework removes that generated-data refresh from #96 and keeps only the infrastructure that generates and checks row-level `status`/`completion` metadata.

`port_manifest.yaml` is intentionally unchanged in this worktree. A separate generated-data refresh is still required before `scripts/generate_manifest --check` can pass on the repository manifest.

## Schema/check behavior

`scripts/generate_manifest` now generates inline `status` and `completion` fields for every row in `source_files`, `examples`, `example_assets`, and `classes`.

Status is deterministic and based only on target-path existence under the selected repository root:

- all target paths exist: `status: ported`, `completion: complete`
- no target paths exist: `status: not_started`, `completion: missing`
- some target paths exist: `status: partial`, `completion: partial`

Generated manifests carry `manifest_schema: {status_metadata: adopted}`. `--check` compares the checked-in generated sections directly against the regenerated manifest, so missing, stale, or deferred row metadata fails instead of being stripped from expectations.

## Generated-data refresh split

The current checked-in `port_manifest.yaml` still lacks row metadata. With the infrastructure in this PR, `python3 scripts/generate_manifest --check` fails with:

```text
error: port_manifest.yaml is stale; run --update-manifest to refresh generated section(s) (status/completion metadata missing or stale): manifest_schema, source_files, examples, example_assets, classes
```

That failure is the intentional proof that #96 detects stale/missing metadata without carrying the rejected 11k-line manifest refresh. The generated-data refresh should be handled in a separate scope/issue/PR if the project wants to check in all row metadata.

## Focused failure fixtures

`tests/oracle/test_port_manifest.py` adds P0.02 fixtures for:

- a deferred manifest with row metadata stripped
- a manifest with one missing `status`
- a manifest with all row status/completion metadata stripped

These fixtures assert that `--check` fails and that generated output still contains adopted row metadata.

## Commands run

- LSP diagnostics for `scripts/generate_manifest` and `tests/oracle/test_port_manifest.py`: no diagnostics found; Python LSP unavailable for extensionless `scripts/generate_manifest`.
- `python3 -m pytest -q tests -k P0_02` exited 0: `3 passed, 265 deselected in 1.36s`.
- `python3 -m pytest -q tests/oracle/test_port_manifest.py` exited 0: `16 passed in 5.07s`.
- `python3 scripts/generate_manifest --check` exited 1 with the expected stale/missing status metadata error shown above.
- `git diff --check` exited 0.
- `git diff --name-only origin/main...HEAD` exited 0 with no output because this rework is left uncommitted as instructed.
- `git diff --name-only` exited 0 and listed the active worktree diff paths: `reports/issues/P0.02/issue-96-completion.md`, `scripts/generate_manifest`, and `tests/oracle/test_port_manifest.py`.

## Artifact paths

- Generator/check logic: `scripts/generate_manifest`
- Focused tests and failure fixtures: `tests/oracle/test_port_manifest.py`
- Completion report: `reports/issues/P0.02/issue-96-completion.md`

## Changed-file ownership check

The active worktree diff is limited to issue-owned manifest/check/test/report paths:

- `reports/issues/P0.02/issue-96-completion.md`
- `scripts/generate_manifest`
- `tests/oracle/test_port_manifest.py`

`WORKFLOW.md` and `port_manifest.yaml` have no active worktree diff.

## Visual validation

Not applicable. This change affects manifest generation/check metadata and tests only; no rendered output or pixel-affecting code changed.
