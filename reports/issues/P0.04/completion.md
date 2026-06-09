# P0.04 manifest ownership proof

## Summary

Added a local manifest ownership gate. `scripts/check_manifest_ownership` verifies that every tracked row in `port_manifest.yaml` has an owning local issue rule in `ownership.yaml`.

## Commands

- `python3 -m pytest -q tests -k P0_04` — exit 0, `3 passed, 276 deselected`
- `scripts/check_manifest_ownership --manifest port_manifest.yaml --ownership ownership.yaml` — exit 0, `manifest ownership check passed`
- `python3 -m pytest -q` — exit 0, `268 passed, 11 skipped`
- `git diff --check` — exit 0, no whitespace errors
- `git diff --name-only origin/main...HEAD` — exit 0, listed the four changed paths below

## Failure fixtures

- `test_P0_04_manifest_ownership_fails_for_unowned_entry` removes the class ownership rule from a fixture ownership map and asserts that the check fails with `unowned manifest entry: classes[0] pyqtgraph/Foo.py`.
- `test_P0_04_manifest_ownership_rejects_stale_owner_section` points an ownership rule at a manifest section absent from the fixture manifest and asserts that the check rejects the stale metadata.

## Scope and ownership

Changed files are within the issue-owned selectors:

- `scripts/check_manifest_ownership`
- `ownership.yaml`
- `tests/test_manifest_ownership_P0_04.py`
- `reports/issues/P0.04/completion.md`

No CMake/shared wiring, production C++ source, examples, generated manifest rows, numeric evidence, visual evidence, or interaction evidence were changed.

## Manifest/dashboard applicability

`port_manifest.yaml` was not changed. This issue adds manifest-infra validation only; no tracked source, class, example, or asset implementation status changed.
