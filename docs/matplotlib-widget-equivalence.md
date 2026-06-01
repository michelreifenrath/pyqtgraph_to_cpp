# MatplotlibWidget C++ equivalence decision

Issue: GitHub [#166](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/166) / P5.09

Validation class: `decision-doc`

## Source policy

This document applies the full-port parity policy in `docs/parity-contract.md` to `pyqtgraph/widgets/MatplotlibWidget.py`. The parity contract explicitly lists Matplotlib embedding/export integration as Python-ecosystem bridge scope and records `pyqtgraph/widgets/MatplotlibWidget.py` and class `MatplotlibWidget` as default non-port entries. It also states that decision-only parity questions are proven by a decision/equivalence document rather than runtime tests unless executable behavior changes.

For this decision-only issue, the decision artifact records the conservative default, rationale, affected manifest entries, accepted C++ equivalence or explicit non-port decision, and follow-up issue links for disputed or out-of-scope behavior, consistent with `FACTORY_RULES.md`.

## Affected manifest entries

Source entry in `port_manifest.yaml`:

- upstream: `pyqtgraph/widgets/MatplotlibWidget.py`
- target header: `include/pyqtgraph/widgets/MatplotlibWidget.hpp`
- target source: `src/pyqtgraph/widgets/MatplotlibWidget.cpp`
- subsystem: `widgets`
- current manifest status: `not_started`
- current manifest completion: `missing`

Class entry in `port_manifest.yaml`:

- class: `MatplotlibWidget`
- upstream: `pyqtgraph/widgets/MatplotlibWidget.py`
- target header: `include/pyqtgraph/widgets/MatplotlibWidget.hpp`
- target source: `src/pyqtgraph/widgets/MatplotlibWidget.cpp`
- subsystem: `widgets`
- base class: `QtWidgets.QWidget`
- upstream line: `11`
- current manifest status: `not_started`
- current manifest completion: `missing`

## Decision

`MatplotlibWidget` is an explicit non-port entry for the native C++ library. The C++ port must not embed Matplotlib, provide a Matplotlib compatibility widget, or add a C++ facade whose purpose is to host Matplotlib figures inside Qt.

Accepted C++ equivalence is native Qt/C++ plotting, rendering, widget composition, and exporter functionality implemented by the port's own graphics/view/exporter surfaces. Any required C++ export behavior should be handled by native C++ exporter issues, not by porting the Matplotlib bridge.

## Rationale

- `MatplotlibWidget` is a Python ecosystem integration layer: it embeds a Matplotlib `FigureCanvas` and `NavigationToolbar` in a Qt widget.
- The project target is a native C++ library for direct C++ use, with Qt/C++ rendering behavior rather than Python frontend embedding.
- Porting this class would introduce an out-of-scope dependency on Matplotlib and Python integration rather than preserving core PyQtGraph plotting/runtime behavior.
- Native C++ users should receive equivalent plotting and export capability from Qt/C++ implementation work, not from a Matplotlib-hosting bridge.

## Runtime, visual, and test applicability

No runtime test, C++ source file, CMake target, or visual artifact is required for this issue because the implementation is a docs-only `decision-doc` and changes no executable behavior.

Visual validation is not applicable: this decision does not change rendering code, image output, screenshots, widget behavior, or exporter output.

Manifest/dashboard updates are not made in this slice. The manifest entries above remain recorded inventory, while this document supplies the policy-backed equivalence/non-port decision for P5.09.

## Follow-up links

- [#166](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/166) resolves the `MatplotlibWidget` equivalence decision.
- [#194](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/194) covers the related Matplotlib exporter equivalence decision.

No additional follow-up issue is required by this decision unless a later native C++ exporter or widget issue identifies concrete Qt/C++ behavior not already covered by the port's rendering/export surfaces.
