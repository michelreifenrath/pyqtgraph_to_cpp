# P2.11 implementation handoff

## Issue

- **ID:** P2.11 / GitHub #244
- **Title:** Split image helpers: core QImage conversion
- **Readiness:** `readiness.json` reports `"ready": true`

## Summary

Added `pyqtgraph::tryMakeQImage` overloads for upstream `try_make_qimage` pass-through conversion (no levels/LUT): `uint8` grayscale/RGB/RGBA and `uint16` grayscale/RGBA64. Unsupported inputs return `std::nullopt`; accepted inputs are copied to contiguous QImage-owned buffers. Legacy `makeQImage` behavior is unchanged.

## Changed files

| Path | Rationale |
| --- | --- |
| `include/pyqtgraph/functions_qimage.hpp` | Declare `tryMakeQImage` overloads; update source note |
| `src/pyqtgraph/functions_qimage.cpp` | Implement core conversion with stride-safe copy |
| `tests/core/test_functions_qimage.cpp` | Focused pixel-buffer tests (owned glob) |
| `CMakeLists.txt` | Register `pyqtgraph_cpp.core.functions_qimage` with label `P2.11` (focused-tests adjunct) |
| `reports/issues/P2.11/core_qimage_conversion.md` | Numeric/oracle rationale and fixture table |

## Validation results

Authoritative run log: `/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/validation.md`. Issue-contract validation: **PASS**.

| Command | Exit | Summary |
| --- | ---: | --- |
| `git diff --check` | 0 | No whitespace errors. |
| changed-files aggregation → `changed-files.txt` | 0 | Six owned paths (see scope artifact). |
| `python3 scripts/factory/check_pr_scope.py` (issue + changed-files) | 0 | `ok: true`; `CMakeLists.txt` as `shared_integration`. |
| `python3 scripts/factory/check_issue_ready.py` | 0 | `ready: true`. |
| `cmake --preset dev` | 0 | Configure succeeded (`build/dev`). |
| `cmake --build --preset dev` | 0 | Built `pyqtgraph_cpp_core_functions_qimage`. |
| `ctest --preset dev -L P2.11 --output-on-failure` | 0 | 1/1: `pyqtgraph_cpp.core.functions_qimage`. |
| `ctest --preset dev -R 'functions_qimage\|makeQImage' --output-on-failure` | 0 | 2/2: legacy `makeQImage` + new `functions_qimage`. |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 0 | Passed. |
| `scripts/gate commit --dry-run` | 0 | Dry-run only. |

Extra full-suite `ctest --preset dev --output-on-failure` reported one unrelated P2.08 oracle failure (missing pinned reference files); P2.11 tests passed in that run.

## Artifact paths

- Handoff: `reports/issues/P2.11/implementation.md`
- Evidence: `reports/issues/P2.11/core_qimage_conversion.md`
- Run log: `/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/validation.md`
