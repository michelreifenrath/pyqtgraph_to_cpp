# Mission

Build **CppQtGraph** as a lightweight native C++/Qt library that matches PyQtGraph's externally visible plotting behavior without embedding or wrapping Python.

This document is the source of truth for product direction and porting strategy.

## Product target

A C++ user should be able to:

- Use PyQtGraph-like class names, example names, and visual behavior from native C++.
- Build, install, and consume the library from CMake with `find_package(CppQtGraph)` and `CppQtGraph::CppQtGraph`.
- Compare behavior against the pinned PyQtGraph reference through deterministic fixtures, numeric oracles, screenshots, and interaction checks.
- Treat the result as a C++/Qt library, not as a Python facade.

## Hard invariants

- The runtime library must be native C++/Qt only: no Python interpreter, Python wrapper, PyQtGraph runtime dependency, or embedded Python bridge.
- PyQtGraph is a development reference/oracle only.
- The pinned reference is `pyqtgraph-0.14.0` at commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- External behavior parity matters more than copying PyQtGraph internals.
- Existing implemented C++ code is preserved unless a focused behavior change requires editing it.
- C++ branding, include root, package name, and namespace are CppQtGraph/`cppqtgraph`.

## Lightweight porting strategy

Do **not** port every Python file line-by-line. Port examples and observable behavior first.

For each PyQtGraph example or visible behavior slice:

1. Read and, when needed, run the pinned upstream PyQtGraph example.
2. Freeze visible inputs and random data into deterministic fixtures.
3. Build the smallest native C++/Qt example with matching title, layout, data, and behavior.
4. Add only the C++ library features needed by that example.
5. Validate with smoke tests, numeric oracle checks, visual comparison, and interaction replay as appropriate.
6. Move to the next example only after the current behavior is accepted.

This keeps the project small: every new class, method, option, or helper should exist because a checked-in example or oracle needs it.

## In scope

- Externally observable PyQtGraph behavior: plots, axes, labels, ranges, mouse interactions, context menus, layouts, images, colors, widgets, and examples.
- Qt 6 Widgets/GraphicsView/QPainter/QGraphicsItem/QTimer/signals as the native GUI/rendering base.
- OpenCV, STL containers, spans/views, and small focused helpers where they replace NumPy-style data handling.
- PyQtGraph-like public names where they reduce mental translation for C++ users.
- Deterministic development oracles: fixture JSON, reference screenshots, numeric probes, and scripted interaction end states.

## Out of scope

- Python wrappers, Python import machinery, REPL/console internals, Jupyter support, multiprocessing/remote graphics internals, monkey-patching behavior, and Python-only debugging helpers.
- A full NumPy clone. Implement explicit C++ loops/views/helpers only when a ported example needs them.
- Blind emulation of dynamic Python APIs. Prefer common overloads and compact C++ option structs over large overload sets or Python-style kwargs.
- Full subsystem completion before examples require it.
- Pixel-perfect cross-platform equality. Use platform-tolerant visual thresholds and semantic review where rendering differences are expected.

## Recommended C++ API style

- Keep useful PyQtGraph names such as `PlotWidget`, `PlotItem`, `ViewBox`, `AxisItem`, `ImageItem`, and `ImageView`.
- Use straightforward Qt ownership. Returning raw pointers for Qt-owned items is acceptable.
- Use option structs for complex calls, for example `PlotOptions{.pen = ..., .symbol = ...}`.
- Use overloads only for common cases such as `plot(y)` and `plot(x, y)`.
- Prefer composition for widgets and inheritance only where user-visible subclassing behavior matters.

## Validation model

Use the lightest validation that proves the behavior:

1. **Smoke:** the example builds, starts headlessly, constructs the window, and can produce a screenshot.
2. **Numeric oracle:** C++ state matches deterministic fixture values such as item count, data sizes, min/max, view ranges, labels, and region bounds.
3. **Visual comparison:** compare reference/actual/diff/metrics artifacts with tolerances for fonts and antialiasing.
4. **Interaction replay:** use Qt Test or equivalent scripted events for drag, wheel zoom, region movement, ROI movement, context menu actions, and crosshair movement.

Python may be used by tests and oracle-generation scripts. It must not become a runtime dependency of the C++ library or examples.

## Active workflow

- `examples/example_manifest.yaml` is the active example-first manifest.
- Work on one example or one directly required behavior slice at a time.
- Keep tasks small, testable, and tied to the pinned PyQtGraph reference.
- Prefer updating existing code over adding speculative scaffolding.
- Run focused tests first, then the local validation commands documented in `README.md`.
- If visual parity, architecture, or scope is unclear, stop and request human direction before expanding scope.
