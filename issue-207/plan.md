# Implementation Plan

## Goal
Resolve P9.07 by documenting the 14 not-applicable PyQtGraph example/support entries with policy-backed explicit non-port/equivalence status and local proof, without adding C++ example files or changing generated manifest semantics.

## Tasks
1. **Create the focused decision/proof artifact**: Add a P9.07 report that is both the decision/equivalence document and completion report.
   - File: `reports/issues/P9.07/not-applicable-examples-equivalence.md`
   - Changes: Record the governing policy citation to `docs/parity-contract.md` and `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`; state that no named human approval is used unless one is actually available; include command/artifact placeholders for final closeout.
   - Acceptance: The report clearly identifies itself as the local proof artifact for P9.07 and cites the parity contract instead of relying on an implicit skip.

2. **Enumerate all 14 affected manifest entries in the artifact**: Add a table with one row per not-applicable example/support entry.
   - File: `reports/issues/P9.07/not-applicable-examples-equivalence.md`
   - Changes: Include `upstream_path`, `target_source_path`, manifest status context (`status: not_started`, `completion: missing`, validation channels all `not_applicable`), explicit P9.07 decision (`non-port`), C++ counterpart/equivalence (`covered by standalone C++ examples for real plotting/runtime features, not by porting Python scaffolding/test harness/support files`), and follow-up/waiver link.
   - Acceptance: The artifact includes exactly these 14 upstream paths and none are omitted: `VideoTemplate_generic.py`, `__init__.py`, `__main__.py`, `_buildParamTypes.py`, `_paramtreecfg.py`, `cx_freeze/setup.py`, `exampleLoaderTemplate_generic.py`, `optics/__init__.py`, `py2exe/setup.py`, `relativity/__init__.py`, `template.py`, `test_examples.py`, `utils.py`, `verlet_chain/__init__.py`.

3. **Preserve generated manifest status fields unless explicitly approved otherwise**: Do not manually change generated `port_manifest.yaml::examples` `status`/`completion` values for these rows.
   - File: `port_manifest.yaml`
   - Changes: No change expected to the generated `examples` inventory. The existing non-generated `example_validation_levels` rows already encode all validation channels as `not_applicable`; reference them in the P9.07 artifact.
   - Acceptance: `scripts/generate_manifest --check` continues to pass. If acceptance reviewers require machine-readable `non_port` or equivalent manifest status fields, stop/escalate because the current generator only derives `status`/`completion` from physical target `.cpp` presence and changing this would exceed the issue-owned scope.

4. **Focused proof / TDD-red expectation**: Establish the initial failing condition as missing P9.07 focused documentation, not failing runtime behavior.
   - File: `reports/issues/P9.07/not-applicable-examples-equivalence.md`
   - Changes: Document that before this issue there is no focused P9.07 decision/completion artifact and the 14 rows are only implicitly covered by `not_applicable` validation metadata plus the broad parity contract. No C++ tests should be added because `decision-doc` proof applies and executable behavior does not change.
   - Acceptance: Review can verify the red-to-green transition by checking that the P9.07 report now exists, lists all 14 entries, cites policy, and records non-port/equivalence decisions.

5. **Record follow-up issue links and waivers**: Make unresolved or future behavior explicit.
   - File: `reports/issues/P9.07/not-applicable-examples-equivalence.md`
   - Changes: Link P9.07/#207 as the application issue for these 14 entries; state that no new follow-up issue is required for Python scaffolding/templates/package initializers/test harness files unless a future concrete native C++ plotting/runtime requirement is identified.
   - Acceptance: Each row has either the #207 link or a clear waiver statement, so there are zero silent skips.

6. **Run and record post-implementation validation**: Execute required and focused local checks and paste command results into the report.
   - File: `reports/issues/P9.07/not-applicable-examples-equivalence.md`
   - Changes: Record commands, exit codes, and relevant artifact path(s).
   - Acceptance: Completion report includes these commands at minimum:
     - `python3 -m pytest -q tests/oracle/test_example_validation_levels.py`
     - `scripts/generate_manifest --check`
     - `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp`
     - `git diff --check`
     - `git diff --name-only origin/main...HEAD`

## Files to Modify
- `reports/issues/P9.07/not-applicable-examples-equivalence.md` - new focused decision/equivalence and completion report proving all 14 not-applicable examples have explicit policy-backed status.

## New Files
- `reports/issues/P9.07/not-applicable-examples-equivalence.md` - local proof artifact and closeout report for P9.07.

## Dependencies
- Task 2 depends on Task 1 creating the report structure.
- Task 4 depends on Tasks 1-2 so the red condition can be described against the completed proof artifact.
- Task 6 depends on all documentation/report content being complete.

## Risks
- The issue wording mentions manifest status fields, but `port_manifest.yaml::examples` `status`/`completion` are generated from target `.cpp` presence; manual edits are likely to fail `scripts/generate_manifest --check`.
- Adding new manifest keys to `example_validation_levels` would fail `tests/oracle/test_example_validation_levels.py` unless tests/schema are changed, which is outside this docs/status-only plan.
- Stop and escalate if reviewers require a new machine-readable manifest status such as `non_port`, generator changes, test schema changes, or actual C++ counterpart files.
- Stop and escalate if any of the 14 entries is found to contain standalone plotting/runtime behavior that should be ported rather than explicitly marked non-port.
