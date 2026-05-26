# OpenGL Backend Policy

Issue: GitHub #180 / P7.01

Validation class: `decision-doc`

This document defines the local OpenGL backend policy for native C++ PyQtGraph OpenGL work. P7.01 is a policy/equivalence decision artifact only; it does not implement renderer behavior.

## Source-of-truth policy

- Decision-only issues use `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`: proof is a decision/equivalence document and completion report, not runtime tests, unless executable behavior changes.
- Later OpenGL rendering issues use `docs/proposed-issues/VALIDATION-GUIDE.md#opengl-render` and the visual-render rules in that guide.
- Validation remains local-only under `docs/local-validation.md` and `docs/parity-contract.md`; GitHub Actions or hosted GPU CI is not required proof.
- Repository workflow gates and handoff rules remain local under `WORKFLOW.md`; do not commit, push, merge, or create PRs from Pi implementation work.

## Local OpenGL backend decision

Local OpenGL evidence is authoritative. OpenGL issues must record evidence from the developer or tester's local checkout and local graphics environment. Completion does not depend on GitHub Actions, hosted GPU runners, or any GPU CI service.

Conservative default: prefer deterministic offscreen/headless validation when it is available and representative for the issue. Real GPU or hardware-backed paths are permitted when the issue requires them, when offscreen/headless support is unavailable, or when the local environment exposes only a hardware path. The report must label which path was used and why that path is acceptable for the issue.

If a usable OpenGL context cannot be created, record the environment as blocked or skipped evidence with the attempted backend and failure details. Do not fake screenshots, placeholder images, synthetic renderer strings, or copied artifacts to satisfy an OpenGL proof.

## Required OpenGL render proof metadata

Every OpenGL render proof must record:

- Renderer string.
- Vendor string.
- Backend/platform path, such as EGL, GLX, WGL, CGL, Wayland, X11, Qt RHI/OpenGL path, Qt offscreen platform, or another explicit Qt/platform route.
- Context profile.
- Context version.
- Framebuffer size.
- Whether the path is software, headless/offscreen, or real GPU/hardware-backed.

When an issue uses multiple paths, record the metadata for each path and identify which path gates completion.

## Accepted C++ equivalence

Accepted C++ equivalence is native Qt/C++ OpenGL, using `QOpenGLWidget`, `QOpenGLContext` with `QOffscreenSurface`, or equivalent Qt offscreen-capable context paths when appropriate for the issue. The implementation must be a native C++ library path, not a Python/OpenGL wrapper, Python shim, or Python-bound renderer.

OpenGL support may use Qt/C++ context management and C++ rendering code directly. OpenCV/C++ may be used where math or data structures are relevant, but OpenGL rendering behavior must remain native C++/Qt rather than delegating to PyQtGraph or Python OpenGL wrappers.

## Later rendering issue proof rules

Later OpenGL rendering issues must also follow visual-render rules:

- Produce non-blank guarded render artifacts when pixels are affected.
- Record metrics, artifact paths, and manual visual inspection notes for reference, actual, and diff outputs when visual comparison is in scope.
- Fail or block rather than accepting both-blank, placeholder, or uninspectable images.
- For camera, navigation, picking, or interaction tasks, include state assertions for camera, transform, selection, item state, emitted signals/callbacks, or other observable state as applicable.

P7.01 itself requires no runtime, visual, upstream-oracle, or performance artifacts because it is a docs-only `decision-doc` issue.

## Explicit P7.01 non-scope decisions

P7.01 does not implement or modify:

- Renderers, shaders, widgets, items, mesh handling, or OpenGL helpers.
- Examples, tests, CMake, CI, scripts, generated dashboards, or validation guide text.
- `port_manifest.yaml` status, class, source, example, or dashboard entries.

Those changes require later issues with owned implementation scope.

## Affected manifest entries

This policy affects the expected validation and equivalence decisions for the following manifest entries, but P7.01 does not edit `port_manifest.yaml`:

- `pyqtgraph/Qt/OpenGLConstants.py`
- `pyqtgraph/Qt/OpenGLHelpers.py`
- `pyqtgraph/opengl/GLGraphicsItem.py`
- `pyqtgraph/opengl/GLViewWidget.py`
- `pyqtgraph/opengl/MeshData.py`
- `pyqtgraph/opengl/__init__.py`
- `pyqtgraph/opengl/shaders.py`
- `pyqtgraph/opengl/items/GLAxisItem.py`
- `pyqtgraph/opengl/items/GLBarGraphItem.py`
- `pyqtgraph/opengl/items/GLBoxItem.py`
- `pyqtgraph/opengl/items/GLGradientLegendItem.py`
- `pyqtgraph/opengl/items/GLGraphItem.py`
- `pyqtgraph/opengl/items/GLGridItem.py`
- `pyqtgraph/opengl/items/GLImageItem.py`
- `pyqtgraph/opengl/items/GLLinePlotItem.py`
- `pyqtgraph/opengl/items/GLMeshItem.py`
- `pyqtgraph/opengl/items/GLScatterPlotItem.py`
- `pyqtgraph/opengl/items/GLSurfacePlotItem.py`
- `pyqtgraph/opengl/items/GLTextItem.py`
- `pyqtgraph/opengl/items/GLVolumeItem.py`
- `pyqtgraph/opengl/items/__init__.py`

## Affected classes and concepts

- `GraphicsViewGLWidget`
- `GLViewWidget`
- All `GL*Item` classes
- `MeshData`
- `Shader`
- `VertexShader`
- `FragmentShader`
- `ShaderProgram`

## Follow-up issues

- [#181](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/181) - Port OpenGL constants and helper context utilities.
- [#182](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/182) - Establish core GL graphics item base behavior.
- [#183](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/183) - Implement GL view widget and camera/navigation shell.
- [#184](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/184) - Port mesh data and geometry preparation.
- [#185](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/185) - Port shader abstractions and shader program handling.
- [#186](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/186) - Implement axis, grid, box, and legend-style OpenGL items.
- [#187](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/187) - Implement line, scatter, graph, and bar OpenGL plot items.
- [#188](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/188) - Implement mesh and surface OpenGL items.
- [#189](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/189) - Implement image, volume, and text OpenGL items.
- [#190](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/190) - Integrate OpenGL package wiring, examples, and validation closeout.
