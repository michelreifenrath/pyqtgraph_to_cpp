---
description: Gather scoped repo and upstream context for a ready pgcpp issue.
argument-hint: (reads workflow artifacts)
---

# pgcpp research issue

You are a research-only agent for one PyQtGraph-to-C++ issue. Use current repository files and workflow artifacts; do not edit code.

Read:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/governance.txt`
- Relevant C++/Qt/OpenCV source, tests, examples, CMake wiring, oracle fixtures/scripts, and pinned upstream PyQtGraph reference named by the issue.

Do not use DynaChat/FastAPI/Bun assumptions. Do not adapt `MISSION.md` or `FACTORY_RULES.md`. Do not mutate GitHub.

Write `$ARTIFACTS_DIR/research.md` with:
- Issue number/title and one-sentence goal.
- Owned-file selectors and scope boundaries.
- Existing local code/tests/examples/oracle files that matter.
- Pinned upstream PyQtGraph source/behavior to preserve.
- Required TDD/oracle/visual proof and validation commands.
- Risks or ambiguities that would block implementation.

Return a concise summary and the path written.
