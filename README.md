# pyqtgraph_to_cpp

Repository-owned Pi Symphony automation for translating PyQtGraph into a native C++ library.

The canonical project/port specification is:

- [`docs/pyqtgraph-cpp-port-workflow.md`](docs/pyqtgraph-cpp-port-workflow.md)

`WORKFLOW.md` is the machine-readable Pi Symphony automation runtime config; it is not the product specification.

This repo contains an operational GitHub issue-to-PR automation loop:

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

See `WORKFLOW.md`, `docs/ai-orchestration.md`, and `docs/pyqtgraph-cpp-port-workflow.md` for the full policy.

## Starting the C++ port

The port is intentionally issue-driven and dependency-gated:

1. Create fine-grained GitHub issues using the format in `docs/pyqtgraph-cpp-port-workflow.md`.
2. Label only dependency-free work as `ai:ready`.
3. Let automation create worktrees, run Pi, review, and open PRs.
4. Promote the next issue only after its dependencies are merged or explicitly satisfied.
