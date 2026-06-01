---
description: Decide which pgcpp specialized review agents should run.
argument-hint: (reads workflow artifacts)
---

# pgcpp review classify

Read `$ARTIFACTS_DIR/review-scope.md`, `$ARTIFACTS_DIR/issue.json`, and changed-file artifacts. Do not use tools that mutate files or GitHub.

Return only the structured JSON requested by the workflow schema:
- `run_cpp_qt_review`: `true` unless the diff is docs-only.
- `run_oracle_visual_review`: `true` when numeric/oracle/visual/example/rendering evidence is required or changed.
- `run_test_coverage_review`: `true` when behavior, tests, examples, or validation changed.
- `run_scope_policy_review`: always `true` for autonomous PRs.
- `run_docs_examples_review`: `true` when docs/examples/user-facing names changed.
- `reasoning`: brief explanation.

Use strings `true`/`false`, not booleans.
