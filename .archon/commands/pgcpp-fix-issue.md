---
description: Implement a ready PyQtGraph-to-C++ issue using prepared research and plan/investigation artifacts.
argument-hint: (reads workflow artifacts)
---

# pgcpp fix issue

Implement exactly one ready PyQtGraph-to-C++ generated-local GitHub issue.

Use only:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/governance.txt`
- `$ARTIFACTS_DIR/research.md`
- `$ARTIFACTS_DIR/investigation.md` or `$ARTIFACTS_DIR/plan.md`
- current repository code and the pinned upstream PyQtGraph reference needed for the issue.

Do not use DynaChat/FastAPI/Bun/web-app assumptions. Do not adapt `MISSION.md` or `FACTORY_RULES.md` content.

Required implementation discipline:
1. Confirm readiness passed: `scripts/factory/check_issue_ready.py --issue-file "$ARTIFACTS_DIR/issue.json"`.
2. Read the issue goal, owned-file selectors, required local proof, TDD plan, validation commands, acceptance criteria, and scope boundaries.
3. Read relevant existing C++/Qt/OpenCV code and pinned upstream PyQtGraph source before editing.
4. Add or update the smallest focused failing test/oracle first when behavior changes.
5. Implement the minimal native C++/Qt/OpenCV change; do not add Python wrappers or unrelated refactors.
6. Keep every changed path inside issue-owned files/selectors or approved common adjuncts. If scope is wrong, stop and report; do not silently expand it.
7. Produce `$ARTIFACTS_DIR/implementation.md` with changed files, rationale, commands run, exit codes, and artifact paths.

Do not write agent scratch files inside the repository. Put durable evidence in `$ARTIFACTS_DIR`; remove any untracked `.archon-artifacts-staging/`, `reports/agents/`, or `.run_validation.sh` scratch files before returning.

Local checks to run when practical after editing:
```bash
git diff --check
{
  git diff --name-only origin/main...HEAD
  git diff --name-only --cached
  git diff --name-only
  git ls-files --others --exclude-standard
} | sort -u | tee "$ARTIFACTS_DIR/changed-files.txt"
python3 scripts/factory/check_pr_scope.py --issue-file "$ARTIFACTS_DIR/issue.json" --changed-files-file "$ARTIFACTS_DIR/changed-files.txt"
scripts/gate commit --dry-run
```
Run the issue's `Validation commands` when local, safe, and available. For visual-required work, produce deterministic visual artifacts and GPT-5.5 semantic visual-review evidence when required. For oracle/numeric work, produce clear fixture/probe evidence.

Forbidden in this implementation step: commit, push, merge, mutate GitHub labels/issues/PRs, start unrelated browser/app services, add GitHub Actions dependencies, edit product files outside issue scope, or leave debug artifacts.

Return a concise implementation summary and the path to `$ARTIFACTS_DIR/implementation.md`.
