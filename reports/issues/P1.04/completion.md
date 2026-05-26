# P1.04 Local Validation Contract Completion Report

- Author/tool: Pi worker implementation subagent; final validation by Pi parent/tester
- Date: 2026-05-26
- Issue: GitHub #108 / P1.04
- Validation class: decision-doc
- Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-108`

## Summary

Implemented the local platform validation contract as a docs-only decision artifact. The contract records local-only validation as the conservative default for P0/P1 issues, defines Linux/macOS/Windows local command expectations, and states that GitHub Actions is not required proof for these AFK issues.

## Pre-implementation evidence

These pre-implementation checks were run before editing:

```text
$ test -f docs/local-validation.md
exit 1

$ test -d reports/issues/P1.04
exit 1

$ git status --short
exit 0; output was empty
```

## Artifacts

- `docs/local-validation.md` - decision artifact defining the local validation contract, source policy citations, platform command matrix, explicit non-CI decision, accepted C++ proof equivalence, follow-up policy, affected manifest entries, and per-issue closeout checklist.
- `reports/issues/P1.04/completion.md` - this completion report recording issue scope, ownership, applicability decisions, exact local commands, exit codes, and artifact paths.

## Scope and ownership

Manifest-expanded target paths: not applicable; this issue has no manifest source or example selectors.

Changed issue-owned paths:

- `docs/local-validation.md`
- `reports/issues/P1.04/completion.md`

Shared wiring paths changed: none.

No source, tests, scripts, CMake, workflow, manifest, dashboard, `.github`, or generated files were intentionally edited.

## Policy and decision recorded

- Conservative default: P0/P1 issue validation is local-only unless a future explicitly owned issue changes the policy.
- Policy citations: `docs/proposed-issues/README.md`, `docs/proposed-issues/VALIDATION-GUIDE.md`, `docs/parity-contract.md`, and `WORKFLOW.md`.
- Affected manifest entries: none; this policy applies to issue validation/reporting rather than tracked source/example/class/asset entries.
- Accepted C++ equivalence: local command evidence plus issue completion artifacts are the accepted proof mechanism for native C++ port tasks.
- Explicit non-CI decision: do not add or require GitHub Actions for this validation contract.
- Follow-up link: future local commit-gate work can build on this policy through [#109](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/109); CI mirrors or alternate validation systems require separately owned scope.

## Manifest/dashboard applicability

Manifest and dashboard updates are not applicable. This issue defines validation and reporting policy only; it does not change tracked source, example, class, asset, or status entries.

## Runtime tests

Runtime tests are not required. This is a docs-only decision-doc issue with no executable behavior changes.

## Visual validation

Visual validation is not applicable. The issue does not affect rendering behavior, pixels, screenshots, example output, or visual artifacts.

## Runtime/oracle/performance artifacts

Runtime, upstream-oracle, and performance artifacts are not applicable because the validation class is decision-doc and the implementation changes only documentation/report files.

## Validation

Post-implementation local validation:

```text
$ test -f docs/local-validation.md
exit 0

$ test -f reports/issues/P1.04/completion.md
exit 0

