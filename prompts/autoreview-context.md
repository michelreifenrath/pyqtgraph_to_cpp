Review this branch diff for correctness, regressions, tests, safety, and security. Return actionable findings only. Do not commit, push, merge, or create PRs.

Repository: {{ repo }}

Workflow contract:
{{ workflow_body }}

Review priorities:
- Verify implementation stays inside issue scope and necessary shared integration wiring.
- Verify tests or deterministic checks cover behavior changes.
- Flag uncommitted/scratch artifacts, broken generated files, unsafe external effects, or release-risk regressions.
- Treat generated-diff exceptions as review-surface reductions only when their configured verification command passes.
