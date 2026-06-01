# pyqtgraph_to_cpp

Repository-owned factory automation and source tree for translating PyQtGraph into a native C++ library.

## Current source of truth

Use these active documents for current work:

- [`MISSION.md`](MISSION.md) — product goal, scope, non-goals, and hard invariants.
- [`FACTORY_RULES.md`](FACTORY_RULES.md) — issue readiness, scope, evidence, validation, auto-merge, attribution, and protected-file rules.
- [`AGENTS.md`](AGENTS.md) — repository-wide instructions for AI agents.
- [`WORKFLOW.md`](WORKFLOW.md) — machine-readable factory runtime configuration.

Older planning, long-form workflow docs, and the retired Pi Symphony runtime are archived under `archive/2026-06-01-stale-docs/` for history only.

## Automation model

This repo contains an issue-to-PR factory loop:

- GitHub Issues are the source of truth.
- Label an issue `ai:ready` only after readiness gates pass.
- Archon workflows and factory scripts own the active automation path.
- Pi workers may run inside isolated git worktrees, but implementation, rework, review, and release workers never merge.
- The validation/merge controller may auto-merge only when `WORKFLOW.md` enables `policy.auto_merge` and all governed gates in `FACTORY_RULES.md` pass.
- Worktrees live under `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp`.

## Common commands

```bash
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
scripts/factory/check_issue_ready.py --issue-file <issue.json>
scripts/factory/check_pr_scope.py --issue-file <issue.json> --changed-files-file <paths.txt>
scripts/gate focus
scripts/gate commit
scripts/run_autoreview --mode branch
python3 -m pytest -q
```

Use `.venv/bin/python -m pytest -q` when the system Python does not have pytest installed.

## Operational flow

1. Create or update one fine-grained GitHub issue with clear scope, owned files/selectors, TDD plan, validation commands, and acceptance criteria.
2. Add `ai:ready` only when dependencies are resolved and readiness gates pass.
3. Automation claims the issue, creates an isolated worktree, and runs the configured workers.
4. Review and release gates produce an evidence-backed PR.
5. The validation/merge controller performs holdout validation and either merges, schedules focused rework, or marks `human-review`.
