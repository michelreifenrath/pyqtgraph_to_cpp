# GitHub #197 / P8.07 Jupyter Embedded Graphics Equivalence Decision

- Validation class: decision-doc
- Scope: docs-only parity decision for Jupyter embedded graphics entries
- Policy sources: `MISSION.md`, `FACTORY_RULES.md`, and `docs/parity-contract.md`

## Policy basis

For this decision-only issue, proof is a decision/equivalence artifact rather than a code test. When human approval is absent or not separately required, the artifact must record the conservative default, rationale, affected manifest entries, accepted C++ equivalence or explicit non-port decision, and follow-up issues or waivers.

`docs/parity-contract.md` defines the native C++ port scope. The port targets native C++ plotting/runtime behavior, not Python ecosystem integration. Python notebook/Jupyter integration and Python-only frontend embedding/export are out of C++ port scope by default. The parity contract already lists the Jupyter entries and records the explicit decision: Jupyter wrappers are non-port; equivalent C++ behavior is native Qt widget/view usage and any future explicitly scoped C++ embedding surface.

## Conservative default

Jupyter remote-framebuffer/notebook wrappers and Python-only frontend embedding are non-port by default. The native C++ port must not add `jupyter_rfb`, notebook wrapper, Python kernel, or browser-front-end compatibility layers for these entries unless a future issue explicitly owns and approves a native C++ embedding surface.

## Accepted C++ equivalence

Accepted C++ equivalence is native Qt widgets/views/items, including `QGraphicsView` and PyQtGraph-native widgets/items where implemented, plus any future explicitly scoped native C++ embedding surfaces. The equivalent behavior is direct use of native C++/Qt plotting views and widgets in a C++ application, not `jupyter_rfb`, notebook wrappers, or Python-only frontend embedding.

## Affected manifest entries and decisions

### Source entries

- `pyqtgraph/jupyter/GraphicsView.py` -> `include/pyqtgraph/jupyter/GraphicsView.hpp`, `src/pyqtgraph/jupyter/GraphicsView.cpp`
  - Decision: explicit non-port for Jupyter remote-framebuffer/notebook wrapper behavior.
  - Equivalence: native Qt view/widget usage and PyQtGraph-native widgets/items where implemented.
- `pyqtgraph/jupyter/__init__.py` -> `include/pyqtgraph/jupyter/__init__.hpp`, `src/pyqtgraph/jupyter/__init__.cpp`
  - Decision: explicit non-port for Python package initializer / Jupyter integration namespace.
  - Equivalence: no C++ package-initializer equivalent is required.

### Class entries in `pyqtgraph/jupyter/GraphicsView.py`

- `GraphicsView` -> `include/pyqtgraph/jupyter/GraphicsView.hpp`, `src/pyqtgraph/jupyter/GraphicsView.cpp`
  - Decision: non-port as a Jupyter wrapper class.
  - Equivalence: native `QGraphicsView` / PyQtGraph-native view usage where implemented.
- `GraphicsLayoutWidget` -> `include/pyqtgraph/jupyter/GraphicsView.hpp`, `src/pyqtgraph/jupyter/GraphicsView.cpp`
  - Decision: non-port as a Jupyter wrapper class.
  - Equivalence: native PyQtGraph `GraphicsLayoutWidget` behavior in C++ where separately implemented, not a notebook wrapper.
- `PlotWidget` -> `include/pyqtgraph/jupyter/GraphicsView.hpp`, `src/pyqtgraph/jupyter/GraphicsView.cpp`
  - Decision: non-port as a Jupyter wrapper class.
  - Equivalence: native PyQtGraph `PlotWidget` behavior in C++ where separately implemented, not a notebook wrapper.

### Context-only example entry

- `pyqtgraph/examples/jupyter_console_example.py` -> `examples/jupyter_console_example.cpp`
  - Decision: context-only for this issue. The example is a Jupyter console/notebook integration example and remains governed by the parity contract's non-port default, but this issue does not change example status because its manifest example selectors are none.
  - Equivalence: no example implementation or validation artifact is required for #197.

## Rationale

The upstream Jupyter module exists to expose PyQtGraph views through Python notebook/frontend infrastructure. That infrastructure depends on Python-specific display protocols and remote-framebuffer behavior rather than native C++ plotting/runtime semantics. Recreating those wrappers in C++ would add a Python/Jupyter compatibility layer outside the native C++ port goal and would not improve direct C++/Qt application behavior.

Native C++ users get the relevant plotting/view behavior through Qt widgets, graphics views, and PyQtGraph-native C++ widgets/items. If a future product requirement needs C++ embedding into a non-Qt host, that must be scoped as a new native C++ embedding surface rather than treating Jupyter wrappers as required parity.

## Follow-up and waiver

#197 resolves the parity-contract Jupyter/native embedding follow-up. No C++ source, C++ test, runtime, visual, oracle, performance, manifest, dashboard, or example follow-up is required for these Jupyter wrapper entries unless a later issue explicitly scopes a native C++ embedding surface.
