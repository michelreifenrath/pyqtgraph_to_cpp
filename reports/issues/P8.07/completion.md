# P8.07 Jupyter Embedded Graphics Equivalence Completion Report

- Author/tool: Pi worker implementation subagent; final validation by Pi parent/tester
- Date: 2026-05-26
- Issue: GitHub #197 / P8.07
- Validation class: decision-doc
- Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-197`

## Summary

Implemented the Jupyter embedded graphics equivalence decision as a docs-only artifact. The decision records Jupyter remote-framebuffer/notebook wrappers and Python-only frontend embedding as non-port by default, and accepts native Qt widgets/views/items plus any future explicitly scoped native C++ embedding surfaces as the C++ equivalence.

Rework update: the prior autoreview failure was caused by live GitHub issue metadata, where open generated issues still referenced already-closed blocker issue IDs. The authoritative GitHub issue bodies were updated to remove only closed blocker references, including #197's stale `Blocked by: P0.01` entry. No repository source, check, workflow, or policy files were changed for that metadata repair, and the required proposed-issue validation now passes.

## Pre-implementation evidence

These pre-implementation checks were run by the parent before editing:

```text
$ test -f docs/jupyter-embedded-graphics-equivalence.md
exit 1

$ test -d reports/issues/P8.07
exit 1
```

## Artifacts

- `docs/jupyter-embedded-graphics-equivalence.md` - decision artifact citing `docs/parity-contract.md` and `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`, recording conservative default, rationale, affected manifest entries, explicit non-port/equivalence decisions, and follow-up/waiver.
- `reports/issues/P8.07/completion.md` - this completion report recording issue scope, ownership, applicability decisions, validation status, and artifact paths.

## Scope and ownership

Manifest-expanded source target paths considered by the decision:

- `pyqtgraph/jupyter/GraphicsView.py` -> `include/pyqtgraph/jupyter/GraphicsView.hpp`, `src/pyqtgraph/jupyter/GraphicsView.cpp`
- `pyqtgraph/jupyter/__init__.py` -> `include/pyqtgraph/jupyter/__init__.hpp`, `src/pyqtgraph/jupyter/__init__.cpp`

Manifest class rows considered by the decision:

- `GraphicsView` in `pyqtgraph/jupyter/GraphicsView.py`
- `GraphicsLayoutWidget` in `pyqtgraph/jupyter/GraphicsView.py`
- `PlotWidget` in `pyqtgraph/jupyter/GraphicsView.py`

Context-only example row:

- `pyqtgraph/examples/jupyter_console_example.py` -> `examples/jupyter_console_example.cpp`

Changed issue-owned paths:

- `docs/jupyter-embedded-graphics-equivalence.md`
- `reports/issues/P8.07/completion.md`

Shared wiring paths changed: none.

No source, examples, tests, scripts, CMake, workflow, automation policy, manifest, dashboard, or generated files were intentionally edited in the repository.

## Rework metadata repair

The required proposed-issue validation previously failed because live open GitHub issues referenced blocker issue IDs that already existed only as closed issues. The authoritative GitHub issue bodies were updated to remove those closed blocker references before rerunning validation:

- `#97` `P0.03`: `P0.02` -> `None`
- `#98` `P0.04`: `P0.02` -> `None`
- `#103` `P0.09`: `P0.02` -> `None`
- `#111` `P1.07`: `P0.08` -> `None`
- `#112` `P1.08`: `P1.06` -> `None`
- `#118` `P1.14`: `P1.04` -> `None`
- `#120` `P2.01`: `P0.06` -> `None`
- `#124` `P2.05`: `P1.03` -> `None`
- `#127` `P2.08`: `P1.04` -> `None`
- `#129` `P2.10`: `P0.01` -> `None`
- `#130` `P3.01`: `P1.01` -> `None`
- `#166` `P5.09`: `P0.01` -> `None`
- `#180` `P7.01`: `P1.04` -> `None`
- `#195` `P8.05`: `P0.01` -> `None`
- `#197` `P8.07`: `P0.01` -> `None`
- `#201` `P9.01`: `P1.01` -> `None`
- `#202` `P9.02`: `P0.02` -> `None`
- `#207` `P9.07`: `P0.01` -> `None`
- `#212` `P10.04`: `P1.01, P10.01` -> `P10.01`

## Policy and decision recorded

- Conservative default: Jupyter remote-framebuffer/notebook wrappers and Python-only frontend embedding are non-port by default.
- Policy citations: `docs/parity-contract.md` and `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`.
- Affected manifest entries: the Jupyter source and class rows listed above, plus the context-only `jupyter_console_example.py` example row.
- Accepted C++ equivalence: native Qt widgets/views/items (`QGraphicsView`, PyQtGraph-native widgets/items where implemented) and future explicitly scoped native C++ embedding surfaces.
- Explicit non-port decision: do not port `jupyter_rfb`, notebook wrappers, Python package initializer behavior, or Python-only frontend embedding for these Jupyter entries.
- Follow-up/waiver: #197 resolves the parity-contract Jupyter/native embedding follow-up. No C++ source/test follow-up is required unless a later issue explicitly scopes a native C++ embedding surface.

## Manifest/dashboard applicability

Manifest and dashboard updates are not applicable. This issue records a decision/equivalence artifact only and does not own edits to `port_manifest.yaml`, generated dashboards, source status, class status, example status, or validation-level metadata.

The `pyqtgraph/examples/jupyter_console_example.py` row is context-only for this issue. Manifest example selectors are none for #197, so no example status change is made.

## Runtime tests

Runtime tests are not required. This is a docs-only decision-doc issue with no executable behavior changes.

## Visual validation

Visual validation is not applicable. The issue does not affect rendering behavior, pixels, screenshots, example output, or visual artifacts.

## Runtime/oracle/performance artifacts

Runtime, upstream-oracle, and performance artifacts are not applicable because the validation class is decision-doc and the implementation changes only documentation/report files.

## Validation

Worker-local checks run after implementation:

```text
$ test -f docs/jupyter-embedded-graphics-equivalence.md
exit 0

$ test -f reports/issues/P8.07/completion.md
exit 0

$ rg -n "GitHub #197|P8.07|decision-doc|Jupyter remote-framebuffer|native Qt widgets|jupyter_console_example|#197 resolves" docs/jupyter-embedded-graphics-equivalence.md reports/issues/P8.07/completion.md
exit 0

$ git diff --check
exit 0
```

Tester/parent post-rework validation:

```text
$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
exit 0

$ git diff --check
exit 0

$ git diff --name-only origin/main...HEAD
docs/jupyter-embedded-graphics-equivalence.md
reports/issues/P8.07/completion.md
exit 0

$ git diff --name-only
reports/issues/P8.07/completion.md
exit 0

$ git status --short --untracked-files=all
 M reports/issues/P8.07/completion.md
exit 0
```

The required `scripts/check_proposed_issues` gate now passes after updating authoritative GitHub issue metadata to remove stale references to closed blocker issues. No local source, manifest, validation script, workflow, automation policy, or issue-owned decision artifact content was changed for the rework.

Validated artifact paths:

- `docs/jupyter-embedded-graphics-equivalence.md`
- `reports/issues/P8.07/completion.md`
