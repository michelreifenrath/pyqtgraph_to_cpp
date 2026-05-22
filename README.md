# pyqtgraph_to_cpp

Repository-owned Pi Symphony automation for translating pyqtgraph-oriented work toward C++.

This repo now contains an operational GitHub issue-to-PR automation loop:

- GitHub Issues are the source of truth.
- Label an issue `ai:ready` to let automation claim it.
- Hermes Kanban board `pyqtgraph-to-cpp` stores the durable task graph.
- Hermes profiles split responsibilities:
  - `pi-orchestrator`: intake/reconciliation only.
  - `pi-worker`: runs Pi CLI/pi-subagents in isolated worktrees.
  - `pi-reviewer`: deterministic review plus mandatory autoreview/Codex gate.
  - `pi-release-manager`: commits, pushes, opens PRs, never merges.
- Work happens in git worktrees under `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp`.
- Auto-merge is disabled by policy.

## Common commands

```bash
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
python3 -m automation.pi_symphony.cli doctor --workflow WORKFLOW.md
python3 -m automation.pi_symphony.cli setup --workflow WORKFLOW.md
python3 -m automation.pi_symphony.cli intake --workflow WORKFLOW.md --dry-run
python3 -m automation.pi_symphony.cli reconcile --workflow WORKFLOW.md
python3 -m automation.pi_symphony.cli run-issue --workflow WORKFLOW.md --issue <N> --phase all
python3 -m pytest -q
```

## Operational flow

1. Create a GitHub issue with clear acceptance criteria.
2. Add labels such as `ai:ready`, optionally `tenant:cpp`, `tag:parser`, `tag:build`, or `tag:docs`.
3. The cron reconciler claims the issue, removes `ai:ready`, creates the Kanban graph, and dispatches workers.
4. Pi implements in a dedicated worktree.
5. Hermes review gates run.
6. Hermes opens a PR and labels the issue `ai:review`.
7. A human reviews and merges when satisfied.

See `WORKFLOW.md` and `docs/ai-orchestration.md` for the full policy.
