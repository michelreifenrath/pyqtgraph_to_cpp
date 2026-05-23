# PGINV-001 Status

Implemented a deterministic source inventory CLI for the pinned PyQtGraph reference.

## Behavior

- Reads `reference/source.lock` and validates required reference metadata.
- Uses the local checkout from `checkout_path` (`reference/pyqtgraph`) when present, validating `git rev-parse HEAD` against `pinned_commit`.
- When the checkout is absent, materializes the locked `repo`/`ref`/`pinned_commit` into a temporary directory, validates the commit, enumerates the inventory, and removes the temporary source without writing to `reference/pyqtgraph`.
- Enumerates `.py` files under the PyQtGraph `pyqtgraph/` package.
- Excludes `pyqtgraph/examples/**` and repo-level `tests/**` from `source_files`, while reporting them under `excluded` and `summary` counts for later phases.
- Emits deterministic YAML by default or JSON with `--format json`.
- Supports read-only `--check`, which generates and serializes the inventory in memory without writing persistent files.

## Mapping Rules

- `upstream_path` uses POSIX separators and is sorted lexicographically.
- `target_header_path` is `include/<upstream_path without .py>.h`.
- `target_source_path` is `src/<upstream_path without .py>.cpp`.
- Top-level `pyqtgraph/*.py` files use subsystem `core`.
- Nested package files use the first component after `pyqtgraph/` as subsystem.
- `__init__.py` remains deterministic as `__init__.h` / `__init__.cpp`.

## Validation

- Initial expected red checks before implementation:
  - `python3 -m pytest tests/oracle/test_source_inventory.py -q` exited 4 because the test path was missing.
  - `python oracle/scripts/generate_source_inventory.py --check` exited 2 because the script was missing.
- Rework validation:
  - `python3 -m pytest tests/oracle/test_source_inventory.py -q` exited 0 (`6 passed`).
  - `python oracle/scripts/generate_source_inventory.py --check` exited 0 (`source inventory verified (213 source files)`) using the temporary pinned-source fallback because `reference/pyqtgraph` is absent.
  - `python3 -m pytest -q` exited 0 (`82 passed`).
  - `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` exited 0.
  - `git diff --check` exited 0.

## Scope Notes

- Did not write `port_manifest.yaml` or bootstrap/modify `reference/pyqtgraph` because those files are not owned for this issue.
- Added the required implementation report at `reports/agents/PGINV-001.md`.
