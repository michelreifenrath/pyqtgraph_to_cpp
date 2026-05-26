# P8.05 Multiprocess Remote Proxy Equivalence Completion Report

- Author/tool: Pi bounded rework pass with scout subagent
- Date: 2026-05-26
- Issue: GitHub #195 / P8.05
- Validation class: decision-doc
- Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-195`

## Summary

Implemented the #195/P8.05 remote proxy boundary decision as a docs-only update to the existing parity contract. The decision records conservative non-port treatment for Python multiprocessing/proxy machinery, the accepted native C++ equivalence, the remote render parity boundary, and follow-up proof policy.

## Pre-implementation evidence

These checks were run before editing:

```text
$ pwd && git status --short --untracked-files=all && git branch --show-current
/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-195
ai/issue-195-p8-05-resolve-multiprocess-remote-proxy-eq
exit 0
```

`git status --short --untracked-files=all` produced no status lines, so the worktree was clean before P8.05 edits.

Read/inspection checks before editing confirmed the existing decision-doc artifacts and policy context:

- `docs/parity-contract.md` already contained the multiprocessing/proxy section and #195 follow-up link.
- `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc` defines decision-doc proof as a decision/equivalence document, not a code test.
- `docs/local-validation.md` permits focused docs-only validation for decision issues.
- Existing completion report examples were inspected at `reports/issues/P0.06/completion.md` and `reports/issues/P1.04/completion.md`.

## Artifacts

- `docs/parity-contract.md` - existing decision/equivalence artifact extended with the P8.05/#195 remote proxy boundary decision.
- `reports/issues/P8.05/completion.md` - this focused decision-doc completion report.

## Manifest-expanded target paths

- `pyqtgraph/multiprocess/__init__.py` -> `include/pyqtgraph/multiprocess/__init__.hpp`, `src/pyqtgraph/multiprocess/__init__.cpp`
- `pyqtgraph/multiprocess/bootstrap.py` -> `include/pyqtgraph/multiprocess/bootstrap.hpp`, `src/pyqtgraph/multiprocess/bootstrap.cpp`
- `pyqtgraph/multiprocess/parallelizer.py` -> `include/pyqtgraph/multiprocess/parallelizer.hpp`, `src/pyqtgraph/multiprocess/parallelizer.cpp`
- `pyqtgraph/multiprocess/processes.py` -> `include/pyqtgraph/multiprocess/processes.hpp`, `src/pyqtgraph/multiprocess/processes.cpp`
- `pyqtgraph/multiprocess/remoteproxy.py` -> `include/pyqtgraph/multiprocess/remoteproxy.hpp`, `src/pyqtgraph/multiprocess/remoteproxy.cpp`

Affected classes recorded in the decision: `CanceledError`, `Parallelize`, `Tasker`, `Process`, `ForkedProcess`, `RemoteQtEventHandler`, `QtProcess`, `FileForwarder`, `ClosedError`, `NoResultError`, `RemoteExceptionWarning`, `RemoteEventHandler`, `Request`, `LocalObjectProxy`, `ObjectProxy`, `DeferredObjectProxy`.

## Scope and ownership

Changed issue-owned paths:

- `docs/parity-contract.md`
- `reports/issues/P8.05/completion.md`

Shared wiring paths changed: none.

No `include/**`, `src/**`, examples, tests, scripts, CMake, `WORKFLOW.md`, `port_manifest.yaml`, dashboard, or generated files were intentionally edited.

## Policy and decision recorded

- Conservative default: Python process proxying, interpreter/process bootstrap, remote object forwarding, request/result marshalling, deferred object proxies, remote exception/warning proxying, and Python stdout/stderr forwarding are explicit non-ports for native C++.
- Accepted C++ equivalence: native Qt/C++ plotting/rendering stays in normal in-process Qt scene/view/item APIs unless a future issue explicitly owns worker-process rendering. Native C++ concurrency/process/async support is only in scope when required by a concrete C++ plotting/runtime feature.
- No Python object-proxy API compatibility layer is accepted.
- Remote render parity boundary: `RemoteGraphicsView`/worker-process rendering is not owned by #195 and is deferred to [#167](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/167) if needed.
- Examples are not owned by #195, so no example files or manifest example entries were changed.
- Follow-up policy: existing #195 records this decision for multiprocess/proxy infrastructure. Follow-up #167 owns any `RemoteGraphicsView`/worker-process rendering plan and proof before requiring runtime or render evidence.

## Manifest/dashboard applicability

`port_manifest.yaml` and generated dashboards were not changed. This issue is a decision-doc update; it records the replacement/exclusion policy for remote proxy equivalents without changing executable inventory or status files.

## Runtime tests

Runtime tests are not required. This is a docs-only decision-doc issue with no executable behavior changes.

## Visual validation

Visual validation is not applicable. This issue does not affect rendering code or pixels. Worker-process/remote render parity proof is explicitly deferred to follow-up #167 if that behavior is required.

## Validation

Post-implementation local validation:

```text
$ test -f docs/parity-contract.md
exit 0

$ test -f reports/issues/P8.05/completion.md
exit 0

$ rg -n "multiprocess|remoteproxy|RemoteEventHandler|ObjectProxy|DeferredObjectProxy|P8[.]05|#195" docs/parity-contract.md reports/issues/P8.05/completion.md
exit 0; 35 matching lines covering the P8.05/#195 decision text, all five multiprocess manifest-expanded rows, `remoteproxy`, and the affected proxy classes in both the parity contract and this report.

$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
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
exit 1

$ git diff --check
exit 0

$ git diff --name-only origin/main...HEAD
exit 0; output was empty because Pi must not commit and the issue changes are uncommitted working-tree edits.

$ git diff --name-only
docs/parity-contract.md
exit 0

$ git status --short --untracked-files=all
 M docs/parity-contract.md
?? reports/issues/P8.05/completion.md
exit 0
```
