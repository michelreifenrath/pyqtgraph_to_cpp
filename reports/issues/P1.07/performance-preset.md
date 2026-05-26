# P1.07 completion evidence: non-empty local performance CTest preset

Issue: #111 `[P1.07] Add non-empty local performance CTest preset`

## Scope summary

Added a dependency-free native performance smoke fixture and wired it into the local `performance` CTest preset. The preset now uses `noTestsAction: error`, so an actual performance CTest run rejects empty performance coverage.

## Changed files

Manifest-expanded target paths: none; this script/build-infra issue does not change tracked source, class, example, or asset manifest targets.

Shared wiring paths changed:

- `CMakePresets.json`
- `tests/CMakeLists.txt`

Issue-owned/supporting paths changed:

- `tests/performance/CMakeLists.txt`
- `tests/performance/test_P1_07_performance_preset.cpp`
- `tests/performance/test_P1_07_performance_preset.py`
- `reports/issues/P1.07/performance-preset.md`

## Focused proof coverage

- Normal path: `test_P1_07_performance_preset_lists_native_smoke_test` configures `release`, runs `ctest --preset performance --show-only=json-v1`, and asserts `P1.07.performance.smoke` is listed with labels `performance` and `P1.07`.
- Failure path: `test_P1_07_performance_preset_rejects_empty_label_selection` copies the repository `performance` test preset into an isolated CMake fixture with no `performance`-labeled tests, asserts `noTestsAction` is `error`, and verifies actual `ctest --preset performance` exits nonzero.
- Native smoke: `P1.07.performance.smoke` runs `tests/performance/test_P1_07_performance_preset.cpp`, which checks a non-empty deterministic workload, warmup checksum, repetition samples, checksum, and min/median/max ordering.
- Command-runner fake-runner assertions: not applicable; no command-runner script was added or modified for this issue.

## Validation results

| Command | Exit code | Result |
| --- | ---: | --- |
| `python3 -m pytest -q tests -k P1_07` before adding focused proof | 5 | `295 deselected` (no P1.07 proof existed) |
| `python3 -m pytest -q tests -k P1_07` after adding pytest, before implementation | 1 | `1 failed, 1 passed, 295 deselected` (`P1.07.performance.smoke` absent) |
| `python3 -m pytest -q tests -k P1_07` | 0 | `2 passed, 295 deselected in 2.46s` |
| `cmake --preset release` | 0 | Configured `build/release` |
| `cmake --build --preset release --target test_P1_07_performance_preset --parallel` | 0 | Built `test_P1_07_performance_preset` |
| `ctest --preset performance --show-only=json-v1` | 0 | Listed 4 performance tests including `P1.07.performance.smoke` with labels `P1.07` and `performance` |
| `ctest --preset performance --output-on-failure` | 0 | `4/4` performance tests passed |
| `ctest --preset performance -L P1.07 --output-on-failure` | 0 | `1/1` P1.07 performance test passed |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 1 | Fails on pre-existing proposed-issue `blocked-by` metadata mismatches; see output below |

Artifact paths:

- Native smoke executable: `build/release/tests/performance/test_P1_07_performance_preset`
- Completion report: `reports/issues/P1.07/performance-preset.md`
- Existing P0.08 performance reports produced by full performance run: `build/release/reports/performance/P0.08/`

## `scripts/check_proposed_issues` failure output

```text
github-issue-97.md: blocked-by entry does not match a local issue: P0.02
github-issue-98.md: blocked-by entry does not match a local issue: P0.02
github-issue-103.md: blocked-by entry does not match a local issue: P0.02
github-issue-111.md: blocked-by entry does not match a local issue: P0.08
github-issue-112.md: blocked-by entry does not match a local issue: P1.06
github-issue-118.md: blocked-by entry does not match a local issue: P1.04
github-issue-212.md: blocked-by entry does not match a local issue: P1.01
github-issue-120.md: blocked-by entry does not match a local issue: P0.06
github-issue-124.md: blocked-by entry does not match a local issue: P1.03
github-issue-127.md: blocked-by entry does not match a local issue: P1.04
github-issue-129.md: blocked-by entry does not match a local issue: P0.01
github-issue-130.md: blocked-by entry does not match a local issue: P1.01
github-issue-166.md: blocked-by entry does not match a local issue: P0.01
github-issue-180.md: blocked-by entry does not match a local issue: P1.04
github-issue-195.md: blocked-by entry does not match a local issue: P0.01
github-issue-197.md: blocked-by entry does not match a local issue: P0.01
github-issue-201.md: blocked-by entry does not match a local issue: P1.01
github-issue-202.md: blocked-by entry does not match a local issue: P0.02
github-issue-207.md: blocked-by entry does not match a local issue: P0.01
```

## Manifest/dashboard status

Not applicable. This issue adds local CTest/performance-preset infrastructure and does not change manifest-tracked C++ sources, classes, examples, or assets.
