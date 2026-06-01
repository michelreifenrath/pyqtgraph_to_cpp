---
description: Simplify pgcpp issue changes after review/self-fix.
argument-hint: (reads workflow artifacts)
---

# pgcpp simplify changes

Perform a narrow simplification pass. Preserve behavior, tests, and evidence. Do not broaden scope.

Read issue artifacts, review findings, validation evidence, and current diff. Remove only:
- Debug artifacts or accidental scratch files.
- Unused imports/includes/code introduced by this change.
- Overly broad abstractions or speculative configurability introduced by this change.
- Unnecessary comments/docs added by this change.

Do not rewrite unrelated code. Do not adapt `MISSION.md` or `FACTORY_RULES.md`. Do not merge.

Do not create repo-root `artifacts/`, `.archon-artifacts-staging/`, `reports/agents/`, or `.run_validation.sh` files. Durable workflow notes must go to the absolute `$ARTIFACTS_DIR` path only; if that env var is unavailable, report that limitation instead of writing relative scratch files.

Run `git diff --check` and any focused validation needed for edited files. If you edit after a PR already exists, first confirm the current branch is not `main`, then commit and push only the PR branch. Write `$ARTIFACTS_DIR/simplify.md` with changes made, validation, and commit SHA if applicable.
