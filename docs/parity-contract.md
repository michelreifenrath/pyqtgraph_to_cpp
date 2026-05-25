# Full-Port Parity Contract

Issue: GitHub #95 / P0.01

This document defines the default parity policy for the native C++ port. It is a policy and equivalence decision artifact only; `port_manifest.yaml` is unchanged because this issue defines policy only.

## Source-of-truth policy

- The canonical port specification is `docs/pyqtgraph-cpp-port-workflow.md`, which defines the project goal as translating PyQtGraph into a native C++ library for direct C++ use, with Qt/C++ rendering and local validation gates.
- Decision-only parity questions use `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc`: proof is this decision/equivalence document, not a code test. The document records conservative defaults, rationale, affected manifest entries, accepted C++ equivalence or explicit non-port decisions, and follow-up placeholders for disputed or out-of-scope behavior.
- Issue-owned scope controls edits. This contract does not modify manifest inventory, dashboard state, source, examples, tests, or automation.

## Native C++ parity contract

The port targets native C++ plotting/runtime behavior rather than Python ecosystem integration. A manifest entry is in port scope when it affects core native C++ plotting behavior, rendering, interaction, data processing, or application runtime behavior that a C++ application should expose directly.

A manifest entry is out of C++ port scope by default when its primary purpose is Python packaging, Python interpreter lifecycle support, Python process proxying, Python notebook integration, Python-specific acceleration wrappers, or embedding/export through a Python-only third-party frontend. These entries may still need an explicit C++ equivalence decision when they provide behavior that should exist natively through Qt/C++ or OpenCV/C++.

## Local-only validation policy

Validation for this repository is local-only unless a later issue explicitly adds owned automation or CI behavior. For decision-doc issues, no runtime tests are required unless executable behavior changes. Pixel-affecting implementation issues must follow the visual-validation levels and artifact rules in `docs/pyqtgraph-cpp-port-workflow.md`; this issue is docs-only and visual validation is not applicable.

## Python-ecosystem default policy

The following Python-ecosystem areas are out of C++ port scope by default unless they affect core native C++ plotting/runtime behavior:

- Jupyter and notebook display integration.
- Matplotlib embedding/export integration.
- Python multiprocessing proxies and remote-object machinery.
- Numba acceleration helpers.
- Python reload helpers and frozen-application packaging helpers.
- Not-applicable examples, templates, package initializers, demo support files, and Python test harness/support modules that do not define standalone C++ plotting examples.

## Affected manifest entries

### Jupyter

- `pyqtgraph/jupyter/GraphicsView.py` -> default non-port for Jupyter remote-framebuffer/notebook integration. Core C++ equivalents are native Qt widgets/items, not notebook wrappers.
- `pyqtgraph/jupyter/__init__.py` -> non-port package initializer.
- `pyqtgraph/examples/jupyter_console_example.py` -> non-port notebook/console integration example unless a future issue identifies native C++ runtime behavior not otherwise covered.

### Matplotlib

- `pyqtgraph/exporters/Matplotlib.py` -> default non-port for Matplotlib-specific export integration.
- `pyqtgraph/widgets/MatplotlibWidget.py` -> default non-port for embedding Matplotlib in a Qt widget.
- Classes `MatplotlibExporter`, `MatplotlibWindow`, and `MatplotlibWidget` -> non-port as Matplotlib ecosystem bridge classes. C++ export/rendering parity should be satisfied by native Qt/C++ exporters or other explicitly scoped C++ exporters, not by embedding Matplotlib.

### Multiprocessing/proxies

- `pyqtgraph/multiprocess/__init__.py`
- `pyqtgraph/multiprocess/bootstrap.py`
- `pyqtgraph/multiprocess/parallelizer.py`
- `pyqtgraph/multiprocess/processes.py`
- `pyqtgraph/multiprocess/remoteproxy.py`
- Key classes: `Parallelize`, `Tasker`, `Process`, `ForkedProcess`, `RemoteQtEventHandler`, `QtProcess`, `RemoteEventHandler`, `Request`, `LocalObjectProxy`, `ObjectProxy`, `DeferredObjectProxy`.

