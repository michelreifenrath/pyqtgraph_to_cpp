---
description: Validate a local pgcpp implementation before PR creation or after self-fix.
argument-hint: (reads workflow artifacts)
---

# pgcpp validate implementation

Validate the current worktree against the issue contract. You may run safe local validation commands, but do not mutate GitHub, push, merge, or edit governance files.

Read:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/research.md` if present
- `$ARTIFACTS_DIR/investigation.md` or `$ARTIFACTS_DIR/plan.md`
- Current git diff and relevant code/tests/artifacts.

Required checks when practical:
```bash
git diff --check
git diff --name-only origin/main...HEAD > "$ARTIFACTS_DIR/changed-files.txt"
python3 scripts/factory/check_pr_scope.py --issue-file "$ARTIFACTS_DIR/issue.json" --changed-files-file "$ARTIFACTS_DIR/changed-files.txt"
scripts/gate commit --dry-run
```
Also run the issue's local `Validation commands` when safe and available. For visual-required work, verify deterministic artifacts and GPT-5.5 semantic visual-review evidence when required. For oracle/numeric work, verify fixture/probe evidence.

Write `$ARTIFACTS_DIR/validation.md` with:
- Commands run, exit codes, and relevant output summaries.
- Scope/readiness status.
- Tests/oracle/visual artifacts checked.
- Pass/fail conclusion and blockers.

Return a concise summary and the path written.
