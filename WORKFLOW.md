---
tracker:
  kind: github
  repo: michelreifenrath/pyqtgraph_to_cpp
  issue_query: "is:issue is:open label:ai:ready -label:ai:claimed -label:ai:blocked -label:ai:ignore"
workspace:
  root: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp
  strategy: git-worktree
  base_branch: main
  preserve_on_success: true
  cleanup_terminal_issues: false
agent:
  max_concurrent_issues: 4
  max_attempts: 10
  normal_continue_delay_ms: 500
  max_retry_backoff_ms: 300000
pi:
  command: pi
  provider: openai-codex
  model: gpt-5.5
  default_thinking: xhigh
  implementation_thinking: xhigh
  use_subagents: true
  session_root: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/.pi-sessions
autoreview:
  enabled: true
  command: /home/michel/.hermes/profiles/pi-reviewer/skills/external/autoreview/scripts/autoreview
  engine: codex
  mode: branch
  base: origin/main
  require_clean: true
  advisory: true
  mandatory_gate: true
github:
  use_gh_cli_auth: true
  ready_label: "ai:ready"
  claimed_label: "ai:claimed"
  blocked_label: "ai:blocked"
  review_label: "ai:review"
  merge_ready_label: "ai:merge-ready"
  failed_label: "ai:failed"
  done_label: "ai:done"
  ignore_label: "ai:ignore"
  human_review_label: human-review
github_output:
  style: compact
  issue_body_max_chars: 1200
  pr_body_max_chars: 900
  comment_max_chars: 300
  comments:
    claim: false
    rework_scheduled: true
    blocked: true
    pr_ready: true
  pr_body:
    include_changed_files: false
    include_safety_section: false
    include_task_ids: false
    include_logs: false
    include_validation: true
  issue_body:
    template: compact
    include_agent_instructions: false
    include_workflow_rules: false
policy:
  require_clean_base: true
  never_push_to_main: true
  require_tests_before_pr: true
  require_independent_review: true
  require_autoreview_before_merge: true
  require_autoreview_before_pr: true
  auto_merge: false
  max_changed_files_without_human_review: 20
  max_diff_lines_without_human_review: 1500
  generated_diff_exceptions:
    - path: port_manifest.yaml
      verify_command: "python3 oracle/scripts/generate_class_inventory.py --check"
    - path: oracle/fixtures/hierarchy_pyqtgraph.json
      verify_command: "python3 oracle/scripts/dump_pyqtgraph_hierarchy.py --check"
validation:
  diff_check: true
  commands:
    - "python3 -m pytest -q"
kanban:
  board_slug: pyqtgraph-to-cpp
  board_scope: project
  tenant_strategy: tags
  default_tenant: cpp
  tenant_label_prefix: 'tenant:'
  tag_label_prefix: 'tag:'
---
# Pi Symphony Workflow

This repository runs a Symphony-style GitHub issue-to-PR loop using Hermes as the durable orchestrator and Pi CLI/pi-subagents as the implementation engine.

## Source of truth

- GitHub Issues are the external source of truth.
- Issues labeled `ai:ready` are eligible for automation.
- Issues labeled `ai:ignore`, `ai:blocked`, `ai:claimed`, or `ai:done` are not claimed by intake.
- Hermes Kanban board `pyqtgraph-to-cpp` is the durable internal task graph and audit trail.
- One board is used for the whole repository; tenants and tags are derived from `tenant:` and `tag:` labels.

## Issue execution prompt

For every issue:

1. Claim the GitHub issue and remove `ai:ready`.
2. Create a Kanban graph: implement -> review -> release.
3. Create an isolated git worktree under `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-<number>`.
4. Run Pi CLI non-interactively with subagents: scout, planner, implementer, tester.
5. Do not commit, push, or merge from Pi.
6. Hermes reruns deterministic validation after Pi exits.
7. Hermes reviewer reruns validation and the mandatory autoreview/Codex review gate.
8. Hermes release manager commits, pushes an `ai/issue-<number>-<slug>` branch, and opens/updates a PR.
9. Auto-merge is disabled; humans decide when to merge.

## Safety gates

- Never push to `main`.
- Do not edit the main checkout for issue work.
- Require `git diff --check` and the configured validation commands before PR creation.
- Use `scripts/gate commit` for the local pre-PR validation wrapper.
- Require independent review and autoreview before PR creation.
- Use `scripts/run_autoreview --mode branch` for the local pre-PR autoreview wrapper.
- Block for human review if the review-surface diff exceeds the configured file or line limits after verified generated-file exceptions.
- Generated-file exceptions must pass their configured read-only verification command before automation discounts them from review-surface size.
- Treat Pi and AI-reviewer reports as advisory; deterministic checks and actual code inspection are authoritative.
- On failure, prefer bounded `ai:rework` unless current evidence proves no safe automated path remains.
- Use `human-review` only for hard human blockers (credentials/auth/permissions, policy or safety constraints, retry budget exhausted/repeated-loop protection) or explicit important design/architecture decisions.
- Do not mark `human-review` for ordinary validation failures, empty/no-diff implementation handoffs, scratch-artifact cleanup, or actionable review findings; schedule rework instead.
