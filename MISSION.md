# Mission

Build a native C++/Qt library that functions and looks like PyQtGraph from the outside.

## In scope

- PyQtGraph-like public classes, names, examples, and visual behavior.
- Qt 6 GUI/rendering code and OpenCV/C++ math/data structures where needed.
- Deterministic C++ tests plus oracle/visual checks against the pinned PyQtGraph reference.
- Clean install and downstream C++ usage through `find_package(pyqtgraph-cpp)`.

## Out of scope

- Python wrappers, import machinery, REPL/Jupyter behavior, and Python-only debugging helpers.
- OpenGL/3D, Matplotlib integration, and broad performance tuning until 2D parity is stable.
- Line-by-line translation of upstream internals that do not affect external C++ behavior.

## Hard invariants

- No Python wrapper replaces the native C++ library goal.
- Normal implementation issues must be small, owned-file-scoped, testable, and dependency-clear.
- Holdout validation is required before any merge-capable automation may proceed.
- Governance and automation files are protected from normal product issues.
