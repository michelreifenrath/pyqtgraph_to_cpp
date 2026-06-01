---
description: Review pgcpp draft PR scope before specialized review fanout.
argument-hint: (reads workflow artifacts)
---

# pgcpp PR review scope

Review the just-created PR from a fresh context. Do not mutate files or GitHub.

Read:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/.pr-number`
- `$ARTIFACTS_DIR/changed-files.txt` or `$ARTIFACTS_DIR/changed-files-final.txt`
- `$ARTIFACTS_DIR/scope.json`
- `$ARTIFACTS_DIR/validation.md`
- Current diff against `origin/main`.

Write `$ARTIFACTS_DIR/review-scope.md` with:
- PR/issue linkage status.
- Changed-file ownership and protected-file status.
- Whether C++/Qt code review is needed.
- Whether oracle/visual review is needed.
- Whether test coverage review is needed.
- Whether docs/examples review is needed.
- Any immediate human-review blockers.

Return a concise routing recommendation.
