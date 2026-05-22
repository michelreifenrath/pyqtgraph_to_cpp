# AI orchestration

This repository implements a Symphony-style automation system using GitHub, Hermes Kanban, Hermes profiles, and Pi CLI/pi-subagents.

For the PyQtGraph→C++ product/engineering specification, see `docs/pyqtgraph-cpp-port-workflow.md`. This file documents the automation layer only.

## Architecture

```text
GitHub issue + ai:ready
  -> cron/webhook reconciliation
  -> pi-orchestrator intake
  -> Hermes Kanban task graph
      implement -> review -> release
  -> pi-worker runs Pi CLI in an isolated git worktree
  -> pi-reviewer runs deterministic checks + autoreview/Codex review
  -> pi-release-manager commits, pushes branch, opens PR
  -> human review/merge
```

## GitHub labels

Required automation labels:

- `ai:ready`: eligible for automation.
- `ai:claimed`: automation has claimed the issue.
- `ai:blocked`: human input or missing prerequisite required.
- `ai:review`: PR exists and needs review.
- `ai:failed`: automation failed a hard gate.
- `ai:done`: PR merged or automation completed.
- `ai:ignore`: never automate this issue.
- `human-review`: diff or risk requires human attention.

Tenant/tag labels:

- `tenant:core`, `tenant:cpp`
- `tag:parser`, `tag:build`, `tag:docs`

## One-board policy

Use one repo-level Hermes Kanban board:

- slug: `pyqtgraph-to-cpp`
- scope: `project`

Do not create boards named for issues, PRs, or agents. Separate workstreams live inside the same board via `tenant:` and `tag:` labels.

## Runtime commands

The `pi-symphony` CLI owns all deterministic side effects:

- `setup`: creates labels and board.
- `intake`: claims `ai:ready` issues and creates Kanban tasks.
- `reconcile`: intake + merged-PR cleanup + dispatcher pass.
- `run-issue --phase implement`: creates worktree, calls Pi CLI, validates diff.
- `run-issue --phase review`: reruns validations and mandatory review gate.
- `run-issue --phase release`: commits, pushes, opens/updates PR.
- `check-prs`: marks merged AI PRs/issues done.

## Safety model

- GitHub Issues remain source of truth.
- Worktrees isolate concurrent issues.
- Pi never commits, pushes, or merges.
- Hermes release manager never merges.
- PR creation requires successful validation and review gates.
- Oversized diffs are blocked with `human-review`.
- Failed runs label the issue `ai:blocked`/`ai:failed` instead of retrying blindly.

## Manual smoke test

Create a small README-only issue, label it `ai:ready` and `tag:docs`, then run:

```bash
python3 -m automation.pi_symphony.cli reconcile --workflow WORKFLOW.md
hermes kanban --board pyqtgraph-to-cpp list
```

The task graph should show implement/review/release cards. Do not start with a large code migration issue until the README smoke test completes and opens a PR.
