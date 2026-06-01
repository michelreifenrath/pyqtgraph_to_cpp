# pgcpp validate PR

Act as the final pass-2 holdout reviewer for one PyQtGraph-to-C++ PR in the Dark-Factory-style validation/merge-controller workflow. Use only workflow artifacts: PR metadata/diff, changed files, linked issue JSON, base governance captured from `origin/main`, pass-1 reviewer summaries, optional fix-attempt evidence, pass-2 deterministic gate outputs, autoreview output, visual/oracle outputs, and the checked-out PR. Do not rely on the implementer's rationale and do not use DynaChat/FastAPI/Bun assumptions.

This workflow may auto-merge after your verdict. You must write a complete verdict JSON to:

```text
$ARTIFACTS_DIR/verdict.json
```

## Holdout inputs

Read and synthesize, when present:

- `$ARTIFACTS_DIR/pr.json`
- `$ARTIFACTS_DIR/pr-metadata-check.json`
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/changed-files.txt`
- `$ARTIFACTS_DIR/base-governance.txt`
- `$ARTIFACTS_DIR/pass1-summary.json`
- `$ARTIFACTS_DIR/fix-attempt.json`
- `$ARTIFACTS_DIR/readiness-status-pass2.json`
- `$ARTIFACTS_DIR/scope-status-pass2.json`
- `$ARTIFACTS_DIR/diff-check-status-pass2.json`
- `$ARTIFACTS_DIR/local-gate-status-pass2.json`
- `$ARTIFACTS_DIR/visual-oracle-pass2.json`
- `$ARTIFACTS_DIR/autoreview-status-pass2.json`
- `$ARTIFACTS_DIR/autoreview-pass2.txt`

## Final review requirements

- Confirm the PR links exactly one issue and that `check_issue_ready.py` passed for that issue in pass 2.
- Confirm PR metadata is safe: open, not draft, base branch `main`, head branch not `main`, and a captured head SHA exists.
- Confirm every changed path is allowed by `check_pr_scope.py` and by the issue's owned files/selectors/common adjuncts.
- Confirm real merge gates ran in pass 2: `scripts/gate merge`, `git diff --check origin/main...HEAD`, and `scripts/run_autoreview --mode merge --base origin/main`.
- Confirm pass-1 findings were either clean, fixed by one scoped fix pass, or escalated. Do not approve unresolved pass-1 human-review findings.
- Inspect the final diff only as needed for holdout validation of native C++/Qt/OpenCV quality, PyQtGraph naming/hierarchy parity, and absence of Python-wrapper/web assumptions.
- Check that behavior changes include focused TDD/oracle proof and that the issue's validation commands were run when practical.
- For numeric/oracle work, require clear command output or artifact evidence.
- For visual/rendering/pixel work, require deterministic visual artifacts plus GPT-5.5 semantic visual-review evidence when the project workflow requires it.
- If deterministic metrics and GPT visual review disagree, route to `human-review`.
- Treat protected-file changes, broad refactors, missing oracle evidence, missing visual evidence, scope expansion, risky findings, or ambiguity as human-review unless explicitly owned and safe for this controller.

## Verdict JSON fields required by the merge helper

```json
{
  "pr_number": 0,
  "linked_issues": [0],
  "pr_state": "OPEN",
  "is_draft": false,
  "base_ref": "main",
  "head_ref": "ai/issue-0-slug",
  "head_sha": "captured-head-sha",
  "auto_merge_enabled": true,
  "readiness": true,
  "evidence": true,
  "scope": true,
  "tests": true,
  "autoreview": true,
  "diff_check": true,
  "visual_required": false,
  "visual": false,
  "gpt_visual_review": false,
  "holdout": true,
  "protected_files_changed": false,
  "risky": false,
  "fixable": false,
  "fix_attempts": 0,
  "max_fix_attempts": 1
}
```

For visual-required PRs, set `visual_required: true`, `visual: true`, and either `gpt_visual_review: true` or a structured object such as:

```json
{"verdict": "pass", "recommendation": "merge_ok"}
```

The controller will recheck the head SHA and then run:

```bash
python3 scripts/factory/apply_pr_verdict.py --input "$ARTIFACTS_DIR/verdict.json" --allow-merge
```

That helper may execute a guarded squash merge with `--match-head-commit` only when every governed gate is passing and `WORKFLOW.md` policy allows auto-merge. Return a concise review summary with decision, blocking findings, evidence checked, commands/artifacts inspected, fix attempt count, and whether the verdict file was written.
