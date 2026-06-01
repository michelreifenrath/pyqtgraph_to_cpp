---
description: Create or update a draft PR for a validated pgcpp issue branch.
argument-hint: (reads workflow artifacts)
---

# pgcpp create PR

Create or update a draft pull request for the current branch. This command may commit and push the PR branch. It must never push to `main`, merge, or alter `MISSION.md`/`FACTORY_RULES.md` content.

Read:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/changed-files.txt`
- `$ARTIFACTS_DIR/scope.json`
- `$ARTIFACTS_DIR/local-gate.txt`
- `$ARTIFACTS_DIR/autoreview-pre-pr.txt` if present
- `$ARTIFACTS_DIR/validation.md`

Procedure:
1. Confirm the current branch is not `main` and `git status` only contains intended issue-scoped changes.
2. Stage explicit changed files only; never use `git add -A` or `git add .`.
3. Commit with a descriptive conventional-style subject if no suitable commit exists.
4. Push the PR branch with `git push -u origin HEAD`.
5. Create or update a draft PR against `main`.
6. PR body must include `Fixes #N`/`Closes #N`/`Resolves #N` for exactly one issue plus summary, changed files, validation commands, and evidence/artifact paths.
7. Apply repo labels only when they exist and are appropriate, such as `ai:review`; do not use DynaChat `factory:needs-review` labels unless this repo defines them.
8. Write `$ARTIFACTS_DIR/.pr-number`, `$ARTIFACTS_DIR/.pr-url`, and `$ARTIFACTS_DIR/pr-body.md`.

Return PR number, URL, commit SHA, and validation summary.
