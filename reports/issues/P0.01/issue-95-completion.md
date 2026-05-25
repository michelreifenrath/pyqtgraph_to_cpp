# Issue #95 Completion Report: P0.01 Full-Port Parity Contract

- Author/tool: Pi worker implementation subagent; rework by Pi parent agent with reviewer subagent
- Date: 2026-05-25
- Issue: GitHub #95 / P0.01
- Validation class: decision-doc

## Policy defaults recorded

- Native C++ port contract: port scope is core native C++ plotting/runtime behavior for direct C++ use.
- Local-only validation policy: decision-doc issues require a decision/equivalence artifact rather than executable tests unless behavior changes.
- Python-ecosystem default policy: Jupyter, Matplotlib embedding/export, multiprocessing proxies/examples, Numba acceleration, reload/frozen-app helpers, and not-applicable examples/support files are out of C++ port scope unless they affect core native C++ plotting/runtime behavior.

## Precheck command results

These precheck results were supplied before implementation:

```text
test -f docs/parity-contract.md
exit 1

test -d reports/issues/P0.01
exit 1

git status before editing: clean
```

## Artifacts created

- `docs/parity-contract.md` - full-port parity policy and explicit non-port/equivalence decision artifact.
- `reports/issues/P0.01/issue-95-completion.md` - this completion report.

## Changed-file ownership expectation

Only issue-owned paths were changed:

- `docs/parity-contract.md`
- `reports/issues/P0.01/issue-95-completion.md`

No source, tests, scripts, workflow files, README, manifest, dashboard, validation guide, or other paths were intentionally edited.

## Manifest/dashboard applicability

- `port_manifest.yaml` was not changed because this issue defines policy only.
- Dashboard or manifest state updates are not applicable for this decision-doc implementation step.

## Runtime tests

No runtime tests were run. This is a docs-only decision artifact with no executable behavior changes.

## Visual validation

Visual validation is not applicable. The issue does not affect rendering, pixels, screenshots, or visual artifacts.

## Follow-up issue links and waivers

The parity contract references existing GitHub follow-up issues for policy application or scoped replacement work:

- [#197](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/197) Resolve jupyter embedded graphics equivalent.
- [#194](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/194) Resolve Matplotlib exporter equivalent.
- [#166](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/166) Resolve MatplotlibWidget equivalent.
- [#195](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/195) Resolve multiprocess remote proxy equivalents, including the multiprocessing-related example manifest entries now listed in `docs/parity-contract.md`.
- [#207](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/207) Resolve not-applicable examples.

Waiver: no new follow-up issue creation is required for Numba helpers or Python reload/frozen-app helpers in this policy-only issue because they are explicit non-port decisions. Future native C++ performance or packaging work should open scoped issues only when a concrete C++ plotting/runtime requirement exists.

Current waiver: manifest/dashboard edits, executable tests, and visual artifacts are not required for this issue because it is policy-only and decision-doc scoped.

## Validation

Post-rework local validation results:

```text
$ test -f docs/parity-contract.md
exit 0

$ test -f reports/issues/P0.01/issue-95-completion.md
exit 0

$ pat='place''holder|P0[.]xx'; ! rg -n "$pat" docs/parity-contract.md reports/issues/P0.01/issue-95-completion.md
exit 0

$ rg -n "pyqtgraph/examples/(multiprocess|parallelize)[.]py" docs/parity-contract.md
55:- `pyqtgraph/examples/multiprocess.py` -> non-port as a Python multiprocessing/proxy example; equivalent C++ coverage is native concurrency/process usage only if a future concrete C++ runtime feature requires it.
56:- `pyqtgraph/examples/parallelize.py` -> non-port as a Python `Parallelize`/multiprocessing example; equivalent C++ coverage is native C++ task/concurrency behavior only if a future concrete C++ runtime feature requires it.
94:- Python multiprocessing proxy infrastructure and the `pyqtgraph/examples/multiprocess.py` and `pyqtgraph/examples/parallelize.py` examples are non-port. Equivalent C++ behavior is native C++ concurrency/process/runtime support only where required by concrete C++ features.
exit 0

$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
exit 0

$ git diff --check
exit 0

$ git diff --name-only origin/main...HEAD
docs/parity-contract.md
reports/issues/P0.01/issue-95-completion.md
exit 0
```

Validated artifact paths:

- `docs/parity-contract.md`
- `reports/issues/P0.01/issue-95-completion.md`

Changed-file ownership check: `git diff --name-only origin/main...HEAD` listed only the issue-owned decision artifact and completion report.
