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
- `$ARTIFACTS_DIR/review-pass1-cpp-qt.json`
- `$ARTIFACTS_DIR/review-pass1-oracle-visual.json`
- `$ARTIFACTS_DIR/review-pass1-scope-governance.json`

## Decision table

Return one structured workflow output field `action`:

- `pass`: all deterministic statuses are passing and all reviewer artifacts pass.
- `fix`: at least one finding/gate failed, every failure is marked fixable, no reviewer reports risky/protected/human-review, and the fix is narrow/issue-owned/deterministic.
- `human-review`: any readiness, metadata, scope, protected-file, visual/GPT, risky, ambiguous, non-fixable, missing-artifact, or repeated/unclear failure exists.

Never route to `fix` for:

- missing or failed issue readiness;
- PR metadata problems;
- protected/governance files;
- changed files outside issue-owned selectors;
- missing required visual/oracle/GPT evidence;
- broad C++/Qt architecture or API decisions;
- failed head SHA or merge-controller safety assumptions.

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
