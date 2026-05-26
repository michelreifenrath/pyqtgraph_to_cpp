# Issue #96 P0.02 completion/rework

## Rework decision

Autoreview found that the generated schema had not been landed in the checked-in manifest and that the configured generated-file verifier still compared raw class rows against the full manifest.

This rework keeps the prior manifest generator behavior, refreshes `port_manifest.yaml`, and makes `oracle/scripts/generate_class_inventory.py --check` compatible with full-manifest class rows that carry `status`/`completion` metadata. `WORKFLOW.md` and automation policy files were not edited.

## Schema/check behavior

`scripts/generate_manifest` generates inline `status` and `completion` fields for every row in `source_files`, `examples`, `example_assets`, and `classes`.

Status is deterministic and based only on target-path existence under the selected repository root:

- all target paths exist: `status: ported`, `completion: complete`
- no target paths exist: `status: not_started`, `completion: missing`
- some target paths exist: `status: partial`, `completion: partial`

`port_manifest.yaml` is refreshed with `manifest_schema: {status_metadata: adopted}` and row metadata in every generated row section.

The class-inventory verifier now compares the class-owned projection of the full manifest:

- strips `status`/`completion` from manifest `classes` rows before comparing to raw class inventory rows
- compares `excluded` as-is
- compares only class-owned `summary` keys (`class_count`, `source_file_count`, `excluded_example_count`, `excluded_test_count`) so full-manifest example summary fields do not make the class verifier stale

`oracle/scripts/generate_class_inventory.py --update-manifest` also writes class rows with status/completion metadata and preserves non-class summary fields.

## Focused failure fixtures

`tests/oracle/test_port_manifest.py` includes P0.02 fixtures for:

- a deferred manifest with row metadata stripped
- a manifest with one missing `status`
- a manifest with all row status/completion metadata stripped

`tests/oracle/test_class_inventory.py` includes verifier coverage for a full-manifest fixture whose class rows include status/completion metadata and whose summary includes extra full-manifest fields.

## Commands run

- LSP diagnostics for `oracle/scripts/generate_class_inventory.py`, `tests/oracle/test_class_inventory.py`, `scripts/generate_manifest`, and `tests/oracle/test_port_manifest.py`: no diagnostics found; Python LSP unavailable for extensionless `scripts/generate_manifest`.
- `python3 -m pytest -q tests -k P0_02` exited 0: `3 passed, 266 deselected in 1.41s`.
- `python3 -m pytest -q tests/oracle/test_port_manifest.py tests/oracle/test_class_inventory.py` exited 0: `31 passed in 9.19s`.
- `python3 scripts/generate_manifest --check` exited 0: `port manifest verified (213 source files, 129 examples, 355 classes)`.
- `python3 oracle/scripts/generate_class_inventory.py --check` exited 0: `class inventory verified (355 classes)`.
- `git diff --check` exited 0 with no output.
- `git diff --name-only origin/main...HEAD` exited 0 and listed committed branch diff paths: `reports/issues/P0.02/issue-96-completion.md`, `scripts/generate_manifest`, `tests/oracle/test_port_manifest.py`.
- `git diff --name-only` exited 0 and listed active rework diff paths: `oracle/scripts/generate_class_inventory.py`, `port_manifest.yaml`, `reports/issues/P0.02/issue-96-completion.md`, `tests/oracle/test_class_inventory.py`.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` exited 1 with repository issue metadata failures (`blocked-by entry does not match a local issue`) across multiple GitHub issues, including issue 96. No files were modified by this command.

## Artifact paths

- Generator/check logic: `scripts/generate_manifest`
- Generated manifest refresh: `port_manifest.yaml`
- Class verifier compatibility: `oracle/scripts/generate_class_inventory.py`
- Focused tests and failure fixtures: `tests/oracle/test_port_manifest.py`, `tests/oracle/test_class_inventory.py`
- Completion report: `reports/issues/P0.02/issue-96-completion.md`

## Changed-file ownership check

Committed branch diff against `origin/main` remains limited to the prior issue-owned manifest/check/test/report paths:

- `reports/issues/P0.02/issue-96-completion.md`
- `scripts/generate_manifest`
- `tests/oracle/test_port_manifest.py`

Active rework diff adds or updates the files directly required by the two autoreview findings:

- `port_manifest.yaml` — required checked-in generated manifest refresh
- `oracle/scripts/generate_class_inventory.py` — required compatibility for the workflow's configured generated-file verifier
- `tests/oracle/test_class_inventory.py` — focused verifier regression coverage
- `reports/issues/P0.02/issue-96-completion.md` — updated completion evidence for this rework

`WORKFLOW.md` and automation policy files have no active worktree diff.

## Visual validation

Not applicable. This change affects manifest generation/check metadata and tests only; no rendered output or pixel-affecting code changed.
