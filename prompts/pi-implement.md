Use Pi subagents for this implementation when available: scout/planner read first, implementer writes only issue-scoped changes, tester runs focused checks. Do not commit, push, merge, or create PRs. Leave a clean git diff in the current worktree.

Repository: {{ repo }}
Branch: {{ branch }}
Issue: #{{ issue_number }} {{ issue_title }}
Author: {{ issue_author }}
URL: {{ issue_url }}
Labels: {{ issue_labels }}

Issue body:
{{ issue_body }}

Repo-owned workflow contract:
{{ workflow_body }}

Acceptance rules:
- Implement only the issue scope: issue-owned files plus directly required shared integration files from `policy.shared_integration_files`.
- Use TDD for behavior changes: add/update a focused failing test first, then implement.
- Run relevant focused checks and `git diff --check` before finalizing.
- Do not leave scratch artifacts such as .pi-lens, temp files, or debug logs in the diff.
- Do not modify WORKFLOW.md or automation policy files unless the issue explicitly asks for it.
