# P0.03 completion dashboard proof

## Summary

`scripts/summarize_status` is covered as the local completion dashboard for manifest status. The focused `P0_03` fixtures verify deterministic source/class/example/asset count output and explicit stale or inconsistent manifest metadata failure paths.

## Commands

- `python3 -m pytest -q tests -k P0_03` — exit 0, `3 passed, 273 deselected`
- `scripts/summarize_status` — exit 0, reported current manifest counts:
  - `source_files: total=213 ported=87 complete=0 incomplete=213`
  - `examples: total=129 ported=2 complete=0 incomplete=129`
  - `example_assets: total=16 ported=0 complete=0 incomplete=16`
  - `classes: total=355 ported=134 complete=0 incomplete=355`

## Failure fixtures

- `test_P0_03_dashboard_rejects_inconsistent_summary_metadata` mutates a fixture manifest so `summary.source_file_count` disagrees with the `source_files` length and asserts that `scripts/summarize_status` exits nonzero with the exact mismatch.
- `test_P0_03_dashboard_rejects_stale_complete_target_metadata` deletes a target file for a `complete` source row and asserts that `scripts/summarize_status --require-complete` exits nonzero with the stale target metadata path.

## Manifest/dashboard applicability

No tracked source, class, example, or asset implementation status changed in this issue. `port_manifest.yaml` is not updated; this slice adds focused dashboard proof and this report only.

## Changed files

- `tests/test_summarize_status_P0_03.py`
- `reports/issues/P0.03/completion.md`
