# Code Context

## Files Retrieved
1. `.github/workflows/ci.yml` (lines 1-91) - defines the five PR CI jobs reported failed: `linux`, `macos`, `windows`, `visual`, and `performance-record`.
2. `CMakeLists.txt` (lines 28-39, 65-73, 140-158) - issue-owned build wiring for Qt Widgets / GraphicsObject sources and focused test executables.
3. `include/pyqtgraph/graphicsItems/GraphicsObject.hpp` (lines 1-28) - issue-owned GraphicsObject declaration and Qt include surface.
4. `src/pyqtgraph/graphicsItems/GraphicsObject.cpp` (lines 1-34) - issue-owned GraphicsObject implementation.
5. `tests/graphicsItems/test_GraphicsObject.cpp` (lines 1-147) - issue-owned focused GraphicsObject behavior tests.
6. `tests/hierarchy/test_cpp_hierarchy.cpp` (lines 1-88) - issue-owned hierarchy/API shape coverage.

## Current branch / dirty state
- Branch: `ai/issue-31-ai-pggi-002-add-graphicsobject-base`
- Upstream: `origin/ai/issue-31-ai-pggi-002-add-graphicsobject-base`
- HEAD: `9214aaaaa2662def59386776a509fb0eaac8f694`
- PR #75 head: same branch/sha (`9214aaaaa2662def59386776a509fb0eaac8f694`) into `main`.
- Dirty state before and after reconnaissance: clean (`git status --short --branch` showed only the branch line; no tracked/untracked changes).

## Failed workflow/run/job names and relevant error snippets
PR #75 status checks point to workflow `CI`, run `26352559097`, event `pull_request`, created `2026-05-24T05:06:56Z`, completed `2026-05-24T05:07:00Z`, conclusion `failure`.

All five failed jobs completed in ~1-2 seconds and reported no executable steps in the Actions jobs API. `gh run view --job <id> --log` and `--log-failed` returned no per-step logs because the jobs did not start. The actionable failure text is in check-run annotations:

| Job | Job id | Conclusion | Most relevant annotation |
| --- | ---: | --- | --- |
| `visual` | `77573277854` | failure | `The job was not started because recent account payments have failed or your spending limit needs to be increased. Please check the 'Billing & plans' section in your settings` |
| `performance-record` | `77573277856` | failure | same billing/spending-limit message |
| `windows` | `77573277859` | failure | same billing/spending-limit message; additional notice: `windows-latest requests are being redirected to windows-2025-vs2026 by June 15, 2026` |
| `linux` | `77573277864` | failure | same billing/spending-limit message |
| `macos` | `77573277868` | failure | same billing/spending-limit message |

Command used for annotations:

```sh
gh api repos/michelreifenrath/pyqtgraph_to_cpp/commits/9214aaaaa2662def59386776a509fb0eaac8f694/check-runs --jq '.check_runs[] | [.id,.name] | @tsv' |
while IFS=$'\t' read id name; do
  gh api repos/michelreifenrath/pyqtgraph_to_cpp/check-runs/$id/annotations --jq '.[] | {path,start_line,end_line,annotation_level,message,title,raw_details}'
done
```

## Key Code

`.github/workflows/ci.yml` (lines 11-91) defines ordinary hosted-runner jobs. There is no job-level source/test failure visible because runner provisioning never reached checkout/configure/build/test.

```yaml
jobs:
  linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: cmake --preset ci-linux
      - run: cmake --build --preset ci-linux --parallel
      - run: ctest --preset ci-linux --output-on-failure
```

`CMakeLists.txt` (lines 28-39) gates GraphicsObject on Qt Widgets:

```cmake
if(TARGET Qt6::Core AND TARGET Qt6::Gui AND TARGET Qt6::Widgets)
    set(_pyqtgraph_cpp_has_graphicsitem TRUE)
elseif(PYQTGRAPH_CPP_REQUIRE_QT)
    message(FATAL_ERROR "Qt 6 Widgets is required for PGGI-001 GraphicsItem and PGGI-002 GraphicsObject...")
endif()
```

