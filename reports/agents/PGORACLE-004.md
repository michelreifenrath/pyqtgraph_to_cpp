# PGORACLE-004 Implementation Report

## Summary

Implemented a deterministic numeric oracle runner for issue #17. The runner loads the pinned PyQtGraph reference lock, generates a stable manifest, writes deterministic JSON numeric fixtures in normal mode, and supports read-only validation with `--check`.

## Files Changed

- `oracle/scripts/generate_numeric_oracles.py`
- `oracle/fixtures/numeric/.gitkeep`
- `tests/oracle/test_numeric_oracles.py`
- `reports/agents/PGORACLE-004.md`

## Fixture Schema

- Manifest top-level keys: `reference`, `cases`, `summary`.
- Initial sorted cases: `affine_transform`, `log_mapping`.
- Fixture top-level keys: `schema_version`, `case`, `reference`, `inputs`, `expected`, `tolerance`.
- JSON output uses stable insertion order, two-space indentation, and a trailing newline.

## Validation

- `python3 -m pytest tests/oracle/test_numeric_oracles.py -q` — passed (`8 passed`).
- `python oracle/scripts/generate_numeric_oracles.py --check` — passed (`numeric oracles verified (2 cases)`).
- `git diff --check` — passed.
- New-file whitespace/final-newline check — passed.
- `python3 -m pytest -q` — passed (`133 passed`).
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — passed (`workflow valid: WORKFLOW.md`).

## Notes

- The numeric cases are deterministic pure-Python calculations and intentionally avoid PyQtGraph and Qt imports because those runtime dependencies are unavailable in this oracle path.
- Check mode exercises lock loading, manifest serialization, fixture serialization, and stale existing fixture detection without creating or modifying `oracle/fixtures/numeric/`.
