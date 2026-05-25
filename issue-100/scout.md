# Code Context

## Files Retrieved
1. `AGENTS.md` (lines 5-30, 36-41) - source-of-truth order, owned-file/TDD rules, and safety constraints.
2. `WORKFLOW.md` (lines 71-93, 114-126) - shared integration files and validation command policy.
3. GitHub issue #100 body - exact scope, owned files, required proof, and validation commands.
4. `docs/proposed-issues/VALIDATION-GUIDE.md` (lines 20-35, 48-49) - adjunct ownership rules and oracle-infra proof definition.
5. `docs/pyqtgraph-cpp-port-workflow.md` (lines 382-404, 606-622) - canonical oracle strategy and Phase C oracle harness context.
6. `reference/source.lock` (lines 1-5) - pinned PyQtGraph repo/ref/commit/checkout path used by oracle probes.
7. `pyproject.toml` (lines 1-15) - pytest configuration (`testpaths = ["tests"]`, `pythonpath = ["."]`).
8. `oracle/scripts/generate_numeric_oracles.py` (lines 19-40, 218-337, 344-371, 421-481, 517-572, 575-615, 638-714) - current pinned-reference probe/generator pattern.
9. `tests/oracle/test_numeric_oracles.py` (lines 12-25, 38-167, 182-216, 232-303, 306-354, 357-491) - existing oracle pytest style and failure-mode coverage.
10. `CMakeLists.txt` (lines 135-178, 330-410) - existing CTest registration locations and absence of labels on current tests.
11. `tests/CMakeLists.txt` (lines 1-13) - minimal subdirectory smoke test registration.
12. `CMakePresets.json` (lines 121-190) - `ctest --preset dev` behavior and existing label-filter presets for visual/performance.

## Key Code

Issue #100 requires:
- Owned globs: `oracle/**`, `tests/oracle/test_*oracle*`, `docs/**oracle*`.
- Common adjunct: `focused-tests`.
- Shared wiring allowed only for registering/running owned tests/artifacts: `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json`, `cmake/**`, focused validation scripts.
- Required proof: `python3 -m pytest -q tests -k P0_06`; `ctest --preset dev -L P0.06 --output-on-failure`; artifact set includes upstream version/commit, fixture inputs/outputs, tolerance policy, and one mismatch failure example.

Current oracle pattern in `oracle/scripts/generate_numeric_oracles.py`:
```python
LOCK_PATH = Path("reference/source.lock")
FIXTURE_PATH = Path("oracle/fixtures/numeric")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")
SCHEMA_VERSION = 1
```
The script embeds a Python `REFERENCE_PROBE`, extracts only required upstream classes by AST from the pinned checkout, and runs it via `subprocess.run([sys.executable, "-c", REFERENCE_PROBE], cwd=root, env=..., input=json.dumps(payload), timeout=30)` with `QT_QPA_PLATFORM=offscreen` and `PYTHONDONTWRITEBYTECODE=1` (`oracle/scripts/generate_numeric_oracles.py` lines 517-572).

Fixture schema produced by `case_definitions()` (`oracle/scripts/generate_numeric_oracles.py` lines 575-615):
```json
{
  "schema_version": 1,
  "case": "...",
  "reference": {"ref": "pyqtgraph-0.14.0", "pinned_commit": "..."},
  "inputs": {...},
  "expected": {...},
  "tolerance": {"absolute": 0.0, "relative": 0.0}
}
```
Existing fixtures under `oracle/fixtures/numeric/*.json` follow this for `affine_transform`, `log_mapping`, and `nan_minmax`; `makeQImage.json` uses a richer `reference` object with `pyqtgraph_version`, `pyqtgraph_commit`, source, notes, and tolerance `rgba_channel_abs`.

