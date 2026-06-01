---
description: Review TDD and validation coverage for pgcpp changes.
argument-hint: (reads workflow artifacts)
---

# pgcpp test coverage review

Act as an independent test/validation reviewer. Do not edit files.

Review:
- Whether behavior changes had a focused failing test/oracle first.
- Whether issue `Validation commands` ran or were honestly skipped with a reason.
- Whether `scripts/gate commit --dry-run`, scope check, and diff check evidence exists.
- Whether tests cover the acceptance criteria without broad unrelated changes.

Write `$ARTIFACTS_DIR/review-test-coverage.md` with verdict, findings, missing evidence, and self-fixability.
