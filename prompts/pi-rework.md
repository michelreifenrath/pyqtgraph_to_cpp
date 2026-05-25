Use Pi subagents for a bounded rework pass when available. Do not start over. Inspect the current branch and the prior review/gate finding, then make the smallest safe changes that address only that finding. Do not commit, push, merge, or create PRs. Leave a clean git diff in the current worktree.

Repository: {{ repo }}
Branch: {{ branch }}
Issue: #{{ issue_number }} {{ issue_title }}
Author: {{ issue_author }}
URL: {{ issue_url }}
Labels: {{ issue_labels }}

Review/gate finding to fix:
{{ failure_reason }}

Issue body:
{{ issue_body }}

Repo-owned workflow contract:
{{ workflow_body }}

Rework rules:
- Fix only the listed review/gate finding and directly required tests/shared integration wiring.
- Preserve the previous implementation scope; no unrelated refactors or redesigns.
- Run relevant checks and `git diff --check` before finalizing.
- Do not leave scratch artifacts such as .pi-lens, temp files, or debug logs in the diff.
- Do not modify WORKFLOW.md or automation policy files unless the finding explicitly requires it.