Existing pytest pattern:
- Helpers create a fake pinned checkout and `reference/source.lock` in `tmp_path`, then run scripts as subprocesses (`tests/oracle/test_numeric_oracles.py` lines 12-25, 38-167).
- Tests assert deterministic stdout/schema and fixture contents (`tests/oracle/test_numeric_oracles.py` lines 182-216, 232-303).
- Failure coverage includes missing/invalid lock, unusable checkout, commit mismatch, dirty checkout, missing fixture, and stale fixture (`tests/oracle/test_numeric_oracles.py` lines 371-491).

Current CMake/CTest pattern:
- `include(CTest)` and `if(BUILD_TESTING)` start at `CMakeLists.txt` lines 135-137.
- Existing C++ tests are registered with `add_executable(...)`, `target_link_libraries(...)`, `pyqtgraph_cpp_enable_sanitizers(...)`, then `add_test(NAME ... COMMAND ...)`; no `set_tests_properties(... PROPERTIES LABELS ...)` was found.
- `tests/CMakeLists.txt` only registers `pyqtgraph_cpp.smoke.empty` (lines 1-13).

## Architecture

The oracle infrastructure is Python-first. Scripts under `oracle/scripts/` read `reference/source.lock`, verify or materialize the pinned PyQtGraph checkout, run deterministic probes, and write/check JSON fixtures under `oracle/fixtures/`. Pytest tests under `tests/oracle/` exercise these scripts using temporary fake checkouts so they do not depend on the real upstream checkout or mutate project fixtures during tests.

C++ behavioral tests are currently built from root `CMakeLists.txt`, not from `tests/CMakeLists.txt` except for the smoke test. To satisfy issue #100's `ctest -L P0.06`, an implementation will likely need narrow shared wiring in `CMakeLists.txt` (or `tests/CMakeLists.txt`) to register one issue-owned comparison test with label `P0.06`. Because existing tests have no labels, a new labeled test is required for that command to select anything useful.

Likely focused test target:
- Add a pytest file such as `tests/oracle/test_P0_06_oracle_probe.py` or `tests/oracle/test_upstream_oracle_probe.py` containing test names with `P0_06` so `python3 -m pytest -q tests -k P0_06` selects only this issue.
- Add probe/template assets under `oracle/**`, preferably issue-named paths such as `oracle/probes/P0_06/**` or an oracle script/template under `oracle/scripts/` if kept generic.
- If a real C++ comparison executable is required, keep its source issue-named under the focused-tests adjunct (for example `tests/oracle/P0_06_oracle_comparison.cpp`) and only touch shared CMake files to register it and label it `P0.06`.
- Docs can be added under `docs/**oracle*` (none currently exist matching that glob), e.g. `docs/upstream-oracle-probes.md`, to explain future probe additions.

## Start Here

Open `tests/oracle/test_numeric_oracles.py` first. It shows the existing pytest/subprocess/fake-checkout pattern and fixture assertions that issue #100 should mirror before adding a reusable probe template.

## Validation commands

From issue #100:
```bash
python3 -m pytest -q tests -k P0_06
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev -L P0.06 --output-on-failure
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
git diff --check
git diff --name-only origin/main...HEAD
```
Repo-wide workflow also lists `python3 -m pytest -q` as the configured validation command (`WORKFLOW.md` lines 90-93).

## Scope risks and open questions

- `ctest --preset dev -L P0.06` cannot pass meaningfully unless the implementation adds a CTest with label `P0.06`; current CMake has no labels. This is allowed only as narrow shared wiring.
- The owned test glob `tests/oracle/test_*oracle*` does not naturally include C++ files. Use focused-tests adjunct for issue-named C++ comparison sources, or keep the C++ comparison generated/compiled inside pytest temp space if avoiding permanent C++ test files.
- No `docs/**oracle*` files currently exist; any instructional doc should be named to match the glob.
- Production source outside `oracle/**`, docs, focused tests, and shared wiring is out of scope.
- `reference/source.lock` and `port_manifest.yaml` contain the pin but are not owned by this issue; read only unless the issue is expanded.
