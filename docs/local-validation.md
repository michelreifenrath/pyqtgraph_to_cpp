# Local Validation Contract

Issue: GitHub #108 / P1.04

This document defines the local platform validation contract for P0/P1 issue work in this repository.

## Source-of-truth policy

- GitHub issues are the source of truth for scoped issue work; do not reintroduce local issue-body mirrors (`docs/proposed-issues/README.md`).
- Validation is local-only for this issue set; do not add GitHub Actions or make completion depend on GitHub infrastructure (`docs/proposed-issues/README.md`, `docs/proposed-issues/VALIDATION-GUIDE.md`).
- Decision-only issues use the `decision-doc` validation class: proof is a decision/equivalence artifact and completion report, not runtime tests unless executable behavior changes (`docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`).
- The full-port parity contract establishes native C++ port scope and already records local-only validation as the default policy (`docs/parity-contract.md`).
- Repository workflow gates are local commands run from a checkout; Pi must not commit, push, merge, or create PRs (`WORKFLOW.md`).

## Conservative default

All P0/P1 issue validation is local-only by default unless a future explicitly owned issue changes this policy. GitHub Actions or other CI may mirror local commands, but CI status is never required proof for these AFK issues.

This is a validation and reporting policy, not a source-porting artifact. It applies to issue closeout evidence, completion reports, and review handoff expectations for native C++ port work.

## Rationale

- Deterministic evidence comes from the developer's local checkout, where the exact branch, changed files, command lines, exit codes, and artifact paths can be recorded together.
- Each issue owns its proof: completion artifacts must cite the commands and artifacts that prove that issue's scope, rather than depending on an external CI status.
- A portable platform command matrix lets Linux, macOS, and Windows contributors run equivalent checks locally without requiring GitHub-hosted infrastructure.
- Avoiding CI dependency keeps AFK validation usable in offline, private, or partially configured environments.

## Affected manifest entries

None. This policy applies to issue validation and reporting rather than tracked source, example, class, asset, or dashboard entries. P1.04 does not change `port_manifest.yaml`, generated dashboards, C++ source, examples, tests, or build wiring.

## Accepted C++ equivalence and explicit non-CI decision

For native C++ port work, local command evidence plus issue completion artifacts are the accepted proof mechanism. The expected proof depends on the issue's validation class:

- Decision-doc issues prove the decision with a policy/equivalence artifact and a completion report.
- Runtime tests are required only when executable behavior changes or when the issue's validation class requires them.
- Visual, oracle, and performance artifacts are required only when the issue's validation class explicitly calls for those artifacts.
- Manifest or dashboard updates are required only when the issue changes tracked source, example, class, asset, or status entries.

Explicit non-CI decision: do not add or require GitHub Actions for this validation contract. A CI mirror may be useful later, but local commands and committed artifacts remain the reviewable proof for this issue set unless a future issue explicitly owns and approves a policy change.

## Follow-up issue policy

Future platform-specific harnesses, CI mirrors, alternate validation systems, or changes to this contract must be implemented only by a separately owned issue. A local commit-gate follow-up such as [#109](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/109) may build on this policy, but it must remain locally runnable unless its own approved scope explicitly changes the policy.

If a required validation path cannot be expressed with local commands and issue-owned artifacts, stop and request scope expansion or named human approval before adding new infrastructure.

## Local command matrix

The commands below are concrete starting points for local validation. Each issue's completion report must record the exact commands actually run and their exit codes; it may use a focused subset when the issue scope does not require every command.

### Local environment doctor

`scripts/doctor_local` is a local diagnostic preflight for checkout setup problems, not a CI gate. Run it from the repository root when CMake configure/build/test validation fails or before starting native C++ work to report required local compiler, Qt 6, OpenCV 4, OpenGL, CMake, CTest, Git, and `pkg-config` availability. It exits nonzero on the first missing required prerequisite so the failing tool or dependency can be fixed locally.

### Linux

Run from the repository root in a POSIX shell:

```sh
git status --short
python3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
git diff --check
git diff --name-only origin/main...HEAD
```

When an issue specifically needs the platform-style CMake preset, run the Linux preset locally on Linux:

```sh
cmake --preset ci-linux
cmake --build --preset ci-linux --parallel
ctest --preset ci-linux --output-on-failure
```

For docs-only decision issues, focused proof may be limited to artifact existence/content checks, `scripts/check_proposed_issues`, `git diff --check`, and an ownership check such as `git diff --name-only origin/main...HEAD` plus `git status --short` for uncommitted handoffs.

### macOS

Run from the repository root in Terminal or another POSIX shell with local Qt/CMake dependencies available:

```sh
git status --short
python3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
git diff --check
git diff --name-only origin/main...HEAD
```

When an issue specifically needs the platform-style CMake preset, run the macOS preset locally on macOS:

```sh
cmake --preset ci-macos
cmake --build --preset ci-macos --parallel
ctest --preset ci-macos --output-on-failure
```

For docs-only decision issues, the same focused local proof used on Linux is acceptable when no executable behavior changes.

### Windows

Run Python, CMake, Git, and CTest commands from PowerShell in the repository root:

```powershell
git status --short
py -3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
git diff --check
git diff --name-only origin/main...HEAD
```

When an issue specifically needs the platform-style CMake preset, run the Windows preset locally on Windows:

```powershell
cmake --preset ci-windows
cmake --build --preset ci-windows --parallel
ctest --preset ci-windows --output-on-failure
```

Repository `scripts/*` commands are POSIX shell scripts. Run them locally with Git Bash, WSL, or an equivalent local POSIX shell; GitHub Actions is not required:

```sh
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
```

For docs-only decision issues on Windows, focused local proof may use PowerShell file checks and Git ownership checks, plus Git Bash/WSL only when a repository script is in scope.

## Per-issue closeout checklist

Every issue completion report must record:

- Exact command lines that were run locally.
- Exit code for each command.
- Artifact paths created or updated and what each artifact proves.
- Changed-file ownership: confirm changed files are inside the issue-owned paths, and list any shared wiring paths actually changed.
- Manifest/dashboard applicability: record the update or explain why it is not applicable.
- Runtime-test applicability: record runtime commands and results, or explain why runtime tests are not required.
- Visual/oracle/performance applicability: record required artifacts and review results, or explain why they are not applicable.
- Any known local validation failures, including command output summaries and why the issue can or cannot close with those failures.

For Pi implementation handoffs, leave a reviewable local worktree diff and do not commit, push, merge, or create PRs.
