# pgcpp issue readiness

Purpose: check whether one issue is small, testable, dependency-clear, and safe for autonomous implementation.

Local command:

```bash
python3 scripts/factory/check_issue_ready.py --issue-file "$ISSUE_FILE"
```

Default behavior is local JSON output only. GitHub label/comment mutations must be performed by a separate explicitly authorized runner, not by this command file.
