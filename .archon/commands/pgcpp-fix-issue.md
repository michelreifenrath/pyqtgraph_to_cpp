# pgcpp fix issue

Purpose: implement exactly one ready issue.

Required local gates:

```bash
python3 scripts/factory/check_issue_ready.py --issue-file "$ISSUE_FILE"
python3 -m pytest -q
scripts/gate commit --dry-run
```

Rules: read `MISSION.md`, `FACTORY_RULES.md`, `AGENTS.md`, and the pinned PyQtGraph reference; add or update a failing test/oracle first; change only owned files; do not merge.
