# Factory Rules

This is the compact governance contract for the PyQtGraph-to-C++ factory. `WORKFLOW.md`, the issue body, and `scripts/factory/*` are the executable gates; if prose and gates disagree, fail closed and request governance review.

## Issue readiness

An autonomous issue must be small enough for one safe worktree run and must define:

- One externally observable outcome.
- A concrete PyQtGraph reference: upstream class, function, example, file, or behavior.
- Dependencies listed as `none` or resolved GitHub issue references.
- Owned files or manifest/repository selectors, including the required selector headings for generated issues.
- Scope boundaries, TDD plan, validation commands, acceptance criteria, and done definition.
- Validation levels for `numeric`, `visual`, and `interaction`; allowed values are only `required` or `not_applicable`.

Automation may claim only issues that carry `ai:ready` and are not blocked, claimed, ignored, or done. `factory:ready-checked` records that the readiness gate has evaluated the issue; it is not a substitute for the executable readiness check.

## Scope budgets

Normal factory work must stay within the live scope gates:

- 10 total owned/changed files.
- 4 production files.
- 4 test/oracle/report files.
- 1 example file.
- 3 shared integration files.

Shared integration edits are allowed only when directly required by the issue and accepted by the configured/script allowlists. Larger review surfaces, ambiguous ownership, or files outside scope fail closed or require human review. Generated-file exceptions count only after their configured read-only verification command passes.

## Implementation rules

- Use one issue, one isolated worktree, and one PR branch.
- Read the issue, `MISSION.md`, `FACTORY_RULES.md`, `AGENTS.md`, `WORKFLOW.md`, and the pinned PyQtGraph source relevant to the behavior.
- Add or update the focused failing C++ test, oracle probe, or visual proof before production implementation.
- Implement the smallest C++/Qt/OpenCV change that satisfies the issue; do not port invisible Python internals.
- Change only owned files plus narrowly required shared integration files.
- Implementation, rework, review, and release workers must not merge. They also must not push to `main`.

## PR evidence and validation

Every PR must provide a compact evidence packet covering the linked issue, scope, baseline failure or oracle rationale, changed files, validation commands, and required artifacts.

Required gates before merge eligibility:

- Exactly one linked issue.
- Open non-draft PR targeting `main` from a non-`main` head branch.
- Guarded head-SHA match.
- Readiness, evidence, owned-file scope, deterministic tests, `git diff --check`, mandatory autoreview, and holdout validation all pass.
- Numeric and oracle evidence pass when declared required.
- Visual-required work includes passing visual evidence and accepted GPT-5.5 semantic visual review.
- No protected-file change, risky verdict, unresolved architecture decision, or high-risk review finding.

Visual-required evidence must include reproducible artifacts under `reports/visual-diffs/<case>/`: `reference.png`, `actual.png`, `diff.png`, `metrics.json`, and `gpt5_vision_review.md`. If deterministic metrics and GPT semantic review disagree, require human review.

## Merge policy

Auto-merge is allowed only when `WORKFLOW.md` sets `policy.auto_merge: true` and the validation/merge controller runs the final governed verdict. The guarded merge command must squash-merge the verified PR head SHA; dry-run verdicts may label `ai:merge-ready` but must not merge.

Implementation, rework, review, and release workers never merge. Humans may merge or override only with explicit review of the evidence and risks.

## Retry and human review

A holdout validator may request at most one focused fresh-context fix attempt by default. Use rework for ordinary actionable validation failures. Require human review for protected files, risky changes, disabled/unsafe auto-merge configuration, PR metadata/head-SHA problems, missing or failed GPT visual review, repeated failures, unclear evidence, credentials/auth/permission blockers, or important design decisions.

## Labels

Primary automation labels are `ai:ready`, `ai:claimed`, `ai:rework`, `ai:review`, `ai:merge-ready`, `ai:failed`, `ai:done`, `ai:blocked`, and `ai:ignore`. Use `human-review` for explicit human gates. `factory:*` labels are supplemental state/evidence labels such as `factory:ready-checked`, `factory:running`, `factory:validating`, `factory:auto-merge-ok`, `factory:gpt-visual`, `factory:failed-with-evidence`, `factory:merged`, and `factory:from-regression`.

## License and attribution policy

Translated or adapted source files must include a source note near the top of the file. Required fields:

```text
Source note: translated/adapted from PyQtGraph <upstream-path>
PyQtGraph ref: pyqtgraph-0.14.0
Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
License: MIT; see THIRD_PARTY_NOTICES.md
```

Generated files must identify their generator and inputs. Required fields:

```text
Generated by <tool/script>
Inputs: <source manifest/upstream path>
Do not edit manually
```

Use `Do not edit manually` when the generated output should only be changed by rerunning the generator. Original project files may instead say:

```text
Original implementation; no PyQtGraph source translation
```

Project-authored tests and benchmarks are under the project license unless a file source note states adaptation from PyQtGraph or another source.

## Protected files

Protected files require explicit automation/governance scope and force human review before merge:

- `MISSION.md`
- `FACTORY_RULES.md`
- `AGENTS.md`
- `WORKFLOW.md`
- `.archon/**`
- `scripts/factory/**`
- `archive/**`
- `.env*`, `**/.env*`, credentials, and secrets
