# Issue #96 P0.02 completion/rework

## Rework decision

Autoreview found that `oracle/scripts/generate_class_inventory.py --check` stripped `status`/`completion` from manifest class rows before comparison, so stale class metadata could pass validation.

This rework keeps the prior manifest generator behavior and changes the class-inventory verifier to compare manifest class rows against the generated class rows plus deterministic `status`/`completion` metadata. `WORKFLOW.md` and automation policy files were not edited.

## Schema/check behavior

`scripts/generate_manifest` generates inline `status` and `completion` fields for every row in `source_files`, `examples`, `example_assets`, and `classes`.

Status is deterministic and based only on target-path existence under the selected repository root:

- all target paths exist: `status: ported`, `completion: complete`
- no target paths exist: `status: not_started`, `completion: missing`
- some target paths exist: `status: partial`, `completion: partial`

`port_manifest.yaml` is refreshed with `manifest_schema: {status_metadata: adopted}` and row metadata in every generated row section.

The class-inventory verifier now compares the class-owned projection of the full manifest:

- compares manifest `classes` rows against `with_completion_metadata(root, inventory["classes"])`
- compares `excluded` as-is
- compares only class-owned `summary` keys (`class_count`, `source_file_count`, `excluded_example_count`, `excluded_test_count`) so full-manifest example summary fields do not make the class verifier stale
- rejects stale class `status`/`completion` metadata as a stale `classes` section

`oracle/scripts/generate_class_inventory.py --update-manifest` also writes class rows with status/completion metadata and preserves non-class summary fields.

## Focused failure fixtures

`tests/oracle/test_port_manifest.py` includes P0.02 fixtures for:

- a deferred manifest with row metadata stripped
- a manifest with one missing `status`
- a manifest with all row status/completion metadata stripped

`tests/oracle/test_class_inventory.py` includes verifier coverage for a full-manifest fixture whose class rows include status/completion metadata and whose summary includes extra full-manifest fields, plus a P0.02 stale class metadata fixture that must fail read-only `--check`.

## Commands run

- LSP diagnostics for `oracle/scripts/generate_class_inventory.py` and `tests/oracle/test_class_inventory.py`: no diagnostics found.
- `python3 -m pytest -q tests/oracle/test_class_inventory.py -k 'P0_02 or metadata or update_manifest'` exited 0: `4 passed, 12 deselected in 1.08s`.
- `python3 -m pytest -q tests -k P0_02` exited 0: `4 passed, 266 deselected in 1.62s`.
- `python3 -m pytest -q tests/oracle/test_class_inventory.py` exited 0: `16 passed in 4.57s`.
- `python3 oracle/scripts/generate_class_inventory.py --check` exited 0: `class inventory verified (355 classes)`.
- `python3 scripts/generate_manifest --check` exited 0: `port manifest verified (213 source files, 129 examples, 355 classes)`.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` exited 1 with pre-existing repository issue metadata failures (`blocked-by entry does not match a local issue`) across 16 GitHub issue metadata files, including issue 96. No files were modified by this command.
- `git diff --check` exited 0 with no output.
- `git diff --name-only origin/main...HEAD` exited 0 and listed branch diff paths: `oracle/scripts/generate_class_inventory.py`, `port_manifest.yaml`, `reports/issues/P0.02/issue-96-completion.md`, `scripts/generate_manifest`, `tests/oracle/test_class_inventory.py`, `tests/oracle/test_port_manifest.py`.
- `git diff --name-only` exited 0 and listed active rework diff paths: `oracle/scripts/generate_class_inventory.py`, `reports/issues/P0.02/issue-96-completion.md`, `tests/oracle/test_class_inventory.py`.

## Artifact paths

- Generator/check logic: `scripts/generate_manifest`
- Generated manifest refresh: `port_manifest.yaml`
- Class verifier compatibility: `oracle/scripts/generate_class_inventory.py`
- Focused tests and failure fixtures: `tests/oracle/test_port_manifest.py`, `tests/oracle/test_class_inventory.py`
- Completion report: `reports/issues/P0.02/issue-96-completion.md`

## Changed-file ownership check

Branch diff against `origin/main` remains limited to issue-owned manifest/check/test/report paths:

- `oracle/scripts/generate_class_inventory.py`
- `port_manifest.yaml`
- `reports/issues/P0.02/issue-96-completion.md`
- `scripts/generate_manifest`
- `tests/oracle/test_class_inventory.py`
- `tests/oracle/test_port_manifest.py`

Active rework diff updates only the files directly required by the current autoreview finding:

- `oracle/scripts/generate_class_inventory.py` — validates class row `status`/`completion` metadata
- `tests/oracle/test_class_inventory.py` — focused stale class metadata regression fixture
- `reports/issues/P0.02/issue-96-completion.md` — updated completion evidence for this rework

`WORKFLOW.md` and automation policy files have no active worktree diff.

## Visual validation

Not applicable. This change affects manifest generation/check metadata and tests only; no rendered output or pixel-affecting code changed.
