# P7.01 OpenGL Backend Policy Completion Report

- Author/tool: Pi worker implementation subagent; final validation by Pi parent/tester
- Date: 2026-05-26
- Issue: GitHub #180 / P7.01
- Validation class: decision-doc
- Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-180`

## Summary

Implemented the local OpenGL backend policy as a docs-only decision artifact. The policy records local OpenGL evidence as authoritative, prefers deterministic offscreen/headless validation when available, permits explicitly labeled real GPU paths when required or unavoidable, and defines required metadata for future OpenGL render proofs.

## Pre-implementation evidence

These pre-implementation checks were run by the parent after cleaning scout artifacts and before this implementation:

```text
$ git status --short --untracked-files=all
exit 0; output was empty

$ test -f docs/opengl-backend-policy.md
exit 1

$ test -f reports/issues/P7.01/completion.md
exit 1
```

This was the expected initial failure for the focused decision-doc proof: the required policy and completion artifacts did not exist yet, and the worktree was clean.

## Artifacts

- `docs/opengl-backend-policy.md` - decision artifact defining the local OpenGL backend policy, required render-proof metadata, accepted native Qt/C++ equivalence, explicit P7.01 non-scope decisions, affected manifest entries/classes, and follow-up issue links.
- `reports/issues/P7.01/completion.md` - this completion report recording issue scope, ownership, policy decisions, applicability decisions, and validation handoff status.

## Scope and ownership

P7.01 records a validation/equivalence policy only; the manifest-expanded source target paths below are affected by the policy but were not edited:

- `include/pyqtgraph/Qt/OpenGLConstants.hpp`, `src/pyqtgraph/Qt/OpenGLConstants.cpp`
- `include/pyqtgraph/Qt/OpenGLHelpers.hpp`, `src/pyqtgraph/Qt/OpenGLHelpers.cpp`
- `include/pyqtgraph/opengl/GLGraphicsItem.hpp`, `src/pyqtgraph/opengl/GLGraphicsItem.cpp`
- `include/pyqtgraph/opengl/GLViewWidget.hpp`, `src/pyqtgraph/opengl/GLViewWidget.cpp`
- `include/pyqtgraph/opengl/MeshData.hpp`, `src/pyqtgraph/opengl/MeshData.cpp`
- `include/pyqtgraph/opengl/__init__.hpp`, `src/pyqtgraph/opengl/__init__.cpp`
- `include/pyqtgraph/opengl/items/GLAxisItem.hpp`, `src/pyqtgraph/opengl/items/GLAxisItem.cpp`
- `include/pyqtgraph/opengl/items/GLBarGraphItem.hpp`, `src/pyqtgraph/opengl/items/GLBarGraphItem.cpp`
- `include/pyqtgraph/opengl/items/GLBoxItem.hpp`, `src/pyqtgraph/opengl/items/GLBoxItem.cpp`
- `include/pyqtgraph/opengl/items/GLGradientLegendItem.hpp`, `src/pyqtgraph/opengl/items/GLGradientLegendItem.cpp`
- `include/pyqtgraph/opengl/items/GLGraphItem.hpp`, `src/pyqtgraph/opengl/items/GLGraphItem.cpp`
- `include/pyqtgraph/opengl/items/GLGridItem.hpp`, `src/pyqtgraph/opengl/items/GLGridItem.cpp`
- `include/pyqtgraph/opengl/items/GLImageItem.hpp`, `src/pyqtgraph/opengl/items/GLImageItem.cpp`
- `include/pyqtgraph/opengl/items/GLLinePlotItem.hpp`, `src/pyqtgraph/opengl/items/GLLinePlotItem.cpp`
- `include/pyqtgraph/opengl/items/GLMeshItem.hpp`, `src/pyqtgraph/opengl/items/GLMeshItem.cpp`
- `include/pyqtgraph/opengl/items/GLScatterPlotItem.hpp`, `src/pyqtgraph/opengl/items/GLScatterPlotItem.cpp`
- `include/pyqtgraph/opengl/items/GLSurfacePlotItem.hpp`, `src/pyqtgraph/opengl/items/GLSurfacePlotItem.cpp`
- `include/pyqtgraph/opengl/items/GLTextItem.hpp`, `src/pyqtgraph/opengl/items/GLTextItem.cpp`
- `include/pyqtgraph/opengl/items/GLVolumeItem.hpp`, `src/pyqtgraph/opengl/items/GLVolumeItem.cpp`
- `include/pyqtgraph/opengl/items/__init__.hpp`, `src/pyqtgraph/opengl/items/__init__.cpp`
- `include/pyqtgraph/opengl/shaders.hpp`, `src/pyqtgraph/opengl/shaders.cpp`

Changed issue-owned paths:

- `docs/opengl-backend-policy.md`
- `reports/issues/P7.01/completion.md`

Shared wiring paths changed: none.

No source, tests, scripts, CMake, workflow, manifest, dashboard, validation-guide, example, or generated files were intentionally edited.

## Policy decisions recorded

- Local OpenGL evidence is authoritative; GitHub Actions, hosted GPU CI, and GPU-runner status are not required proof.
- Conservative default: prefer deterministic offscreen/headless validation when available and representative.
- Real GPU/hardware-backed paths are acceptable when required by the issue or when offscreen/headless is unavailable, but they must be explicitly labeled.
- Every OpenGL render proof must record renderer, vendor, backend/platform path, context profile, context version, framebuffer size, and whether the path is software, headless/offscreen, or real GPU/hardware-backed.
- Unusable OpenGL contexts must be recorded as blocked/skipped environment evidence, not replaced with fake screenshots, placeholders, or synthetic metadata.
- Accepted C++ equivalence is native Qt/C++ OpenGL through `QOpenGLWidget`, `QOpenGLContext`/`QOffscreenSurface`, or equivalent Qt offscreen-capable context paths as appropriate; Python/OpenGL wrappers are not accepted as the native C++ port.
- Later OpenGL rendering issues must follow visual-render rules for non-blank guards, metrics, artifacts, and manual inspection; interaction tasks also need state assertions.
- P7.01 itself requires no runtime, visual, upstream-oracle, or performance artifacts.

## Affected manifest entries summary

The policy lists the OpenGL manifest area affected by this decision without editing `port_manifest.yaml`:

- Qt OpenGL helper entries: `pyqtgraph/Qt/OpenGLConstants.py`, `pyqtgraph/Qt/OpenGLHelpers.py`.
- Core OpenGL entries: `pyqtgraph/opengl/GLGraphicsItem.py`, `pyqtgraph/opengl/GLViewWidget.py`, `pyqtgraph/opengl/MeshData.py`, `pyqtgraph/opengl/__init__.py`, `pyqtgraph/opengl/shaders.py`.
- OpenGL item entries: all `pyqtgraph/opengl/items/*.py` entries named in the policy, including `GLAxisItem`, `GLBarGraphItem`, `GLBoxItem`, `GLGradientLegendItem`, `GLGraphItem`, `GLGridItem`, `GLImageItem`, `GLLinePlotItem`, `GLMeshItem`, `GLScatterPlotItem`, `GLSurfacePlotItem`, `GLTextItem`, `GLVolumeItem`, and `__init__`.

Affected classes/concepts recorded: `GraphicsViewGLWidget`, `GLViewWidget`, all `GL*Item` classes, `MeshData`, `Shader`, `VertexShader`, `FragmentShader`, and `ShaderProgram`.

## Follow-up issues recorded

The policy links follow-up issues [#181](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/181) through [#190](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/190) with concise purpose labels for later OpenGL helpers, base items, view/camera behavior, mesh data, shaders, item families, package wiring, examples, and validation closeout.

## Manifest/dashboard applicability

Manifest and dashboard updates are not applicable. P7.01 defines validation policy only; it does not change tracked source, example, class, asset, or status entries.

## Runtime tests

Runtime tests are not required. This is a docs-only decision-doc issue with no executable behavior changes.

## Visual validation

Visual validation is not applicable to P7.01. The issue does not affect rendering behavior, pixels, screenshots, example output, or visual artifacts. Later OpenGL rendering issues must follow the policy and the visual-render rules.

## Runtime/oracle/performance artifacts

Runtime, upstream-oracle, and performance artifacts are not applicable because the validation class is decision-doc and the implementation changes only documentation/report files.

## Validation

Pre-implementation checks run by the worker before creating these artifacts:

```text
$ git status --short --untracked-files=all && test ! -e docs/opengl-backend-policy.md && test ! -e reports/issues/P7.01/completion.md
exit 0; output was empty
```

Focused post-implementation artifact/content checks:

```text
$ test -f docs/opengl-backend-policy.md
exit 0

$ test -f reports/issues/P7.01/completion.md
exit 0

$ rg -n "GitHub #180|P7\.01|decision-doc|renderer|vendor|context profile|context version|framebuffer size|QOpenGLWidget|QOpenGLContext|QOffscreenSurface|#181|#190" docs/opengl-backend-policy.md reports/issues/P7.01/completion.md
exit 0
```

Issue-required validation commands run after implementation:

```text
$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
exit 1
```

The proposed-issue linter failure is pre-existing/live GitHub issue metadata outside P7.01 ownership. Current failure examples include `github-issue-97.md` through `github-issue-207.md` blocked-by entries that do not match local issues, including `github-issue-180.md: blocked-by entry does not match a local issue: P1.04`. P7.01 did not edit local issue mirrors, linter code, GitHub labels, or issue bodies.

```text
$ git diff --check
exit 0

$ git diff --name-only origin/main...HEAD
exit 0; output was empty
```

`git diff --name-only origin/main...HEAD` is empty because Pi must not commit. The docs/report artifacts are left as uncommitted local diff using intent-to-add so the reviewable worktree diff lists them without committing:

```text
$ git diff --name-only
exit 0
docs/opengl-backend-policy.md
reports/issues/P7.01/completion.md

$ git status --short --untracked-files=all
exit 0
 A docs/opengl-backend-policy.md
 A reports/issues/P7.01/completion.md
```

Validated artifact paths:

- `docs/opengl-backend-policy.md`
- `reports/issues/P7.01/completion.md`
