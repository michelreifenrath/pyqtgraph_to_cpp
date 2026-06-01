# pgcpp issue readiness

Purpose: summarize the deterministic generated-local GitHub issue readiness triage.

Workflow argument:

```text
$ARGUMENTS
```

The workflow writes these artifacts:

```text
$ARTIFACTS_DIR/readiness.txt
$ARTIFACTS_DIR/label-plan.json
$ARTIFACTS_DIR/applied-label-plan.json
```

Instructions:

- Read the artifact files above; do not rerun shell commands.
- Treat `scripts/check_proposed_issues --source github` as the source of truth.
- Summarize whether the generated-local issue contract passed.
- Summarize applied label actions exactly: `ai:ready` for dependency-free ready issues, `ai:blocked` for blocked issues.
- Call out blockers using this repo's required sections: `Goal`, `Current evidence`, `Scope`, `Owned files`, `Required local proof`, `TDD plan`, `Validation commands`, `Acceptance criteria`, `Done definition`, and `Scope boundaries`.
- State clearly that this Dark-Factory-style readiness workflow mutates only readiness labels through the deterministic label plan; it does not close issues, invent triage decisions, adapt `MISSION.md`/`FACTORY_RULES.md`, or use `factory:accepted`.
