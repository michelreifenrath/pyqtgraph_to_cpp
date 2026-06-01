# Full-Port Parity Contract

Issue: GitHub #95 / P0.01

This document defines the default parity policy for the native C++ port. It is a policy and equivalence decision artifact only; `port_manifest.yaml` is unchanged because this issue defines policy only.

## Source-of-truth policy

- `MISSION.md` defines the native C++/Qt product goal and non-goals; `FACTORY_RULES.md` defines the current issue, evidence, validation, and merge gates.
- Decision-only parity questions are proven by the issue-owned decision/equivalence document, not by runtime tests unless executable behavior changes. The document records conservative defaults, rationale, affected manifest entries, accepted C++ equivalence or explicit non-port decisions, and follow-up issue links or waivers for disputed or out-of-scope behavior.
- Issue-owned scope controls edits. This contract does not modify manifest inventory, dashboard state, source, examples, tests, or automation.

## Native C++ parity contract

The port targets native C++ plotting/runtime behavior rather than Python ecosystem integration. A manifest entry is in port scope when it affects core native C++ plotting behavior, rendering, interaction, data processing, or application runtime behavior that a C++ application should expose directly.

A manifest entry is out of C++ port scope by default when its primary purpose is Python packaging, Python interpreter lifecycle support, Python process proxying, Python notebook integration, Python-specific acceleration wrappers, or embedding/export through a Python-only third-party frontend. These entries may still need an explicit C++ equivalence decision when they provide behavior that should exist natively through Qt/C++ or OpenCV/C++.

## Local-only validation policy

Validation for this repository follows the assigned issue and `FACTORY_RULES.md`. For decision-doc issues, no runtime tests are required unless executable behavior changes. Pixel-affecting implementation issues must follow the issue's visual-validation level and the artifact rules in `FACTORY_RULES.md`; this issue is docs-only and visual validation is not applicable.

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

- `pyqtgraph/multiprocess/__init__.py` -> `include/pyqtgraph/multiprocess/__init__.hpp`, `src/pyqtgraph/multiprocess/__init__.cpp`
- `pyqtgraph/multiprocess/bootstrap.py` -> `include/pyqtgraph/multiprocess/bootstrap.hpp`, `src/pyqtgraph/multiprocess/bootstrap.cpp`
- `pyqtgraph/multiprocess/parallelizer.py` -> `include/pyqtgraph/multiprocess/parallelizer.hpp`, `src/pyqtgraph/multiprocess/parallelizer.cpp`
- `pyqtgraph/multiprocess/processes.py` -> `include/pyqtgraph/multiprocess/processes.hpp`, `src/pyqtgraph/multiprocess/processes.cpp`
- `pyqtgraph/multiprocess/remoteproxy.py` -> `include/pyqtgraph/multiprocess/remoteproxy.hpp`, `src/pyqtgraph/multiprocess/remoteproxy.cpp`
- `pyqtgraph/examples/multiprocess.py` -> non-port as a Python multiprocessing/proxy example; equivalent C++ coverage is native concurrency/process usage only if a future concrete C++ runtime feature requires it.
- `pyqtgraph/examples/parallelize.py` -> non-port as a Python `Parallelize`/multiprocessing example; equivalent C++ coverage is native C++ task/concurrency behavior only if a future concrete C++ runtime feature requires it.
- Key classes: `CanceledError`, `Parallelize`, `Tasker`, `Process`, `ForkedProcess`, `RemoteQtEventHandler`, `QtProcess`, `FileForwarder`, `ClosedError`, `NoResultError`, `RemoteExceptionWarning`, `RemoteEventHandler`, `Request`, `LocalObjectProxy`, `ObjectProxy`, `DeferredObjectProxy`.

P8.05 / #195 decision: non-port for Python process proxying, interpreter/process bootstrap, remote object forwarding, request/result marshalling, deferred object proxies, remote exception/warning proxying, and Python stdout/stderr forwarding. No Python object-proxy API compatibility layer is accepted for the native C++ port.

Accepted C++ equivalence: native Qt/C++ plotting and rendering stay in the normal in-process Qt scene/view/item APIs unless a future issue explicitly owns worker-process rendering. Native C++ concurrency, process, or async support is in scope only when required by a concrete C++ plotting/runtime feature.

Remote render parity boundary: `RemoteGraphicsView`/worker-process rendering is not owned by #195 and is deferred to follow-up [#167](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/167) if the native C++ port needs that behavior. Runtime or render proof for worker-process rendering must be supplied by that scoped issue; no executable or visual proof is required for this decision-doc update.

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
- Python multiprocessing proxy infrastructure, interpreter/process bootstrap, remote object forwarding, request/result marshalling, deferred object proxies, remote exception/warning proxying, Python stdout/stderr forwarding, and the `pyqtgraph/examples/multiprocess.py` and `pyqtgraph/examples/parallelize.py` examples are non-port. Equivalent C++ behavior is normal in-process Qt scene/view/item rendering and native C++ concurrency/process/runtime support only where required by concrete C++ features; no Python object-proxy API compatibility layer is accepted.
- Numba helpers are non-port. Equivalent C++ behavior is native optimized code and benchmark-backed performance work.
- Python reload/frozen-app helpers are non-port. Equivalent C++ packaging/deployment behavior requires future owned packaging issues.
- Not-applicable example/support files are non-port. Equivalent C++ coverage is provided by standalone examples for core plotting/runtime features, not by porting Python package scaffolding or test harness files.

## Follow-up issue links and waivers

Existing GitHub issues that can apply or revisit this contract when their owned scope is reached:

- Jupyter/native embedding equivalence: [#197](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/197) Resolve jupyter embedded graphics equivalent.
- Matplotlib exporter/widget equivalence: [#194](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/194) Resolve Matplotlib exporter equivalent and [#166](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/166) Resolve MatplotlibWidget equivalent.
- Multiprocessing/proxy infrastructure and the `multiprocess.py`/`parallelize.py` examples: [#195](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/195) Resolve multiprocess remote proxy equivalents.
- `RemoteGraphicsView`/worker-process rendering plan and proof: [#167](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/167) Complete RemoteGraphicsView plan and implementation.
- Not-applicable examples/support manifest application: [#207](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/207) Resolve not-applicable examples.

Waiver: no new follow-up issue creation is required for the Numba helpers or Python reload/frozen-app helpers in this policy-only issue. They are explicit non-port decisions; future native C++ performance or packaging work should open scoped issues only when a concrete C++ plotting/runtime requirement exists.
