# Code Context

## Files Retrieved
1. `AGENTS.md` (lines 1-43) - repo-level source-of-truth, owned-file, TDD, and safety constraints.
2. `README.md` (lines 1-51) - high-level repo purpose and common Python automation commands.
3. `WORKFLOW.md` (lines 1-97) - machine-readable workflow config, validation command, and safety gates.
4. `docs/pyqtgraph-cpp-port-workflow.md` (lines 1-140) - canonical port goals, non-negotiable rules, and pinned reference requirements.
5. `docs/pyqtgraph-cpp-port-workflow.md` (lines 145-207) - target repository layout and expected script/test/report locations.
6. `docs/pyqtgraph-cpp-port-workflow.md` (lines 306-346) - baseline stack and standard validation commands.
7. `docs/pyqtgraph-cpp-port-workflow.md` (lines 430-525) - issue body format, owned-files policy, validation/done-definition expectations.
8. `docs/pyqtgraph-cpp-port-workflow.md` (lines 532-546) - Phase A list showing `PGBOOT-006` as “Add license attribution workflow.”
9. `pyproject.toml` (lines 1-20) - Python package/test configuration and pytest discovery settings.
10. `scripts/bootstrap_reference` (lines 1-220) - current script style, argument parsing, error handling, and metadata validation conventions.
11. `scripts/claim_ticket` (lines 1-199) - current executable Python script style and registry validation patterns.
12. `tests/test_bootstrap_reference.py` (lines 1-220) - subprocess-based tests for repository scripts and temporary workspace fixtures.
13. `tests/test_claim_ticket.py` (lines 1-240) - focused script tests using `tmp_path`, YAML fixtures, and stderr/stdout assertions.
14. `tests/test_cli.py` (lines 1-37) - small subprocess tests invoking scripts with `sys.executable`.
15. `tests/test_config.py` (lines 1-117) - pytest style and workflow-validation expectations.
16. `CMakeLists.txt` (lines 1-17) - C++20/CMake baseline and test add-subdirectory.
17. `CMakePresets.json` (lines 1-36) - `dev` configure/build/test presets requiring Qt/OpenCV.
18. `port_manifest.yaml` (lines 1-5) - current reference pin metadata.
19. `reference/source.lock` (lines 1-5) - current pinned PyQtGraph reference lock.
20. `.gitignore` (lines 1-8) - ignored generated/cache/build paths.

## Key Code

### Owned files: current state
- `LICENSE` - **missing** at repo root.
- `THIRD_PARTY_NOTICES.md` - **missing** at repo root.
- `scripts/check_attribution` - **missing**.
- `tests/test_attribution.py` - **missing**.
- `docs/pyqtgraph-cpp-port-workflow.md` - exists and is issue-owned for this ticket; no current source-note/attribution policy section found by grep.

### Existing repo layout
Top-level currently includes automation/docs/CMake skeleton plus empty-ish C++ namespace directories:
- `include/pyqtgraph/`
- `src/pyqtgraph/`
- `cmake/` with options/warnings/sanitizers modules
- `scripts/` with executable extensionless Python scripts: `bootstrap_reference`, `claim_ticket`, `pi-symphony-board-info`
- `tests/` with Python automation tests, `tests/smoke/`, and `tests/visual/`
- `reference/` has metadata files only; `reference/pyqtgraph/` checkout is not present in this worktree.

Git status was clean before writing this scout artifact.

### Validation config
`WORKFLOW.md` frontmatter sets the repo validation command to:
```bash
python3 -m pytest -q
```
`docs/pyqtgraph-cpp-port-workflow.md` also lists, until `scripts/gate` exists:
```bash
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```
CMake baseline commands exist in the docs, but this attribution issue is Python/docs/license focused:
```bash
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
```

### Script/test conventions to mirror
- Scripts in `scripts/` are extensionless executable Python with `#!/usr/bin/env python3`.
- Tests invoke scripts with `subprocess.run([sys.executable, str(SCRIPT), ...], text=True, capture_output=True)`.
- Existing script failures print concise messages to stderr and return nonzero rather than raising tracebacks from `main`.
- Existing tests use `tmp_path` to create isolated fixture repos/workspaces and assert no unwanted writes on check modes.

