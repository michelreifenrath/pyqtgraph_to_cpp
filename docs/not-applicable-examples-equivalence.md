# GitHub #207 / P9.07 not-applicable examples equivalence decision

This document records the P9.07 decision for the 14 PyQtGraph example/support entries whose validation level is `not_applicable`. It is a policy-backed decision/equivalence artifact for GitHub #207, not a runtime implementation artifact.

## Authority and policy basis

- `docs/parity-contract.md` defines the native C++ parity contract. Its default policy says not-applicable examples, templates, package initializers, demo support files, Python test harnesses, and support modules that do not define standalone C++ plotting examples are non-port.
- `MISSION.md` defines the current evidence/validation gate; for this decision-only issue, proof is this decision/equivalence document, not runtime tests, unless executable behavior changes.
- `docs/examples/validation-levels.md` records that 14 examples have all validation channels `not_applicable`.

No named human approval was used for this decision. The policy citations above are the authority.

## Conservative default, rationale, and accepted equivalence

The conservative default is that a PyQtGraph example/support file is only ported as a standalone C++ example when it demonstrates native C++ plotting/runtime behavior that should be exercised directly by a C++ user. Files that are Python loader templates, package initializers, package/demo support, Python packaging setup, Python test harnesses, or Python-only helper/configuration modules are explicit non-ports.

The rationale is that the native C++ port targets Qt/C++ plotting and runtime behavior, not Python packaging, interpreter entry points, or Python test/demo scaffolding. Porting these files to empty or artificial `.cpp` files would create misleading physical targets without adding C++ plotting coverage.

Accepted C++ equivalence is the existing and future set of standalone C++ examples for concrete plotting/runtime features. No standalone C++ counterpart is required for the entries below. The active `examples/example_manifest.yaml` tracks example-first work only, so these Python support files are intentionally absent unless a future concrete C++ example needs them.

## Explicit non-port decisions

| # | Upstream path | Former target path | Validation status | P9.07 decision | C++ counterpart / equivalence | Follow-up / waiver |
|---:|---|---|---|---|---|---|
| 1 | `pyqtgraph/examples/VideoTemplate_generic.py` | `examples/VideoTemplate_generic.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Video-loader template support; equivalent coverage is standalone C++ video/image plotting examples when those concrete features are owned. | #207 resolves this not-applicable support entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 2 | `pyqtgraph/examples/__init__.py` | `examples/__init__.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python package initializer; native C++ has no package-initializer counterpart. | #207 resolves this initializer entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 3 | `pyqtgraph/examples/__main__.py` | `examples/__main__.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python module entry-point/demo launcher; equivalent coverage is individual standalone C++ examples, not a Python `__main__` port. | #207 resolves this launcher entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 4 | `pyqtgraph/examples/_buildParamTypes.py` | `examples/_buildParamTypes.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python build/helper script; equivalent C++ coverage is native parameter-tree examples or tests when concrete parameter behavior is owned. | #207 resolves this helper entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 5 | `pyqtgraph/examples/_paramtreecfg.py` | `examples/_paramtreecfg.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python parameter-tree demo configuration support; equivalent coverage is standalone C++ parameter-tree examples for concrete behavior. | #207 resolves this config/support entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 6 | `pyqtgraph/examples/cx_freeze/setup.py` | `examples/cx_freeze/setup.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python frozen-application packaging setup; native C++ packaging/deployment is out of scope for this example artifact. | #207 resolves this packaging setup entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 7 | `pyqtgraph/examples/exampleLoaderTemplate_generic.py` | `examples/exampleLoaderTemplate_generic.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python example-loader template; equivalent coverage is standalone C++ examples and any separately owned native example runner. | #207 resolves this loader-template entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 8 | `pyqtgraph/examples/optics/__init__.py` | `examples/optics/__init__.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python package initializer for optics examples; equivalent coverage is concrete C++ optics examples, not an initializer. | #207 resolves this initializer entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 9 | `pyqtgraph/examples/py2exe/setup.py` | `examples/py2exe/setup.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python/Windows packaging setup; native C++ packaging/deployment is out of scope for this example artifact. | #207 resolves this packaging setup entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 10 | `pyqtgraph/examples/relativity/__init__.py` | `examples/relativity/__init__.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python package initializer for relativity examples; equivalent coverage is concrete C++ relativity examples, not an initializer. | #207 resolves this initializer entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 11 | `pyqtgraph/examples/template.py` | `examples/template.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python example template; equivalent coverage is actual standalone C++ examples for concrete plotting/runtime behavior. | #207 resolves this template entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 12 | `pyqtgraph/examples/test_examples.py` | `examples/test_examples.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python example test harness; equivalent validation belongs in C++ tests or local example validation, not a standalone example port. | #207 resolves this test-harness entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 13 | `pyqtgraph/examples/utils.py` | `examples/utils.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python demo utility/support module; equivalent coverage is concrete standalone C++ examples or tests that own the supported behavior. | #207 resolves this utility/support entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |
| 14 | `pyqtgraph/examples/verlet_chain/__init__.py` | `examples/verlet_chain/__init__.cpp` | numeric/visual/interaction/gpt_visual_review: `not_applicable` | non-port | Python package initializer for Verlet-chain examples; equivalent coverage is concrete C++ Verlet-chain examples, not an initializer. | #207 resolves this initializer entry; no new issue required unless a future concrete native C++ plotting/runtime requirement is found. |

## Follow-up outcome

This decision resolves the not-applicable examples/support follow-up for #207. The active example-first manifest remains focused on standalone native C++ examples; P9.07 records that no standalone C++ counterpart is required for these 14 support files.

No new issue is required unless future work identifies a concrete native C++ plotting/runtime requirement currently hidden behind one of these Python support files.
