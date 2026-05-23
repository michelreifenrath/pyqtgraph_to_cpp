# PGINV-003 Implementation Report

## Summary
Implemented deterministic PyQtGraph top-level class inventory generation for the pinned upstream source.

## Behavior
- Added `oracle/scripts/generate_class_inventory.py` with `--root`, `--format {yaml,json}`, `--check`, and `--update-manifest` options.
- Validates `reference/source.lock`, pinned checkout commit, and clean local checkout state before inventory generation.
- Falls back to a temporary pinned clone when `reference/pyqtgraph` is absent without creating persistent checkout files.
- Enumerates only module-body `ast.ClassDef` nodes under `pyqtgraph/**/*.py`, excluding `pyqtgraph/examples/**` and reporting repo `tests/**/*.py` as excluded.
- Emits deterministic class records with PyQtGraph upstream path, C++ target header/source paths, subsystem, bases, and source line.
- `--check` stays read-only and, when `port_manifest.yaml` exists, fails if generated `classes`, `excluded`, or `summary` sections are missing or stale; it intentionally does not compare the preserved `reference` section.
- Updates only generated `classes`, `excluded`, and `summary` sections in `port_manifest.yaml` while preserving existing manifest sections such as `reference`.

## Generated Manifest
The repository manifest now contains:
- `class_count`: 355
- `source_file_count`: 213
- `excluded_example_count`: 129
- `excluded_test_count`: 74

## Validation
- `python3 -m pytest tests/oracle/test_class_inventory.py -q` passed.
- `python oracle/scripts/generate_class_inventory.py --check` passed.
- `python3 -m pytest -q` passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` passed.
- `git diff --check` passed.

## Caveats
No known functional caveats.
