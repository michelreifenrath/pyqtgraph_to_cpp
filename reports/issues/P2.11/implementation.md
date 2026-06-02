# P2.11 implementation handoff

Copy to run artifacts: `/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/implementation.md`

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

## Commands to run

```bash
python3 scripts/factory/check_issue_ready.py --issue-file "/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/issue.json"
cmake --preset dev
cmake --build --preset dev
ctest --preset dev -L P2.11 --output-on-failure
ctest --preset dev -R 'functions_qimage|makeQImage' --output-on-failure
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
git diff --check
{
  git diff --name-only origin/main...HEAD
  git diff --name-only --cached
  git diff --name-only
  git ls-files --others --exclude-standard
} | sort -u | tee "/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/changed-files.txt"
python3 scripts/factory/check_pr_scope.py --issue-file "/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/issue.json" --changed-files-file "/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/changed-files.txt"
scripts/gate commit --dry-run
cp reports/issues/P2.11/implementation.md "/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/implementation.md"
```

## Validation results (validation subagent session)

Shell execution was blocked in the validation subagent environment (only `ls` was allowlisted). No build directory exists at `build/dev`. Exit codes below were **not captured live**; re-run the commands above in the worktree to confirm.

| Step | Command | Exit code | Notes |
| --- | --- | --- | --- |
| 1 | `cmake --preset dev && cmake --build --preset dev` | **not run** | Shell blocked; `build/dev` absent |
| 2 | `ctest --preset dev -L P2.11 --output-on-failure` | **not run** | Target: `pyqtgraph_cpp.core.functions_qimage` |
| 3 | `ctest --preset dev -R 'functions_qimage\|makeQImage' --output-on-failure` | **not run** | Includes legacy `makeQImage` regression |
| 4a | `git diff --check` | **not run** | |
| 4b | `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | **not run** | |
| 5a | changed-files aggregation | **not run** | `changed-files.txt` written manually (6 paths) |
| 5b | `check_pr_scope.py` | **not run** | Manual review: all paths in owned scope |
| 6 | copy/update `implementation.md` | **0** (Write tool) | Artifacts path updated |

## Artifact paths

- Issue-owned copy: `reports/issues/P2.11/implementation.md`
- Required run artifact (copy via `cp` above): `/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp/artifacts/runs/c8faa2e815bb5747025364e46a5fda83/implementation.md`
- Evidence: `reports/issues/P2.11/core_qimage_conversion.md`