$ rg -n "local-only|GitHub Actions|decision-doc|Linux|macOS|Windows" docs/local-validation.md reports/issues/P1.04/completion.md
reports/issues/P1.04/completion.md:6:- Validation class: decision-doc
reports/issues/P1.04/completion.md:11:Implemented the local platform validation contract as a docs-only decision artifact. The contract records local-only validation as the conservative default for P0/P1 issues, defines Linux/macOS/Windows local command expectations, and states that GitHub Actions is not required proof for these AFK issues.
reports/issues/P1.04/completion.md:48:Runtime tests are not required. This is a docs-only decision-doc issue with no executable behavior changes.
reports/issues/P1.04/completion.md:56:Runtime, upstream-oracle, and performance artifacts are not applicable because the validation class is decision-doc and the implementation changes only documentation/report files.
docs/local-validation.md:10:- Validation is local-only for this issue set; do not add GitHub Actions or make completion depend on GitHub infrastructure (`docs/proposed-issues/README.md`, `docs/proposed-issues/VALIDATION-GUIDE.md`).
docs/local-validation.md:11:- Decision-only issues use the `decision-doc` validation class: proof is a decision/equivalence artifact and completion report, not runtime tests unless executable behavior changes (`docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`).
docs/local-validation.md:12:- The full-port parity contract establishes native C++ port scope and already records local-only validation as the default policy (`docs/parity-contract.md`).
docs/local-validation.md:17:All P0/P1 issue validation is local-only by default unless a future explicitly owned issue changes this policy. GitHub Actions or other CI may mirror local commands, but CI status is never required proof for these AFK issues.
docs/local-validation.md:25:- A portable platform command matrix lets Linux, macOS, and Windows contributors run equivalent checks locally without requiring GitHub-hosted infrastructure.
docs/local-validation.md:41:Explicit non-CI decision: do not add or require GitHub Actions for this validation contract. A CI mirror may be useful later, but local commands and committed artifacts remain the reviewable proof for this issue set unless a future issue explicitly owns and approves a policy change.
docs/local-validation.md:53:### Linux
docs/local-validation.md:68:When an issue specifically needs the platform-style CMake preset, run the Linux preset locally on Linux:
docs/local-validation.md:78:### macOS
docs/local-validation.md:93:When an issue specifically needs the platform-style CMake preset, run the macOS preset locally on macOS:
docs/local-validation.md:101:For docs-only decision issues, the same focused local proof used on Linux is acceptable when no executable behavior changes.
docs/local-validation.md:103:### Windows
docs/local-validation.md:117:When an issue specifically needs the platform-style CMake preset, run the Windows preset locally on Windows:
docs/local-validation.md:125:Repository `scripts/*` commands are POSIX shell scripts. Run them locally with Git Bash, WSL, or an equivalent local POSIX shell; GitHub Actions is not required:
docs/local-validation.md:131:For docs-only decision issues on Windows, focused local proof may use PowerShell file checks and Git ownership checks, plus Git Bash/WSL only when a repository script is in scope.
exit 0

$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
github-issue-96.md: blocked-by entry does not match a local issue: P0.01
github-issue-106.md: blocked-by entry does not match a local issue: P1.01
github-issue-107.md: blocked-by entry does not match a local issue: P1.01
github-issue-108.md: blocked-by entry does not match a local issue: P0.01
github-issue-109.md: blocked-by entry does not match a local issue: P1.01
github-issue-110.md: blocked-by entry does not match a local issue: P0.07
github-issue-111.md: blocked-by entry does not match a local issue: P0.08
github-issue-212.md: blocked-by entry does not match a local issue: P1.01
github-issue-120.md: blocked-by entry does not match a local issue: P0.06
github-issue-129.md: blocked-by entry does not match a local issue: P0.01
github-issue-130.md: blocked-by entry does not match a local issue: P1.01
github-issue-166.md: blocked-by entry does not match a local issue: P0.01
github-issue-195.md: blocked-by entry does not match a local issue: P0.01
github-issue-197.md: blocked-by entry does not match a local issue: P0.01
github-issue-201.md: blocked-by entry does not match a local issue: P1.01
github-issue-207.md: blocked-by entry does not match a local issue: P0.01
exit 1

$ git diff --check
exit 0

$ git diff --name-only origin/main...HEAD
exit 0
```

`git diff --name-only origin/main...HEAD` is empty because Pi must not commit; the issue changes remain uncommitted working-tree files. The new files were marked intent-to-add so the reviewable local diff lists them without committing:

```text
$ git diff --name-only
docs/local-validation.md
reports/issues/P1.04/completion.md
exit 0

$ git status --short --untracked-files=all
 A docs/local-validation.md
 A reports/issues/P1.04/completion.md
exit 0
```

The proposed-issue linter failure is live GitHub issue metadata outside this issue's owned files. No local issue mirrors, linter code, GitHub labels, or issue bodies were changed for P1.04.

Validated artifact paths:

- `docs/local-validation.md`
- `reports/issues/P1.04/completion.md`