`CMakeLists.txt` (lines 65-73, 140-158) adds `src/pyqtgraph/graphicsItems/GraphicsObject.cpp` and the focused tests only under the same Qt Widgets guard.

`include/pyqtgraph/graphicsItems/GraphicsObject.hpp` (lines 10-28) declares the intended multiple inheritance from `QGraphicsObject` and `GraphicsItem`.

## Architecture
- PR #75 adds the PGGI-002 `GraphicsObject` bridge in the Qt Widgets path.
- `GraphicsObject` inherits `QGraphicsObject` and the project `GraphicsItem` helper, binding the helper host pointer to `this` as a `QGraphicsItem` in `src/pyqtgraph/graphicsItems/GraphicsObject.cpp` lines 13-18.
- `itemChange()` invalidates inherited view-widget cache on parent/scene changes, then delegates to `QGraphicsObject::itemChange()` (`src/pyqtgraph/graphicsItems/GraphicsObject.cpp` lines 22-33).
- CMake compiles this only when Qt Core/Gui/Widgets are present and adds the focused `GraphicsObject` and hierarchy tests under that guard.
- GitHub Actions never reached this architecture; all jobs failed at hosted-runner/account provisioning before checkout.

## Ranked likely root cause(s)
1. **GitHub Actions billing/spending-limit/account issue (highest confidence).** Every failed job has the same annotation: jobs were not started because recent account payments failed or spending limit must be increased. Jobs ran for only seconds with no steps/logs.
2. **No source/build/test failure is evidenced by PR #75 CI logs.** The local focused Linux configure/build/test passed on the same HEAD.
3. **Non-CI note only:** repository cache contains a stale/static `.pi-lens` finding about `include/pyqtgraph/graphicsItems/GraphicsObject.hpp:10` (`QtWidgets/QGraphicsObject: No such file or directory`), but local `cmake --preset ci-linux`, build, and focused tests passed, so this is not supported as the PR #75 Actions failure cause.

## Smallest safe fix inside issue-owned files only
- **No code fix is indicated inside issue-owned files.** The CI failures are external to the source tree: GitHub hosted runners did not start because of billing/spending-limit/account state.
- Smallest safe action: resolve GitHub billing/spending limit for the repository/account, then rerun failed jobs / rerun workflow `26352559097` for PR #75.
- If constrained strictly to issue-owned files, leave files unchanged; changing `CMakeLists.txt`, `GraphicsObject.*`, or tests would not address the observed CI failure.

## Focused validation commands to run
After billing/spending-limit is fixed and Actions can start runners:

```sh
# Verify PR check annotations and status
gh pr view 75 --repo michelreifenrath/pyqtgraph_to_cpp --json statusCheckRollup

# Rerun CI once account/runners are usable
gh run rerun 26352559097 --repo michelreifenrath/pyqtgraph_to_cpp --failed

# Local focused validation for issue #31 / PGGI-002
cmake --preset ci-linux
cmake --build --preset ci-linux --parallel
ctest --preset ci-linux --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.GraphicsObject|hierarchy\.cpp)'

# Issue-prescribed gates when practical
scripts/gate focus PGGI-002
scripts/gate commit
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```

Local command already run during reconnaissance:

```sh
cmake --preset ci-linux && \
cmake --build --preset ci-linux --parallel && \
ctest --preset ci-linux --output-on-failure -R 'pyqtgraph_cpp\.(graphicsItems\.GraphicsObject|hierarchy\.cpp)'
```

Result: exit `0`; `pyqtgraph_cpp.graphicsItems.GraphicsObject` and `pyqtgraph_cpp.hierarchy.cpp` passed (`2/2` tests).

## Start Here
Start with the PR #75 check-run annotations, not the source files: `gh api repos/michelreifenrath/pyqtgraph_to_cpp/check-runs/<check_run_id>/annotations`. They show the failure occurred before any workflow step executed.
