# pgcpp validate PR

Purpose: independently validate a PR from the linked issue and PR diff only.

Local commands:

```bash
python3 scripts/factory/check_pr_scope.py --issue-file "$ISSUE_FILE" --changed-files-file "$CHANGED_FILES_FILE"
python3 scripts/factory/apply_pr_verdict.py --input "$VERDICT_JSON"
```

`apply_pr_verdict.py` is dry-run by default and never executes `gh`. A workflow may only prepare `gh pr merge` when an explicit merge-enable flag is supplied and all gates pass.
