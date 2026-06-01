# Mission

Build a native C++/Qt library that functions and looks like PyQtGraph from the outside.

## Product target

A C++ user should be able to:

- Use PyQtGraph-like public classes, names, examples, and visual behavior.
- Build, install, and consume the library from CMake with `find_package(pyqtgraph-cpp)`.
- Compare behavior against a pinned PyQtGraph reference through deterministic tests, oracles, examples, screenshots, and interaction checks.
- Treat the implementation as a native C++ library, not as Python embedded behind a C++ facade.

## In scope

- Externally observable PyQtGraph behavior: plotting, axes, curves, images, colors, interactions, layouts, widgets, and examples.
- Qt 6 GUI/rendering code and OpenCV/C++ math/data structures where they replace Python/NumPy behavior.
- PyQtGraph-aligned class names, object names, file names, folder hierarchy, and example names unless an issue explicitly authorizes divergence.
- Incremental, issue-scoped ports backed by C++ tests, PyQtGraph oracle probes, and visual evidence when pixels are part of correctness.
- Clean downstream C++ package usage and narrowly required build/test integration.

## Out of scope

- Python wrappers, Python import machinery, REPL/Jupyter behavior, multiprocessing/remote graphics internals, and Python-only debugging helpers.
- Line-by-line translation of upstream internals that do not affect external C++ behavior.
- Matplotlib integration, OpenGL/3D, and broad performance tuning until stable 2D parity exists, unless a specific issue owns the exception.
- Broad refactors, placeholder trees, or speculative APIs not needed for the issue's observable outcome.

## Operating model

- GitHub issues are the work packets. Normal implementation issues must be small, dependency-clear, owned-file-scoped, testable, and tied to a PyQtGraph reference.
- Implementation is test/oracle first: if behavior is unclear, probe the pinned PyQtGraph reference before guessing.
- Archon/Pi workers may implement, rework, review, and release in isolated worktrees, but those workers never merge.
- Only the validation/merge controller may auto-merge, and only after `WORKFLOW.md` enables auto-merge and every governed readiness, scope, evidence, deterministic-test, oracle/visual, autoreview, protected-file, and holdout gate passes.

## Hard invariants

- Do not replace the native C++ library goal with a Python wrapper.
- External behavior parity matters more than copying upstream Python internals.
- Do not edit files outside the issue-owned scope except narrowly allowed shared integration files.
- Protected governance/automation files require explicit automation/governance scope and must not be auto-merged without human review.
- If requirements, visual parity, architecture, or safety are unclear, fail closed and request human direction.