Default decision: non-port for Python multiprocessing/proxy infrastructure. Native C++ concurrency, worker process, or async behavior may be implemented in later issues only when required by a C++ plotting/runtime feature.

### Numba

- `pyqtgraph/functions_numba.py` -> non-port as a Python/Numba acceleration wrapper.
- `pyqtgraph/util/numba_helper.py` -> non-port as Python dependency-detection/helper code.

Default decision: C++ performance parity should come from native C++ implementations and targeted benchmarks, not from a Numba compatibility layer.

### Reload/frozen helpers

- `pyqtgraph/reload.py` -> non-port Python module reload helper.
- `pyqtgraph/frozenSupport.py` -> non-port Python frozen-app support helper unless a future C++ packaging issue explicitly owns an equivalent.
- `pyqtgraph/examples/cx_freeze/setup.py` -> non-port Python packaging setup example.
- `pyqtgraph/examples/py2exe/setup.py` -> non-port Python packaging setup example.

### Not-applicable examples and support entries

Default decision: non-port for examples/support files that are templates, package initializers, Python package/demo support, Python test harnesses, or build/config helpers rather than standalone C++ plotting examples. Affected entries include at least:

- `pyqtgraph/examples/VideoTemplate_generic.py`
- `pyqtgraph/examples/__init__.py`
- `pyqtgraph/examples/__main__.py`
- `pyqtgraph/examples/_buildParamTypes.py`
- `pyqtgraph/examples/_paramtreecfg.py`
- `pyqtgraph/examples/exampleLoaderTemplate_generic.py`
- `pyqtgraph/examples/template.py`
- `pyqtgraph/examples/test_examples.py`
- `pyqtgraph/examples/utils.py`
- package/demo support entries such as `pyqtgraph/examples/optics/__init__.py`, `pyqtgraph/examples/relativity/__init__.py`, `pyqtgraph/examples/verlet_chain/__init__.py`, `pyqtgraph/examples/cx_freeze/setup.py`, and `pyqtgraph/examples/py2exe/setup.py`.

## Explicit non-port/equivalence decisions

- Jupyter wrappers are non-port. Equivalent C++ behavior is native Qt widget/view usage and any future explicitly scoped C++ embedding surface.
- Matplotlib bridge classes are non-port. Equivalent C++ behavior is native C++ rendering/export functionality owned by later exporter issues.
- Python multiprocessing proxy infrastructure is non-port. Equivalent C++ behavior is native C++ concurrency/process/runtime support only where required by concrete C++ features.
- Numba helpers are non-port. Equivalent C++ behavior is native optimized code and benchmark-backed performance work.
- Python reload/frozen-app helpers are non-port. Equivalent C++ packaging/deployment behavior requires future owned packaging issues.
- Not-applicable example/support files are non-port. Equivalent C++ coverage is provided by standalone examples for core plotting/runtime features, not by porting Python package scaffolding or test harness files.

## Follow-up issue links/placeholders

- Follow-up: mark Jupyter manifest entries as non-port/not-applicable or create a dedicated native embedding issue if needed. Placeholder: `P0.xx-jupyter-policy-application`.
- Follow-up: mark Matplotlib manifest entries/classes as non-port or replace with native exporter parity issues. Placeholder: `P0.xx-matplotlib-policy-application`.
- Follow-up: mark multiprocessing/proxy entries/classes as non-port or open scoped native C++ concurrency issues only for concrete runtime needs. Placeholder: `P0.xx-multiprocess-policy-application`.
- Follow-up: mark Numba entries as non-port and ensure performance issues cite native C++ benchmarks. Placeholder: `P0.xx-numba-policy-application`.
- Follow-up: mark reload/frozen packaging helpers as non-port unless C++ packaging support is explicitly required. Placeholder: `P0.xx-packaging-policy-application`.
- Follow-up: mark not-applicable examples/support entries as non-port/not-applicable and keep standalone plotting examples in normal port scope. Placeholder: `P0.xx-example-na-policy-application`.
