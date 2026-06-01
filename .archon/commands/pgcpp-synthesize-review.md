---
description: Synthesize pgcpp issue-to-PR review findings into self-fix instructions.
argument-hint: (reads workflow artifacts)
---

# pgcpp synthesize review

Aggregate independent review artifacts. Do not re-review the diff and do not edit files.

Read any present files:
- `$ARTIFACTS_DIR/review-cpp-qt.md`
- `$ARTIFACTS_DIR/review-oracle-visual.md`
- `$ARTIFACTS_DIR/review-test-coverage.md`
- `$ARTIFACTS_DIR/review-scope-policy.md`
- `$ARTIFACTS_DIR/review-docs-examples.md`

Write `$ARTIFACTS_DIR/review-findings.json` with:
```json
{
  "verdict": "pass|fix|human-review",
  "findings": [
    {"severity": "critical|high|medium|low", "path": "file or empty", "description": "actionable finding", "self_fixable": true}
  ],
  "human_review_reason": ""
}
```

Rules:
- `human-review` for protected-file changes, scope expansion, missing required visual/GPT/oracle evidence, risky C++/Qt architecture, or ambiguity needing product/API/scope decisions.
- `fix` only for small, deterministic, issue-owned findings.
- `pass` only when all review artifacts pass or contain no blockers.

Return a concise synthesis and the artifact path.