Useful snippets:
```python
# tests/test_bootstrap_reference.py
result = subprocess.run(
    [sys.executable, str(SCRIPT), "--check", "--offline"],
    text=True,
    capture_output=True,
)
assert result.returncode == 0, result.stderr
```

```python
# scripts/claim_ticket
if __name__ == "__main__":
    raise SystemExit(main())
```

### Current pinned reference metadata
`reference/source.lock` and `port_manifest.yaml` agree on:
- repo: `https://github.com/pyqtgraph/pyqtgraph`
- ref: `pyqtgraph-0.14.0`
- pinned_commit: `a20028b98294b9cc8770f2015a92eb342224b788`
- docs_url: `https://pyqtgraph.readthedocs.io/`
- checkout_path in lock: `reference/pyqtgraph`

## Architecture

This repository is a C++ port scaffold plus Python automation harness. The canonical product/engineering spec is `docs/pyqtgraph-cpp-port-workflow.md`; `WORKFLOW.md` is runtime automation config and should not be edited for this issue. `AGENTS.md` requires issue-owned-file discipline and TDD.

For `PGBOOT-006`, the implementation surface is self-contained:
1. Add root-level project license text in `LICENSE`.
2. Add root-level third-party notices in `THIRD_PARTY_NOTICES.md` for PyQtGraph, Qt, OpenCV, Python test tooling, and benchmark tooling.
3. Add source-note policy in the owned docs file (`docs/pyqtgraph-cpp-port-workflow.md`) so future translated/generated files have auditable upstream/source attribution.
4. Add `scripts/check_attribution` to audit repository files/metadata for attribution compliance.
5. Add `tests/test_attribution.py` first to define expected audit behavior, likely using temporary fixture trees plus a repo-level smoke check.

The checker should avoid requiring files outside this issue’s owned set to change immediately unless the test fixtures are scoped carefully. Since the repo currently has almost no translated C++ source and no generated manifest inventory yet, a practical first version can validate presence/content of `LICENSE`, `THIRD_PARTY_NOTICES.md`, and policy text, and test source-note scanning on fixture translated/generated files. If it scans the real repo, exclude `.git`, caches, `build/`, and non-source docs/automation files deliberately.

## Relevant Conventions

- Owned-file policy is strict: do not suggest edits to `WORKFLOW.md`, reports, automation config, or non-owned source for this issue.
- The target structure in the spec says not to create empty placeholders unless the issue says so; here the issue explicitly owns the license/notice/script/test/doc files.
- Python tests are under `tests/` and discovered by pytest per `pyproject.toml`.
- Existing scripts rely only on stdlib plus PyYAML where needed; attribution checker may be pure stdlib unless YAML metadata is needed.
- C++ baseline is C++20, CMake 3.26+, Qt 6 first, OpenCV 4.x, Qt Test, and Google Benchmark or nanobench for benchmarks.

## Likely Validation Commands

Focused first:
```bash
python3 -m pytest -q tests/test_attribution.py
```

Then repository validation:
```bash
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```

Optional if CMake dependency environment is available:
```bash
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
```

## Risks / Open Questions

- License choice for this repository is not stated in existing files. PyQtGraph is commonly MIT, but the implementer should verify from the pinned upstream/license source or authoritative package metadata before writing notices.
- Qt licensing can be LGPL/GPL/commercial depending on distribution/use; notices should avoid overclaiming and cite the intended supported license terms for this project.
- OpenCV 4.x license should be verified against the version/distribution intended by the project.
- Benchmark tooling is not yet implemented; docs mention “Google Benchmark or nanobench,” so notices may need to cover planned/allowed benchmark dependencies without pretending they are vendored.
- `reference/pyqtgraph/` checkout is absent, so checking upstream license files locally may require running `scripts/bootstrap_reference --refresh` or consulting pinned upstream separately; this scout did not modify the checkout.
- A too-strict real-repo attribution scan could fail on current bootstrap files that are not translated/generated. Keep initial checker policy narrowly tied to files with explicit source-note markers or translated/generated paths/extensions defined in the new policy.

## Start Here

Open `tests/test_attribution.py` first and write the failing tests that define the attribution contract. Then implement `scripts/check_attribution`, add `LICENSE`/`THIRD_PARTY_NOTICES.md`, and update the source-note policy in `docs/pyqtgraph-cpp-port-workflow.md`.
