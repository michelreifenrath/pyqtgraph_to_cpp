---
description: Produce the final issue-to-PR completion report for pgcpp.
argument-hint: (reads workflow artifacts)
---

# pgcpp issue completion report

Produce a concise final report. Do not edit code or mutate GitHub.

Read available artifacts:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/.pr-number`
- `$ARTIFACTS_DIR/.pr-url`
- `$ARTIFACTS_DIR/factory-evidence.md`
- `$ARTIFACTS_DIR/validation.md`
- `$ARTIFACTS_DIR/review-findings.json`
- `$ARTIFACTS_DIR/self-fix.md`
- `$ARTIFACTS_DIR/simplify.md`
- `$ARTIFACTS_DIR/local-gate-final.txt`
- `$ARTIFACTS_DIR/autoreview-final.txt`

Write `$ARTIFACTS_DIR/completion-report.md` with:
- Issue and PR identifiers.
- Summary of changed files and behavior.
- Validation commands and outcomes.
- Oracle/visual artifacts and GPT-5.5 semantic visual-review status when applicable.
- Review/self-fix/simplify outcome.
- Open risks or human-review blockers.
- Recommended next workflow: `pgcpp-validate-pr` for the PR.

Return the same concise summary and path written.
