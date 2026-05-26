# P5.09 MatplotlibWidget equivalence completion report

- Author/tool: Pi worker implementation subagent
- Date: 2026-05-26
- Issue: GitHub #166 / P5.09
- Validation class: decision-doc
- Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-166`

## Summary

Implemented a docs-only decision artifact for `pyqtgraph/widgets/MatplotlibWidget.py`. The decision applies `docs/parity-contract.md` and records `MatplotlibWidget` as explicit non-port Matplotlib bridge scope. Accepted C++ equivalence is native Qt/C++ plotting, rendering, widget composition, and exporter functionality rather than embedding Matplotlib.

## Pre-implementation evidence

```text
$ test -f docs/matplotlib-widget-equivalence.md
exit 1

$ test -d reports/issues/P5.09
exit 1

$ git status --short --untracked-files=all
exit 0; output was empty
```

## Artifacts

- `docs/matplotlib-widget-equivalence.md` - policy-backed equivalence/non-port decision for `MatplotlibWidget`.
- `reports/issues/P5.09/completion.md` - this completion/proof report.

## Affected manifest entries recorded

Source entry from `port_manifest.yaml`:

- `pyqtgraph/widgets/MatplotlibWidget.py`
- `include/pyqtgraph/widgets/MatplotlibWidget.hpp`
- `src/pyqtgraph/widgets/MatplotlibWidget.cpp`
- subsystem `widgets`
- status `not_started`
- completion `missing`

Class entry from `port_manifest.yaml`:

- class `MatplotlibWidget`
- upstream `pyqtgraph/widgets/MatplotlibWidget.py`
- target header `include/pyqtgraph/widgets/MatplotlibWidget.hpp`
- target source `src/pyqtgraph/widgets/MatplotlibWidget.cpp`
- base `QtWidgets.QWidget`
- line `11`
- status `not_started`
- completion `missing`

## Scope and ownership

Changed issue-owned paths:

- `docs/matplotlib-widget-equivalence.md`
- `reports/issues/P5.09/completion.md`

No `port_manifest.yaml`, CMake, source, tests, scripts, dashboards, `WORKFLOW.md`, or automation policy files were edited.

## Policy and decision recorded

- Policy citations: `docs/parity-contract.md` and `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`.
- Conservative default: Matplotlib bridge classes are out of native C++ port scope.
- Explicit non-port decision: do not port `MatplotlibWidget` as a Matplotlib-hosting Qt widget.
- Accepted C++ equivalence: native Qt/C++ rendering/export/widget functionality.
- Follow-up links: [#166](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/166) and [#194](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/194).

## Runtime tests

Runtime tests are not required. This is a docs-only `decision-doc` issue and changes no executable behavior.

## Visual validation

Visual validation is not applicable. This issue does not affect rendering behavior, screenshots, examples, or generated image/export output.

## Manifest/dashboard applicability

Manifest and dashboard updates are not part of this slice. The affected entries are listed as evidence only; no manifest status, completion field, dashboard, or generated inventory file was edited.

## Validation

Post-implementation local validation:

```text
$ test -f docs/matplotlib-widget-equivalence.md
exit 0

$ test -f reports/issues/P5.09/completion.md
exit 0

$ rg -n "MatplotlibWidget|decision-doc|parity-contract|Qt/C\+\+|#166|#194|non-port|pyqtgraph/widgets/MatplotlibWidget.py|include/pyqtgraph/widgets/MatplotlibWidget.hpp|src/pyqtgraph/widgets/MatplotlibWidget.cpp|QtWidgets\.QWidget|visual validation|Runtime tests" docs/matplotlib-widget-equivalence.md reports/issues/P5.09/completion.md
exit 0

$ git add -N docs/matplotlib-widget-equivalence.md reports/issues/P5.09/completion.md
exit 0

$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
exit 1; output:
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

$ git diff --check
exit 0

$ git diff --name-only origin/main...HEAD
exit 0; output was empty because this handoff leaves uncommitted working-tree changes as requested.

$ git diff --name-only
exit 0; output:
docs/matplotlib-widget-equivalence.md
reports/issues/P5.09/completion.md

$ git status --short --untracked-files=all
exit 0; output:
 A docs/matplotlib-widget-equivalence.md
 A reports/issues/P5.09/completion.md
```

The `scripts/check_proposed_issues` failure is a repository proposed-issue metadata blocker outside this docs-only change; this issue's changed files remain limited to the two focused decision/report artifacts above.

No broad runtime, C++, CMake, visual, or exporter suites were run because this issue is a docs-only `decision-doc` and changes no executable behavior.
