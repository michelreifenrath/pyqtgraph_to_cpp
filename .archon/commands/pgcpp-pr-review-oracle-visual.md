---
description: Pass-1 holdout reviewer for oracle, numeric, visual, and interaction evidence in a pgcpp PR.
argument-hint: (no arguments - reads workflow artifacts)
---

# pgcpp PR Review: Oracle and visual evidence

You are one independent pass-1 holdout reviewer in the PR validation workflow.

Use only workflow artifacts and the checked-out PR branch. Do not read implementation plans, coder rationale, PR comments, review threads, or sibling workflow artifacts. Do not use DynaChat, FastAPI, Bun, or browser-app assumptions.

## Inputs to inspect

- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/pr.json`
- `$ARTIFACTS_DIR/changed-files.txt`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/visual-oracle-pass1.json`
- `$ARTIFACTS_DIR/local-gate-pass1.txt`
- `$ARTIFACTS_DIR/diff-check-pass1.txt`
- visual/oracle artifacts under `$ARTIFACTS_DIR/visual-pass1/`, `reports/`, `oracle/`, and issue-specific report paths when referenced by the PR body

## Review focus

- Determine from the linked issue whether numeric/oracle, visual, rendering, or interaction validation is required.
- Confirm required issue validation commands were run or that the workflow ran an equivalent deterministic gate.
- For numeric/oracle work, require clear command output or committed artifact evidence.
- For visual/rendering/pixel work, require deterministic visual artifacts and GPT-5.5 semantic visual-review evidence when required by the issue, `FACTORY_RULES.md`, or `WORKFLOW.md`.
- If deterministic metrics and GPT visual review disagree, set `requires_human_review: true`.
- Do not approve missing visual/oracle proof merely because generic CTest passed.

## Fixability rule

Missing or ambiguous oracle/visual evidence is normally not self-fixable in PR validation unless the required command is explicit, local, safe, and inside the linked issue scope. Never mark fixable when a new oracle design, new visual baseline decision, or GPT semantic judgment is needed.

## Required artifact

Write JSON to:

```text
$ARTIFACTS_DIR/review-pass1-oracle-visual.json
```

Schema:

```json
{
  "pass": false,
  "fixable": false,
  "risky": false,
  "protected_files_changed": false,
  "requires_human_review": false,
  "visual_required": false,
  "oracle_required": false,
  "gpt_visual_review_accepted": false,
  "findings": [
    {
      "severity": "high",
      "path": "reports/visual/...",
      "evidence": "specific missing or failing proof",
      "suggested_fix_scope": "minimal scoped fix or human-review reason"
    }
  ]
}
```

Return a concise summary of evidence checked and whether the artifact was written.
