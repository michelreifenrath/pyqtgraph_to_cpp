---
description: Apply one scoped fix pass to a pgcpp PR based only on pass-1 validation findings.
argument-hint: (no arguments - reads pass1-summary.json)
---

# pgcpp Fix PR Issues

You are a fresh-context fixer for one PR validation pass. You are not the original implementer. Read only pass-1 validation artifacts, especially:

```text
$ARTIFACTS_DIR/pass1-summary.json
```

Do not read implementation plans, coder rationale, PR comments, review threads, or sibling workflow artifacts. Do not use DynaChat/FastAPI/Bun assumptions.

## Hard rules

1. Fix only findings listed in `pass1-summary.json`.
2. Edit only issue-owned files/common adjuncts already allowed by the linked issue and `check_pr_scope.py`.
3. Do not edit protected/governance files, `.archon/**`, `WORKFLOW.md`, `MISSION.md`, `FACTORY_RULES.md`, or docs/governance policy unless the issue explicitly owns an automation/governance change.
4. Do not invent product/API/scope decisions. Stop and report if a finding is ambiguous.
5. Do not modify tests to hide a product bug; add/update focused proof only when the finding explicitly requires missing coverage and the issue owns the test/oracle path.
6. Commit and push only to the current PR head branch. Never push to `main` and never merge.

## Procedure

1. Confirm `pass1-summary.json` has `action: "fix"` and concrete findings.
2. Group findings by file and apply the smallest deterministic C++/Qt/OpenCV/test/oracle changes.
3. Run relevant focused validation immediately.
4. Run the required local gates before committing:

```bash
git diff --check
git diff --name-only origin/main...HEAD > "$ARTIFACTS_DIR/changed-files-after-fix.txt"
python3 scripts/factory/check_pr_scope.py --issue-file "$ARTIFACTS_DIR/issue.json" --changed-files-file "$ARTIFACTS_DIR/changed-files-after-fix.txt"
scripts/gate merge --base origin/main --reports-dir "$ARTIFACTS_DIR/gate-fix"
```

5. Commit with an explicit validation-fix message and push the PR branch:

```bash
git add <only changed files>
git commit -m "fix: address pgcpp validation findings"
git push
```

## Required artifact

Write JSON to:

```text
$ARTIFACTS_DIR/fix-attempt.json
```

Schema:

```json
{
  "attempted": true,
  "fix_attempts": 1,
  "commit": "sha",
  "pushed": true,
  "changed_files": ["src/..."],
  "validation": ["commands run with exit codes"],
  "unresolved_findings": []
}
```

If you cannot safely fix every requested finding, do not pretend. Leave unresolved items in the artifact with a concrete reason and avoid broad cleanup.
