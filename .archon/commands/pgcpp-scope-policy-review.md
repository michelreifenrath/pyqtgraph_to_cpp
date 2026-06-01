---
description: Review scope, ownership, and policy compliance for pgcpp PRs.
argument-hint: (reads workflow artifacts)
---

# pgcpp scope and policy review

Act as an independent policy reviewer. Do not edit files.

Check:
- The linked issue passed `scripts/factory/check_issue_ready.py --issue-file "$ARTIFACTS_DIR/issue.json"`.
- Changed files pass `scripts/factory/check_pr_scope.py` and match issue-owned selectors/common adjuncts.
- Protected files are not changed unless the issue explicitly owns automation/governance work.
- No GitHub Actions dependency, main-branch push, merge, or DynaChat/web-app assumption was introduced.
- `MISSION.md` and `FACTORY_RULES.md` were not adapted as part of this task.

Write `$ARTIFACTS_DIR/review-scope-policy.md` with verdict, findings, and self-fixability.
