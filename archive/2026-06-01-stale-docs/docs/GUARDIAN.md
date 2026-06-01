# pyqtgraph_to_cpp Guardian Agent

This project has an optional **test-phase Guardian agent** that can be triggered every 10 minutes by Hermes cron.

The Guardian is intentionally **advisory-only** for now. It supervises the supervisor, keeps history by resuming the same Hermes session, reports to Michel over Telegram, and stores every real tick report locally for later review. It does not mutate workflow state.

## Architecture

The Guardian setup follows a strict three-layer autonomous-agent architecture:

1. **Worker layer — Execution**
   - `pi-worker` and `pi-release-manager` execute implementation, rework, and release preparation.
   - They use the workflow contract, prompt files, repository tools, and deterministic commands.

2. **Supervisor layer — Evaluation & guardrails**
   - `pi-reviewer`, autoreview proof, deterministic tests, workflow validation, release gates, and PR safety checks evaluate Worker output before publication or execution.
   - Execution and evaluation stay separate: Workers do not self-approve releases.

3. **Meta layer — Continuous learning & optimization**
   - `pyqtgraphguardian` analyzes the workflow itself: repeated failures, missing context, stale-state patterns, weak prompts, missing deterministic checks, and improvement opportunities.
   - The Meta layer proposes synthetic regression tests and workflow/prompt improvements, but does **not** silently change the system.

## Purpose

The Guardian checks the workflow around:

- active supervisor/worker processes
- Hermes Kanban board state
- stale `running` / `blocked` cards
- GitHub open issues and PRs
- local git/workflow validation state
- recent Pi Symphony logs
- duplicate worker/supervisor symptoms
- autoreview/release evidence risks
- workflow design flaws and improvement opportunities
- incidents that should become permanent regression tests

## Persistent agent identity

Profile:

```text
pyqtgraphguardian
```

Persistent session:

```text
pyqtgraph-cpp-guardian
```

Every tick resumes that same session, so the Guardian can use its prior tick history.

## Advisory-only authority boundary

Allowed during test phase:

- inspect supplied snapshots
- reason over prior tick history
- classify health/progress/blockers
- propose fixes
- propose synthetic regression-test candidates
- ask Michel one clear Telegram question if needed

Not allowed unless Michel explicitly approves later:

- kill/restart processes
- dispatch workers
- archive/unblock Kanban cards
- change GitHub labels/issues/PRs
- edit project/workflow files
- edit prompts or workflow policy
- rebase/reset/force-push/merge/bypass checks
- create or modify cron jobs

## Continuous learning rule

When the Guardian sees a real incident or repeated failure mode, it should propose a **synthetic regression candidate** unless the problem is clearly one-off external infrastructure.

A good candidate identifies:

- incident observed
- likely root cause
- missing context / prompt instruction / deterministic guardrail / test
- proposed fixture or assertion
- whether a workflow, prompt, doc, or code patch is needed

During the test phase these are proposals only. They should become actual changes only after deterministic validation and reviewer/Michel approval.

## Files

Project module:

```text
automation/pi_symphony/guardian_agent_tick.py
```

Hermes cron entrypoint:

```text
/home/michel/.hermes/scripts/pyqtgraph_cpp_guardian_agent_tick.py
```

Script-side state/lock files:

```text
/home/michel/.hermes/scripts/state/pyqtgraph_cpp_guardian_agent.json
/home/michel/.hermes/scripts/state/pyqtgraph_cpp_guardian_agent.lock
```

Local report archive:

```text
/home/michel/code/pyqtgraph_to_cpp/.hermes/guardian/reports/YYYY/MM/DD/YYYYMMDDTHHMMSSZ-guardian-report.md
/home/michel/code/pyqtgraph_to_cpp/.hermes/guardian/reports/latest.json
```

Profile persona:

```text
/home/michel/.hermes/profiles/pyqtgraphguardian/SOUL.md
```

## Intended 10-minute cron setup

Do **not** create/start this until Michel asks to start it.

When ready, create a controlled cron job equivalent to:

```text
name: pyqtgraph-cpp-guardian-agent
schedule: every 10m
script: /home/michel/.hermes/scripts/pyqtgraph_cpp_guardian_agent_tick.py
no_agent: true
deliver: origin
```

Why `no_agent: true`:

- the cron job is only a launcher
- the launched process resumes the persistent Guardian Hermes session
- a normal cron LLM run would be fresh and would not be the same history-bearing Guardian

Why `deliver: origin`:

- Guardian reports go back to Michel on Telegram
- the script also prints `Local report: <path>`, so each Telegram report links to the archived local markdown report

## Activation command draft

When Michel asks to start it, use the Hermes cron tool or CLI to create the job with:

```text
schedule: every 10m
script: pyqtgraph_cpp_guardian_agent_tick.py
no_agent: true
deliver: origin
```

Do not start it before explicit approval.

## Manual dry run without starting the Guardian

This only builds the prompt and does **not** invoke Hermes or save a report:

```bash
/home/michel/.hermes/scripts/pyqtgraph_cpp_guardian_agent_tick.py --dry-run
```

This prints the raw JSON snapshot and does **not** invoke Hermes or save a report:

```bash
/home/michel/.hermes/scripts/pyqtgraph_cpp_guardian_agent_tick.py --json-snapshot
```

A real tick, when intentionally tested later, is:

```bash
/home/michel/.hermes/scripts/pyqtgraph_cpp_guardian_agent_tick.py
```

A real tick sends the Guardian output to stdout for Telegram delivery and saves the full local markdown report.

## Expected Telegram output

During test phase, every successful tick should send a compact status:

```text
Guardian tick: pyqtgraph_to_cpp
Health: OK | Warning | Critical
Workflow: progressing | idle | blocked | unknown
Findings:
- ...
Meta-learning candidates:
- none | <incident -> root cause -> proposed regression/guardrail>
Action for Michel: none | <one question/recommendation>
Local report: /home/michel/code/pyqtgraph_to_cpp/.hermes/guardian/reports/...
```

## Later production direction

After the test phase, consider switching to a deterministic script-first watchdog that is silent when healthy and escalates to this Guardian agent only on anomalies. That reduces cost/noise while preserving agent judgement for unexpected failures.
