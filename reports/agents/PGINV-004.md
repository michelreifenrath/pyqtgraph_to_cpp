# PGINV-004 Implementation Report

## Scope
Implemented a deterministic PyQtGraph hierarchy oracle for the pinned upstream source. The generator reads `reference/source.lock`, uses a clean local checkout or temporary fallback clone, parses tracked top-level PyQtGraph classes, resolves unambiguous internal inheritance edges, and verifies or updates the checked-in hierarchy fixture.

No PR was opened because this Pi handoff must not push or merge; repository automation will handle PR creation.

## Implemented Files
- `oracle/scripts/dump_pyqtgraph_hierarchy.py` — new hierarchy manifest CLI with `--check`, `--update-fixture`, JSON/YAML stdout, pinned checkout validation, and deterministic output.
- `oracle/fixtures/hierarchy_pyqtgraph.json` — generated fixture for pinned PyQtGraph commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- `tests/hierarchy/test_pyqtgraph_hierarchy_manifest.py` — focused tests for CLI behavior, sorting, inheritance resolution, exclusions, read-only checks, fixture updates, fallback clone behavior, and dirty/wrong-checkout rejection.
- `reports/agents/PGINV-004.md` — this report.

## Fixture Schema
The fixture top-level keys are `reference`, `classes`, `edges`, `excluded`, and `summary`.

Generated counts:
- Classes: 355
- Edges: 192
- Unresolved bases: 158
- Source files: 213
- Excluded examples: 129
- Excluded tests: 74

## Validation
- `python3 -m pytest tests/hierarchy/test_pyqtgraph_hierarchy_manifest.py -q` — passed, 14 tests.
- `python oracle/scripts/dump_pyqtgraph_hierarchy.py --check` — passed, `hierarchy fixture verified (355 classes, 192 edges)`.
- `git diff --check` — passed.
- `python3 -m pytest -q` — passed, 167 tests.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — passed, workflow valid.

## Deviations / Notes
- Did not commit, push, merge, or change branches.
- No PR was opened from Pi; automation is expected to handle PR creation.
- No scratch artifacts remain in the worktree.
