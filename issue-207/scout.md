I did not write `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-207/issue-207/scout.md` because this task is explicitly read-only/no edits, and that path/directory does not currently exist.

# Code Context

## Files Retrieved
1. `AGENTS.md` (lines 1-32) - issue scope/owned files outrank repo rules; no out-of-scope edits.
2. `WORKFLOW.md` (lines 1-71) - local validation, no commits/pushes/merges, validation commands.
3. GitHub issue `#207` body via `gh issue view` - owned files and acceptance criteria.
4. `port_manifest.yaml` (lines 1744-2043, 6204-6411) - exact 14 entries and current validation/status.
5. `docs/parity-contract.md` (lines 7-12, 15-32, 79-111) - policy basis and #207 follow-up.
6. `docs/proposed-issues/VALIDATION-GUIDE.md` (lines 8-44, 79-82) - decision-doc and ownership conventions.
7. `docs/examples/validation-levels.md` (lines 1-30) - example validation metadata policy and count of 14.
8. `tests/oracle/test_example_validation_levels.py` (lines 29-54, 75-140) - focused validation-level coverage/invariants.
9. `scripts/generate_manifest` (lines 20-31, 80-96, 147-150, 181-223) - status/completion fields are generated from target file presence.

## Key Code / Data

Exact 14 not-applicable examples; all currently have `status: not_started`, `completion: missing`, no target `.cpp` exists:

| Upstream path | Target | Manifest status lines | Validation lines |
|---|---|---:|---:|
| `pyqtgraph/examples/VideoTemplate_generic.py` | `examples/VideoTemplate_generic.cpp` | 1744-1749 | 6204-6211 |
| `pyqtgraph/examples/__init__.py` | `examples/__init__.cpp` | 1768-1773 | 6224-6227 |
| `pyqtgraph/examples/__main__.py` | `examples/__main__.cpp` | 1774-1779 | 6228-6231 |
| `pyqtgraph/examples/_buildParamTypes.py` | `examples/_buildParamTypes.cpp` | 1780-1785 | 6232-6235 |
| `pyqtgraph/examples/_paramtreecfg.py` | `examples/_paramtreecfg.cpp` | 1786-1791 | 6236-6239 |
| `pyqtgraph/examples/cx_freeze/setup.py` | `examples/cx_freeze/setup.cpp` | 1846-1851 | 6276-6279 |
| `pyqtgraph/examples/exampleLoaderTemplate_generic.py` | `examples/exampleLoaderTemplate_generic.cpp` | 1864-1869 | 6288-6291 |
| `pyqtgraph/examples/optics/__init__.py` | `examples/optics/__init__.cpp` | 1942-1947 | 6340-6343 |
| `pyqtgraph/examples/py2exe/setup.py` | `examples/py2exe/setup.cpp` | 1978-1983 | 6368-6371 |
| `pyqtgraph/examples/relativity/__init__.py` | `examples/relativity/__init__.cpp` | 1984-1989 | 6372-6375 |
| `pyqtgraph/examples/template.py` | `examples/template.cpp` | 2014-2019 | 6392-6395 |
| `pyqtgraph/examples/test_examples.py` | `examples/test_examples.cpp` | 2020-2025 | 6396-6399 |
| `pyqtgraph/examples/utils.py` | `examples/utils.cpp` | 2032-2037 | 6404-6407 |
| `pyqtgraph/examples/verlet_chain/__init__.py` | `examples/verlet_chain/__init__.cpp` | 2038-2043 | 6408-6411 |

Policy basis:
- `docs/parity-contract.md` says not-applicable examples/templates/package initializers/demo support/test harness files are out of C++ port scope by default (lines 23-32), lists these entries (lines 79-91), and records non-port/equivalence: standalone examples cover core runtime behavior instead of Python scaffolding/test harnesses (lines 99-101).
- `docs/proposed-issues/VALIDATION-GUIDE.md#decision-doc` says proof is a decision/equivalence document; tests only if executable behavior changes (lines 41-44).

## Architecture

- `port_manifest.yaml::examples` is a generated inventory with `status`/`completion` derived from target file existence.
- `port_manifest.yaml::example_validation_levels` is non-generated policy metadata; current test enforces coverage/schema/policy.
- `scripts/generate_manifest --check` compares generated sections only; manual edits to generated `examples` status/completion will fail unless generator semantics change or matching target files are added.
- Issue #207 owns “manifest status fields and docs only” plus `focused-doc-report`; production source/example `.cpp` files are out of scope.

## Start Here

Open `docs/parity-contract.md` first, then `port_manifest.yaml` around lines 1744-2043 and 6204-6411. The main implementation decision is whether #207 should add a focused decision doc/report only, or also encode a manifest status for non-port examples despite generated-status constraints.

## Gaps / Risks

- Biggest risk: issue asks for “C++ counterpart/status,” but generated manifest status is currently tied to physical target `.cpp` presence; changing status manually conflicts with `scripts/generate_manifest --check`.
- No existing `docs/not-applicable-examples-equivalence.md` or `reports/issues/P9.07/` artifact exists.
- The parity contract already lists/decides the 14 entries, but #207 acceptance requires a focused policy-backed artifact and zero silent skips.
- Safe focused commands likely:
  - `python3 -m pytest -q tests/oracle/test_example_validation_levels.py`
  - `scripts/generate_manifest --check`
  - `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp`
  - `git diff --check`
  - `git diff --name-only origin/main...HEAD`

Current checks run read-only:
- `python3 -m pytest -q tests/oracle/test_example_validation_levels.py` → 4 passed.
- `scripts/generate_manifest --check` → manifest verified.
- `git diff --name-only origin/main...HEAD && git diff --check` → no output.

Git state: branch `ai/issue-207-p9-07-resolve-not-applicable-examples...origin/main`, no working-tree changes reported.