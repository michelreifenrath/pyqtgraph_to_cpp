# pgcpp comprehensive test

Purpose: run post-merge or scheduled local regression checks and prepare evidence-backed regression issues.

Local commands:

```bash
python3 -m pytest -q
scripts/gate commit --dry-run
python3 scripts/factory/file_regression_issue.py --input "$REGRESSION_JSON"
```

The regression issue helper renders JSON only; it does not call GitHub.
