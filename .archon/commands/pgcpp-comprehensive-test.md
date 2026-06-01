# pgcpp comprehensive test

Review the Dark-Factory-style comprehensive regression run for the native C++/Qt PyQtGraph port. This workflow runs against a clean `origin/main` worktree and is not a PR holdout validator.

Do not use DynaChat, FastAPI, Bun, browser-app, RAG, or web-service assumptions. Do not self-heal code, push branches, merge, or edit governance files. `MISSION.md` and `FACTORY_RULES.md` content adaptation is out of scope.

## Artifacts to read

- `$ARTIFACTS_DIR/origin-main-sha.txt`
- `$ARTIFACTS_DIR/reset-status.txt`
- `$ARTIFACTS_DIR/merge-gate-status.json`
- `$ARTIFACTS_DIR/merge-gate.txt`
- `$ARTIFACTS_DIR/visual-gates.json`
- `$ARTIFACTS_DIR/visual-SimplePlot.txt`
- `$ARTIFACTS_DIR/oracle-gates.json`
- `$ARTIFACTS_DIR/oracle-gates.txt`
- `$ARTIFACTS_DIR/autoreview-status.json`
- `$ARTIFACTS_DIR/autoreview.txt`
- `$ARTIFACTS_DIR/comprehensive-report.json`
- `$ARTIFACTS_DIR/comprehensive-report.md`
- `$ARTIFACTS_DIR/regression-dedupe.json`
- `$ARTIFACTS_DIR/rendered-regression-issues.json`
- `$ARTIFACTS_DIR/regression-filing.json`

## What to summarize

- The tested `origin/main` SHA and whether the reset was clean.
- Merge gate status (`scripts/gate merge --base origin/main`).
- Visual gate status for supported native targets, currently `SimplePlot`.
- Oracle/hierarchy parity status.
- Autoreview status.
- Regression candidates with command, expected/actual behavior, artifacts, suspected subsystem, validation class, and whether they were deduped, rendered, or filed.
- Whether any rendered issue payload is blocked because it lacks a real issue id or owned selectors/path globs.

## Self-healing issue rendering rules

The workflow may render local generated issue payloads with:

```bash
python3 scripts/factory/file_regression_issue.py --input "$ARGUMENTS"
```

and with synthesized regression metadata. It may file GitHub issues only when explicitly enabled by workflow arguments and only for non-duplicate, evidence-backed, readiness-passing payloads. Otherwise, report the payload paths for human review.

## Final response

Return a concise comprehensive regression report:

- `status`: pass/fail/human-review needed;
- tested SHA;
- failed commands and artifact paths;
- generated issue payload paths;
- filing/dedupe results;
- risks or missing evidence.
