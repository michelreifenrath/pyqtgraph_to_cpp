# P0.10 manifest-to-proposed-issue audit proof

## Summary

Added granular `proposed_issue_ownership` metadata to `port_manifest.yaml` and extended `scripts/check_manifest_ownership` to audit that every tracked source file, class, example, and example asset maps to exactly one local proposed issue. The check prints a deterministic per-section entry/issue summary on success.

## Red phase (before implementation)

- `python3 -m pytest -q tests -k P0_10` — exit 5, `no tests ran` (no P0.10 proof existed)
- `scripts/check_manifest_ownership --manifest port_manifest.yaml --ownership ownership.yaml` — exit 0, ownership-only pass with no proposed-issue audit output

## Green phase (after implementation)

- `python3 -m pytest -q tests -k P0_10` — exit 0, `5 passed, 286 deselected`
- `scripts/check_manifest_ownership --manifest port_manifest.yaml --ownership ownership.yaml` — exit 0:

```
manifest ownership check passed
proposed issue audit passed (713 entries)
  source_files: entries=213 issues=87
  examples: entries=129 issues=1
  example_assets: entries=16 issues=1
  classes: entries=355 issues=83
```

- `python3 scripts/summarize_status` — exit 0, dashboard summary unchanged and consistent
- `python3 -m pytest -q` — exit 0, full suite pass
- `git diff --check` — exit 0, no whitespace errors
- `git diff --name-only` — exit 0, working-tree paths listed in Scope and ownership below

## Failure fixtures

- `test_P0_10_proposed_issue_audit_fails_for_unmapped_entry` — drops the classes rule and expects `unmapped manifest entry: classes[0] pyqtgraph/Foo.py`
- `test_P0_10_proposed_issue_audit_rejects_stale_rule_section` — points a rule at a manifest section absent from the fixture and expects a stale-section error
- `test_P0_10_proposed_issue_audit_rejects_inconsistent_overlap` — overlapping rules with different issue ids expect `inconsistent proposed issue mapping`
- `test_P0_10_proposed_issue_audit_rejects_stale_pattern` — a pattern matching no rows expects `stale proposed issue rule`
- `test_P0_10_real_manifest_maps_scatterplotitem_to_specific_p4_shard` — proves `graphicsItems/ScatterPlotItem.py` maps to `P4.01`, not the graphicsItems rollup.
- `test_P0_10_real_manifest_maps_spinbox_to_specific_p5_shard` — proves `widgets/SpinBox.py` maps to `P5.19`, not the widgets rollup.

## Scope and ownership

Changed files match issue-owned selectors:

- `port_manifest.yaml` — added top-level `proposed_issue_ownership` (non-generated metadata preserved by `scripts/generate_manifest`)
- `scripts/check_manifest_ownership` — proposed-issue audit logic
- `tests/test_proposed_issue_audit_P0_10.py` — focused P0.10 proof
- `reports/issues/P0.10/completion.md` — this report

No production C++ source, examples, CMake wiring, or `docs/proposed-issues/**` mirrors were added.

## Manifest/dashboard applicability

`port_manifest.yaml` gained reviewable proposed-issue mapping metadata for 713 tracked rows across `source_files`, `examples`, `example_assets`, and `classes`. The mapping uses concrete shard issues where a path group already has a known owner, including P4 graphicsItems and P5 widget shards, and rollup issues only for broad coverage/example groups. `python3 scripts/summarize_status` output is unchanged aside from reading the same manifest file.
