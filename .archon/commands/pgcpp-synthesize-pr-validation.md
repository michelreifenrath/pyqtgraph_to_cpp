---
description: Synthesize pgcpp pass-1 PR validation reviewer findings into pass/fix/human-review routing.
argument-hint: (no arguments - reads pass-1 artifacts)
---

# pgcpp PR Validation Synthesis — Pass 1

You are the deterministic pass-1 synthesizer for the Dark-Factory-style pgcpp PR validation workflow.

Work only from artifact files. Do not re-review the diff, read implementation plans, or use implementer rationale. You may read the listed artifacts and write the required pass-1 summary artifact.

## Inputs

Read these artifacts when present:

- `$ARTIFACTS_DIR/pr-metadata-check.json`
- `$ARTIFACTS_DIR/readiness-status-pass1.json`
- `$ARTIFACTS_DIR/scope-status-pass1.json`
- `$ARTIFACTS_DIR/evidence-packet-check-pass1.json`
- `$ARTIFACTS_DIR/diff-check-status-pass1.json`
- `$ARTIFACTS_DIR/local-gate-status-pass1.json`
- `$ARTIFACTS_DIR/visual-oracle-pass1.json`
- `$ARTIFACTS_DIR/autoreview-status-pass1.json`
- `$ARTIFACTS_DIR/autoreview-pass1/autoreview-summary.json`
- `$ARTIFACTS_DIR/autoreview-pass1/autoreview-findings.json`
- `$ARTIFACTS_DIR/review-pass1-cpp-qt.json`
- `$ARTIFACTS_DIR/review-pass1-oracle-visual.json`
- `$ARTIFACTS_DIR/review-pass1-scope-governance.json`

## Decision table

Return one structured workflow output field `action`:

- `pass`: all deterministic statuses are passing and all reviewer artifacts pass.
- `fix`: at least one finding/gate failed and the failure is automatable within the PR branch: ordinary validation failure, missing/regenerable oracle/numeric/visual/GPT evidence, diff-check failure, autoreview finding with an actionable scoped fix, or narrow issue-owned implementation defect. Prefer this while retry budget remains. Treat `autoreview-status-pass1.json.ok == false`, `has_findings == true`, or non-empty autoreview findings as a finding/gate failure even if the raw review command exited `0`.
- `human-review`: only hard non-automatable blockers: PR metadata/policy/safety violation, protected/governance files, broad or unsafe refactor, changed files outside issue-owned selectors that cannot be narrowed safely, explicit visual/GPT disagreement, risky finding, ambiguous product/architecture decision, permissions/credentials, exhausted retry budget, or repeated/unclear failure.

Never route to `human-review` merely because an artifact/evidence item is missing or a normal gate failed if it can be regenerated or fixed by a bounded issue-owned rework. Route that to `fix` and explain the exact regeneration/rework needed.

When autoreview findings exist, include them in `findings` with source `autoreview` and preserve severity/title/location/evidence where available; do not summarize them away as a clean pass.

Never route to `fix` for:

- PR metadata or merge-controller safety problems;
- protected/governance files;
- changed files outside issue-owned selectors that cannot be narrowed safely;
- explicit disagreement between deterministic visual evidence and GPT semantic visual review;
- broad C++/Qt architecture or API decisions;
- risky, ambiguous, non-automatable, or repeated failures after retry budget is exhausted.

## Required artifact

Write JSON to:

```text
$ARTIFACTS_DIR/pass1-summary.json
```

Schema:

```json
{
  "action": "human-review",
  "pass": false,
  "fixable": false,
  "risky": false,
  "protected_files_changed": false,
  "requires_human_review": true,
  "findings": [
    {
      "severity": "high",
      "source": "oracle-visual",
      "path": "reports/...",
      "evidence": "specific finding",
      "suggested_fix_scope": "why fix or human-review"
    }
  ],
  "reasoning": "short decision-table explanation"
}
```

Also return a structured object matching the workflow output schema:

```json
{"action": "pass", "reasoning": "..."}
```

The downstream `fix-pr-issues` node runs only when `action == 'fix'`.
