# Factory Rules

## Issue readiness

An autonomous issue must define one externally observable outcome, a PyQtGraph reference, dependencies, owned files, scope, TDD plan, validation levels, validation commands, and done criteria. Dependencies must be `none` or resolved issue references. Validation levels are only `required` or `not_applicable` for numeric, visual, and interaction checks.

## Scope limits

Default caps are 10 total files, 4 production files, 4 test/oracle files, 3 shared integration files, 800 diff lines, 2 public classes, and 1 example. Ambiguous scope fails closed.

## Merge gates

Default local factory behavior is dry-run only. Merge-capable automation may only be enabled by an explicit workflow flag/argument after all gates pass: readiness, PR evidence, owned-file scope, protected-file checks, deterministic tests, required visual/oracle checks, holdout validation, and no high-risk review finding.

## Retry policy

A holdout validator may request at most one focused fresh-context fix attempt by default. Repeated failure, risky changes, unclear evidence, or protected-file edits require human review.

## Protected files

- `MISSION.md`
- `FACTORY_RULES.md`
- `AGENTS.md`
- `WORKFLOW.md` during migration
- `docs/pyqtgraph-cpp-port-workflow.md`
- `.archon/**` unless the issue is explicitly automation/governance work
- `scripts/factory/**` unless the issue is explicitly automation/governance work
- credentials and `.env*`
