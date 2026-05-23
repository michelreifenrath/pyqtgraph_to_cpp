# PGINV-005 Status

Implemented the repository-level deterministic port manifest generator for the pinned PyQtGraph reference.

## Behavior

- `scripts/generate_manifest` reads `reference/source.lock`, validates the pinned checkout through the existing inventory helpers, and uses the temporary clone fallback when `reference/pyqtgraph` is absent.
- The generator composes source, example, and class inventories from the same validated checkout.
- Default output is deterministic YAML; `--format json` emits the same schema as JSON.
- `--update-manifest` rewrites generated sections in `port_manifest.yaml` and preserves unrelated keys.
- `--check` is read-only and fails when `port_manifest.yaml` is missing or any generated section is stale.

## Manifest Schema

Canonical generated top-level keys are ordered as:

1. `reference`
2. `source_files`
3. `examples`
4. `example_assets`
5. `example_inventory_summary`
6. `classes`
7. `excluded`
8. `summary`

The checked-in manifest now records:

- Source files: 213
- Examples: 129
- Example assets: 16
- Total example tree files: 145
- Classes: 355
- Excluded example Python files: 129
- Excluded test Python files: 74

## Validation

- `python3 -m pytest tests/oracle/test_port_manifest.py -q` — passed, 13 tests.
- `python3 scripts/generate_manifest --check` — passed, `port manifest verified (213 source files, 129 examples, 355 classes)`.
- `python3 -m pytest tests/oracle/test_source_inventory.py tests/oracle/test_example_inventory.py tests/oracle/test_class_inventory.py -q` — passed, 32 tests.
- `python3 -m pytest -q` — passed, 217 tests.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — passed, workflow valid.
- `git diff --check` — passed.

## Scope Notes

- `port_manifest.yaml` was regenerated via `python3 scripts/generate_manifest --update-manifest`.
- The manifest `reference` section preserves the existing repository schema (`repo`, `ref`, `pinned_commit`, `docs_url`); checkout validation still uses `checkout_path` from `reference/source.lock` internally.
- No commits, pushes, merges, or workflow-policy edits were performed.
