---
description: Investigate a pgcpp bug issue and write implementation-ready findings.
argument-hint: (reads workflow artifacts)
---

# pgcpp investigate issue

Use this command only for issues classified as bugs. Do not edit code.

Read:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/research.md`
- Relevant current code, tests, oracle fixtures, visual artifacts, and pinned upstream PyQtGraph reference.

Investigate the externally observable failure described by the issue. When safe, run focused read-only or reproducer commands; do not run broad destructive commands and do not mutate GitHub.

Write `$ARTIFACTS_DIR/investigation.md` with:
- Reproduction or evidence summary.
- Expected PyQtGraph behavior and current C++/Qt behavior.
- Likely root cause with file/function references.
- Minimal owned-file fix strategy.
- Failing test/oracle/visual proof to add or update first.
- Validation commands to run after the fix.
- Any blocker requiring human direction.

Return a concise summary and the path written.
