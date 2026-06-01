---
description: Apply only synthesized self-fixable pgcpp review findings.
argument-hint: (reads workflow artifacts)
---

# pgcpp self-fix review findings

Apply a narrow fix pass after PR review. You may edit issue-owned files only for findings marked self-fixable. You may commit and push the PR branch if changes are made. Never push to `main`, merge, edit governance files, or broaden scope.

Read:
- `$ARTIFACTS_DIR/review-findings.json`
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/scope.json`
- Current diff and relevant files.

Procedure:
1. If verdict is `pass`, write `$ARTIFACTS_DIR/self-fix.md` saying no fixes were needed and make no edits.
2. If verdict is `human-review`, stop without edits and write the reason.
3. For verdict `fix`, address only listed self-fixable findings.
4. Run focused validation for changed files plus `git diff --check` and scope check when practical.
5. Commit with an explicit validation-fix message and push the PR branch only if changes were made.
6. Write `$ARTIFACTS_DIR/self-fix.md` with files changed, findings addressed, commands run, commit SHA, and any unresolved findings.

Do not modify tests to hide failures; fix source unless the finding specifically identifies a test/oracle fixture defect inside issue scope.
